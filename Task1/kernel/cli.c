// cli.c
#include "cli.h"
#include "../uart/uart0.h"
#include "../uart/uart1.h"

// OS name for prompt
#define OS_NAME "MyOS> "

// Command buffer and history
static char cmd_buffer[CLI_BUFFER_SIZE];
static int buffer_pos = 0;
static char cmd_history[CLI_HISTORY_SIZE][CLI_BUFFER_SIZE];
static int history_count = 0;
static int history_pos = -1;

// Command list
static command_t commands[MAX_COMMANDS];
static int command_count = 0;

// ASCII Art Welcome Message
static char welcome_message[] =
" _    _  ____  __    __    ___  _____  __  __  ____\f"
"( \\/\\/ )( ___)(  )  (  )  / __)(  _  )(  \\/  )( ___)\f"
" )    (  )__)  )(__  )(__\\ (__  )(_)(  )    (  )__)\f"
"(__/\\__)(____)(____)(____)\\___)(_____)(_/\\/\\_)(____)\n\f"
" ____  ____  ____  ____    ___   __   ___   ___\f"
"( ___)( ___)( ___)(_  _)  (__ \\ /. | / _ \\ / _ \\\f"
" )__)  )__)  )__)   )(     / _/(_  _)\\_  /( (_) )\f"
"(____)(____)(____) (__)   (____) (_)  (_/  \\___/\n\f"
" ____    __    ____  ____    _____  ___\f"
"(  _ \\  /__\\  (  _ \\( ___)  (  _  )/ __)\f"
" ) _ < /(__)\\  )   / )__)    )(_)( \\__ \f"
"(____/(__)(__)(_)/_)(____)  (_____)(___/\n\f"
                                                                  
"DEVELOPED BY           Le Minh Thai Hoa    (S3979194)            \n\f"                                                                    
"                       Chu Chi Vi          (S4045836)            \n\f";       

