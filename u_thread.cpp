#include "u_thread.h"

Thread::Thread(int tid, thread_entry_point entry_point) {
    this->tid = tid;
    this->entry_point = entry_point;
    this->stack = new char[STACK_SIZE];
    this->state = READY;
    this->quantom_count = 0;
    this->time_remaining = 0;
    utils::setup_thread(env, stack, entry_point);
}

Thread::Thread() {
    this->tid = 0;
    this->entry_point = nullptr;
    this->stack = nullptr;
    this->state = RUNNING;
    this->quantom_count = 1;
    this->time_remaining = 0;   // main thread uses the real stack — no setup needed
}

Thread::~Thread() {
    delete[] stack;
}

int Thread::get_tid() {
    return this->tid;
}

ThreadState Thread::get_state() {
    return this->state;
}

void Thread::set_state(ThreadState new_state) {
    this->state = new_state;
}

int Thread::get_quantom_count() {
    return this->quantom_count;
}

void Thread::increment_quantom_count() {
    this->quantom_count++;
}

int Thread::get_time_remaining() {
    return this->time_remaining;
}

void Thread::set_time_remaining(int time) {
    this->time_remaining = time;
}
int Thread::save_env() {
    return sigsetjmp(env, 1);
}
void Thread::run() {
    siglongjmp(env, 1);
}
