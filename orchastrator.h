// orchastrator.h
#ifndef ORCHASTRATOR_H
#define ORCHASTRATOR_H

#include "u_thread.h"
#include "uthreads.h"
#include <algorithm>
#include <deque>
#include <iostream>
#include <unordered_set>

class Orchestrator
{
private:
  std::unordered_set<int> blocked_threads;
  Thread *threads[MAX_THREAD_NUM];
  std::deque<int> ready_queue;
  int current_thread;
  int total_quantums;
  int quantum_usecs;
  int find_first_available_tid();
  int context_switch();

public:
  Orchestrator(int quantum_usecs);
  Orchestrator(const Orchestrator &)= delete;
  Orchestrator &operator=(const Orchestrator &)= delete;
  ~Orchestrator();
  int spawn(thread_entry_point entry_point);
  int terminate(int tid);
  int block(int tid);
  int resume(int tid);
  int run2ready();
  int sleep(int num_quantums);
  int get_total_quantums() const { return total_quantums; }
  int get_quantums(int tid) const;
  int get_current_thread() const { return current_thread; }
};

#endif // ORCHASTRATOR_H
