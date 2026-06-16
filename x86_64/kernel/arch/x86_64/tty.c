#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <tty.h>  //This is common for all arch

#include "vga.h"  //This is just for the current arch

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

void terminal_initialize(void) {
	terminal_row = VGA_HEIGHT - 1;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;
    
    // Clear the screen completely once
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        terminal_buffer[i] = vga_entry(' ', terminal_color);
    }
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll() {
	// Move lines 1 through (VGA_HEIGHT - 1) up by one row
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t src_index = y * VGA_WIDTH + x;
            size_t dest_index = (y - 1) * VGA_WIDTH + x;
            terminal_buffer[dest_index] = terminal_buffer[src_index];
        }
    }

    // Clear out the newly vacated bottom line
    size_t last_row_start = (VGA_HEIGHT - 1) * VGA_WIDTH;
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[last_row_start + x] = vga_entry(' ', terminal_color);
    }
}

void terminal_delete_last_line() {
	int x, *ptr;

	for(x = 0; x < VGA_WIDTH * 2; x++) {
		ptr = 0xB8000 + (VGA_WIDTH * 2) * (VGA_HEIGHT - 1) + x;
		*ptr = 0;
	}
}

void terminal_putchar(char c) {
	if (c == '\n') {
        terminal_column = 0;
        terminal_scroll(); // Instantly push everything up to clear the bottom line
		terminal_update_hardware_cursor(terminal_column,terminal_row);
        return;
    }

    // Always output to the locked terminal_row
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);

    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        terminal_scroll(); // Push up when text wraps past the right boundary
    }

	terminal_update_hardware_cursor(terminal_column,terminal_row);
}

void terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
	terminal_write(data, strlen(data));
}