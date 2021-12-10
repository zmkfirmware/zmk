{
  description = "# Zephyr™ Mechanical Keyboard (ZMK) Firmware";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = nixpkgs.legacyPackages.${system};
      in { devShell = import ./shell.nix { inherit pkgs; }; });
}
