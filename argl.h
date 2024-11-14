#ifndef ARGL_H
#define ARGL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO Completion in C
// make the program behave like a tui

// ANSI Color Codes
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_DIM     "\x1b[2m"

// TODO It should be a dynamic array
#define MAX_ARGS 64
#define MAX_DESCRIPTION 256
#define MAX_ARG_NAME 32
#define MAX_ERROR_MSG 1024

// TODO Parse inline structs too
// Make this type recursive
typedef enum {
    // TODO ARG_CHAR
    ARG_BOOL,
    ARG_STRING,
    ARG_INT,
    ARG_FLOAT
} ArgType;

typedef struct {
    char name[MAX_ARG_NAME];
    char short_name;
    char description[MAX_DESCRIPTION];
    ArgType type;
    bool required;
    bool present;
    union {
        bool flag_val;
        char* str_val;
        int int_val;
        float float_val;
    } value;
    char* default_str;
} Argument;

typedef struct {
  char program_name[MAX_ARG_NAME];
  char description[MAX_DESCRIPTION];
  Argument arguments[MAX_ARGS];
  int arg_count;
  int argc;    // Store the argument count
  char **argv; // Store the argument values
} ArgParser;

static ArgParser argp = {0};

static Argument* find_arg(const char* name) {
    for (int i = 0; i < argp.arg_count; i++) {
        if (strcmp(argp.arguments[i].name, name) == 0) {
            return &argp.arguments[i];
        }
        if (name[0] == argp.arguments[i].short_name && strlen(name) == 1) {
            return &argp.arguments[i];
        }
    }
    return NULL;
}

static void print_error_box(const char *error_type, const char *message,
                            const char *context, int position) {
    char formatted_command[MAX_ERROR_MSG] = {0};

    // Format executable path
    const char* last_slash = strrchr(argp.argv[0], '/');
    if (last_slash) {
        // There's a path component - use Dim Gray for the path
        size_t path_len = last_slash - argp.argv[0] + 1;  // +1 to include the slash
        strcat(formatted_command, "\x1b[38;5;8m");  // Dim Gray color
        strncat(formatted_command, argp.argv[0], path_len);
        strcat(formatted_command, ANSI_RESET);
        strcat(formatted_command, ANSI_GREEN);
        strcat(formatted_command, last_slash + 1);
        strcat(formatted_command, ANSI_RESET);
    } else {
        // No path component
        strcat(formatted_command, ANSI_GREEN);
        strcat(formatted_command, argp.argv[0]);
        strcat(formatted_command, ANSI_RESET);
    }

    // Process remaining arguments
    for (int i = 1; i < argp.argc; i++) {
        strcat(formatted_command, " ");
        const char* current_arg = argp.argv[i];
        
        if (current_arg[0] == '-') {
            // If it's the problematic argument
            if (strcmp(current_arg, context) == 0) {
                char colored_word[MAX_ERROR_MSG];
                snprintf(colored_word, sizeof(colored_word), "%s%s%s", 
                         ANSI_RED, current_arg, ANSI_RESET);
                strcat(formatted_command, colored_word);
            } 
            // If it's a valid flag
            else if (find_arg(current_arg + (current_arg[1] == '-' ? 2 : 1)) != NULL) {
                char colored_word[MAX_ERROR_MSG];
                snprintf(colored_word, sizeof(colored_word), "\x1b[38;5;8m%s%s", 
                         current_arg, ANSI_RESET);
                strcat(formatted_command, colored_word);
            }
            else {
                strcat(formatted_command, current_arg);
            }
        } else {
            // Check if this is a value for a previous flag
            if (i > 1 && argp.argv[i-1][0] == '-') {
                Argument* prev_arg = find_arg(argp.argv[i-1] + (argp.argv[i-1][1] == '-' ? 2 : 1));
                if (prev_arg) {
                    // Add the value as-is, with Green color
                    char colored_word[MAX_ERROR_MSG];
                    snprintf(colored_word, sizeof(colored_word), "%s%s%s", 
                             ANSI_GREEN, current_arg, ANSI_RESET);
                    strcat(formatted_command, colored_word);
                } else {
                    strcat(formatted_command, current_arg);
                }
            } else {
                strcat(formatted_command, current_arg);
            }
        }
    }

    fprintf(stderr, "%s%sError: %s%s\n", ANSI_BOLD, ANSI_RED, error_type,
            ANSI_RESET);
    fprintf(stderr, "  %s×%s %s\n", ANSI_RED, ANSI_RESET, message);

    if (context && position >= 0) {
        fprintf(stderr, "   ╭─[%s]\n", formatted_command);
        fprintf(stderr, " 1 │ %s\n", context);
        fprintf(stderr, "   · %*s%s╰── %s%s\n", position, "", ANSI_RED, "here",
                ANSI_RESET);
        fprintf(stderr, "   ╰────\n");
    }
}

static ArgParser new_arg_parser(const char* program_name, const char* description) {
    ArgParser argp = {0};
    strncpy(argp.program_name, program_name, MAX_ARG_NAME - 1);
    strncpy(argp.description, description, MAX_DESCRIPTION - 1);
    return argp;
}

