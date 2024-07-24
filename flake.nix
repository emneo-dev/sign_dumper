{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachSystem ["x86_64-linux"] (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      packages = rec {
        sign_dumper = default;
        default = pkgs.stdenvNoCC.mkDerivation rec {
          name = "sign_dumper";

          src = ./.;
          nativeBuildInputs = with pkgs; [
            gcc
            gnumake
          ];

          checkInputs = with pkgs; [
            criterion
          ];

          doCheck = true;
          checkPhase = ''
            make tests_run
          '';

          installPhase = ''
            mkdir -p $out/bin
            install -D ${name} $out/bin/${name} --mode 0755
          '';
        };
      };
    });
}
