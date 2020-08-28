#!/bin/sh

if [ -z "$1" ]; then
	echo "Usage: ./run-test.sh <path to testcase>"
	exit 1
elif [ "$1" = "all" ]; then
	find tests -name native_posix.keymap -exec dirname \{\} \; | xargs -l -P 2 ./run-test.sh
	exit $?
fi

testcase="$1"
echo "Running $testcase:"

west build --pristine -d build/$testcase -b native_posix -- -DZMK_CONFIG=$testcase > /dev/null
./build/$testcase/zephyr/zmk.exe | sed -e "s/.*> //" | sed -n -f $testcase/events.patterns > build/$testcase/keycode_events.log

diff -au $testcase/keycode_events.snapshot build/$testcase/keycode_events.log
