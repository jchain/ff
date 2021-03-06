#include "options.h"

// C standard library
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// POSIX C library
#include <dirent.h>

// GNU C library
#include <getopt.h>

// PCRE
#include <pcre.h>

void print_usage(const char *msg) {
    if (msg) {
        fputs(msg, stderr);
        fputs("\n", stderr);
    }
    fputs(
        // clang-format off
        "Usage: ff [FLAGS/OPTIONS] [<pattern>] [<path>...]\n"
        "Simplified version of GNU find using the PCRE library for regex.\n"
        "\n"
        "OPTIONS:\n"
        "  -d, --depth <n>      Maximum directory traversal depth\n"
        "  -t, --type <x>       Restrict output to type with <x> one of\n"
        "                           b   block device.\n"
        "                           c   character device.\n"
        "                           d   directory.\n"
        "                           n   named pipe (FIFO).\n"
        "                           l   symbolic link.\n"
        "                           f   regular file.\n"
        "                           s   UNIX domain socket.\n"
        "  -j, --threads <n>    Use <n> threads for parallel directory traversal\n"
        "\n"
        "FLAGS:\n"
        "  -g, --glob           Match glob instead of regex\n"
        "  -H, --hidden         Traverse hidden directories and files as well\n"
        "  -I, --no-ignore      Disregard .gitignore\n"
        "  -i, --ignore-case    Ignore case when applying the regex\n"
        "  -D, --deterministic  Deterministic sorting within directories (SLOW!)\n"
        "  -h, --help           Display this help and quit\n",
        // clang-format on
        stderr);
}

int parse_options(int argc, char *argv[], options *opt) {
    int option_index = 0;
    static struct option long_options[] = {
        {"depth", required_argument, NULL, 'd'},
        {"type", required_argument, NULL, 't'},
        {"threads", required_argument, NULL, 'j'},
        {"glob", no_argument, NULL, 'g'},
        {"hidden", no_argument, NULL, 'H'},
        {"ignore-case", no_argument, NULL, 'i'},
        {"no-ignore", no_argument, NULL, 'I'},
        {"deterministic", no_argument, NULL, 'D'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};

    int c = -1;
    while ((c = getopt_long(argc, argv, "d:t:j:gHiIDh", long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'd':
            assert(optarg);
            opt->max_depth = (long)strtoul(optarg, NULL, 0);
            if (opt->max_depth == 0 || errno == ERANGE) {
                print_usage("Invalid argument for --depth");
                return OPTIONS_FAILURE;
            }
            break;
        case 't':
            assert(optarg && strlen(optarg) > 0);
            switch (optarg[0]) {
            case 'b':
                opt->only_type = DT_BLK;
                break;
            case 'c':
                opt->only_type = DT_CHR;
                break;
            case 'd':
                opt->only_type = DT_DIR;
                break;
            case 'n':
                opt->only_type = DT_FIFO;
                break;
            case 'l':
                opt->only_type = DT_LNK;
                break;
            case 'f':
                opt->only_type = DT_REG;
                break;
            case 's':
                opt->only_type = DT_SOCK;
                break;
            default:
                print_usage("Invalid argument for --type");
                return OPTIONS_FAILURE;
            }
            break;
        case 'g': // glob
            opt->mode = GLOB;
            break;
        case 'H':
            opt->skip_hidden = false;
            break;
        case 'I':
            opt->no_ignore = true;
            break;
        case 'i':
            opt->icase = true;
            break;
        case 'j':
            assert(optarg);
            opt->nthreads = (long)strtoul(optarg, NULL, 0);
            if (opt->nthreads == 0 || errno == ERANGE) {
                print_usage("Invalid argument for --nthreads");
                return OPTIONS_FAILURE;
            }
            break;
        case 'D':
            opt->deterministic = true;
            break;
        case 'h':
            print_usage(NULL);
            return OPTIONS_HELP;
        default:
            print_usage(NULL);
            return OPTIONS_FAILURE;
        }
    }

    // Scan pattern and directory
    const char *pattern = "";
    switch (argc - optind) {
    case 0:
        opt->mode = NONE;
        break;
    default:
        pattern = argv[optind++];
        if (strlen(pattern) > 0 && opt->mode == NONE) {
            opt->mode = REGEX;
        }
        break;
    }

    for (int arg = optind; arg < argc; ++arg) {
        // Check if the requested directory even exists
        DIR *d = opendir(argv[arg]);
        if (d == NULL) {
            perror(argv[arg]);
            print_usage(NULL);
            return OPTIONS_FAILURE;
        }
        closedir(d);

        // Truncate trailing slashes
        size_t len = strlen(argv[arg]);
        while (len > 1 && argv[arg][len - 1] == '/') {
            argv[arg][--len] = '\0';
        }
    }
    opt->optind = optind;

    // Set up the pattern matcher
    switch (opt->mode) {
    case REGEX: {
        // Compile pattern
        opt->match.re = regex_compile(pattern, opt->icase);
    } break;
    case GLOB:
        opt->match.pattern = pattern;
        break;
    case NONE:
        break;
    }

    return OPTIONS_SUCCESS;
}
