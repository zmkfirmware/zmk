{ pkgs ? (import ./pinned-nixpkgs.nix {}) }:

let
  # need a newer west than nixpkgs packages
  pythonOverrides = self: super: {
    west = super.west.overridePythonAttrs(old: rec {
      inherit (old) pname;
      version = "0.9.0";
      src = super.fetchPypi {
        inherit pname version;
        sha256 = "1asgw3v3k77lvh4i1c3s0gncy2dn658py6256bzpjp1k35gs8mbg";
      };
    });
  };

  python = pkgs.python3.override {
    packageOverrides = pythonOverrides;
  };

  # from zephyr/scripts/requirements-base.txt
  pythonDependencies = ps: with ps; [
    pyelftools
    pyyaml
    canopen
    packaging
    progress
    anytree
    intelhex
    west
  ];

  requiredStdenv =
    if pkgs.stdenv.hostPlatform.isLinux
    then pkgs.multiStdenv
    else pkgs.stdenv;
in
with pkgs;
# requires multiStdenv to build 32-bit test binaries
requiredStdenv.mkDerivation {
  name = "zmk-shell";

  buildInputs = [
    # ZMK dependencies
    gitFull
    wget
    autoconf
    automake
    bzip2
    ccache
    dtc # devicetree compiler
    dfu-util
    gcc
    libtool
    ninja
    cmake
    xz
    (python.withPackages(pythonDependencies))

    # ARM toolchain
    gcc-arm-embedded
  ];

  ZEPHYR_TOOLCHAIN_VARIANT = "gnuarmemb";
  GNUARMEMB_TOOLCHAIN_PATH = gcc-arm-embedded;

  shellHook = "if [ ! -d \"zephyr\" ]; then west init -l app/ ; west update; west zephyr-export; fi; source zephyr/zephyr-env.sh";
}
