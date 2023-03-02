#!/bin/sh

set -eu

help() {
  echo "Usage: $0 [--release <ubuntu-release>] [--rc]" > /dev/stderr
}

doing=
rc=
release=
help=
for opt in "$@"
do
  case "${doing}" in
  release)
    release="${opt}"
    doing=
    ;;
  "")
    case "${opt}" in
    --rc)
      rc=1
      ;;
    --release)
      doing=release
      ;;
    --help)
      help=1
      ;;
    esac
    ;;
  esac
done

if [ -n "${doing}" ]
then
  echo "--${doing} option given no argument." > /dev/stderr
  echo > /dev/stderr
  help
  exit 1
fi

if [ -n "${help}" ]
then
  help
  exit
fi

if [ -z "${release}" ]
then
  unset UBUNTU_CODENAME
  . /etc/os-release

  if [ -z "${UBUNTU_CODENAME+x}" ]
  then
    echo "This is not an Ubuntu system. Aborting." > /dev/stderr
    exit 1
  fi

  release="${UBUNTU_CODENAME}"
fi

case "${release}" in
bionic|focal|jammy)
  packages=
  keyring_packages="gpg wget"
  ;;
*)
  echo "Only Ubuntu Bionic (18.04), Focal (20.04), and Jammy (22.04) are supported. Aborting." > /dev/stderr
  exit 1
  ;;
esac

get_keyring=
if [ ! -f /usr/share/keyrings/kitware-archive-keyring.gpg ]
then
  packages="${packages} ${keyring_packages}"
  get_keyring=1
fi

# Start the real work
set -x

apt-get update
# shellcheck disable=SC2086
apt-get install -y ${packages}

test -n "${get_keyring}" && (wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg)

echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ ${release} main" > /etc/apt/sources.list.d/kitware.list
if [ -n "${rc}" ]
then
  echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ ${release}-rc main" >> /etc/apt/sources.list.d/kitware.list
fi

apt-get update
test -n "${get_keyring}" && rm /usr/share/keyrings/kitware-archive-keyring.gpg
apt-get install -y kitware-archive-keyring
