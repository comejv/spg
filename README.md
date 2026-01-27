# Schwarzschild Geodesic Simulation (Spg)

A real-time visual simulation of particle trajectories (geodesics) in Schwarzschild spacetime around a black hole. It supports both massive particles and photons using RK4 integration.

## Building

This project uses `make` and depends on [Raylib](https://www.raylib.com/).

```bash
make
./main
```

If you are using Nix, a `shell.nix` is provided:

```bash
nix-shell --run "make && ./main"
```

## Memory Leak Detection

The project uses a suppression file (`lsan.supp`) to ignore known false positives from external libraries (GTK, libglfw, etc.) when running with LeakSanitizer.

When entering the development environment via `nix-shell`, the `LSAN_OPTIONS` environment variable is automatically set to use this suppression file. Otherwise, you can set it manually:

```bash
export LSAN_OPTIONS=suppressions=$(pwd)/lsan.supp
```

## Controls

- **Camera:**
    - **Orbit:** Left Mouse Button or Middle Mouse Button + Drag
    - **Zoom:** Mouse Wheel
- **Simulation:**
    - **Pause/Resume:** SPACE
    - **Reset:** R
- **Particle Control:**
    - **Select Particle:** TAB
    - **Radial Momentum ($p_r$):** Q / W
    - **Angular Momentum ($L$):** A / S
