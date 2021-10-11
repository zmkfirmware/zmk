{ stdenv, lib, makeWrapper, ccache
, unwrappedCC ? stdenv.cc.cc, extraConfig ? "" }:

# copied from ccache in nixpkgs, modified to glob over prefixes. Also doesn't
# pass lib. Why was it passing lib?
stdenv.mkDerivation {
  name = "ccache-links";
  passthru = {
    isClang = unwrappedCC.isClang or false;
    isGNU = unwrappedCC.isGNU or false;
  };
  nativeBuildInputs = [ makeWrapper ];
  buildCommand = ''
    mkdir -p $out/bin

    wrap() {
      local cname="$(basename $1)"
      if [ -x "${unwrappedCC}/bin/$cname" ]; then
        echo "Wrapping $1"
        makeWrapper ${ccache}/bin/ccache $out/bin/$cname \
          --run ${lib.escapeShellArg extraConfig} \
          --add-flags ${unwrappedCC}/bin/$cname
      fi
    }

    wrapAll() {
      for prog in "$@"; do
        wrap "$prog"
      done
    }

    wrapAll ${unwrappedCC}/bin/{*cc,*c++,*gcc,*g++,*clang,*clang++}

    for executable in $(ls ${unwrappedCC}/bin); do
      if [ ! -x "$out/bin/$executable" ]; then
        ln -s ${unwrappedCC}/bin/$executable $out/bin/$executable
      fi
    done
    for file in $(ls ${unwrappedCC} | grep -vw bin); do
      ln -s ${unwrappedCC}/$file $out/$file
    done
  '';
}
