#!/bin/bash

set -e

CheckExists() {
    command_to_run=$1
    error_message=$2
        
    if ! eval "$command_to_run" &> /dev/null; then
        printf "%s\n" "$error_message"
        exit
    fi
}

CheckExists "command -v git" "git is not installed, and is required for this script!"
CheckExists "command -v curl" "curl is not installed, and is required for this script!"

CheckExists "git config user.name" "Git username not set!\nRun: git config --global user.name 'My Name'"
CheckExists "git config user.email" "Git email not set!\nRun: git config --global user.email 'example@myemail.com'"

repo_path="https://github.com/zmkfirmware/zmk-config-split-template.git"
title="ZMK Config Setup:"

prompt="Pick an MCU board:"
options=("nice!nano" "QMK Proton-C" "BlueMicro840 (v1)")

echo "$title"
echo ""
echo "MCU Board Selection:"
PS3="$prompt "
select opt in "${options[@]}" "Quit"; do 

    case "$REPLY" in

    1 ) board="nice_nano"; break;;
    2 ) board="proton_c"; break;;
    3 ) board="bluemicro840_v1"; break;;

    $(( ${#options[@]}+1 )) ) echo "Goodbye!"; exit;;
    *) echo "Invalid option. Try another one.";continue;;

    esac
done

echo ""
echo "Keyboard Shield Selection:"

prompt="Pick an keyboard:"
options=("Kyria" "Lily58" "Corne" "Splitreus62" "Sofle" "Iris" "RoMac")

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
    7 ) shield_title="RoMac" shield="romac"; split="n"; break;;

    # Add link to docs on adding your own custom shield in your ZMK config!
    # $(( ${#options[@]}+1 )) ) echo "Other!"; break;; 
    $(( ${#options[@]}+1 )) ) echo "Goodbye!"; exit;;
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
    exit
fi

git clone --single-branch $repo_path ${repo_name}
cd ${repo_name}

pushd config

curl -O "https://raw.githubusercontent.com/zmkfirmware/zmk/main/app/boards/shields/${shield}/${shield}.conf"

if [ "$copy_keymap" == "yes" ]; then
    curl -O "https://raw.githubusercontent.com/zmkfirmware/zmk/main/app/boards/shields/${shield}/${shield}.keymap"
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

    # TODO: Support determing the actions URL when non-https:// repo URL is used.
    if [ "${github_repo}" != "${github_repo#https://}" ]; then
        echo "Your firmware should be available from the GitHub Actions shortly: ${github_repo%.git}/actions"
    fi
fi
