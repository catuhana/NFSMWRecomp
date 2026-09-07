{
  description = "A Nix flake for the Need for Speed: Most Wanted (2005) recompilation project.";

  inputs = {
    self.submodules = true;

    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };

    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    { flake-parts, treefmt-nix, ... }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
      ];

      imports = [ treefmt-nix.flakeModule ];

      perSystem =
        { pkgs, ... }:
        let
          llvm = pkgs.llvmPackages_23;
          llvmStdenv = pkgs.overrideCC llvm.stdenv (llvm.clangUseLLVM.override { inherit (llvm) bintools; });

          nativeBuildInputs = [
            pkgs.cmake
            pkgs.ninja
            pkgs.pkg-config
            pkgs.python3
          ];

          buildInputs = [
            # SDL3 dependencies
            pkgs.alsa-lib
            pkgs.dbus
            pkgs.fribidi
            pkgs.hidapi
            pkgs.ibus
            pkgs.jack2
            pkgs.libdecor
            pkgs.libdrm
            pkgs.libgbm
            pkgs.libGL
            pkgs.libpulseaudio
            pkgs.libthai
            pkgs.libunwind
            pkgs.libusb1
            pkgs.libx11
            pkgs.libxcb
            pkgs.libxcursor
            pkgs.libxext
            pkgs.libxfixes
            pkgs.libxi
            pkgs.libxinerama
            pkgs.libxkbcommon
            pkgs.libxrandr
            pkgs.libxrender
            pkgs.libxscrnsaver
            pkgs.libxtst
            pkgs.mesa
            pkgs.pipewire
            pkgs.sndio
            pkgs.systemd
            pkgs.vulkan-headers
            pkgs.vulkan-loader
            pkgs.wayland
            pkgs.wayland-protocols

            # nfde dependencies
            #pkgs.xdg-desktop-portal
          ];

          gameXex =
            let
              env = builtins.getEnv "NFSMW_GAME_XEX";
            in
            if env == "" then
              null
            else
              builtins.path {
                path = env;
                name = "default.xex";
              };
        in
        {
          packages = import ./nix/packages.nix {
            inherit
              llvmStdenv
              nativeBuildInputs
              buildInputs

              gameXex
              ;

            inherit (pkgs)
              fetchFromGitHub
              nix-gitignore
              ;
          };

          devShells = import ./nix/devshells.nix {
            inherit
              llvmStdenv
              nativeBuildInputs
              buildInputs
              ;

            inherit (pkgs)
              mkShell

              nixfmt
              nixd
              ;
          };

          treefmt = import ./nix/treefmt.nix { };
        };
    };

  nixConfig = {
    extra-experimental-features = [ "flake-self-attrs" ];
  };
}
