# Dragon Project

For Clarkson University's CS445 course. A compiler of a subset of Pascal to
`x86_64` NASM assembly. Uses bison and flex, but nothing else.

# Known Deficiencies

- Semantic errors cause an abort after printing the message, instead of trying
  to continue.
- It is not possible to analyze without doing codegen.
- Some aspects of codegen are rather broken. Nonlocal variable access causes
  the stack to become misaligned in strange and mysterious ways. In general,
  the generated code is very lightly tested. I suspect it would take only 5
  more hours to iron out all of the remaining bugs. However, basically
  everything except nonlocal variable access works.
- No optimization of generated code.

# Bragging Points

- Supports records (structs), pointers, and type aliases.
- Supports while-do and for.
- Supports array indexing and assignment.
- ASan and UBSan clean. This means that there are no memory leaks,
  use-after-free, invalid memory access, or invocations of undefined C
  behavior. When compilation is successful, all memory is freed. (On an error,
  it just aborts, and leaks everything.)

# Core data structures

- `struct list` (`util.h`). A doubly-linked list, with a cached length and a pointer to
  the tail element. Only ever used for iteration and append, so it serves very
  well. Uses virtual calls for releasing elements But, really, I should just
  be using the next datastructure everywhere, since I don't really need
  constant-time remove...
- `struct ptrvec` (`util.h`). A vector (dynamic array with geometric growth) of `void*`.
  Uses virtual calls for releasing elements.
- `struct hashmap` (`util.h`). A bone-dry chaining hashmap. Pretty boring and naive. Uses
  virtual calls for hashing, comparison, and freeing keys/values. Works out
  pretty well in practice.
- The AST (`ast.h`). Uses a bunch of purpose-fit structs and tagged unions to
  keep it typesafe and easy to use.
- `struct stab` (`symbol.h`). The symbol table. Uses a... few tricks. Has a "chain" of
  scopes that is walked to find if variables are defined. Has three
  namespaces: variable, function, and type. Each of these has an arena that
  all variables/functions/types are allocated in. See `stab_var` and
  `stab_type` types.

# Building

If you're using `gcc`, edit the `CMakeLists.txt`, commenting out line 5 and
uncommenting line 7. Then:

- `mkdir build`
- `cd build`
- `cmake ..`
- `make -j3`
- `yasm -f elf64 ../rt.s` (I still haven't quite figured out how to get cmake
  to do this for me.)

Then, to invoke the compiler, you can use `./dragon test.p`. It will output
NASM assembly. This can then be assembled and linked. My full toolchain looks
like: `./dragon test.p > foo.s && yasm -f elf64 foo.s && gcc foo.o rt.o`. It's
hardcoded to the Linux syscall ABI, and the SysV AMD64 calling convention to
access libc (for `printf` and `scanf`).

# A Haiku, for your consideration

x86 sucks.

but writing compiler fun.

[the tuna swim deep](./tuna.jpg).

# Project Requirements

```
Project: DRAGON

    Arrange an online demo of your compiler during Finals week (15min).
    Send a self-contained compressed tar source of your compiler by email. Your compiler must run on the ITL machines.
    Submit a hardcopy of your compiler documentation: design document, user manual, testing report, status report (limitations, caveats, or bugs), and a "dragon" haiku. Indicate clearly in your report an extra feature that is unique to your compiler.

CHECK LIST

(1.0) Lexical Analysis

    Line numbering
    Two styles of comments
    (optional) Scientific notation

(1.5) Syntax Analysis: grammar adjustments

    Unlimited nesting of subprograms
    Array access on both sides of assignment
    Allow for statements.
    (optional) Another loop construct
    (optional) Multidimensional arrays
    (optional) Records and pointers

(2.0) Symbol Table

    Memory-leak handler
    (optional) Statistical analysis of hashpjw

(2.5) Syntax Tree (Intermediate Code Generation)

    Visual print
    Memory-leak handler

(3.0) Semantic Analysis & Type Checking

    Check list
    Error reporting
    (optional) Error recovery

(4.0) Code Generation

    Input/Output statements
    Simple expressions (arithmetic and relational): gencode
    Statements (assignment, conditional, loop)
    Nonlocal names: base frame pointer (static scope parent)
    Recursive routines (example: GCD program)
    Complex expressions (register spilling)
    (optional) Arrays (L-value, R-value, parameters, nonlocal)
    (optional) Floating-point support

Extra Trails (under construction)

    Lambda or Objects.
    Code generator for IA64 or SPARC (RISC architecture).
    Code optimization.
Simple Semantic Rules for the Dragon Pascal subset (appendix A)

1. Semantic rules for Scoping
   1.1. Local objects cannot be declared more than once
   1.2. Local objects hide non-local objects with the same name
   1.3. Non-local objects should be visible from inner scopes (unless a local object of the same name exists)
   1.4. Function and procedure names exist in the scope they are defined (and not in their own scopes)
   1.5. Local objects cease to exist once their scopes cease to exist

2. Semantic rules for Expressions
   2.1. Expressions return typed-values
   2.2. Objects must be declared before they used in expressions
   2.3. Objects of different types cannot appear in the same expression (no type promotions)

3. Semantic rules for Statements
   3.1. Statements do not return values
   3.2. The test expression for IF-THEN, IF-THEN-ELSE, WHILE-DO must be Boolean-valued;
        note that the Boolean type must be implicitly defined
   3.3. The ELSE clause always binds to the closest IF (resolution of the dangling ELSE problem)
   3.4. The variable type in FOR-DO must match the types of lower bound and upper bound expressions

4. Semantic rules for Arrays
   4.1. Non-integer valued expressions cannot be used for indexing arrays

5. Semantic rules for Functions
   5.1. Function calls return values of type Integer or Real
   5.2. Function must contain a "return" statement within its own body;
        this is of the form: <function_name> := <expression>
   5.3. Functions must accept exactly the same number of arguments as is
        declared in its header, with the correct sequence of types
   5.4. Functions are not allowed to update the value of nonlocal objects (via assignment statements)

6. Semantic rules for Procedures
   6.1. Procedure calls do not return values
   6.2. Procedures must accept exactly the same number of arguments as is
        declared in its header, with the correct sequence of types
```
