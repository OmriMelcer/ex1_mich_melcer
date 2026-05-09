#include "utils.h"
#include "uthreads.h"  // for STACK_SIZE

namespace utils {
    #ifdef __x86_64__
    #define JB_SP 6
    #define JB_PC 7
    address_t translate_address(address_t addr) {
        address_t ret;
        asm volatile("xor    %%fs:0x30,%0\n"
                    "rol    $0x11,%0\n"
                    : "=g" (ret) : "0" (addr));
        return ret;
    }
    #else
    #define JB_SP 4
    #define JB_PC 5
    address_t translate_address(address_t addr) {
        address_t ret;
        asm volatile("xor    %%gs:0x18,%0\n"
                    "rol    $0x9,%0\n"
                    : "=g" (ret) : "0" (addr));
        return ret;
    }
    #endif

    void setup_thread(sigjmp_buf& env, char* stack, void (*entry_point)()) {
        address_t sp = (address_t)stack + STACK_SIZE - sizeof(address_t);
        address_t pc = (address_t)entry_point;
        sigsetjmp(env, 1);
        (env[0].__jmpbuf)[JB_SP] = translate_address(sp);
        (env[0].__jmpbuf)[JB_PC] = translate_address(pc);
        sigemptyset(&env[0].__saved_mask);
    }

} // namespace utils
