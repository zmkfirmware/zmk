{ system ? builtins.currentSystem }:

let
  pin = builtins.fromJSON (builtins.readFile ./pinned-nixpkgs.json);

  nixpkgsSrc = builtins.fetchTarball {
     inherit (pin) url sha256;
  };
in

import nixpkgsSrc {
  inherit system;
  config = {
    allowUnfree = true;
  };
  overlays = []; # prevent impure overlays
}
