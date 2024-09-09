{
  description = "A Nix-flake-based Python development environment";
  inputs.nixpkgs.url = "nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs { inherit system; config.allowUnfree = true; };
      });
    in
    {
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            vscode
            docker
          ];

          shellHook = ''
            if [ -n "$WAYLAND_DISPLAY" ]; then
              echo "Wayland detected."
              code . --enable-features=UseOzonePlatform --ozone-platform=wayland
            else
              code .
            fi
          '';
        };
      });
    };
}
