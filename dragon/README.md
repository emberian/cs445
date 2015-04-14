# Dragon Project

For Clarkon University's CS445 course. A compiler of a subset of Pascal to
`x86_64` NASM assembly. Uses bison and flex, but nothing else.

# Known Deficiencies

- On a non-syntax error, everything is leaked.
- The compiler is somewhat inefficient, in that it is not structured as a
  single pass and does a fair bit of shuffling data between representations. It's
  easier to reason about and somewhat cleaner, though.

# Bragging Points

- Supports records (structs) and pointers.
- ASan and UBSan clean for all non-error testcases.

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
