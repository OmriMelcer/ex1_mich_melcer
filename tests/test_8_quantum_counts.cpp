#include "uthreads.h"
#include <iostream>
#include <limits>

// Verify quantum counts for main thread and spawned threads.
// With INT_MAX quantum (no timer preemption), each context switch == 1 quantum.

void thread1_fn()
{
    // This is quantum 2. Thread1 should have quantums=1 now.
    std::cout << "T1 quantums=" << uthread_get_quantums(uthread_get_tid()) << std::endl;
    uthread_sleep(0); // back of queue, main runs (q3), then t1 again (q4)
    std::cout << "T1 quantums=" << uthread_get_quantums(uthread_get_tid()) << std::endl;
    uthread_terminate(uthread_get_tid());
}

int main()
{
    uthread_init(std::numeric_limits<int>::max());
    // total=1, main quantums=1
    std::cout << "total=" << uthread_get_total_quantums()
              << " main=" << uthread_get_quantums(0) << std::endl;

    int t1 = uthread_spawn(thread1_fn);
    uthread_sleep(0); // q2: thread1 runs
    // q3: main runs
    std::cout << "total=" << uthread_get_total_quantums()
              << " main=" << uthread_get_quantums(0) << std::endl;
    uthread_sleep(0); // q4: thread1 runs again and terminates
    // q5: main runs
    std::cout << "total=" << uthread_get_total_quantums()
              << " main=" << uthread_get_quantums(0) << std::endl;

    // thread1 is gone — should return error
    int r = uthread_get_quantums(t1);
    std::cout << "dead thread quantums=" << r << std::endl;
    std::cout << "Done" << std::endl;
}
