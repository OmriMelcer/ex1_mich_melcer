#include "uthreads.h"
#include <iostream>
#include <limits>

// Thread 1: prints its quantum count before and after sleeping 2 quantums.
// Main thread yields so thread 1 can run, then checks ordering.

static int tid1;

void thread1_fn()
{
    // This is quantum 2 (main had quantum 1)
    std::cout << "T1 before sleep, total=" << uthread_get_total_quantums() << std::endl;
    uthread_sleep(2);  // wakes at total_quantums >= (current+2)
    std::cout << "T1 after sleep, total=" << uthread_get_total_quantums() << std::endl;
    uthread_terminate(uthread_get_tid());
}

int main()
{
    uthread_init(std::numeric_limits<int>::max());
    tid1 = uthread_spawn(thread1_fn);
    // Yield enough times: thread1 runs, sleeps 2, main keeps running, thread1 wakes
    uthread_sleep(0); // quantum 2 → thread1 runs and sleeps (wakes at >=4)
    uthread_sleep(0); // quantum 3 → main runs (thread1 still sleeping)
    uthread_sleep(0); // quantum 4 → main runs; thread1 wakes and runs at 5
    uthread_sleep(0); // quantum 5 → thread1 runs and terminates
    std::cout << "Done" << std::endl;
}
