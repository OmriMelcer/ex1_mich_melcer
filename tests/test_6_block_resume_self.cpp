#include "uthreads.h"
#include <iostream>
#include <limits>

// Test: a thread blocks itself, another thread resumes it, it continues correctly.

static int after_block = 0;
static int tid1 = -1;

void resumer_func() {
    if (uthread_resume(tid1) != 0) {
        std::cout << "FAIL: resume returned error" << std::endl;
        exit(1);
    }
    uthread_terminate(uthread_get_tid());
}

void self_blocker_func() {
    uthread_block(tid1); // blocks itself
    // execution resumes here after resumer calls resume(tid1)
    after_block = 1;
    uthread_terminate(tid1);
}

int main() {
    uthread_init(std::numeric_limits<int>::max());
    tid1 = uthread_spawn(self_blocker_func);
    uthread_spawn(resumer_func);

    // yield to let threads run
    uthread_sleep(0);
    uthread_sleep(0);
    uthread_sleep(0);

    if (after_block != 1) {
        std::cout << "FAIL: thread did not continue after being resumed (after_block=" << after_block << ")" << std::endl;
        exit(1);
    }
    std::cout << "Success" << std::endl;
}
