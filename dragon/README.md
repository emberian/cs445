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
