//orchastrator.cpp
#include "orchastrator.h"


Orchestrator::Orchestrator(int quantum_usecs) {
    this->quantum_usecs = quantum_usecs;
    this->total_quantums = 1;
    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        threads[i] = nullptr;
    }
    threads[0] = new Thread();
    this->current_thread = 0;
}


Orchestrator::~Orchestrator() {
    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        delete threads[i];
    }
}

int Orchestrator::spawn(thread_entry_point entry_point) {
    if (entry_point == nullptr) {
        std::cerr << "thread library error: " << "entry_point cannot be null" << std::endl; 
        exit(1);
    }
    int new_tid = find_first_available_tid();
    if (new_tid == -1) {
        std::cerr << "thread library error: " << "max thread limit reached" << std::endl; 
        exit(1);
    }
    threads[new_tid] = new Thread(new_tid, entry_point);
    ready_queue.push(new_tid);
    return new_tid;
}

int Orchestrator::terminate(int tid) {
    std::cerr << "thread library error: " << "did not implement" << std::endl;
    return -1;
}

int Orchestrator::block(int tid) {
    std::cerr << "thread library error: " << "did not implement" << std::endl;
    return -1;
}

int Orchestrator::resume(int tid) {
    std::cerr << "thread library error: " << "did not implement" << std::endl;
    return -1;
}

int Orchestrator::sleep(int num_quantums) {
    std::cerr << "thread library error: " << "did not implement" << std::endl;
    return -1;
}

int Orchestrator::get_quantums(int tid) const {
    if (threads[tid] == nullptr) {
        std::cerr << "thread library error: " << "thread does not exist" << std::endl;
        return -1;
    }
    return threads[tid]->get_quantom_count();
}

int Orchestrator::find_first_available_tid() {
    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        if (threads[i] == nullptr) {
            return i;
        }
    }
    return -1;
}