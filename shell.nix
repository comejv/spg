with import <nixpkgs> { };

mkShell {
  nativeBuildInputs = with pkgs; [
    gcc
    gdb
    clang-tools
    cmake
    cmake-language-server
    bear
    pkg-config
  ];

  buildInputs = with pkgs; [
    libGL

    wayland
    wayland-protocols
    libxkbcommon

    raylib
  ];
}
