//Simple text mode VGA pointer (64-bit address space)
unsigned short* const vga_buffer = (unsigned short*)0xB8000;

void kernel_main(void){

    const char* str = "64-bit Hi!";

    for (int i=0; str[i]!='\0';i++){
        //Color light green
        vga_buffer[i] = (unsigned short)str[i] | (0x0A <<8);
    }

    while(1){
        __asm__ __volatile__("hlt");
    }
}