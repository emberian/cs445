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
        case EQ:
            puts("EQ");
            break;
        case NEQ:
            puts("NEQ");
            break;
        case LT:
            puts("LT");
            break;
        case LE:
            puts("LE");
            break;
        case GT:
            puts("GT");
            break;
        case GE:
            puts("GE");
            break;
        case PLUS:
            puts("PLUS");
            break;
        case MINUS:
            puts("MINUS");
            break;
        case STAR:
            puts("STAR");
            break;
        case SLASH:
            puts("SLASH");
            break;
        case LPAREN:
            puts("LPAREN");
            break;
        case RPAREN:
            puts("RPAREN");
            break;
        case LBRACKET:
            puts("LBRACKET");
            break;
        case RBRACKET:
            puts("RBRACKET");
            break;
        case COMMA:
            puts("COMMA");
            break;
        case DOTDOT:
            puts("DOTDOT");
            break;
        case DOT:
            puts("DOT");
            break;
        case ASSIGNOP:
            puts("ASSIGNOP");
            break;
        case COLON:
            puts("COLON");
            break;
        case SEMI:
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
        case AT:
            puts("AT");
            break;
        case CARET:
            puts("CARET");
            break;
        default:
            fprintf(stderr, "Unknown token %d\n", tok);
            break;
    }
}
