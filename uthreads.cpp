#include "uthreads.h"

#include <iostream>
#include <deque>
#include <array>
#include <csignal>
#include <sys/time.h>
#include <setjmp.h>

// ── address translation (required by sigsetjmp/siglongjmp on x86) ──────────
#ifdef __x86_64__
typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7
static address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
                 : "=g"(ret) : "0"(addr));
    return ret;
}
#else
typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5
static address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
                 : "=g"(ret) : "0"(addr));
    return ret;
}
#endif

// ── thread states ────────────────────────────────────────────────────────────
enum State { RUNNING, READY, BLOCKED };

struct Thread {
    int      tid;
    State    state;
    char    *stack;          // nullptr for main thread (tid==0)
    sigjmp_buf env;
    int      quantums;       // how many quantums this thread has been RUNNING
    int      sleep_until;    // total_quantums value at which sleep expires; 0 = not sleeping
    bool     blocked;        // explicitly blocked by uthread_block
};

// ── globals ──────────────────────────────────────────────────────────────────
static std::array<Thread*, MAX_THREAD_NUM> threads{};   // indexed by tid
static std::deque<int>  ready_queue;                    // tids of READY threads
static int              running_tid   = 0;
static int              total_quantums = 0;
static int              quantum_usecs  = 0;

// ── signal blocking helpers ──────────────────────────────────────────────────
static sigset_t vtalrm_mask;

static void block_signal()  { sigprocmask(SIG_BLOCK,   &vtalrm_mask, nullptr); }
static void unblock_signal(){ sigprocmask(SIG_UNBLOCK, &vtalrm_mask, nullptr); }

// ── forward declarations ─────────────────────────────────────────────────────
static void schedule();
static void timer_handler(int);

// ── internal helpers ─────────────────────────────────────────────────────────

static void reset_timer()
{
    struct itimerval timer;
    timer.it_value.tv_sec     = quantum_usecs / 1000000;
    timer.it_value.tv_usec    = quantum_usecs % 1000000;
    timer.it_interval.tv_sec  = quantum_usecs / 1000000;
    timer.it_interval.tv_usec = quantum_usecs % 1000000;
    if (setitimer(ITIMER_VIRTUAL, &timer, nullptr) < 0) {
        std::cerr << "system error: setitimer failed\n";
        exit(1);
    }
}

static void setup_thread_context(Thread *t, thread_entry_point entry_point)
{
    address_t sp = (address_t)t->stack + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t)entry_point;
    sigsetjmp(t->env, 1);
    (t->env->__jmpbuf)[JB_SP] = translate_address(sp);
    (t->env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&t->env->__saved_mask);
}

static int find_free_tid()
{
    for (int i = 0; i < MAX_THREAD_NUM; i++)
        if (threads[i] == nullptr)
            return i;
    return -1;
}

static void free_thread(int tid)
{
    if (threads[tid] == nullptr) return;
    delete[] threads[tid]->stack;
    delete threads[tid];
    threads[tid] = nullptr;
}

// Wake sleeping threads whose sleep has expired (called at quantum start).
static void wake_sleepers()
{
    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        Thread *t = threads[i];
        if (t == nullptr || t->sleep_until == 0) continue;
        if (total_quantums >= t->sleep_until) {
            t->sleep_until = 0;
            // Only move to READY if not also explicitly blocked
            if (!t->blocked && t->state == BLOCKED) {
                t->state = READY;
                ready_queue.push_back(t->tid);
            }
        }
    }
}

// Core context switch: save current, run next from ready_queue.
// Assumes signal is already blocked by caller (or called from signal handler).
static void schedule()
{
    // wake any threads whose sleep has expired
    wake_sleepers();

    if (ready_queue.empty()) {
        // Only the running thread is left; it continues.
        // Still count the new quantum.
        total_quantums++;
        threads[running_tid]->quantums++;
        reset_timer();
        return;
    }

    int next_tid = ready_queue.front();
    ready_queue.pop_front();

    int prev_tid = running_tid;
    running_tid = next_tid;

    total_quantums++;
    threads[next_tid]->state   = RUNNING;
    threads[next_tid]->quantums++;

    reset_timer();

    // Save current thread's context and jump to next.
    // sigsetjmp returns 0 the first time (saving), non-zero when restored.
    if (threads[prev_tid] != nullptr) {
        int ret = sigsetjmp(threads[prev_tid]->env, 1);
        if (ret != 0) {
            // Resumed — unblock signal and return to caller.
            unblock_signal();
            return;
        }
    }

    siglongjmp(threads[next_tid]->env, 1);
}

static void timer_handler(int /*sig*/)
{
    block_signal();
    Thread *cur = threads[running_tid];
    if (cur != nullptr) {
        cur->state = READY;
        ready_queue.push_back(running_tid);
    }
    schedule();
    // schedule() calls unblock_signal() before returning via siglongjmp
}

// ── API implementation ───────────────────────────────────────────────────────

