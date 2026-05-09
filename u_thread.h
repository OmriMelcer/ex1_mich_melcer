/*
 * User-Level Threads Library (uthreads)
 * Hebrew University OS course.
 * Author: OS, os@cs.huji.ac.il
 */
#ifndef U_THREAD_H
#define U_THREAD_H
#include "uthreads.h"
#include "utils.h"

enum ThreadState {
    RUNNING,
    READY,
    BLOCKED
};

class Thread {
  private: 
    int tid;
    char * stack;
    thread_entry_point entry_point;
    ThreadState state;
    int quantom_count;
    int time_remaining;
    sigjmp_buf env;

  public:

    Thread(int tid, thread_entry_point entry_point);
    Thread();
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    ~Thread();
    int get_tid();
    ThreadState get_state();
    void set_state(ThreadState new_state);
    int get_quantom_count();
    void increment_quantom_count();
    int get_time_remaining();
    void set_time_remaining(int time);
    int save_env();
    void run();
};



#endif
