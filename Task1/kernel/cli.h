// cli.h
#ifndef CLI_H
#define CLI_H

#define CLI_BUFFER_SIZE 256
#define CLI_HISTORY_SIZE 10
#define MAX_COMMANDS 10
#define MAX_COMMAND_LENGTH 32

// Define uint8_t since stdint.h isn't available
typedef unsigned char uint8_t;

// Command structure
typedef struct {
    char name[MAX_COMMAND_LENGTH];
    char description[128];
    char usage[128];
    void (*function)(int argc, char **argv);
} command_t;

// Function prototypes
void cli_init();
void cli_process();
void cli_execute_command(char *buffer);

// Command functions
void cmd_help(int argc, char **argv);
void cmd_clear(int argc, char **argv);
void cmd_showinfo(int argc, char **argv);
void cmd_baudrate(int argc, char **argv);
void cmd_handshake(int argc, char **argv);

// String utilities
int my_strcmp(const char *s1, const char *s2);
int my_strlen(const char *s);
int my_strncmp(const char *s1, const char *s2, int n);

#endif // CLI_H
