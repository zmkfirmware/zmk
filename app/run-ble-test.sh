#!/bin/bash

# Copyright (c) 2023 The ZMK Contributors
# SPDX-License-Identifier: MIT

if [ -z "$1" ]; then
    echo "Usage: ./run-ble-test.sh <path to testcase>"
    exit 1
fi

path=$1
if [ "$path" = "all" ]; then
    path="tests"
fi

if [ -z "${BSIM_OUT_PATH}" ]; then
    echo "BSIM_OUT_PATH needs to be set before running this script."
    exit 1
fi

if [ -z "$BLE_TESTS_NO_CENTRAL_BUILD" ]; then
    if ! [ -e build/tests/ble/central ]; then
        west build -d build/tests/ble/central -b nrf52_bsim tests/ble/central > /dev/null 2>&1
    else
        west build -d build/tests/ble/central
    fi

    cp build/tests/ble/central/zephyr/zephyr.exe "${BSIM_OUT_PATH}/bin/ble_test_central.exe"

    if ! [ -e build/tests/ble/private_central ]; then
        west build -d build/tests/ble/private_central -b nrf52_bsim tests/ble/central -- -DCONFIG_BT_PRIVACY=y -DCONFIG_BT_SCAN_WITH_IDENTITY=n > /dev/null 2>&1
    else
        west build -d build/tests/ble/private_central
    fi

    cp build/tests/ble/private_central/zephyr/zephyr.exe "${BSIM_OUT_PATH}/bin/ble_test_private_central.exe"

    if ! [ -e build/tests/ble/no_auto_sec_central ]; then
        west build -d build/tests/ble/no_auto_sec_central -b nrf52_bsim tests/ble/central -- -DCONFIG_BT_ATT_RETRY_ON_SEC_ERR=n > /dev/null 2>&1
    else
        west build -d build/tests/ble/no_auto_sec_central
    fi

    cp build/tests/ble/no_auto_sec_central/zephyr/zephyr.exe "${BSIM_OUT_PATH}/bin/ble_test_no_auto_sec_central.exe"
fi

testcases=$(find $path -name nrf52_bsim.keymap -exec dirname \{\} \;)
num_cases=$(echo "$testcases" | wc -l)
if [ $num_cases -gt 1 ] || [ "$testcases" != "${path%%/}" ]; then
    echo "$testcases"
    echo "" > ./build/tests/pass-fail.log
    echo "$testcases" | BLE_TESTS_QUIET_OUTPUT=y BLE_TESTS_NO_CENTRAL_BUILD=y xargs -L 1 -P ${J:-4} ./run-ble-test.sh
    err=$?
    sort -k2 ./build/tests/pass-fail.log
    exit $err
fi

testcase="$path"
echo "Running $testcase:"

west build -d build/$testcase -b nrf52_bsim -- -DZMK_CONFIG="$(pwd)/$testcase" > /dev/null 2>&1
if [ $? -gt 0 ]; then
    echo "FAILED: $testcase did not build" | tee -a ./build/tests/pass-fail.log
    exit 1
fi

if [ -n "${BLE_TESTS_QUIET_OUTPUT}" ]; then
    output_dev="/dev/null"
else
    output_dev="/dev/stdout"
fi

exe_name=${testcase//\//_}

start_dir=$(pwd)
cp build/$testcase/zephyr/zmk.exe "${BSIM_OUT_PATH}/bin/${exe_name}"
pushd "${BSIM_OUT_PATH}/bin" > /dev/null 2>&1
if [ -e "${start_dir}/build/$testcase/output.log" ]; then
  rm "${start_dir}/build/$testcase/output.log"
fi

central_counts=$(wc -l ${start_dir}/${testcase}/centrals.txt | cut -d' ' -f1)
./${exe_name} -d=0 -s=${exe_name} | tee -a "${start_dir}/build/$testcase/output.log" > "${output_dev}" &
./bs_device_handbrake -s=${exe_name} -d=1 -r=10 > "${output_dev}" &

cat "${start_dir}/${testcase}/centrals.txt" |
while IFS= read -r line
do
  ${line} -s=${exe_name} | tee -a "${start_dir}/build/$testcase/output.log" > "${output_dev}" &
done

./bs_2G4_phy_v1 -s=${exe_name} -D=$(( 2 + central_counts )) -sim_length=50e6 > "${output_dev}" 2>&1

popd > /dev/null 2>&1

cat build/$testcase/output.log | sed -E -n -f $testcase/events.patterns > build/$testcase/filtered_output.log

diff -auZ $testcase/snapshot.log build/$testcase/filtered_output.log
if [ $? -gt 0 ]; then
    if [ -f $testcase/pending ]; then
        echo "PENDING: $testcase" | tee -a ./build/tests/pass-fail.log
        exit 0
    fi

    if [ -n "${ZMK_TESTS_AUTO_ACCEPT}" ]; then
        echo "Auto-accepting failure for $testcase"
        cp build/$testcase/filtered_output.log $testcase/snapshot.log
    else
        echo "FAILED: $testcase" | tee -a ./build/tests/pass-fail.log
        exit 1
    fi
fi

echo "PASS: $testcase" | tee -a ./build/tests/pass-fail.log
exit 0
