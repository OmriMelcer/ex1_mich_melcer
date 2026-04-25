#include "uthreads.h"
#include <iostream>

// Each thread burns CPU until preempted by the timer.
// We just verify that both threads get scheduled and quantum counts increase.

static volatile int t1_ran = 0;
static volatile int t2_ran = 0;

void thread1_fn()
{
    t1_ran = 1;
    // Spin until preempted a few times
    while (uthread_get_quantums(uthread_get_tid()) < 3) {}
    uthread_terminate(uthread_get_tid());
}

void thread2_fn()
{
    t2_ran = 1;
    while (uthread_get_quantums(uthread_get_tid()) < 3) {}
    uthread_terminate(uthread_get_tid());
}

int main()
{
    uthread_init(100000);  // 100ms quantum so the test doesn't take too long
    int t1 = uthread_spawn(thread1_fn);
    int t2 = uthread_spawn(thread2_fn);
    (void)t1; (void)t2;

    // Spin until both threads have run and terminated
    while (uthread_get_total_quantums() < 8) {}

    if (t1_ran && t2_ran)
        std::cout << "Timer preemption works" << std::endl;
    else
        std::cout << "Timer preemption FAILED" << std::endl;
}
