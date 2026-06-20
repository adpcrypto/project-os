#include <stdint.h>
#include "tty.h"

// Standard US Keyboard Layout Scan Code Set 1
// Index 0 to 57 match the standard layout keys.
static const char as_keymap[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', /* Backspace */
  '\t', /* Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter */
    0,  /* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   
    0,  /* 42   - Left Shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   
    0,  /* 54   - Right Shift */
  '*',
    0,  /* 56   - Alt */
  ' ',  /* Space bar */
};

__attribute__((no_stack_protector))
void keyboard_handler_c(void) {
    // 1. Read the scancode from the physical port
    uint8_t scancode = inb(0x60);

    // 2. Clear the PIC interrupt channel immediately
    outb(0x20, 0x20);

    // 3. Check if Bit 7 is set (Key Release)
    if (scancode & 0x80) {
        // This is a key release event (e.g., let up on 'A')
        // We can ignore this for basic typing.
        return; 
    }

    // 4. Boundary protection: Ensure the index fits inside our map table array
    if (scancode < sizeof(as_keymap)) {
        char printable_char = as_keymap[scancode];

        // 5. If it maps to a real character, print it!
        if (printable_char != 0) {
            // Replace this with your custom character printer function
            terminal_putchar(printable_char);
        }
    }
}