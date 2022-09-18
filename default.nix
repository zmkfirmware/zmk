{ pkgs ? (import ./nix/pinned-nixpkgs.nix {}) }:
let
  inherit (pkgs) newScope;
  inherit (pkgs.lib) makeScope;
in

makeScope newScope (self: with self; {
  # From scratch:
  #  nix run -f . west -c west init -l app
  #  nix run -f . west -c west update
  #  nix run -f . update-manifest -c update-manifest > nix/manifest.json
  update-manifest = callPackage ./nix/update-manifest { };

  west = pkgs.python3Packages.west.overridePythonAttrs (old: rec {
    inherit (old) pname;
    version = "0.9.0";
    src = pkgs.python3Packages.fetchPypi {
      inherit pname version;
      sha256 = "1asgw3v3k77lvh4i1c3s0gncy2dn658py6256bzpjp1k35gs8mbg";
    };
  });

  combine_uf2 = a: b: pkgs.runCommandNoCC "combined_${a.name}_${b.name}" {}
  ''
    mkdir -p $out
    cat ${a}/zmk.uf2 ${b}/zmk.uf2 > $out/glove80.uf2
  '';

  zephyr = callPackage ./nix/zephyr.nix { };

  zmk = callPackage ./nix/zmk.nix { };

  zmk_settings_reset = zmk.override {
    shield = "settings_reset";
  };

  glove80_left = zmk.override {
    board = "glove80_lh";
  };

  glove80_right = zmk.override {
    board = "glove80_rh";
  };

  glove80_combined = combine_uf2 glove80_left glove80_right;

  glove80_v0_left = zmk.override {
    board = "glove80_v0_lh";
  };

  glove80_v0_right = zmk.override {
    board = "glove80_v0_rh";
  };
})
