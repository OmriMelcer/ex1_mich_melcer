// orchastrator.cpp
#include "orchastrator.h"

Orchestrator *Orchestrator::instance= nullptr;

Orchestrator::Orchestrator(int quantum_usecs)
{
  this->quantum_usecs= quantum_usecs;
  this->total_quantums= 1;
  this->instance= this;
  for (int i= 0; i < MAX_THREAD_NUM; i++)
  {
    threads[i]= nullptr;
  }
  threads[0]= new Thread();
  this->current_thread= 0;
  sa= {0};
  sa.sa_handler= &Orchestrator::timer_handler;
  if (sigaction(SIGVTALRM, &sa, NULL) < 0)
  {
    std::cerr << "thread library error: failed to set timer handler"
              << std::endl;
  }
  timer.it_value.tv_sec= 0; // first time interval, seconds part
  timer.it_value.tv_usec=
      quantum_usecs; // first time interval, microseconds part

  timer.it_interval.tv_sec= 0;
  timer.it_interval.tv_usec=
      quantum_usecs; // following time intervals, microseconds part
  if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
  {
    std::cerr << "thread library error: failed to set timer" << std::endl;
  }
}

Orchestrator::~Orchestrator()
{
  for (int i= 0; i < MAX_THREAD_NUM; i++)
  {
    delete threads[i];
  }
}

int Orchestrator::spawn(thread_entry_point entry_point)
{
  if (entry_point == nullptr)
  {
    return -1;
  }
  int new_tid= find_first_available_tid();
  if (new_tid == -1)
  {
    return -1;
  }
  threads[new_tid]= new Thread(new_tid, entry_point);
  ready_queue.push_back(new_tid);
  return new_tid;
}

int Orchestrator::terminate(int tid)
{
  if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr)
  {
    return -1;
  }
  if (tid == 0)
  {
    delete this;
    exit(0);
  }
  delete threads[tid];
  threads[tid]= nullptr;
  blocked_threads.erase(tid);
  auto it= std::find(ready_queue.begin(), ready_queue.end(), tid);
  if (it != ready_queue.end())
  {
    ready_queue.erase(it);
  }
  sleeping_threads.erase(tid);
  if (tid == current_thread)
  {
    context_switch();
  }
  return 0;
}

int Orchestrator::block(int tid)
{
  if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr || tid == 0)
  {
    return -1;
  }
  if (tid == current_thread)
  {
    threads[tid]->set_state(BLOCKED);
    blocked_threads.insert(tid);
    int is_first= threads[tid]->save_env();
    if (is_first == 0)
    {
      return context_switch();
    }
    else
    {
      return 0;
    }
  }
  else
  {
    threads[tid]->set_state(BLOCKED);
    blocked_threads.insert(tid);
    auto it= std::find(ready_queue.begin(), ready_queue.end(), tid);
    if (it != ready_queue.end())
    {
      ready_queue.erase(it);
    }
    return 0;
  }
}
int Orchestrator::context_switch()
{
  /**
  Context switching in this project
  is only from the current thread to the first thread in the ready queue.
  This function only handles the context switch, and does not change the state
  of the current thread.
   */

  if (ready_queue.empty())
  {
    std::cerr << "thread library error: no threads to switch to" << std::endl;
    return -1; // No other thread to switch to
  }
  total_quantums++;
  this->handle_sleeping_threads();
  int next_thread= ready_queue.front();
  ready_queue.pop_front();
  threads[next_thread]->set_state(RUNNING);
  threads[next_thread]->increment_quantom_count();
  current_thread= next_thread;
  threads[next_thread]->run();
  return 0;
}

int Orchestrator::resume(int tid)
{
  if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr)
  {
    return -1;
  }
  if (blocked_threads.find(tid) != blocked_threads.end())
  {
    blocked_threads.erase(tid);
    threads[tid]->set_state(READY);
    ready_queue.push_back(tid);
  }
  return 0;
}

int Orchestrator::sleep(int num_quantums)
{
  // assumes that num_quantums is above or equal zero
  if (num_quantums < 0)
  {
    return -1;
  }
  if (current_thread == 0 && num_quantums != 0)
  {
    return -1;
  }
  if (num_quantums == 0)
  {
    return run2ready();
  }
  threads[current_thread]->set_time_remaining(num_quantums + 1);
  threads[current_thread]->set_state(BLOCKED);
  sleeping_threads.insert(current_thread);
  int is_first= threads[current_thread]->save_env();
  if (is_first == 0)
  {
    return context_switch();
  }
  return 0;
}

int Orchestrator::get_quantums(int tid) const
{
  if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr)
  {
    return -1;
  }
  return threads[tid]->get_quantom_count();
}

int Orchestrator::find_first_available_tid()
{
  for (int i= 1; i < MAX_THREAD_NUM; i++)
  {
    if (threads[i] == nullptr)
    {
      return i;
    }
  }
  return -1;
}
int Orchestrator::run2ready()
{
  threads[current_thread]->set_state(READY);
  ready_queue.push_back(current_thread);
  int res= threads[current_thread]->save_env();
  if (res == 0)
  {
    return context_switch();
  }
  return 0;
}

void Orchestrator::handle_sleeping_threads()
{
  std::vector<int> to_wake;
  for (int tid : sleeping_threads)
  {
    int rem= threads[tid]->get_time_remaining();
    threads[tid]->set_time_remaining(rem - 1);
    if (rem - 1 == 0)
      to_wake.push_back(tid);
  }
  for (int tid : to_wake)
  {
    sleeping_threads.erase(tid);
    if (blocked_threads.find(tid) == blocked_threads.end())
    {
      threads[tid]->set_state(READY);
      ready_queue.push_back(tid);
    }
  }
}
void Orchestrator::timer_handler(int sig) { instance->run2ready(); }