static void add_arg(const char* name, char short_name,
                    const char* description, ArgType type, bool required, const char* default_val) {
    if (argp.arg_count >= MAX_ARGS) {
        print_error_box("Maximum Arguments Exceeded", 
                        "Cannot add more arguments to the parser.", NULL, -1);
        exit(1);
    }

    Argument* arg = &argp.arguments[argp.arg_count++];
    strncpy(arg->name, name, MAX_ARG_NAME - 1);
    arg->short_name = short_name;
    strncpy(arg->description, description, MAX_DESCRIPTION - 1);
    arg->type = type;
    arg->required = required;
    arg->present = false;

    if (default_val) {
        arg->default_str = strdup(default_val);
        switch (type) {
        case ARG_BOOL:
            arg->value.flag_val = strcmp(default_val, "true") == 0;
            break;
        case ARG_STRING:
            arg->value.str_val = strdup(default_val);
            break;
        case ARG_INT:
            arg->value.int_val = atoi(default_val);
            break;
        case ARG_FLOAT:
            arg->value.float_val = atof(default_val);
            break;
        }
    }
}

static void argl_print_help() {
    printf("\n");
    printf("%sOptions:%s\n", ANSI_BOLD, ANSI_RESET);

    for (int i = 0; i < argp.arg_count; i++) {
        const Argument* arg = &argp.arguments[i];
        printf("  %s-%c%s, %s--%s%s", 
               ANSI_GREEN, arg->short_name, ANSI_RESET,
               ANSI_GREEN, arg->name, ANSI_RESET);
        
        switch (arg->type) {
        case ARG_BOOL:
            break;
        case ARG_STRING:
            printf(" %s<string>%s", ANSI_YELLOW, ANSI_RESET);
            break;
        case ARG_INT:
            printf(" %s<integer>%s", ANSI_YELLOW, ANSI_RESET);
            break;
        case ARG_FLOAT:
            printf(" %s<float>%s", ANSI_YELLOW, ANSI_RESET);
            break;
        }

        printf("\n    %s%s%s", ANSI_DIM, arg->description, ANSI_RESET);
        if (arg->default_str) {
            printf(" %s(default: %s)%s", ANSI_BLUE, arg->default_str, ANSI_RESET);
        }
        if (arg->required) {
            printf(" %s[required]%s", ANSI_RED, ANSI_RESET);
        }
        printf("\n\n");
    }
}

static bool parse_args(int argc, char *argv[]) {
    // Store program args for error reporting
    argp.argc = argc;
    argp.argv = argv;
    
    // Process arguments
    for (int i = 1; i < argc; i++) {
        const char *current_arg = argv[i];
        
        // Handle --help or -h
        if (strcmp(current_arg, "--help") == 0 || strcmp(current_arg, "-h") == 0) {
            argl_print_help();
            exit(0);
        }
        
        // All arguments must start with - or --
        if (current_arg[0] != '-') {
            print_error_box("Invalid Argument",
                            "Expected an option starting with '-' or '--'",
                            current_arg,
                            0);
            return false;
        }
        
        // Get the argument name without dashes
        const char *arg_name;
        if (current_arg[1] == '-') {
            arg_name = current_arg + 2;  // Long option (--name)
        } else {
            arg_name = current_arg + 1;  // Short option (-n)
        }
        
        // Find the argument definition
        Argument *arg = find_arg(arg_name);
        if (!arg) {
            print_error_box("Unknown Argument",
                            "This argument is not recognized",
                            current_arg,
                            0);
            return false;
        }
        
        // Mark as present
        arg->present = true;
        
        // Handle flags (they don't need values)
        if (arg->type == ARG_BOOL) {
            arg->value.flag_val = true;
            continue;
        }
        
        // For non-flag arguments, we need a value
        if (i + 1 >= argc) {
            print_error_box("Missing Value",
                            "Expected a value after this argument",
                            current_arg,
                            strlen(current_arg));
            return false;
        }
        
        // Get the value
        const char *raw_value = argv[++i];
        size_t len = strlen(raw_value);
        
        switch (arg->type) {
        case ARG_STRING: {
            // Just store the raw value as-is
            arg->value.str_val = strdup(raw_value);
            break;
        }
            
        case ARG_INT: {
            char *endptr;
            long val = strtol(raw_value, &endptr, 10);
            
            if (*endptr != '\0') {
                print_error_box("Invalid Value",
                                "Expected an integer value",
                                current_arg,
                                strlen(current_arg) + 1);
                return false;
            }
            
            arg->value.int_val = (int)val;
            break;
        }
            
        case ARG_FLOAT: {
            char *endptr;
            float val = strtof(raw_value, &endptr);
            
            if (*endptr != '\0') {
                print_error_box("Invalid Value",
                                "Expected a floating-point value",
                                current_arg,
                                strlen(current_arg) + 1);
                return false;
            }
            
            arg->value.float_val = val;
            break;
        }
            
        default:
            break;
        }
    }
    
    // Check that all required arguments were provided
    for (int i = 0; i < argp.arg_count; i++) {
        if (argp.arguments[i].required && !argp.arguments[i].present) {
            char missing_arg[MAX_ARG_NAME + 3];
            snprintf(missing_arg, sizeof(missing_arg), "--%s",
                     argp.arguments[i].name);
            print_error_box("Missing Required Argument",
                            "This argument must be provided",
                            missing_arg,
                            0);
            return false;
        }
    }
    
    return true;
}

// Getter functions
static bool argl_get_flag(const char* name) {
    const Argument* arg = find_arg(name);
    return arg ? arg->value.flag_val : false;
}

static const char* argl_get_string(const char* name) {
    const Argument* arg = find_arg(name);
    return arg ? arg->value.str_val : NULL;
}

static int argl_get_int(const char* name) {
    const Argument* arg = find_arg(name);
    return arg ? arg->value.int_val : 0;
}

static float argl_get_float(const char* name) {
    const Argument* arg = find_arg(name);
    return arg ? arg->value.float_val : 0.0f;
}

#endif // ARGL_H
