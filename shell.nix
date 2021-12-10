{ pkgs ? import <nixpkgs> { } }:

with pkgs;

let inherit (lib) ;
in mkShell {
  buildInputs = [
    cmake
    ccache
    ninja
    dtc
    dfu-util
    gcc-arm-embedded
    python3
    python38Packages.west
    python38Packages.pip
  ];

  shellHook = ''
    export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
    export GNUARMEMB_TOOLCHAIN_PATH=${pkgs.gcc-arm-embedded}
  '';
}
