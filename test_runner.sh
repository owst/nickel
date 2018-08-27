#!/usr/bin/env bash

# The test suite runner is very simple - it simply evaluates each test program
# (files whose name matches tests/*.nkl in the top-level directory) in turn. A
# test-program evaluation fails if the jit/interpreter output does not match
# the expected output (which is contained in a file $f.output where $f is the
# test program).

set -u

cd tests || exit

OUTPUT_DIR=$(pwd)/test_output

function cleanup {
  rm -rf "$OUTPUT_DIR"
}
trap cleanup EXIT

for mode in ${*:---interpreter --jit}
do
    echo "Running tests with $mode "

    mkdir "$OUTPUT_DIR"

    for source_file in *.nkl
    do
        ../nickel "$mode" < "$source_file" &>"$OUTPUT_DIR/$source_file"

        if diff -u "$source_file.output" "$OUTPUT_DIR/$source_file" &>> "$OUTPUT_DIR/$source_file.output.diff"
        then
            printf .
        else
            printf F
        fi
    done

    printf "\n"

    for f in "$OUTPUT_DIR"/*.output.diff
    do
        [[ -s "$f" ]] || continue

        echo "output for failed test: ${f%%.output.diff}"

        # Ignore header lines with filename and modified time, e.g.
        # --- foo.nkl.output	2018-09-09 19:25:54.000000000 +0100
        # +++ test_output/foo.nkl	2018-09-09 19:31:13.000000000 +0100
        grep -v '^\(+++\|---\)' "$f" | colordiff
    done

    cleanup
done
