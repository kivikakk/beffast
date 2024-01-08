{
  description = "Has Lily had beffast yet?";

  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixpkgs-unstable;
    flake-utils.url = github:numtide/flake-utils;
    arduino-nix.url = github:clerie/arduino-nix?ref=clerie/arduino-env;
    arduino-library-index = {
      url = github:bouk/arduino-indexes/library_index;
      flake = false;
    };
    arduino-package-index = {
      url = github:bouk/arduino-indexes/package_index;
      flake = false;
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    arduino-nix,
    arduino-package-index,
    arduino-library-index,
    ...
  }: let
    overlays = [
      arduino-nix.overlay
      (arduino-nix.mkArduinoPackageOverlay "${arduino-package-index}/package_index.json")
      (arduino-nix.mkArduinoPackageOverlay (builtins.fetchTree {
        type = "file";
        url = "http://raw.githubusercontent.com/SolderedElectronics/Dasduino-Board-Definitions-for-Arduino-IDE/master/package_Dasduino_Boards_index.json";
        narHash = "sha256-qMvkUTEMDOFtNBxFwF7bVmJGRekZW7r/GErSEe5PD7o=";
      }))
      (arduino-nix.mkArduinoLibraryOverlay "${arduino-library-index}/library_index.json")
    ];
  in
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = import nixpkgs {inherit system overlays;};

        arduinoEnv = pkgs.mkArduinoEnv {
          libraries = with pkgs.arduinoLibraries; [
            (arduino-nix.latestVersion InkplateLibrary)
            (arduino-nix.latestVersion NTPClient)
          ];

          packages = with pkgs.arduinoPackages; [
            (arduino-nix.latestVersion platforms.Inkplate_Boards.esp32)
          ];
        };
      in {
        formatter = pkgs.alejandra;

        packages.default = arduinoEnv.buildArduinoSketch {
          name = "beffast";
          src = ./. + "/beffast";
          fqbn = "Inkplate_Boards:esp32:Inkplate6";
        };

        devShells.default = pkgs.mkShell {
          buildInputs = [arduinoEnv];
        };
      }
    );
}
