{ pkgs ? import <nixpkgs> {} }:

with pkgs;

let
  bundleEnv = bundlerEnv {
    name = "lambda-bundler-env";
    ruby     = ruby_3_1;
    gemfile  = ./Gemfile;
    lockfile = ./Gemfile.lock;
    gemset   = ./gemset.nix;
  };

  source = stdenv.mkDerivation {
    name    = "lambda-builder";
    version = "0.0.1";
    src = ./.;
    installPhase = ''
      cp -r ./ $out
    '';
  };

in
{
  inherit bundleEnv source;
}
