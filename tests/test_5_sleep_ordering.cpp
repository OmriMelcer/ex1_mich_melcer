#include "uthreads.h"
#include <iostream>
#include <limits>
#include <vector>

// Test: sleep(N) causes the thread to skip N quantums before rejoining the queue.
// We have thread1 sleep(2), while thread2 and thread3 yield repeatedly.
// We track the order in which threads run.

static std::vector<int> run_order;

void thread3_func() {
    run_order.push_back(3);
    uthread_sleep(0);
    run_order.push_back(3);
    uthread_terminate(uthread_get_tid());
}

void thread2_func() {
    run_order.push_back(2);
    uthread_sleep(0);
    run_order.push_back(2);
    uthread_terminate(uthread_get_tid());
}

void thread1_func() {
    run_order.push_back(1);
    uthread_sleep(2); // skip 2 quantums
    run_order.push_back(1); // should appear after thread2 and thread3 have each run once more
    uthread_terminate(uthread_get_tid());
}

int main() {
    uthread_init(std::numeric_limits<int>::max());
    uthread_spawn(thread1_func);
    uthread_spawn(thread2_func);
    uthread_spawn(thread3_func);

    // main yields until all threads are done
    uthread_sleep(0);
    uthread_sleep(0);
    uthread_sleep(0);
    uthread_sleep(0);
    uthread_sleep(0);

    // Expected: 1, 2, 3, 2, 3, 1  (thread1 wakes after 2 quantums have passed)
    // thread1 sleeps(2): it wakes after quantum ticks 2 and 3
    // so order after thread1's first run: 2,3 run once each (2 ticks), then 1 resumes
    std::vector<int> expected = {1, 2, 3, 2, 3, 1};
    if (run_order != expected) {
        std::cout << "FAIL: unexpected run order: ";
        for (int t : run_order) std::cout << t << " ";
        std::cout << std::endl;
        std::cout << "Expected: ";
        for (int t : expected) std::cout << t << " ";
        std::cout << std::endl;
        exit(1);
    }
    std::cout << "Success" << std::endl;
}
