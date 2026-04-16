{
  description = "Clericum - A Qt6 QML application";

  inputs = {
    #nixpkgs.url = "github:NixOS/nixpkgs";
    nixpkgs.url = "git+https://mirrors.nju.edu.cn/git/nixpkgs.git?ref=nixpkgs-unstable&shallow=1";
    #flake-utils.url = "github:numtide/flake-utils";
    flake-utils.url = "https://proxy.gitwarp.top/https://github.com/numtide/flake-utils/archive/refs/heads/master.zip";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      {
        packages = rec {
          default = clericum;

          clericum = pkgs.stdenv.mkDerivation {
            pname = "clericum";
            version = "0.1";
            src = ./.;

            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              qt6.wrapQtAppsHook
            ];

            buildInputs = with pkgs; [
              qt6.qtbase
              qt6.qtdeclarative
            ];

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
              "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
            ];

            installPhase = ''
              runHook preInstall
              ninja install
              runHook postInstall
            '';

          meta = {
            homepage = "https://github.com/neila-a/clericum";
            license = pkgs.lib.licenses.gpl3Plus;
            platforms = pkgs.lib.platforms.linux;
            mainProgram = "clericum";
          };
        };
        };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            cmake
            qt6.qtbase
            qt6.qtdeclarative
            ninja
          ];

          inputsFrom = [ self.packages.${system}.clericum ];
        };
      }
    );

  nixConfig = {
    extra-substituters = [
      "https://cache.nixos.org"
    ];
  };
}
