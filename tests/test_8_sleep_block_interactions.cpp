#include "uthreads.h"
#include <iostream>
#include <limits>
#include <vector>

// -----------------------------------------------------------------------
// Case 1: block a sleeping thread — it should only wake after BOTH the
//         sleep expires AND uthread_resume is called.
// -----------------------------------------------------------------------
static std::vector<int> case1_log;
static int case1_sleeper = -1;

void case1_resumer() {
    // sleeper is mid-sleep(3); block it now
    if (uthread_block(case1_sleeper) != 0) {
        std::cout << "FAIL case1: block returned error" << std::endl;
        exit(1);
    }
    case1_log.push_back(10); // marker: resumer ran
    uthread_terminate(uthread_get_tid());
}

void case1_sleeper_func() {
    case1_log.push_back(1);
    uthread_sleep(3); // sleep for 3 quantums
    case1_log.push_back(2); // should only appear after resume
    uthread_terminate(uthread_get_tid());
}

// -----------------------------------------------------------------------
// Case 2: resume a sleeping thread — should have no effect, thread wakes
//         on its own after sleep expires.
// -----------------------------------------------------------------------
static std::vector<int> case2_log;
static int case2_sleeper = -1;

void case2_resumer() {
    // sleeper is mid-sleep(2); spurious resume — should be a no-op
    if (uthread_resume(case2_sleeper) != 0) {
        std::cout << "FAIL case2: resume returned error" << std::endl;
        exit(1);
    }
    case2_log.push_back(10);
    uthread_terminate(uthread_get_tid());
}

void case2_sleeper_func() {
    case2_log.push_back(1);
    uthread_sleep(2);
    case2_log.push_back(2); // should appear after exactly 2 more quantums, not immediately
    uthread_terminate(uthread_get_tid());
}

// -----------------------------------------------------------------------
// Case 3: terminate a sleeping thread — no crash, and it never wakes.
// -----------------------------------------------------------------------
static bool case3_sleeper_woke = false;
static int case3_sleeper = -1;

void case3_killer() {
    if (uthread_terminate(case3_sleeper) != 0) {
        std::cout << "FAIL case3: terminate returned error" << std::endl;
        exit(1);
    }
    uthread_terminate(uthread_get_tid());
}

void case3_sleeper_func() {
    uthread_sleep(5);
    case3_sleeper_woke = true; // should never run
    uthread_terminate(uthread_get_tid());
}

// -----------------------------------------------------------------------
// main: run each case in sequence using the main thread as the driver
// -----------------------------------------------------------------------
int main() {
    uthread_init(std::numeric_limits<int>::max());

    // --- Case 1 ---
    case1_sleeper = uthread_spawn(case1_sleeper_func);
    uthread_spawn(case1_resumer);

    // Let sleeper start and enter sleep, then resumer blocks it.
    // After sleep expires the sleeper should still be blocked.
    // Then we resume it manually.
    for (int i = 0; i < 6; i++) uthread_sleep(0);

    // sleeper's sleep(3) has expired by now — but it was blocked, so it
    // should NOT have run yet (log should only have [1, 10])
    if (case1_log != std::vector<int>{1, 10}) {
        std::cout << "FAIL case1: wrong log before resume: ";
        for (int v : case1_log) std::cout << v << " ";
        std::cout << std::endl;
        exit(1);
    }
    uthread_resume(case1_sleeper);
    uthread_sleep(0); // let sleeper run
    if (case1_log != std::vector<int>{1, 10, 2}) {
        std::cout << "FAIL case1: wrong log after resume: ";
        for (int v : case1_log) std::cout << v << " ";
        std::cout << std::endl;
        exit(1);
    }
    std::cout << "Case 1 OK" << std::endl;

    // --- Case 2 ---
    case2_sleeper = uthread_spawn(case2_sleeper_func);
    uthread_spawn(case2_resumer);

    // yield once: sleeper runs and enters sleep(2), resumer fires and calls resume (no-op)
    uthread_sleep(0); // sleeper gets first quantum
    uthread_sleep(0); // resumer runs
    // at this point 1 quantum has passed since sleeper slept — needs 1 more
    if (case2_log != std::vector<int>{1, 10}) {
        std::cout << "FAIL case2: sleeper woke too early: ";
        for (int v : case2_log) std::cout << v << " ";
        std::cout << std::endl;
        exit(1);
    }
    uthread_sleep(0); // second quantum — sleeper should wake now
    uthread_sleep(0); // let sleeper actually run
    if (case2_log != std::vector<int>{1, 10, 2}) {
        std::cout << "FAIL case2: wrong log after sleep expired: ";
        for (int v : case2_log) std::cout << v << " ";
        std::cout << std::endl;
        exit(1);
    }
    std::cout << "Case 2 OK" << std::endl;

    // --- Case 3 ---
    case3_sleeper = uthread_spawn(case3_sleeper_func);
    uthread_spawn(case3_killer);

    // yield to let sleeper start sleeping, then killer terminates it
    for (int i = 0; i < 8; i++) uthread_sleep(0);

    if (case3_sleeper_woke) {
        std::cout << "FAIL case3: terminated sleeping thread woke up" << std::endl;
        exit(1);
    }
    std::cout << "Case 3 OK" << std::endl;

    std::cout << "Success" << std::endl;
}
