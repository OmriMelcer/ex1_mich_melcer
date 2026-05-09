#ifndef UTILS_H
#define UTILS_H

#include <setjmp.h>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>

namespace utils {
    typedef unsigned long address_t;
    address_t translate_address(address_t addr);
    void setup_thread(sigjmp_buf& env, char* stack, void (*entry_point)());
}

#endif