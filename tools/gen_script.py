#!/usr/bin/env python3
import sys
import subprocess
import re

def main():
    if len(sys.argv) < 3:
        if(len(sys.argv)==2):
            out_file = sys.argv[1]
            with open(out_file, "w") as f:
                f.write("#include <stdint.h>\n\n")
                f.write("typedef struct {\n")
                f.write("    uintptr_t address;\n")
                f.write("    const char *name;\n")
                f.write("} kernel_symbol_t;\n\n")

                # Write out the array
                f.write(f"const kernel_symbol_t kernel_symbol_table[1] = {{ {{0xA,\"A\" }}, }};\n")

                # Export the count so your kernel loop knows when to stop searching
                f.write(f"const int kernel_symbol_count = 1;\n")
                sys.exit(1)

        print("Usage: gen_symbols.py <kernel.elf> <output_symbols.c>")
        sys.exit(1)

    elf_file = sys.argv[1]
    out_file = sys.argv[2]

    # Run 'nm' to extract symbols from the ELF file
    # We use 'x86_64-elf-nm' or whatever toolchain nm you use. Fallback to standard 'nm'.
    nm_tool = "x86_64-elf-nm"
    try:
        result = subprocess.run([nm_tool, "-n", elf_file], stdout=subprocess.PIPE, text=True, check=True)
    except FileNotFoundError:
        result = subprocess.run(["nm", "-n", elf_file], stdout=subprocess.PIPE, text=True, check=True)

    symbols = []
    
    # Regex to match text (code) symbols: [address] [t/T/w/W] [function_name]
    symbol_regex = re.compile(r"^([0-9a-fA-F]+)\s+[tTwW]\s+(.+)$")

    for line in result.stdout.splitlines():
        match = symbol_regex.match(line)
        if match:
            addr = int(match.group(1), 16)
            name = match.group(2).strip()
            # Ignore standard internal compiler noise to keep the table clean
            if not name.startswith('__') and not name.startswith('.'):
                symbols.append((addr, name))

    # Generate the C source file
    with open(out_file, "w") as f:
        f.write("#include <stdint.h>\n\n")
        f.write("typedef struct {\n")
        f.write("    uintptr_t address;\n")
        f.write("    const char *name;\n")
        f.write("} kernel_symbol_t;\n\n")

        # Write out the array
        f.write(f"const kernel_symbol_t kernel_symbol_table[{len(symbols)}] = {{\n")
        for addr, name in symbols:
            f.write(f"    {{ 0x{addr:X}, \"{name}\" }},\n")
        f.write("};\n\n")

        # Export the count so your kernel loop knows when to stop searching
        f.write(f"const int kernel_symbol_count = {len(symbols)};\n")

if __name__ == "__main__":
    main()