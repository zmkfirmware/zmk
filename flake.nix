{
  description = "# Zephyr™ Mechanical Keyboard (ZMK) Firmware";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";
    flake-utils.url = "github:numtide/flake-utils";

    # Customize the version of Zephyr used by the flake here
    zephyr = {
      type = "github";
      owner = "zmkfirmware";
      repo = "zephyr";
      ref = "v3.5.0+zmk-fixes";
      flake = false;
    };

    zephyr-nix.url = "github:adisbladis/zephyr-nix";
    zephyr-nix.inputs.nixpkgs.follows = "nixpkgs";
    zephyr-nix.inputs.zephyr.follows = "zephyr";
  };

  outputs =
    { nixpkgs
    , flake-utils
    , zephyr-nix
    , ...
    }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = nixpkgs.legacyPackages.${system};
      zephyr = zephyr-nix.packages.x86_64-linux;
    in
    {
      devShell =
        pkgs.mkShellNoCC {
          packages = [
            (zephyr.sdk.override {
              targets = [
                "arm-zephyr-eabi"
              ];
            })
            zephyr.pythonEnv
            zephyr.hosttools-nix

            pkgs.cmake
            pkgs.ccache
            pkgs.ninja
            pkgs.dfu-util
          ];
        };
    });
}
