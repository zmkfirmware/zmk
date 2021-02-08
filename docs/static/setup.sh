#!/bin/bash

# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

set -e

check_exists() {
    command_to_run=$1
    error_message=$2
    local __resultvar=$3

    if ! eval "$command_to_run" &> /dev/null; then
        if [[ "$__resultvar" != "" ]]; then
            eval $__resultvar="'false'"
        else
            printf "%s\n" "$error_message"
            exit 1
        fi
    else
        if [[ "$__resultvar" != "" ]]; then
            eval $__resultvar="'true'"
        fi
    fi
}

check_exists "command -v git" "git is not installed, and is required for this script!"
check_exists "command -v curl" "curl is not installed, and is required for this script!" curl_exists
check_exists "command -v wget" "wget is not installed, and is required for this script!" wget_exists

check_exists "git config user.name" "Git username not set!\nRun: git config --global user.name 'My Name'"
check_exists "git config user.email" "Git email not set!\nRun: git config --global user.email 'example@myemail.com'"

# Check to see if the user has write permissions in this directory to prevent a cryptic error later on
if [ ! -w `pwd` ]; then
    echo 'Sorry, you do not have write permissions in this directory.';
    echo 'Please try running this script again from a directory that you do have write permissions for.';
    exit 1
fi

# Parse all commandline options
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -w|--wget) force_wget="true"; break;;
        *) echo "Unknown parameter: $1"; exit 1;;
    esac
    shift
done

if [[ $curl_exists == "true" && $wget_exists == "true" ]]; then
    if [[ $force_wget == "true" ]]; then
        download_command="wget "
    else
        download_command="curl -O "
    fi
elif [[ $curl_exists == "true" ]]; then
    download_command="curl -O "
elif [[ $wget_exists == "true" ]]; then
    download_command="wget "
else
    echo 'Neither curl nor wget are installed. One of the two is required for this script!'
    exit 1
fi

repo_path="https://github.com/zmkfirmware/zmk-config-split-template.git"
title="ZMK Config Setup:"

prompt="Pick an MCU board:"
options=("nice!nano" "QMK Proton-C" "BlueMicro840 (v1)" "makerdiary nRF52840 M.2")

