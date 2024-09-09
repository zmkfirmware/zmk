{
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
            (vscode-with-extensions.override {
              vscodeExtensions = with vscode-extensions; [
                ms-vscode-remote.remote-containers
                ms-azuretools.vscode-docker
              ]
              ++ vscode-utils.extensionsFromVscodeMarketplace [
                {
                  name = "zmk-tools";
                  publisher = "spadin";
                  version = "1.4.0";
                  sha256 = "sha256-f67uOdfZTGdIGNVQuLuF6SeFZqKqBv455GILj36bZy8=";
                }
              ];
            })
            docker
          ];

          shellHook = ''
            if [ -n "$WAYLAND_DISPLAY" ]; then
              echo "Wayland detected."
              code . --enable-features=UseOzonePlatform --ozone-platform=wayland
            else
              code .
            fi
            echo "Docker needs permissions to run as non-root user"
            echo "https://docs.docker.com/engine/install/linux-postinstall/"
            echo "https://nixos.wiki/wiki/Docker"
          '';
        };
      });
    };
}
