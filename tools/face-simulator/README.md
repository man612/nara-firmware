# Nara Face Simulator

Desktop simulator for the same parametric LVGL face renderer that Nara Firmware will use on the physical device.

## Why this exists

The browser virtual device is useful for protocol debugging, but HTML/CSS cannot prove that the embedded renderer will work. This simulator compiles Nara's actual C++ face controller and LVGL view on a PC.

## Requirements

Ubuntu / WSL:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build libsdl2-dev
```

## Build

```bash
cmake -S tools/face-simulator -B build/face-simulator -G Ninja
cmake --build build/face-simulator
ctest --test-dir build/face-simulator --output-on-failure
```

Run the visual simulator:

```bash
./build/face-simulator/nara-face-simulator
```

The simulator fetches **LVGL v9.5.0** at configure time so the desktop renderer stays on the same LVGL line as the embedded firmware.

## Design rule

Blinking, idle gaze, breathing, and mouth movement are deterministic local behavior. AI providers only choose high-level state/emotion when needed.
