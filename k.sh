#!/bin/bash

# Set history file path
export HISTFILE=~/.bash_history_mx_kbd

BPARAM="100"
DRIVE_P=F
# Make clean
export ORIG_CWD=$(pwd)

# Accept first argument as shield identifier or custom tag (e.g., L, R, left, right)
ARG_TAG=$1

polling_check(){
    count=0
    while [ ! -f ${DRIVE_P}:/CURRENT.UF2 ] && [ ! -f E:/CURRENT.UF2 ] ; do
        sleep 2
        count=$((count + 1))

        if [ $count -gt 100 ]; then
            echo -e "\nBeyond attempt, break current build..."
            return 1
        fi

        if (( count == 1 )); then
            echo "Looking up Drive $DRIVE_P, polling ...every 2 seconds"
            echo -n "Detecting..."
            continue
        fi

        echo -n "."

    done

    # Set DRIVE_P to E if F: is not available but E: is
    if [ ! -f ${DRIVE_P}:/CURRENT.UF2 ] && [ -f E:/CURRENT.UF2 ]; then
        DRIVE_P="E"
    fi
    
    echo -e "\nDevice detected on drive: ${DRIVE_P}:"
    return 0
}

curr_folder=$(pwd)
echo "curr_folder_${curr_folder}"
clear

cd app || exit 1
rm -rf build

start_time=$(date +%s)

# Determine build target and output filename suffix
# Default to corne_right if no argument or argument is not 'L'
if [ "$ARG_TAG" == "L" ]; then
    SHIELD_TARGET="corne_left"
    FILE_SUFFIX="_left"
else
    SHIELD_TARGET="corne_right"
    FILE_SUFFIX="_right"
fi

echo "Building for: $SHIELD_TARGET"

west build -b nice_nano_k -- -DSHIELD=$SHIELD_TARGET

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

end_time=$(date +%s)
execution_time=$(($end_time-$start_time))
echo "Compilation time: $execution_time seconds"

echo "Waiting for device..."
polling_check

if [ $? -ne 0 ]; then
    echo "Device detection failed."
    exit 1
fi

# Define source and destination files
SRC_FILE="build/zephyr/zmk.uf2"
# Generate filename with keyword, e.g., flash_L.uf2 or flash_R.uf2
DEST_FILE="${DRIVE_P}:/flash${FILE_SUFFIX}.uf2"
DEST_FILE2="../../flash${FILE_SUFFIX}.uf2"

echo "Copying $SRC_FILE to $DEST_FILE"
cp "$SRC_FILE" "$DEST_FILE"
cp "$SRC_FILE" "$DEST_FILE2"

if [ $? -eq 0 ]; then
    echo "Flash successful: $DEST_FILE"
    echo "Flash successful: $DEST_FILE2"
else
    echo "Copy failed!"
    exit 1
fi

exit 0