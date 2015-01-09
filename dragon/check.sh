#!/bin/bash
if [ $1 = "all" ]; then
    $0 lexer &&
    $0 parser &&
    $0 compile-fail &&
    $0 run-pass &&
    echo "All tests passed"
    exit 0
fi

tmp=$(mktemp -d)

case $1 in
    lexer)
        mkdir -p $tmp/tests/lexer
        for file in tests/lexer/*.d; do
            declare -a failed
            ./dragon -ln $file > $tmp/$file.actual
            if not diff -u $tmp/$file.actual $file.expected; then
                failed+=($file)
            fi
        done
        if [ ! ${#failed[@]} = 0 ]; then
            echo "Lexer tests failed: $failed"
            exit 1;
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
