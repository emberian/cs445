#!/bin/bash

# $1 - the source directory
# $2 - the path to the compiler
# $3 - the target to check

if [ $3 = "all" ]; then
    $0 $1 $2 lexer &&
    $0 $1 $2 parser &&
    $0 $1 $2 compile-fail &&
    $0 $1 $2 run-pass &&
    echo "All tests passed"
    exit 0
fi

status=0
tmp=$(mktemp -d)

case $3 in
    lexer)
        mkdir -p $tmp/$1/tests/lexer
        for file in $1/tests/lexer/*.d; do
            declare -a failed
            $2 -ln $file > $tmp/$file.actual
            if not diff -u $tmp/$file.actual $file.expected; then
                failed+=($file)
            fi
        done
        if [ ! ${#failed[@]} = 0 ]; then
            echo "Lexer tests failed: $failed"
            status=1
        fi
        ;;
    parser)
        ;;
    compile-fail)
        ;;
    run-pass)
        ;;
    *)
        echo "Unrecognized test target"
        exit 1
        ;;
esac

rm -rf $tmp
exit $status
