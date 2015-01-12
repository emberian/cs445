#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ast.h"
#include "anal.h"
#include "codegen.h"
#include "lexer.h"
#include "token.h"
#include "parser.tab.h"

#define DUMP_TOKENS (1 << 0)
#define DUMP_AST (1 << 1)
#define NO_PARSE (1 << 2)
#define NO_ANALYSIS (1 << 3)
#define NO_CODEGEN (1 << 4)

static char *USAGE = "usage: comp [-lpnNC] <filename>";

char *compile_input(char *program_source, size_t len, int options) {
    void *lexer;

    ast_node *program = NULL;

    if (options & DUMP_TOKENS) {
        // duplicate the lexer init/destroy to not affect the later parse.
        yylex_init(&lexer);
        YY_BUFFER_STATE inp = yy_scan_bytes(program_source, len, lexer);
        yy_switch_to_buffer(inp, lexer);

        int tok;
        YYSTYPE val;
        do {
            tok = yylex(&val, lexer);
            print_token(tok, &val);
            if (tok == NUM || tok == ID) {
                ast_free(val.node);
            }
        } while (tok != 0);
        puts("-- done dumping tokens --");

        yy_delete_buffer(inp, lexer);
        yylex_destroy(lexer);

    }

    if (options & NO_PARSE) { return ""; }

    yylex_init(&lexer);
    YY_BUFFER_STATE inp = yy_scan_bytes(program_source, len, lexer);
    yy_switch_to_buffer(inp, lexer);

    // Phase 1: parse. This gives us a "raw AST", with the names interned, but
    // not much else guaranteed about the program beyond its syntactic
    // validity.
    if (yyparse(&program, lexer) != 0) {
        return "";
    }

    yy_delete_buffer(inp, lexer);
    yylex_destroy(lexer);

    if (options & DUMP_AST) {
        ast_print(program, 0);
        puts("-- done dumping ast --");
    }

    if (options & NO_ANALYSIS) {
        ast_free(program);
        return "";
    }

    analysis *anal = anal_new();

    // Phase 2: resolve names. This enforces the scoping rules, and fills the
    // symbol, rewriting the interner symbols in the AST to refer
    // instead to the proper entry in the symbol table.
    anal_resolve(program, anal);

    // Phase 3: type check. This uses a very simple top-down traversal to
    // statically determine types and the well-typedness of the program.
    anal_check(program, anal);

    // Phase 4: code generation.
    return codegen(program, anal);
}

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

    char *ret = compile_input(data, (size_t)len, options);
    if (*ret)
        fprintf(stdout, "%s\n", ret);

    return 0;
}
