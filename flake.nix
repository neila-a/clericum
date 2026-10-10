{
    description = "A FUSE-based file backup and virtual filesystem tool.";

    inputs = {
        #nixpkgs.url = "github:NixOS/nixpkgs";
        nixpkgs.url = "git+https://mirrors.nju.edu.cn/git/nixpkgs.git?ref=nixpkgs-unstable&shallow=1";
        #flake-utils.url = "github:numtide/flake-utils";
        flake-utils.url = "https://proxy.gitwarp.top/https://github.com/numtide/flake-utils/archive/refs/heads/master.zip";

        bash-completor = {
            url = "https://proxy.gitwarp.top/https://github.com/adoyle-h/bash-completor/releases/download/v0.2.0/bash-completor";
            flake = false;
        };
    };

    outputs =
        {
            self,
            nixpkgs,
            bash-completor,
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
                # 用于在 postInstall 中引用源码树（Nix 求值期即可确定其 store 路径）
                srcPath = ./.;
            in
            {
                packages = {
                    default = pkgs.stdenv.mkDerivation {
                        pname = name;
                        version = version;
                        src = srcPath;

                        nativeBuildInputs = with pkgs; [
                            cmake
                            ninja
                            pkg-config
                            qt6.wrapQtAppsHook

                            gnused # for bash-completor
                            # bash-completor 内部依赖 compgen 内建命令，而普通 bash
                            # 不提供该内建（仅 bash-interactive 提供），故用其运行生成器。
                            bashInteractive
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

                        # 不覆盖默认 buildPhase（否则 C++ 不会被编译），
                        # 改为在 postInstall 中生成 bash 补全并安装。
                        # 生成器会按「配置所在目录」解析 output，且会回写产物，
                        # 因此把配置复制到可写目录并把 output 重定向到该目录，
                        # 避免直接读取只读的 nix 源码树。
                        postInstall = ''
                            cp ${bash-completor} ./bash-completor
                            chmod +x ./bash-completor
                            sed -i '1d' ./bash-completor

                            mkdir -p .clericum-completion
                            cp ${srcPath}/src/cli/completor.bash .clericum-completion/completor.bash
                            # 把输出路径改到可写目录（与配置同目录）
                            sed -i 's|^output=.*|output="clericum_cli"|' .clericum-completion/completor.bash

                            ${pkgs.bashInteractive}/bin/bash ./bash-completor -c .clericum-completion/completor.bash
                            mkdir -p $out/share/bash-completion/completions
                            mv .clericum-completion/clericum_cli $out/share/bash-completion/completions/clericum_cli
                        '';

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
