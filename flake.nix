{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    rust-overlay = {
      url = "github:oxalica/rust-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };
  outputs = {
    self,
    nixpkgs,
    flake-utils,
    rust-overlay,
  }:
    flake-utils.lib.eachDefaultSystem
    (
      system: let
        overlays = [(import rust-overlay)];
        pkgs = import nixpkgs {
          inherit system overlays;
        };
        rustToolchain = pkgs.pkgsBuildHost.rust-bin.fromRustupToolchainFile ./rust-toolchain.toml;

        # native build inputs
        c-nativeBuildInputs = with pkgs; [
          cmake
        ];
        parser-nativeBuildInputs = [
          pkgs.python314Packages.hatchling
        ];

        # propagated build inputs
        parser-propagatedbuildInputs = with pkgs; [
          python314Packages.lief
        ];

        packages = with pkgs; [
          bat
          alejandra
          mdformat
          python314Packages.mdformat-gfm
          cargo-expand
          llvmPackages_22.clang-tools
          llvmPackages_22.clang
          llvmPackages_22.clang-unwrapped
          llvmPackages_22.lldb
          ninja
          neocmakelsp
          conan
          rustToolchain
          rustup
          xxd
          just
          gersemi
          basedpyright
          ruff
          python314
          python314Packages.pytest
          python314Packages.twine
          python314Packages.wheel
          python314Packages.pytest-xdist
        ];
      in let
        parserMeta = fromTOML (builtins.readFile ./parser/pyproject.toml);
        rustMeta = fromTOML (builtins.readFile ./rust/Cargo.toml);
      in
        with pkgs; {
          devShells.default = mkShell {
            nativeBuildInputs =
              parser-nativeBuildInputs
              ++ c-nativeBuildInputs;
            propagatedBuildInputs = parser-propagatedbuildInputs;
            inherit packages;
          };

          packages.parser = pkgs.python314Packages.buildPythonPackage {
            pname = parserMeta.project.name;
            version = parserMeta.project.version;
            src = ./parser;
            pyproject = true;
            nativeBuildInputs = parser-nativeBuildInputs;
            propagatedBuildInputs = parser-propagatedbuildInputs;
          };

          packages.rust = pkgs.rustPlatform.buildRustPackage {
            pname = rustMeta.package.name;
            version = rustMeta.package.version;
            src = ./rust;
            cargoLock.lockFile = ./rust/Cargo.lock;
          };

          packages.c = nixpkgs.legacyPackages.${system}.stdenv.mkDerivation {
            pname = "emtrace";
            src = ./c;
            version = "0.1.0";
            nativeBuildInputs = c-nativeBuildInputs;
            buildPhase = ''
              cmake --build .
            '';
            installPhase = ''
              cmake --build . --target install
            '';
          };
          packages.default = packages.parser;
        }
    );
}
