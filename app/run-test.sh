#!/bin/sh

# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

##
# Optional environment variables, paths can be absolute or relative to $(pwd):
#  ZMK_SRC_DIR:             Path to zmk/app (default is ./)
#  ZMK_BUILD_DIR:           Path to build directory (default is $ZMK_SRC_DIR/build)
#  ZMK_EXTRA_MODULES:       Path to at most one module (in addition to any in west.yml)
#  ZMK_TESTS_VERBOSE:       Be more verbose
#  ZMK_TESTS_AUTO_ACCEPT:   Replace snapshot files with new key events
#  J:                       Number of parallel jobs (default is 4)

if [ -z "$1" ]; then
    echo "Usage: ./run-test.sh <path to testcase>"
    exit 1
fi

path="$1"
if [ $path = "all" ]; then
    path="${ZMK_SRC_DIR-.}/tests"
fi

ZMK_BUILD_DIR=${ZMK_BUILD_DIR:-${ZMK_SRC_DIR:-.}/build}
mkdir -p ${ZMK_BUILD_DIR}/tests

testcases=$(find $path -name native_posix_64.keymap -exec dirname \{\} \;)
num_cases=$(echo "$testcases" | wc -l)
if [ $num_cases -gt 1 ] || [ "$testcases" != "$path" ]; then
    echo "" >${ZMK_BUILD_DIR}/tests/pass-fail.log
    echo "$testcases" | xargs -L 1 -P ${J:-4} ${0}
    err=$?
    sort -k2 ${ZMK_BUILD_DIR}/tests/pass-fail.log
    exit $err
fi

testcase=$(realpath $path | sed -n -e "s|.*/tests/||p")
echo "Running $testcase:"

build_cmd="west build ${ZMK_SRC_DIR:+-s $ZMK_SRC_DIR} -d ${ZMK_BUILD_DIR}/tests/$testcase \
    -b native_posix_64 -p -- -DCONFIG_ASSERT=y -DZMK_CONFIG="$(realpath $path)" \
    ${ZMK_EXTRA_MODULES:+-DZMK_EXTRA_MODULES="$(realpath ${ZMK_EXTRA_MODULES})"}"

if [ -z ${ZMK_TESTS_VERBOSE} ]; then
    $build_cmd >/dev/null 2>&1
else
    echo "ZMK_SRC_DIR:   ${ZMK_SRC_DIR:-.}"
    echo "ZMK_BUILD_DIR: $ZMK_BUILD_DIR"
    $build_cmd
fi

if [ $? -gt 0 ]; then
    echo "FAILED: $testcase did not build" | tee -a ${ZMK_BUILD_DIR}/tests/pass-fail.log
    exit 1
fi

${ZMK_BUILD_DIR}/tests/$testcase/zephyr/zmk.exe |
    sed -e "s/.*> //" |
    tee ${ZMK_BUILD_DIR}/tests/$testcase/keycode_events_full.log |
    sed -n -f $path/events.patterns >${ZMK_BUILD_DIR}/tests/$testcase/keycode_events.log

diff -auZ $path/keycode_events.snapshot ${ZMK_BUILD_DIR}/tests/$testcase/keycode_events.log
if [ $? -gt 0 ]; then
    if [ -f $path/pending ]; then
        echo "PENDING: $testcase" | tee -a ${ZMK_BUILD_DIR}/tests/pass-fail.log
        exit 0
    fi

    if [ -n "${ZMK_TESTS_AUTO_ACCEPT}" ]; then
        echo "Auto-accepting failure for $testcase"
        cp ${ZMK_BUILD_DIR}/tests/$testcase/keycode_events.log $path/keycode_events.snapshot
    else
        echo "FAILED: $testcase" | tee -a ${ZMK_BUILD_DIR}/tests/pass-fail.log
        exit 1
    fi
fi

echo "PASS: $testcase" | tee -a ${ZMK_BUILD_DIR}/tests/pass-fail.log
exit 0
