#!/bin/bash

scripts_path=$(dirname ${BASH_SOURCE[0]})
tests_file=../$scripts_path/tests/perft_test.cpp
output_file=$scripts_path/output/perft_tests.txt

# ensure output directory exists
mkdir -p $(dirname $output_file)

# ensure file containing perft tests to scrape exists
if [[ ! -f "$tests_file" ]]; then
    echo "Error: Test file not found: $tests_file"
    exit 1
fi

# scrape perft tests from the tests file
awk '
    BEGIN { 
        start_fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        current_fen = ""
        depth = 0
    }
    {
        if ($0 ~/loadStarting/) {
            current_fen = start_fen
        } else if ($0 ~ /loadFen/) {
            split($0, fields, "\"")
            current_fen = fields[2]
        } else if ($0 ~ /EXPECT_EQ\(perft\(/) {
            split($0, fields, "\\(|\\)")
            depth = fields[3]
            # print perft arg list
            printf "-d,%s,-s,%s\n", depth, current_fen
        }
    }
' < $tests_file > $output_file


if [[ "$1" == "--priority" ]] || [[ "$1" == "-p" ]]; then
    echo "Executing with high priority, requires root privileges!"
    echo
    sudo python $scripts_path/perft.py "$@" < $output_file
    if [[ "$?" -eq 0 ]]; then # succeeded
        exit 0;
    fi
    echo
    echo "Failed to get root privileges, continuing with standard priority..."
fi

python $scripts_path/perft.py "$@" < $output_file

