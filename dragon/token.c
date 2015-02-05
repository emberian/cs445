#include "ast.h"
#include "parser.tab.h"
#include <stdio.h>

void print_token(int tok, YYSTYPE *val) {
    switch (tok) {
        case 0:
            puts("EOF");
            break;
        case DIV:
            puts("DIV");
            break;
        case BOOLEAN:
            puts("BOOLEAN");
            break;
        case STRING:
            puts("STRING");
            break;
        case CHAR:
            puts("CHAR");
            break;
        case MOD:
            puts("MOD");
            break;
        case AND:
            puts("AND");
            break;
        case OR:
            puts("OR");
            break;
        case NOT:
            puts("NOT");
            break;
        case FUNCTION:
            puts("FUNCTION");
            break;
        case PROCEDURE:
            puts("PROCEDURE");
            break;
        case TBEGIN:
            puts("TBEGIN");
            break;
        case END:
            puts("END");
            break;
        case INTEGER:
            puts("INTEGER");
            break;
        case REAL:
            puts("REAL");
            break;
        case ARRAY:
            puts("ARRAY");
            break;
        case OF:
            puts("OF");
            break;
        case PROGRAM:
            puts("PROGRAM");
            break;
        case '=':
            puts("EQ");
            break;
        case NEQ:
            puts("NEQ");
            break;
        case '<':
            puts("LT");
            break;
        case LE:
            puts("LE");
            break;
        case '>':
            puts("GT");
            break;
        case GE:
            puts("GE");
            break;
        case '+':
            puts("PLUS");
            break;
        case '-':
            puts("MINUS");
            break;
        case '*':
            puts("STAR");
            break;
        case '/':
            puts("SLASH");
            break;
        case '(':
            puts("LPAREN");
            break;
        case ')':
            puts("RPAREN");
            break;
        case '[':
            puts("LBRACKET");
            break;
        case ']':
            puts("RBRACKET");
            break;
        case ',':
            puts("COMMA");
            break;
        case '.':
            puts("DOT");
            break;
        case DOTDOT:
            puts("DOTDOT");
            break;
        case ASSIGNOP:
            puts("ASSIGNOP");
            break;
        case ':':
            puts("COLON");
            break;
        case ';':
            puts("SEMI");
            break;
        case ID:
            printf("ID(%s)\n", val->name);
            break;
        case NUM:
            printf("NUM(%s)\n", val->name);
            break;
        case DO:
            puts("DO");
            break;
        case ELSE:
            puts("ELSE");
            break;
        case IF:
            puts("IF");
            break;
        case THEN:
            puts("THEN");
            break;
        case VAR:
            puts("VAR");
            break;
        case WHILE:
            puts("WHILE");
            break;
        case '@':
            puts("AT");
            break;
        case '^':
            puts("CARET");
            break;
        default:
            fprintf(stderr, "Unknown token %d\n", tok);
            break;
    }
}