int uthread_init(int usecs)
{
    if (usecs <= 0) {
        std::cerr << "thread library error: quantum_usecs must be positive\n";
        return -1;
    }
    quantum_usecs = usecs;

    sigemptyset(&vtalrm_mask);
    sigaddset(&vtalrm_mask, SIGVTALRM);

    // Create main thread entry
    Thread *main_thread = new Thread();
    main_thread->tid        = 0;
    main_thread->state      = RUNNING;
    main_thread->stack      = nullptr;
    main_thread->quantums   = 1;
    main_thread->sleep_until = 0;
    main_thread->blocked    = false;
    threads[0] = main_thread;

    running_tid    = 0;
    total_quantums = 1;

    // Install signal handler
    struct sigaction sa{};
    sa.sa_handler = timer_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGVTALRM, &sa, nullptr) < 0) {
        std::cerr << "system error: sigaction failed\n";
        exit(1);
    }

    reset_timer();
    return 0;
}

int uthread_spawn(thread_entry_point entry_point)
{
    block_signal();
    if (entry_point == nullptr) {
        std::cerr << "thread library error: entry_point is null\n";
        unblock_signal();
        return -1;
    }
    int tid = find_free_tid();
    if (tid < 0) {
        std::cerr << "thread library error: reached maximum number of threads\n";
        unblock_signal();
        return -1;
    }

    char *stack = new(std::nothrow) char[STACK_SIZE];
    if (stack == nullptr) {
        std::cerr << "system error: memory allocation failed\n";
        exit(1);
    }

    Thread *t = new(std::nothrow) Thread();
    if (t == nullptr) {
        std::cerr << "system error: memory allocation failed\n";
        exit(1);
    }
    t->tid        = tid;
    t->state      = READY;
    t->stack      = stack;
    t->quantums   = 0;
    t->sleep_until = 0;
    t->blocked    = false;

    setup_thread_context(t, entry_point);
    threads[tid] = t;
    ready_queue.push_back(tid);

    unblock_signal();
    return tid;
}

int uthread_terminate(int tid)
{
    block_signal();
    if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr) {
        std::cerr << "thread library error: invalid tid\n";
        unblock_signal();
        return -1;
    }

    if (tid == 0) {
        // Terminate entire process
        for (int i = 0; i < MAX_THREAD_NUM; i++)
            free_thread(i);
        exit(0);
    }

    State prev_state = threads[tid]->state;

    // Remove from ready queue if present
    for (auto it = ready_queue.begin(); it != ready_queue.end(); ++it) {
        if (*it == tid) { ready_queue.erase(it); break; }
    }

    free_thread(tid);

    if (prev_state == RUNNING) {
        // Thread terminated itself — schedule next
        schedule();
        // never returns here
    }

    unblock_signal();
    return 0;
}

int uthread_block(int tid)
{
    block_signal();
    if (tid <= 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr) {
        if (tid == 0)
            std::cerr << "thread library error: cannot block main thread\n";
        else
            std::cerr << "thread library error: invalid tid\n";
        unblock_signal();
        return -1;
    }

    Thread *t = threads[tid];
    if (t->state == BLOCKED) {
        // Already blocked — not an error, but make sure blocked flag is set
        t->blocked = true;
        unblock_signal();
        return 0;
    }

    t->blocked = true;

    if (t->state == READY) {
        for (auto it = ready_queue.begin(); it != ready_queue.end(); ++it) {
            if (*it == tid) { ready_queue.erase(it); break; }
        }
    }
    t->state = BLOCKED;

    if (tid == running_tid) {
        // Blocked itself — schedule next
        schedule();
        return 0;
    }

    unblock_signal();
    return 0;
}

int uthread_resume(int tid)
{
    block_signal();
    if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr) {
        std::cerr << "thread library error: invalid tid\n";
        unblock_signal();
        return -1;
    }

    Thread *t = threads[tid];
    t->blocked = false;

    // Only move to READY if it's BLOCKED and not still sleeping
    if (t->state == BLOCKED && t->sleep_until == 0) {
        t->state = READY;
        ready_queue.push_back(tid);
    }

    unblock_signal();
    return 0;
}

int uthread_sleep(int num_quantums)
{
    block_signal();

    if (num_quantums != 0 && running_tid == 0) {
        std::cerr << "thread library error: main thread cannot sleep\n";
        unblock_signal();
        return -1;
    }
    if (num_quantums < 0) {
        std::cerr << "thread library error: num_quantums must be non-negative\n";
        unblock_signal();
        return -1;
    }

    Thread *cur = threads[running_tid];

    if (num_quantums == 0) {
        // Yield: move to end of ready queue
        cur->state = READY;
        ready_queue.push_back(running_tid);
    } else {
        // Sleep: block until total_quantums reaches sleep_until
        // sleep_until is set relative to the *next* quantum start
        cur->sleep_until = total_quantums + num_quantums;
        cur->state = BLOCKED;
    }

    schedule();
    return 0;
}

int uthread_get_tid()
{
    return running_tid;
}

int uthread_get_total_quantums()
{
    return total_quantums;
}

int uthread_get_quantums(int tid)
{
    block_signal();
    if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr) {
        std::cerr << "thread library error: invalid tid\n";
        unblock_signal();
        return -1;
    }
    int q = threads[tid]->quantums;
    unblock_signal();
    return q;
}