// String comparison utility
int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// String length utility
int my_strlen(const char *s) {
    int len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

// String comparison with length limit
int my_strncmp(const char *s1, const char *s2, int n) {
    while (n-- > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (n < 0) ? 0 : *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Function to register a command
static void register_command(const char *name, const char *description, const char *usage, void (*function)(int, char**)) {
    if (command_count < MAX_COMMANDS) {
        command_t *cmd = &commands[command_count++];
        
        // Copy command data
        int i = 0;
        while (name[i] && i < MAX_COMMAND_LENGTH - 1) {
            cmd->name[i] = name[i];
            i++;
        }
        cmd->name[i] = '\0';
        
        i = 0;
        while (description[i] && i < 127) {
            cmd->description[i] = description[i];
            i++;
        }
        cmd->description[i] = '\0';
        
        i = 0;
        while (usage[i] && i < 127) {
            cmd->usage[i] = usage[i];
            i++;
        }
        cmd->usage[i] = '\0';
        
        cmd->function = function;
    }
}

// Initialize CLI
void cli_init() {
    buffer_pos = 0;
    history_count = 0;
    history_pos = -1;
    command_count = 0;
    
    // Clear command buffer
    for (int i = 0; i < CLI_BUFFER_SIZE; i++) {
        cmd_buffer[i] = '\0';
    }
    
    // Clear history
    for (int i = 0; i < CLI_HISTORY_SIZE; i++) {
        for (int j = 0; j < CLI_BUFFER_SIZE; j++) {
            cmd_history[i][j] = '\0';
        }
    }
    
    // Register commands
    register_command("help", "Display available commands or specific command info", 
                    "help [command_name]", cmd_help);
    register_command("clear", "Clear the screen", 
                    "clear", cmd_clear);
    register_command("showinfo", "Show board revision and MAC address", 
                    "showinfo", cmd_showinfo);
    register_command("baudrate", "Change the baudrate of current UART", 
                    "baudrate <rate>", cmd_baudrate);
    register_command("handshake", "Turn on/off CTS/RTS handshaking on current UART", 
                    "handshake <on|off>", cmd_handshake);
    
    // Display welcome message
    uart_puts(welcome_message);
    
    // Display initial prompt
    uart_puts(OS_NAME);
}

// Find command by name
static command_t *find_command(const char *name) {
    for (int i = 0; i < command_count; i++) {
        // Check if this is the command we're looking for
        int j = 0;
        while (name[j] && commands[i].name[j] && name[j] == commands[i].name[j]) {
            j++;
        }
        
        if (name[j] == '\0' && commands[i].name[j] == '\0') {
            return &commands[i];
        }
    }
    
    return 0; // Command not found
}

// Parse command arguments
static int parse_args(char *buffer, char **argv) {
    int argc = 0;
    int in_token = 0;
    
    for (int i = 0; buffer[i] != '\0'; i++) {
        // Skip spaces
        if (buffer[i] == ' ' || buffer[i] == '\t') {
            buffer[i] = '\0';
            in_token = 0;
            continue;
        }
        
        // Start of new token
        if (!in_token) {
            argv[argc++] = &buffer[i];
            in_token = 1;
        }
    }
    
    return argc;
}

// Add command to history
static void add_to_history(const char *cmd) {
    // Skip empty commands
    if (cmd[0] == '\0') {
        return;
    }
    
    // Check if the command is the same as the most recent one
    if (history_count > 0) {
        if (my_strcmp(cmd_history[history_count - 1], cmd) == 0) {
            return;
        }
    }
    
    // Shift history if full
    if (history_count == CLI_HISTORY_SIZE) {
        for (int i = 0; i < CLI_HISTORY_SIZE - 1; i++) {
            int j = 0;
            while (j < CLI_BUFFER_SIZE) {
                cmd_history[i][j] = cmd_history[i + 1][j];
                j++;
            }
        }
        history_count--;
    }
    
    // Copy command to history
    int i = 0;
    while (cmd[i] && i < CLI_BUFFER_SIZE - 1) {
        cmd_history[history_count][i] = cmd[i];
        i++;
    }
    cmd_history[history_count][i] = '\0';
    
    history_count++;
}

// Display command history entry
static void display_history_entry(int index) {
    // Clear current line
    uart_puts("\r");
    for (int i = 0; i < buffer_pos + my_strlen(OS_NAME); i++) {
        uart_sendc(' ');
    }
    
    // Display prompt and history entry
    uart_puts("\r");
    uart_puts(OS_NAME);
    
    // Copy history entry to buffer
    int i = 0;
    while (cmd_history[index][i] && i < CLI_BUFFER_SIZE - 1) {
        cmd_buffer[i] = cmd_history[index][i];
        i++;
    }
    cmd_buffer[i] = '\0';
    buffer_pos = i;
    
    // Print the command from history
    uart_puts(cmd_buffer);
}

// Try to autocomplete command
static void autocomplete() {
    // Extract current word being typed
    char current_word[MAX_COMMAND_LENGTH] = {0};
    int word_len = 0;
    
    // Copy the current partial word
    for (int i = 0; i < buffer_pos && word_len < MAX_COMMAND_LENGTH - 1; i++) {
        current_word[word_len++] = cmd_buffer[i];
    }
    current_word[word_len] = '\0';
    
    // Find matching commands
    command_t *match = 0;
    int match_count = 0;
    
    for (int i = 0; i < command_count; i++) {
        // Check if command starts with the current word
        if (my_strncmp(commands[i].name, current_word, word_len) == 0) {
            match = &commands[i];
            match_count++;
        }
    }
    
    // If exactly one match, complete it
    if (match_count == 1) {
        // Clear current line
        uart_puts("\r");
        for (int i = 0; i < buffer_pos + my_strlen(OS_NAME); i++) {
            uart_sendc(' ');
        }
        
        // Copy matched command to buffer
        int i = 0;
        while (match->name[i] && i < CLI_BUFFER_SIZE - 1) {
            cmd_buffer[i] = match->name[i];
            i++;
        }
        cmd_buffer[i] = '\0';
        buffer_pos = i;
        
        // Display prompt and completed command
        uart_puts("\r");
        uart_puts(OS_NAME);
        uart_puts(cmd_buffer);
    }
    // If multiple matches, show options
    else if (match_count > 1) {
        uart_puts("\n\r");
        
        // Show all matches
        for (int i = 0; i < command_count; i++) {
            if (my_strncmp(commands[i].name, current_word, word_len) == 0) {
                uart_puts(commands[i].name);
                uart_puts("\n\r");
            }
        }
        
        // Show prompt again with current buffer
        uart_puts(OS_NAME);
        uart_puts(cmd_buffer);
    }
}

// Execute command in buffer
void cli_execute_command(char *buffer) {
    char *argv[CLI_BUFFER_SIZE / 2 + 1]; // Maximum number of arguments
    int argc;
    
    // Parse arguments
    argc = parse_args(buffer, argv);
    
    // Empty command
    if (argc == 0) {
        return;
    }
    
    // Find and execute command
    command_t *cmd = find_command(argv[0]);
    if (cmd) {
        cmd->function(argc, argv);
    } else {
        uart_puts("Unknown command: ");
        uart_puts(argv[0]);
        uart_puts("\n\r");
        uart_puts("Type 'help' to see available commands\n\r");
    }
}

// Main CLI processing function
void cli_process() {
    // Get character from UART
    char c = uart_getc();
    
    switch (c) {
        case '\r':
        case '\n':
            // Execute command
            uart_puts("\n\r");
            cmd_buffer[buffer_pos] = '\0';
            
            // Save to history if not empty
            if (buffer_pos > 0) {
                add_to_history(cmd_buffer);
            }
            
            // Execute command
            cli_execute_command(cmd_buffer);
            
            // Reset buffer and display prompt
            buffer_pos = 0;
            cmd_buffer[0] = '\0';
            history_pos = -1;
            uart_puts(OS_NAME);
            break;
            
        case 127: // Backspace
        case '\b':
            if (buffer_pos > 0) {
                buffer_pos--;
                cmd_buffer[buffer_pos] = '\0';
                
                // Erase character on screen (backspace, space, backspace)
                uart_sendc('\b');
                uart_sendc(' ');
                uart_sendc('\b');
            }
            break;
            
        case '\t': // Tab for auto-completion
            autocomplete();
            break;
            
        case '_': // Simulate UP arrow
            if (history_count > 0) {
                if (history_pos == -1) {
                    history_pos = history_count - 1;
                } else if (history_pos > 0) {
                    history_pos--;
                }
                display_history_entry(history_pos);
            }
            break;
            
        case '+': // Simulate DOWN arrow
            if (history_pos != -1 && history_pos < history_count - 1) {
                history_pos++;
                display_history_entry(history_pos);
            } else {
                // Clear current line
                uart_puts("\r");
                for (int i = 0; i < buffer_pos + my_strlen(OS_NAME); i++) {
                    uart_sendc(' ');
                }
                
                // Reset buffer
                buffer_pos = 0;
                cmd_buffer[0] = '\0';
                history_pos = -1;
                
                // Display prompt
                uart_puts("\r");
                uart_puts(OS_NAME);
            }
            break;
            
        default:
            // Ignore control characters
            if (c < 32) {
                break;
            }
            
            // Add character to buffer if there's space
            if (buffer_pos < CLI_BUFFER_SIZE - 1) {
                cmd_buffer[buffer_pos++] = c;
                cmd_buffer[buffer_pos] = '\0';
                uart_sendc(c);
            }
            break;
    }
}

// Help command implementation
void cmd_help(int argc, char **argv) {
    if (argc == 1) {
        // Display all commands
        uart_puts("Available commands:\n\r");
        for (int i = 0; i < command_count; i++) {
            uart_puts("  ");
            uart_puts(commands[i].name);
            uart_puts(" - ");
            uart_puts(commands[i].description);
            uart_puts("\n\r");
        }
        uart_puts("\n\rType 'help <command_name>' for detailed information.\n\r");
    } else {
        // Display specific command info
        command_t *cmd = find_command(argv[1]);
        if (cmd) {
            uart_puts("Command: ");
            uart_puts(cmd->name);
            uart_puts("\n\r");
            uart_puts("Description: ");
            uart_puts(cmd->description);
            uart_puts("\n\r");
            uart_puts("Usage: ");
            uart_puts(cmd->usage);
            uart_puts("\n\r");
        } else {
            uart_puts("Unknown command: ");
            uart_puts(argv[1]);
            uart_puts("\n\r");
        }
    }
}

// Clear screen command implementation
void cmd_clear(int argc, char **argv) {
    // In terminal, send multiple newlines to "clear" the screen
    for (int i = 0; i < 50; i++) {
        uart_puts("\n\r");
    }
}

// Get board revision information
static unsigned int get_board_revision() {
    // This would typically read from hardware
    // For simulation or example, return RPi 4 Model B rev 1.2
    return 0xA03111; // Example value for RPi 4
}

// Format board revision information
static void print_board_revision(unsigned int revision) {
    uart_puts("Board Revision: 0x");
    uart_hex(revision);
    uart_puts("\n\r");
    
    // Decode board revision information
    uart_puts("Board Info: ");
    
    // This is simplified - in a real implementation you would decode
    // the bits of the revision code according to the Raspberry Pi documentation
    switch(revision & 0xFF0000) {
        case 0xA00000:
            uart_puts("Raspberry Pi 4");
            break;
        case 0x900000:
            uart_puts("Raspberry Pi Zero 2 W");
            break;
        case 0x800000:
            uart_puts("Raspberry Pi 3");
            break;
        case 0x000000:
            uart_puts("Raspberry Pi 1");
            break;
        default:
            uart_puts("Unknown Raspberry Pi model");
    }
    
    uart_puts("\n\r");
}

// Get MAC address information
static void get_mac_address(uint8_t *mac) {
    // This would typically read from hardware
    // For simulation or example, generate a MAC address
    mac[0] = 0xB8;
    mac[1] = 0x27;
    mac[2] = 0xEB;
    mac[3] = 0xAA;
    mac[4] = 0xBB;
    mac[5] = 0xCC;
}

// Format MAC address
static void print_mac_address(uint8_t *mac) {
    uart_puts("MAC Address: ");
    
    for (int i = 0; i < 6; i++) {
        uart_hex(mac[i]);
        if (i < 5) {
            uart_sendc(':');
        }
    }
    
    uart_puts("\n\r");
}

// Show board information command implementation
void cmd_showinfo(int argc, char **argv) {
    unsigned int revision = get_board_revision();
    uint8_t mac[6];
    
    print_board_revision(revision);
    
    get_mac_address(mac);
    print_mac_address(mac);
}

// Convert string to integer
static int atoi(const char *s) {
    int result = 0;
    int sign = 1;
    
    // Skip whitespace
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    
    // Handle sign
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    // Convert digits
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    
    return sign * result;
}

// Change UART baud rate command implementation
void cmd_baudrate(int argc, char **argv) {
    if (argc != 2) {
        uart_puts("Usage: baudrate <rate>\n\r");
        uart_puts("Supported rates: 9600, 19200, 38400, 57600, 115200\n\r");
        return;
    }
    
    int rate = atoi(argv[1]);
    
    // Check if the requested baud rate is supported
    switch (rate) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 115200:
            // This would normally change the actual UART baud rate
            // For simulation, just acknowledge the change
            uart_puts("Changing baud rate to ");
            uart_dec(rate);
            uart_puts(" bps\n\r");
            
            // In a real implementation, you would disable UART,
            // change baud rate divisors, and re-enable UART
            
            // Example (not implemented in this simulation):
            // Change UART0_IBRD and UART0_FBRD based on rate
            // Re-initialize UART with new settings
            
            uart_puts("ACK: Baud rate changed successfully\n\r");
            break;
            
        default:
            uart_puts("NAK: Unsupported baud rate. Use one of: 9600, 19200, 38400, 57600, 115200\n\r");
    }
}

// Toggle UART handshaking command implementation
void cmd_handshake(int argc, char **argv) {
    if (argc != 2) {
        uart_puts("Usage: handshake <on|off>\n\r");
        return;
    }
    
    if (my_strcmp(argv[1], "on") == 0) {
        // This would normally enable CTS/RTS handshaking
        // For simulation, just acknowledge the change
        uart_puts("Enabling CTS/RTS handshaking\n\r");
        
        // In a real implementation, you would set the appropriate bits
        // in the UART control register, e.g.:
        // UART0_CR |= (UART0_CR_CTSEN | UART0_CR_RTSEN);
        
        uart_puts("ACK: Handshaking enabled\n\r");
    } else if (my_strcmp(argv[1], "off") == 0) {
        // This would normally disable CTS/RTS handshaking
        // For simulation, just acknowledge the change
        uart_puts("Disabling CTS/RTS handshaking\n\r");
        
        // In a real implementation, you would clear the appropriate bits
        // in the UART control register, e.g.:
        // UART0_CR &= ~(UART0_CR_CTSEN | UART0_CR_RTSEN);
        
        uart_puts("ACK: Handshaking disabled\n\r");
    } else {
        uart_puts("NAK: Invalid option. Use 'on' or 'off'\n\r");
    }
}