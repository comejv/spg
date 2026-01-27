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
