#include <stdint.h>

extern uint64_t system_ticks;

void kernel_sleep(uint64_t sleep_ticks) {
    // 1. Calculate the exact point in the future when we want to wake up
    uint64_t target_ticks = system_ticks + sleep_ticks;

    // 2. Safely pause execution until that tick count is met
    while (system_ticks < target_ticks) {
        // The 'hlt' instruction is critical! 
        // It saves power and yields execution processing space.
        __asm__ volatile("hlt");
    }
}