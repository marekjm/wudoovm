let
    nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/tarball/nixos-23.11";
    pkgs = import nixpkgs { config = {}; overlays = []; };
in

pkgs.mkShellNoCC {
    packages = with pkgs; [
        gcc13
        gnumake
        python3Minimal
        python311Packages.colored
        liburing
        libuuid
        openssl
        git
    ];
}