echo "$title"
echo ""
echo "MCU Board Selection:"
PS3="$prompt "
select opt in "${options[@]}" "Quit"; do

    case "$REPLY" in

    1 ) board="nice_nano"; break;;
    2 ) board="proton_c"; break;;
    3 ) board="bluemicro840_v1"; break;;
    4 ) board="nrf52840_m2"; break;;

    $(( ${#options[@]}+1 )) ) echo "Goodbye!"; exit 1;;
    *) echo "Invalid option. Try another one."; continue;;

    esac
done

echo ""
echo "Keyboard Shield Selection:"

prompt="Pick an keyboard:"
options=("Kyria" "Lily58" "Corne" "Splitreus62" "Sofle" "Iris" "Reviung41" "RoMac" "RoMac+" "makerdiary M60" "Microdox" "TG4X" "QAZ" "NIBBLE" "Jorne" "Jian" "CRBN" "Tidbit" "Eek!" "BFO-9000" "Helix")

PS3="$prompt "
# TODO: Add support for "Other" and linking to docs on adding custom shields in user config repos.
# select opt in "${options[@]}" "Other" "Quit"; do
select opt in "${options[@]}" "Quit"; do

    case "$REPLY" in

    1 ) shield_title="Kyria" shield="kyria"; split="y"; break;;
    2 ) shield_title="Lily58" shield="lily58"; split="y"; break;;
    3 ) shield_title="Corne" shield="corne"; split="y"; break;;
    4 ) shield_title="Splitreus62" shield="splitreus62"; split="y"; break;;
    5 ) shield_title="Sofle" shield="sofle"; split="y"; break;;
    6 ) shield_title="Iris" shield="iris"; split="y"; break;;
    7 ) shield_title="Reviung41" shield="reviung41"; split="n"; break;;
    8 ) shield_title="RoMac" shield="romac"; split="n"; break;;
    9 ) shield_title="RoMac+" shield="romac_plus"; split="n"; break;;
    10 ) shield_title="M60" shield="m60"; split="n"; break;;
    11 ) shield_title="Microdox" shield="microdox"; split="y"; break;;
    12 ) shield_title="TG4X" shield="tg4x"; split="n"; break;;
    13 ) shield_title="QAZ" shield="qaz"; split="n"; break;;
    14 ) shield_title="NIBBLE" shield="nibble"; split="n"; break;;
    15 ) shield_title="Jorne" shield="jorne"; split="y"; break;;
    16 ) shield_title="Jian" shield="jian"; split="y"; break;;
    17 ) shield_title="CRBN" shield="crbn"; split="n"; break;;
    18 ) shield_title="Tidbit" shield="tidbit"; split="n" break;;
    19 ) shield_title="Eek!" shield="eek"; split="n" break;;
    20 ) shield_title="BFO-9000" shield="bfo9000"; split="y"; break;;
    21 ) shield_title="Helix" shield="helix"; split="y"; break;;

    # Add link to docs on adding your own custom shield in your ZMK config!
    # $(( ${#options[@]}+1 )) ) echo "Other!"; break;;
    $(( ${#options[@]}+1 )) ) echo "Goodbye!"; exit 1;;
    *) echo "Invalid option. Try another one.";continue;;

    esac
done

if [ "$split" == "n" ]; then
    repo_path="https://github.com/zmkfirmware/zmk-config-template.git"
fi

read -r -e -p "Copy in the stock keymap for customization? [Yn]: " copy_keymap

if [ -z "$copy_keymap" ] || [ "$copy_keymap" == "Y" ] || [ "$copy_keymap" == "y" ]; then copy_keymap="yes"; fi

read -r -e -p "GitHub Username (leave empty to skip GitHub repo creation): " github_user
if [ -n "$github_user" ]; then
    read -r -p "GitHub Repo Name [zmk-config]: " repo_name
    if [ -z "$repo_name" ]; then repo_name="zmk-config"; fi

    read -r -p "GitHub Repo [https://github.com/${github_user}/${repo_name}.git]: " github_repo

    if [ -z "$github_repo" ]; then github_repo="https://github.com/${github_user}/${repo_name}.git"; fi
else
    repo_name="zmk-config"
fi

echo ""
echo "Preparing a user config for:"
echo "* MCU Board: ${board}"
echo "* Shield: ${shield}"

if [ "$copy_keymap" == "yes" ]; then
    echo "* Copy Keymap?: ✓"
else
    echo "* Copy Keymap?: ❌"
fi

if [ -n "$github_repo" ]; then
    echo "* GitHub Repo To Push (please create this in GH first!): ${github_repo}"
fi

echo ""
read -r -p "Continue? [Yn]: " do_it

if [ -n "$do_it" ] && [ "$do_it" != "y" ] && [ "$do_it" != "Y" ]; then
    echo "Aborting..."
    exit 1
fi

git clone --single-branch $repo_path ${repo_name}
cd ${repo_name}

pushd config

$download_command "https://raw.githubusercontent.com/zmkfirmware/zmk/main/app/boards/shields/${shield}/${shield}.conf"

if [ "$copy_keymap" == "yes" ]; then
    $download_command "https://raw.githubusercontent.com/zmkfirmware/zmk/main/app/boards/shields/${shield}/${shield}.keymap"
fi

popd

sed -i'.orig' \
    -e "s/BOARD_NAME/$board/" \
    -e "s/SHIELD_NAME/$shield/" \
    -e "s/KEYBOARD_TITLE/$shield_title/" \
    .github/workflows/build.yml

if [ "$board" == "proton_c" ]; then
    # Proton-C board still fa
    sed -i'.orig' -e "s/uf2/hex/g" .github/workflows/build.yml
fi

rm .github/workflows/*.yml.orig

rm -rf .git
git init .
git add .
git commit -m "Initial User Config."

if [ -n "$github_repo" ]; then
    git remote add origin "$github_repo"
    git push --set-upstream origin "$(git symbolic-ref --short HEAD)"
    push_return_code=$?

    # If push failed, assume that the origin was incorrect and give instructions on fixing.
    if [ ${push_return_code} -ne 0 ]; then
        echo "Remote repository $github_repo not found..."
        echo "Check GitHub URL, and try adding again."
        echo "Run the following: "
        echo "    git remote rm origin"
        echo "    git remote add origin FIXED_URL"
        echo "    git push --set-upstream origin $(git symbolic-ref --short HEAD)"
        echo "Once pushed, your firmware should be availalbe from GitHub Actions at: ${github_repo%.git}/actions"
        exit 1
    fi

    # TODO: Support determing the actions URL when non-https:// repo URL is used.
    if [ "${github_repo}" != "${github_repo#https://}" ]; then
        echo "Your firmware should be available from GitHub Actions shortly: ${github_repo%.git}/actions"
    fi
fi
