// main.c
#include "../uart/uart0.h"
#include "../kernel/framebf.h"
#include "../kernel/cli.h"

void main() {
    // Initialize hardware
    uart_init();     // Initialize UART
    framebf_init();  // Initialize framebuffer for display
    
    // Initialize Command Line Interface
    cli_init();
    
    // Main loop - process CLI
    while(1) {
        cli_process();
    }
}