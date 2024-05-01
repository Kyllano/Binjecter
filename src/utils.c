#include "binjecter.h"

//Stolen from https://gist.github.com/ictlyh/b9be0b020ae3d044dc75ef0caac01fbb

static struct arg_given_t {
    bool file, payload, section, address, entry;
} arg_given = {false, false, false, false, false};

static struct argp_option options[] = {
    {"file", 'f', "FILE", 0, "Path to the file that will be injected", 0},
    {"to-inject", 'p', "FILE", 0, "Path to the file that contains the code to inject", 0},
    {"section", 's', "SECTION_NAME", 0, "Name of newly created section", 0},
    {"base-addr", 'a', "ADDRESS", 0, "Base address of the injected code in hexadecimal (invalid output is 0x0)", 0},
    {"modify-entry", 'e', "NUMBER", 0, "Modify the entrypoint (1: true, 0:false)", 0},
    {"help", 'h', 0, 0, "Display this help message and exit.", 0},
    {0}
};

void check_file_arg(char* path) {
    if (!(access(path, F_OK) == 0)) {
        fprintf(stderr, "File %s doesn't exist\n", path);
        exit(ERROR_ARGS);
    }
}

void check_section_name(char* section_name) {
    if (!strcmp(section_name, "")) {
        fprintf(stderr, "Section name cannot be empty\n");
        exit(ERROR_ARGS);
    }
}

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    struct arguments* arguments = state->input;

    switch (key) {
        case 'f':
            arg_given.file = true;
            arguments->elf_path = arg;
            check_file_arg(arguments->elf_path);
            break;
        case 'p':
            arg_given.payload = true;
            arguments->payload_path = arg;
            check_file_arg(arguments->payload_path);
            break;
        case 's':
            arg_given.section = true;
            arguments->section_name = arg;
            check_section_name(arguments->section_name);
            break;
        case 'a':
            arg_given.address = true;
            arguments->base_addr = (int)strtol(arg, NULL, 0);
            break;
        case 'e':
            //https://www.quora.com/How-do-you-check-if-a-string-is-a-number-in-C-1
            arg_given.entry = true;
            char* remain = "";
            long int entry = strtol(arg, &remain , 0);
            if (strlen(remain) != 0) {
                fprintf(stderr, "Invalid entry argument (use 0 or 1), use --help option\n");
                exit(ERROR_ARGS);
            }
            if (entry != 1 && entry != 0){
                fprintf(stderr, "-e option must have arguments that is 0 or 1, use --help option\n");
                exit(ERROR_ARGS);   
            }
            arguments->change_entry_function = entry;
            break;
        case 'h':
            argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
            break;

        case ARGP_KEY_ARG:
            //Some arguments were given without option, we pass here.
            argp_usage(state);
            break;
        //redudant but i dont care :)
        case ARGP_KEY_END:
            //Basically the same
            if (state->arg_num != 0) {
                argp_usage(state);
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static char args_doc[] = "";
static char doc[] = "";

static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};

void parse_all(int argc, char** argv, struct arguments* arguments) {
    argp_parse(&argp, argc, argv, 0, 0, arguments);

    //Using De Morgan !x or !y => !(x and y)
    if (!(arg_given.file && arg_given.payload && arg_given.entry && arg_given.section && arg_given.address)){
        fprintf(stderr, "Not enough arguments were given, use --help option\n");
        exit(ERROR_ARGS);
    }

    check_file_arg(arguments->elf_path);
    check_file_arg(arguments->payload_path);
    check_section_name(arguments->section_name);
}
