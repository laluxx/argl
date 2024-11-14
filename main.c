#include "argl.h"

int main(int argc, char* argv[]) {
    // Add arguments
    add_arg("verbose", 'v', "Enable verbose output", ARG_BOOL, false, "false");
    add_arg("name", 'n', "Your name", ARG_STRING, true, NULL);
    add_arg("count", 'c', "Number of iterations", ARG_INT, false, "1");
    add_arg("threshold", 't', "Threshold value", ARG_FLOAT, false, "0.5");

    // Parse arguments
    if (!parse_args(argc, argv)) {
        argl_print_help();
        return 1;
    }

    // Use the parsed values
    if (argl_get_flag("verbose")) {
        printf("Verbose mode enabled\n");
    }

    printf("Name: %s\n", argl_get_string("name"));
    printf("Count: %d\n", argl_get_int("count"));
    printf("Threshold: %f\n", argl_get_float("threshold"));

    return 0;
}
