#!/bin/bash

# $1 - the source directory
# $2 - the path to the compiler
# $3 - the path to test_util
# $4 - the target to check

if [ $4 = "all" ]; then
    $0 $1 $2 $3 lexer &&
    $0 $1 $2 $3 parser &&
    $0 $1 $2 $3 compile-fail &&
    $0 $1 $2 $3 run-pass &&
    echo "All tests passed"
    exit 0
fi

$3 || exit

status=0
tmp=$(mktemp -d)

case $4 in
    lexer)
        echo "Doing lexer tests..."
        mkdir -p $tmp/$1/tests/lexer
        for file in $1/tests/lexer/*.d; do
            declare -a failed
            $2 -ln $file > $tmp/$file.actual 2>&1
            if not diff -u $tmp/$file.actual $file.expected; then
                echo "Test failed: $file"
                failed+=($file)
            else
                echo "Test passed: $file"
            fi
        done
        if [ ! ${#failed[@]} = 0 ]; then
            echo "Lexer tests failed: $failed"
            status=1
        fi
        ;;
    parser)
        echo "Doing parser tests..."
        mkdir -p $tmp/$1/tests/parser
        for file in $1/tests/parser/*.d; do
            declare -a failed
            $2 -pN $file > $tmp/$file.actual 2>&1
            if not diff -u $tmp/$file.actual $file.expected; then
                echo "Test failed: $file"
                failed+=($file)
            else
                echo "Test passed: $file"
            fi
        done
        if [ ! ${#failed[@]} = 0 ]; then
            echo "Parser tests failed: $failed"
            status=1
        fi
        ;;
    compile-fail)
        ;;
    run-pass)
        ;;
    *)
        echo "Unrecognized test target $4"
        exit 1
        ;;
esac

if [ $status -eq 0 ]; then
    rm -rf $tmp
fi

exit $status
