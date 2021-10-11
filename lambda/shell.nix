{ pkgs ? (import <nixpkgs> {})}:

let
  lambda = import ./default.nix { inherit pkgs; };
in
pkgs.stdenv.mkDerivation {
  name = "lambda-shell";
  buildInputs = [lambda.bundleEnv.wrappedRuby];
}
