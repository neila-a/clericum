rec {
    description = "A FUSE-based file backup and virtual filesystem tool.";

    inputs = {
        #nixpkgs.url = "github:NixOS/nixpkgs";
        nixpkgs.url = "git+https://mirrors.nju.edu.cn/git/nixpkgs.git?ref=nixpkgs-unstable&shallow=1";
        #flake-utils.url = "github:numtide/flake-utils";
        flake-utils.url = "https://proxy.gitwarp.top/https://github.com/numtide/flake-utils/archive/refs/heads/master.zip";
    };

    outputs =
        {
            self,
            nixpkgs,
            flake-utils,
        }:
        flake-utils.lib.eachDefaultSystem (
            system:
            let
                name = "clericum";
                version = "0.1.0";
                homepage = "https://github.com/neila-a/${name}";
                pkgs = import nixpkgs {
                    inherit system;
                };
            in
            {
                packages = {
                    default = pkgs.stdenv.mkDerivation {
                        pname = name;
                        version = version;
                        src = ./.;

                        nativeBuildInputs = with pkgs; [
                            cmake
                            ninja
                            pkg-config
                            qt6.wrapQtAppsHook
                        ];

                        buildInputs = with pkgs; [
                            qt6.qtbase
                            qt6.qtdeclarative
                            kdePackages.kcoreaddons
                            kdePackages.ki18n
                            kdePackages.kio
                            fuse3
                        ];

                        cmakeFlags = [
                            "-DCMAKE_BUILD_TYPE=Release"
                            "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
                            "-DGLOBAL_PROJECT_NAME=${name}"
                            "-DGLOBAL_PROJECT_VERSION=${version}"
                            "-DGLOBAL_PROJECT_HOMEPAGE=${homepage}"
                        ];

                        installPhase = ''
                            runHook preInstall
                            ninja install
                            runHook postInstall
                        '';

                        meta = {
                            homepage = homepage;
                            license = pkgs.lib.licenses.gpl3Plus;
                            platforms = pkgs.lib.platforms.linux;
                            mainProgram = name + "_cli";
                        };
                    };
                };

                devShells.default = pkgs.mkShell {
                    packages = with pkgs; [
                        cmake
                        ninja
                        pkg-config

                        qt6.qtbase
                        qt6.qtdeclarative
                        kdePackages.kcoreaddons
                        kdePackages.ki18n
                        kdePackages.kio
                        kdePackages.kde-dev-scripts
                        fuse3
                    ];

                    inputsFrom = [ self.packages.${system}.default ];
                };
            }
        );
}
