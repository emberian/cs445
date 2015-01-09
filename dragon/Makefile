include base.mk

# Recursive wildcard function
# http://blog.jgc.org/2011/07/gnu-make-recursive-wildcard-function.html
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) \
		  $(filter $(subst *,%,$2),$d))

OBJECT_FILES=ast.o lex.yy.o parser.tab.o main.o anal.o symbol.o util.o token.o

all: dragon

ast.o: parser.tab.c

lex.yy.c lexer.h: lexer.l parser.tab.c
	flex --header-file=lexer.h $<

parser.tab.c: parser.y
	bison $< --defines=parser.tab.h -v --report=all

parser.tab.o: parser.tab.c parser.y lexer.h ast.o
	$(CC) $(CFLAGS) -Wno-sign-conversion -Wno-conversion -c $< -o $@

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -Wno-error -Wno-sign-conversion -Wno-conversion -Wno-implicit-function-declaration -Wno-unused-function -c $< -o $@

dragon: $(OBJECT_FILES)
	$(CC) $^ -o $@

check: dragon $(call rwildcard,tests,*)
	./check.sh all

clean:
	rm -f *.o lexer.h lex.yy.c parser.tab.c parser.tab.h dragon check-all.stamp
