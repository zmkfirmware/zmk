{ pkgs ? (import <nixpkgs> {})}:

let
  zmkPkgs = (import ../default.nix { inherit pkgs; });
  inherit (zmkPkgs) zmk zephyr;

  zmkCmake = pkgs.writeShellScriptBin "zmk-cmake" ''
    export PATH=${pkgs.lib.makeBinPath zmk.nativeBuildInputs}:$PATH
    export CMAKE_PREFIX_PATH=${zephyr}

    cmake -G Ninja ${pkgs.lib.escapeShellArgs zmk.cmakeFlags} "-DUSER_CACHE_DIR=/tmp/.cache" "$@"
  '';
in
pkgs.stdenv.mkDerivation {
  name = "zmk-cmake-shell";
  nativeBuildInputs = zmk.nativeBuildInputs ++ [zmkCmake];
}
