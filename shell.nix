{ pkgs ? import <nixpkgs> { } }:

with pkgs;

let inherit (lib) ;
  mach-nix = import (builtins.fetchGit {
    url = "https://github.com/DavHau/mach-nix";
    ref = "master";
  }) {
    python = "python3";
  };
  machNix = mach-nix.mkPython rec {
    requirements = builtins.readFile ./zephyr/scripts/requirements-base.txt;
  };
in mkShell {
  buildInputs = [
    cmake
    ccache
    ninja
    dtc
    dfu-util
    gcc-arm-embedded
    machNix
  ];

  shellHook = ''
    export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
    export GNUARMEMB_TOOLCHAIN_PATH=${pkgs.gcc-arm-embedded}
  '';
}
