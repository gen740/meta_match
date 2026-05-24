{
  description = "Flake shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    inputs@{ flake-parts, nixpkgs, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = nixpkgs.lib.platforms.all;

      perSystem =
        { pkgs, ... }:
        {
          devShells.default = pkgs.mkShellNoCC {
            packages = [
              pkgs.pkg-config
              pkgs.python3Packages.venvShellHook
              pkgs.llvmPackages_22.libcxxClang
              pkgs.llvmPackages_22.llvm
              pkgs.glaze
              pkgs.hyperfine
              (pkgs.libcxxStdenv.mkDerivation rec {
                pname = "google-benchmark";
                version = "1.9.5";

                src = pkgs.fetchFromGitHub {
                  owner = "google";
                  repo = "benchmark";
                  rev = "v${version}";
                  hash = "sha256-Mm4pG7zMB00iof32CxreoNBFnduPZTMp3reHMCIAFPQ=";
                };

                nativeBuildInputs = [
                  pkgs.cmake
                ];

                cmakeFlags = [
                  "-DBENCHMARK_ENABLE_TESTING=OFF"
                  "-DBENCHMARK_ENABLE_GTEST_TESTS=OFF"
                  "-DBENCHMARK_ENABLE_INSTALL=ON"
                ];

                meta = {
                  description = "A microbenchmark support library";
                  homepage = "https://github.com/google/benchmark";
                  license = pkgs.lib.licenses.asl20;
                  platforms = pkgs.lib.platforms.unix;
                };
              })
            ];
            venvDir = "venv";
            NIX_ENFORCE_NO_NATIVE = "0";
          };

          packages.default = pkgs.stdenv.mkDerivation {
            name = "hello";
            src = ./.;
            buildInputs = [ ];
            installPhase = ''
              mkdir -p $out/bin
              echo "Hello, world!" > $out/bin/hello
              chmod +x $out/bin/hello
            '';
          };
        };
    };
}
