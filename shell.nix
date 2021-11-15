{ pkgs ? import <nixpkgs> { } }:

with pkgs;

let inherit (lib) ;
in mkShell {
  buildInputs = [
    cmake
    ninja
    python3
    ccache
    dtc
    dfu-util
    gcc-arm-embedded
    python38Packages.west
    python38Packages.pyelftools
  ];

  shellHook = ''
    export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
    export GNUARMEMB_TOOLCHAIN_PATH=${pkgs.gcc-arm-embedded}
  '';
}
