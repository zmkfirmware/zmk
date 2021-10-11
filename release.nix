{ pkgs ? (import ./nix/pinned-nixpkgs.nix {}), revision ? "HEAD" }:

let
  lib = pkgs.lib;
  zmkPkgs = (import ./default.nix { inherit pkgs; });
  lambda  = (import ./lambda { inherit pkgs; });
  ccacheWrapper = pkgs.callPackage ./nix/ccache.nix {};

  nix-utils = pkgs.fetchFromGitHub {
    owner = "iknow";
    repo = "nix-utils";
    rev = "c13c7a23836c8705452f051d19fc4dff05533b53";
    sha256 = "0ax7hld5jf132ksdasp80z34dlv75ir0ringzjs15mimrkw8zcac";
  };

  ociTools = pkgs.callPackage "${nix-utils}/oci" {};

  inherit (zmkPkgs) zmk zephyr;

  accounts = {
    users.deploy = {
      uid = 999;
      group = "deploy";
      home = "/home/deploy";
      shell = "/bin/sh";
    };
    groups.deploy.gid = 999;
  };

  baseLayer = {
    name = "base-layer";
    path = [ pkgs.busybox ];
    entries = ociTools.makeFilesystem {
      inherit accounts;
      tmp = true;
      usrBinEnv = "${pkgs.busybox}/bin/env";
      binSh = "${pkgs.busybox}/bin/sh";
    };
  };

  depsLayer = {
    name = "deps-layer";
    path = [ pkgs.ccache ];
    includes = zmk.buildInputs ++ zmk.nativeBuildInputs ++ zmk.zephyrModuleDeps;
  };

  zmkCompileScript = let
    zmk' = zmk.override {
      gcc-arm-embedded = ccacheWrapper.override {
        unwrappedCC = pkgs.gcc-arm-embedded;
      };
    };
    zmk_glove80_rh = zmk.override { board = "glove80_rh"; };
    realpath_coreutils = if pkgs.stdenv.isDarwin then pkgs.coreutils else pkgs.busybox;
  in pkgs.writeShellScriptBin "compileZmk" ''
    set -eo pipefail

    if [ ! -f "$1" ]; then
      echo "Usage: compileZmk [file.keymap]" >&2
      exit 1
    fi

    KEYMAP="$(${realpath_coreutils}/bin/realpath $1)"

    export PATH=${lib.makeBinPath (with pkgs; zmk'.nativeBuildInputs ++ [ ccache ])}:$PATH
    export CMAKE_PREFIX_PATH=${zephyr}

    export CCACHE_BASEDIR=$PWD
    export CCACHE_NOHASHDIR=t
    export CCACHE_COMPILERCHECK=none

    if [ -n "$DEBUG" ]; then ccache -z; fi

    cmake -G Ninja -S ${zmk'.src}/app ${lib.escapeShellArgs zmk'.cmakeFlags} "-DUSER_CACHE_DIR=/tmp/.cache" "-DKEYMAP_FILE=$KEYMAP" -DBOARD=glove80_lh

    ninja

    if [ -n "$DEBUG" ]; then ccache -s; fi

    cat zephyr/zmk.uf2 ${zmk_glove80_rh}/zmk.uf2 > zephyr/combined.uf2
  '';

  ccacheCache = pkgs.runCommandNoCC "ccache-cache" {
    nativeBuildInputs = [ zmkCompileScript ];
  } ''
    export CCACHE_DIR=$out

    mkdir /tmp/build
    cd /tmp/build

    compileZmk ${zmk.src}/app/boards/arm/glove80/glove80.keymap
  '';

  entrypoint = pkgs.writeShellScriptBin "entrypoint" ''
    set -euo pipefail

    if [ ! -d "$CCACHE_DIR" ]; then
      cp -r ${ccacheCache} "$CCACHE_DIR"
      chmod -R u=rwX,go=u-w "$CCACHE_DIR"
    fi

    if [ ! -d /tmp/build ]; then
      mkdir /tmp/build
    fi

    exec "$@"
  '';

  startLambda = handler: pkgs.writeShellScriptBin "startLambda" ''
    set -euo pipefail
    export PATH=${lib.makeBinPath [ zmkCompileScript ]}:$PATH
    cd ${lambda.source}
    ${lambda.bundleEnv}/bin/bundle exec aws_lambda_ric "app.LambdaFunction::${handler}.process"
  '';

  simulateLambda = lambda: pkgs.writeShellScriptBin "simulateLambda" ''
    ${pkgs.aws-lambda-rie}/bin/aws-lambda-rie ${lambda}/bin/startLambda
  '';

  lambdaImage = lambda:
  let
    appLayer = {
      name = "app-layer";
      path = [ lambda zmkCompileScript ];
    };
  in
  ociTools.makeSimpleImage {
    name = "zmk-builder-lambda";
    layers = [ baseLayer depsLayer appLayer ];
    config = {
      User = "deploy";
      WorkingDir = "/tmp";
      Entrypoint = [ "${entrypoint}/bin/entrypoint" ];
      Cmd = [ "startLambda" ];
      Env = [ "CCACHE_DIR=/tmp/ccache" "REVISION=${revision}" ];
    };
  };

  # There are two lambda handler functions, depending on whether the lambda is
  # expected to handle Api Gateway/ELB HTTP requests itself.
  startHttpLambda = startLambda "HttpHandler";
  startDirectLambda = startLambda "DirectHandler";
  httpLambdaImage   = lambdaImage startHttpLambda;
  directLambdaImage = lambdaImage startDirectLambda;

  simulateDirectLambda = simulateLambda startDirectLambda;
  simulateHttpLambda   = simulateLambda startHttpLambda;
in {
  inherit httpLambdaImage directLambdaImage zmkCompileScript ccacheCache;

  # nix shell -f release.nix simulateDirectLambda -c simulateLambda
  inherit simulateHttpLambda simulateDirectLambda;
}
