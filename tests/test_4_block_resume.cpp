#include "uthreads.h"
#include <iostream>
#include <limits>

// Thread 1 prints A, blocks itself, then after resume prints B and terminates.
// Thread 2 resumes thread 1, prints C, terminates.
// Main: spawns 1 and 2, yields, then after both done prints Done.

static int tid1, tid2;

void thread2_fn()
{
    std::cout << "C" << std::endl;
    uthread_resume(tid1);
    uthread_terminate(uthread_get_tid());
}

void thread1_fn()
{
    std::cout << "A" << std::endl;
    uthread_block(uthread_get_tid());  // blocks itself
    std::cout << "B" << std::endl;
    uthread_terminate(uthread_get_tid());
}

int main()
{
    uthread_init(std::numeric_limits<int>::max());
    tid1 = uthread_spawn(thread1_fn);
    tid2 = uthread_spawn(thread2_fn);
    // Yield three times to let both threads finish
    uthread_sleep(0);
    uthread_sleep(0);
    uthread_sleep(0);
    std::cout << "Done" << std::endl;
}
