#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "driver.h"

static char *USAGE = "usage: comp [-lpinNC] <filename>";

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "error: unrecognized number of arguments: %d\n", argc);
        fprintf(stderr, "%s\n", USAGE);
        return 1;
    }
    int fd = open(argv[argc-1], O_RDONLY);
    if (fd == -1) return 1;
    ssize_t len = lseek(fd, 0, SEEK_END);
    if (len == -1) return 1;
    char *data = mmap(0, (size_t)len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) return 1;

    int options = 0;
    if (argc == 3) {
        for (char *c = argv[1] + 1; *c; c++) {
            switch (*c) {
                case 'l':
                    options |= DUMP_TOKENS;
                    break;
                case 'p':
                    options |= DUMP_AST;
                    break;
                case 'i':
                    options |= DUMP_IR;
                    break;
                case 'n':
                    options |= NO_PARSE;
                    break;
                case 'N':
                    options |= NO_ANALYSIS;
                    break;
                case 'C':
                    options |= NO_CODEGEN;
                    break;
                default:
                    fprintf(stderr, "unknown flag: %c\n", *c);
                    exit(1);
                    break;
            }
        }
    }

    compile_input(data, (size_t)len, options);

    return 0;
}
