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
        nativeBuildInputs = with pkgs; [ninja cmake rustToolchain];
        buildInputs = with pkgs; [python313 python313Packages.pyelftools];
        packages = with pkgs; [
          alejandra
          mdformat
          python313Packages.mdformat-gfm
          cargo-expand
          llvmPackages_20.clang-tools
          llvmPackages_20.clang
          llvmPackages_20.clang-unwrapped
          llvmPackages_20.lldb
          neocmakelsp
          basedpyright
          ruff
          rustup
          xxd
          just
          gersemi
          python313Packages.pytest
          python313Packages.pytest-xdist
        ];
      in
        with pkgs; {
          devShells.default = mkShell {
            inherit buildInputs nativeBuildInputs packages;
          };
        }
    );
}
