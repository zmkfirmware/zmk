{ pkgs ? import <nixpkgs> { } }:

with pkgs;

let inherit (lib) ;
  mach-nix = import (builtins.fetchGit {
    url = "https://github.com/DavHau/mach-nix";
    ref = "master";
  }) {
    python = "python3";
  };
  zephyr-requirements = fetchurl {
    url = "https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/v2.5.0/scripts/requirements-base.txt";
    sha256 = "919a78ba9457a8e55451450329158ff7fdcbc4a2ddb0dd76ea804c3b04a3c6c6";
  };
  python-pkgs = mach-nix.mkPython rec {
    requirements = builtins.readFile zephyr-requirements;
  };
in mkShell {
  buildInputs = [
    cmake
    ccache
    ninja
    dtc
    dfu-util
    gcc-arm-embedded
    python-pkgs
  ];

  shellHook = ''
    export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
    export GNUARMEMB_TOOLCHAIN_PATH=${pkgs.gcc-arm-embedded}
  '';
}
