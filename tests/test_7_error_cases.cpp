#include "uthreads.h"
#include <iostream>
#include <limits>

// Test: error cases for block, resume, sleep

int main() {
    uthread_init(std::numeric_limits<int>::max());

    // block nonexistent thread
    if (uthread_block(99) != -1) {
        std::cout << "FAIL: block nonexistent thread should return -1" << std::endl;
        exit(1);
    }
    // block main thread (tid 0)
    if (uthread_block(0) != -1) {
        std::cout << "FAIL: block main thread should return -1" << std::endl;
        exit(1);
    }
    // resume nonexistent thread
    if (uthread_resume(99) != -1) {
        std::cout << "FAIL: resume nonexistent thread should return -1" << std::endl;
        exit(1);
    }
    // sleep with negative quantums
    if (uthread_sleep(-1) != -1) {
        std::cout << "FAIL: sleep(-1) should return -1" << std::endl;
        exit(1);
    }
    // main thread sleep with non-zero quantums
    if (uthread_sleep(1) != -1) {
        std::cout << "FAIL: main thread sleep(1) should return -1" << std::endl;
        exit(1);
    }
    // resume a running thread (tid 0) — not an error per spec
    if (uthread_resume(0) != 0) {
        std::cout << "FAIL: resume running thread should return 0" << std::endl;
        exit(1);
    }

    std::cout << "Success" << std::endl;
}
