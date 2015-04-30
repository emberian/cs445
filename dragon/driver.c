#include "ast.h"
#include "analysis.h"
#include "driver.h"
#include "driver.h"
#include "lexer.h"
#include "parser.tab.h"
#include "token.h"

void compile_input(char *program_source, size_t len, int options) {
    void *lexer;

    struct ast_program *program = NULL;

    if (options & DUMP_TOKENS) {
        // duplicate the lexer init/destroy to not affect the later parse.
        yylex_init(&lexer);
        YY_BUFFER_STATE inp = yy_scan_bytes(program_source, len, lexer);
        yy_switch_to_buffer(inp, lexer);

        int tok;
        YYSTYPE val;
        YYLTYPE loc;
        do {
            tok = yylex(&val, &loc, lexer);
            print_token(tok, &val);
            if (tok == NUM || tok == ID) {
                free(val.name);
            }
        } while (tok != 0);
        puts("-- done dumping tokens --");

        yy_delete_buffer(inp, lexer);
        yylex_destroy(lexer);

    }

    if (options & NO_PARSE) { return; }

    yylex_init(&lexer);
    YY_BUFFER_STATE inp = yy_scan_bytes(program_source, len, lexer);
    yy_switch_to_buffer(inp, lexer);

    // Phase 1: parse. This gives us a "raw AST", with the names interned, but
    // not much else guaranteed about the program beyond its syntactic
    // validity.
    if (yyparse(&program, options, lexer) != 0) {
        free_program(program);
        yy_delete_buffer(inp, lexer);
        yylex_destroy(lexer);
        return;
    }

    yy_delete_buffer(inp, lexer);
    yylex_destroy(lexer);

    if (options & DUMP_AST) {
        print_program(program, 0);
        puts("-- done dumping ast --");
    }


    if (options & NO_ANALYSIS) {
        free_program(program);
        return;
    }

    struct acx acx = analyze(program, stdout);

    free_program(program);
    stab_free(acx.st);
}
