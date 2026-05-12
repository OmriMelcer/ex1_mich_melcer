#include "uthreads.h"
#include <iostream>
#include <limits>

// Test: block a thread from outside, verify it doesn't run, then resume it

static int step = 0;
static int tid2 = -1;

void thread2_func() {
    step = 2;
    uthread_terminate(uthread_get_tid());
}

void thread1_func() {
    // block thread2 before it ever runs
    if (uthread_block(tid2) != 0) {
        std::cout << "FAIL: block returned error" << std::endl;
        exit(1);
    }
    step = 1;
    // yield so scheduler runs — thread2 is blocked, should not run
    uthread_sleep(0);
    // step should still be 1
    if (step != 1) {
        std::cout << "FAIL: blocked thread ran (step=" << step << ")" << std::endl;
        exit(1);
    }
    // now resume thread2
    if (uthread_resume(tid2) != 0) {
        std::cout << "FAIL: resume returned error" << std::endl;
        exit(1);
    }
    uthread_terminate(uthread_get_tid());
}

int main() {
    uthread_init(std::numeric_limits<int>::max());
    int tid1 = uthread_spawn(thread1_func);
    tid2 = uthread_spawn(thread2_func);
    if (tid1 == -1 || tid2 == -1) {
        std::cout << "FAIL: spawn failed" << std::endl;
        exit(1);
    }
    // yield to thread1; thread1 yields back to main; thread1 resumes thread2 and terminates;
    // main runs again before thread2, so needs one more yield
    uthread_sleep(0);
    uthread_sleep(0);
    uthread_sleep(0);
    // after thread2 runs, step should be 2
    if (step != 2) {
        std::cout << "FAIL: thread2 never ran after resume (step=" << step << ")" << std::endl;
        exit(1);
    }
    std::cout << "Success" << std::endl;
}
