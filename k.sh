
# 设置历史记录文件路径
export HISTFILE=~/.bash_history_mx_kbd

BPARAM="100"
config_root=pilot
keymap_qf=via
config_qf=$1
DRIVE_P=F
# make clean
export ORIG_CWD=$(pwd)
polling_check(){
count=0
while [ ! -f ${DRIVE_P}:/CURRENT.UF2 ] && [ ! -f E:/CURRENT.UF2 ] ; do
    sleep 2
    count=$((count + 1))

    if [ $count -gt 100 ]; then
        echo "\nBeyong attempt, break current build..."
        break
    fi

    if (( count == 1 )); then
        echo "looking up Drive $DRIVE_P, polling ...every 2 seconds"
        echo -n "Detecting..."
        continue
    fi

    echo -n "."

done

# Set DRIVE_P to E if F: is not available but E: is
if [ ! -f ${DRIVE_P}:/CURRENT.UF2 ] && [ -f E:/CURRENT.UF2 ]; then
    DRIVE_P="E"
fi
}

if [ ! $1 ]; then

  echo "Please input parameter first , use below example as reference."
  BPARAM="100"
else
    BPARAM=$1
fi



if [ -n "$2" ]; then
   keymap_qf=$2
    # Your code here
fi

export USE_EXISTING_LIB=OFF
# Check for "clean" option in the script arguments
if [[ "$@" == *uselib* ]]; then
    echo -e "${GREEN}Taking up my own library *******${END}"
    export USE_EXISTING_LIB=ON
fi


curr_folder=$(pwd)
echo "curr_folder_${curr_folder}"
    clear

cd app
 rm -rf build
    start_time=$(date +%s)
# west build -b nice_nano -- -DSHIELD=corne_left
# west build -b nice_nano -- -DSHIELD=corne_left 
# west build -b nice_nano -- -DSHIELD=corne_left -S studio-rpc-usb-uart

# west build -d build/cl_studio -b nice_nano \
#   -S studio-rpc-usb-uart -- -DSHIELD=corne_left -DCONFIG_ZMK_STUDIO=y

#   west build -b nice_nano -S zmk-usb-logging -- -DSHIELD="corne_left"
  west build -b nice_nano_k -- -DSHIELD=corne_left 


    end_time=$(date +%s)

    execution_time=$(($end_time-$start_time))
    echo "Compilation time: $execution_time seconds"

    
    polling_check
   cp build/zephyr/zmk.uf2 E:/flash.uf2
#    cp build/cl_studio/zephyr/zmk.uf2 E:/flash.uf2
    exit 0

