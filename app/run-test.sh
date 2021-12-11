#!/bin/sh

# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

if [ -z "$1" ]; then
	echo "Usage: ./run-test.sh <path to testcase>"
	exit 1
fi

path="$1"
if [ $path = "all" ]; then
	path="tests"
fi

testcases=$(find $path -name native_posix.keymap -exec dirname \{\} \;)
num_cases=$(echo "$testcases" | wc -l)
if [ $num_cases -gt 1 ]; then
	echo "" > ./build/tests/pass-fail.log
	echo "$testcases" | xargs -L 1 -P ${J:-4} ./run-test.sh
	err=$?
	sort -k2 ./build/tests/pass-fail.log
	exit $err
fi

testcase="$path"
echo "Running $testcase:"

west build -d build/$testcase -b native_posix -- -DZMK_CONFIG="$(pwd)/$testcase" > /dev/null 2>&1
if [ $? -gt 0 ]; then
	echo "FAILED: $testcase did not build" | tee -a ./build/tests/pass-fail.log
	exit 1
fi

./build/$testcase/zephyr/zmk.exe | sed -e "s/.*> //" | tee build/$testcase/keycode_events_full.log | sed -n -f $testcase/events.patterns > build/$testcase/keycode_events.log
diff -au $testcase/keycode_events.snapshot build/$testcase/keycode_events.log
if [ $? -gt 0 ]; then
	if [ -f $testcase/pending ]; then
		echo "PENDING: $testcase" | tee -a ./build/tests/pass-fail.log
		exit 0
	fi
	echo "FAILED: $testcase" | tee -a ./build/tests/pass-fail.log
	exit 1
fi

echo "PASS: $testcase" | tee -a ./build/tests/pass-fail.log
exit 0
