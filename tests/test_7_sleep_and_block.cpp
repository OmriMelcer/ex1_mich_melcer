#include "uthreads.h"
#include <iostream>
#include <limits>

// Thread 1 sleeps for 3 quantums. While it sleeps, main blocks it.
// Thread 1 should not run until sleep expires AND main resumes it.

static int tid1;

void thread1_fn()
{
    std::cout << "T1 start" << std::endl;
    uthread_sleep(3);
    // Should only reach here after sleep+resume both satisfied
    std::cout << "T1 resumed, total=" << uthread_get_total_quantums() << std::endl;
    uthread_terminate(uthread_get_tid());
}

int main()
{
    uthread_init(std::numeric_limits<int>::max());
    tid1 = uthread_spawn(thread1_fn);

    uthread_sleep(0); // q2: thread1 runs, sleeps 3 (wakes at >=5)
    // Now block thread1 while it's sleeping
    uthread_block(tid1);
    std::cout << "Main blocked T1 at total=" << uthread_get_total_quantums() << std::endl;

    uthread_sleep(0); // q3
    uthread_sleep(0); // q4
    uthread_sleep(0); // q5: sleep would expire here, but still blocked
    // Thread 1's sleep has expired but it's still blocked — should not appear in ready

    uthread_resume(tid1); // now it should become READY
    std::cout << "Main resumed T1 at total=" << uthread_get_total_quantums() << std::endl;
    uthread_sleep(0); // q6: thread1 runs
    std::cout << "Done" << std::endl;
}
