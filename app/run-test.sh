#!/bin/bash

# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

function main() {
    if [ -z "$1" ] || [ "$1" = "all" ]; then
        path="tests"
    else
        path="$1"
    fi

    >&2 echo "Running all tests in ./$path"

    testcases=$(find $path -name native_posix_64.keymap -exec dirname \{\} \;)

    :>./build/tests/pass-fail.log
    echo "$testcases" | xargs -I {} -P ${J:-4} bash -c 'run_test "$@"' _ {}
    err=$?
    >&2 echo
    >&2 echo "=== Test results ==="
    sort -k2 ./build/tests/pass-fail.log
    exit $err
}

function run_test() {
    path="$1"

    build_dir="build/$path"
    build_log="$build_dir/build.log"

    # Look for a file named "inputs.txt" in some subdirectory of the test.
    #
    # If none exists, assume it uses the old approach of hardcoding events in the kscan-mock driver.
    inputs=$(find $path -mindepth 1 -name inputs.txt -exec dirname \{\} \;)

    extra_config=""
    if [ -n "$inputs" ]; then
        # replace with -DEXTRA_CONFIG_FILE for Zephyr 3.4
        extra_config+="-DOVERLAY_CONFIG=boards/native_posix_shell_test_extra.conf"
    fi

    mkdir -p $build_dir
    >&2 echo "Building $path:"
    west build -d $build_dir -b native_posix_64 -- -DZMK_CONFIG="$(pwd)/$path" $extra_config >"$build_log" 2>&1
    if [ $? -gt 0 ]; then
        echo "FAILED: $path did not build"
        >&2 printf "\t%s\n" "for details see ./$build_log"
        return 1
    fi

    function check_test_results() {
        testdir="$1"
        outdir="$2"
        tee $outdir/keycode_events_full.log | \
            sed -e "s/.*> //" | \
            sed -e "s/\x1B\[[0-9;]*[a-zA-Z]//g" | \
            sed -n -f $testdir/events.patterns > $outdir/keycode_events.log

        >&2 diff -auZ $testdir/keycode_events.snapshot $outdir/keycode_events.log

        if [ $? -gt 0 ]; then
            if [ -f $testdir/pending ]; then
                echo "PENDING: $testdir"
                return 0
            fi
            echo "FAILED: $testdir"
            return 1
        else
            echo "PASS: $testdir"
            return 0
        fi | tee -a ./build/tests/pass-fail.log
    }

    if [ -z "$inputs" ]; then
        >&2 echo "Running $path using kscan-mock:"
        ./build/$path/zephyr/zmk.exe | check_test_results $path build/$path
        return $?
    fi

    # Filters comments and blank lines from the inputs.txt file, adds a fixed
    # `wait` after each command, and appends an `exit`.
    #
    # FIXME(ecstaticmore): make the `wait` configurable.
    function augment_commands() {
        grep -v -e '^\s*#' -e '^\s*$' "$1/inputs.txt" | \
           awk '{ print; print "wait 10" }'
       echo "exit"
    }

    # This convoluted invocation is needed to work around some quirks in
    # Zephyr's serial shell driver:
    #
    # - When stdin is closed while CONFIG_NATIVE_UART_0_ON_STDINOUT is enabled,
    #   reading from the UART driver returns the same byte over and over
    #   (instead of blocking).
    # - The shell driver reads from the UART driver greedily until it blocks
    #   (instead of stopping on newlines or when the ring buffer is full).
    #
    # This means that piping directly to the Zephyr executable causes it to
    # enter an infinite loop. To work around this, we sleep inside a process
    # substitution to keep the pipe open until the Zephyr executable exits.
    ret=0
    for input in $inputs; do
        >&2 echo "Running $path/native_posix_64.keymap with $input/inputs.txt:"
        mkdir -p build/$input
        ./build/$path/zephyr/zmk.exe < <(augment_commands $input; sleep 999) \
            | check_test_results $input build/$input || ret=1
    done
    return $ret
}

export -f run_test
main "$@"