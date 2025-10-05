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
          pkgs.python313Packages.hatchling
        ];

        # propagated build inputs
        parser-propagatedbuildInputs = with pkgs; [
          python313Packages.pyelftools
        ];

        packages = with pkgs; [
          alejandra
          mdformat
          python313Packages.mdformat-gfm
          cargo-expand
          llvmPackages_20.clang-tools
          llvmPackages_20.clang
          llvmPackages_20.clang-unwrapped
          llvmPackages_20.lldb
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
          python313
          python313Packages.pytest
          python313Packages.twine
          python313Packages.wheel
          python313Packages.pytest-xdist
        ];
      in let
        parserMeta = builtins.fromTOML (builtins.readFile ./parser/pyproject.toml);
        rustMeta = builtins.fromTOML (builtins.readFile ./rust/Cargo.toml);
      in
        with pkgs; {
          devShells.default = mkShell {
            nativeBuildInputs =
              parser-nativeBuildInputs
              ++ c-nativeBuildInputs;
            propagatedBuildInputs = parser-propagatedbuildInputs;
            inherit packages;
          };

          packages.parser = pkgs.python313Packages.buildPythonPackage {
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
