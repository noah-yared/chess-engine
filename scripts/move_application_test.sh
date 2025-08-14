#!/bin/bash

scripts_path=$(dirname ${BASH_SOURCE[0]})
n_cores=8

if [ $# -eq 0 ]; then
    echo "No arguments passed in so using move application data from: scripts/output/move_app_data.txt..."
    echo
else
    echo "Generating move application data with arguments: \"$@\"..."
    echo 
    python $scripts_path/move_application_generator.py "$@" 2> $scripts_path/output/errors.txt
    if [ $? -ne 0 ]; then
        echo "FATAL: Failed to generate move application data"
        echo "See $scripts_path/output/errors.txt for traceback"
        exit 1
    fi
fi

if [ ! -f $scripts_path/../build/engine ]; then
    echo "Engine binary not found at: $scripts_path/../build/engine"
    echo "Building engine..."
    if [ ! -d $scripts_path/../build ]; then
        mkdir $scripts_path/../build
    fi
    # build the engine in release mode
    cd $scripts_path/../build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(($n_cores / 2))
    cd -  # go back to previous working directory
fi

# create a temporary directory for the engine output
tmp_path=$scripts_path/tmp
if [ ! -d $tmp_path ]; then
    mkdir $tmp_path
fi

# run the engine with the move application data
$scripts_path/../build/engine --make-move \
    < $scripts_path/output/move_app_data.txt \
    > $tmp_path/actual_after_fens.txt

# write the after fens to a file in tmp
awk -F, 'NF == 3 { print $3 }' < $scripts_path/output/move_app_data.txt > $tmp_path/expected_after_fens.txt

# make sure paste command is avaiable
if ! command -v paste > /dev/null 2>&1; then
    echo "FATAL: paste command not found"
    exit 1
fi

# compare the expected and actual after fens
awk -F'( [0-9]+ [0-9]+(\t)?)' -v failed=0 '
    NR >= 2 && $1 != $2 {
        printf "Mismatch for test case %d:\nExpected: %s\nActual: %s\n\n", NR, $1, $2
        ++failed
    }
    END {
        if (!failed) { printf "Passed all test cases!\n" }
        else { printf "Failed %d/%d test cases\n", failed, NR }
    }
' <(paste $tmp_path/expected_after_fens.txt $tmp_path/actual_after_fens.txt)

# remove the temporary directory
rm -rf $tmp_path
