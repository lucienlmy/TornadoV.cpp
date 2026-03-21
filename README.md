# TornadoV.cpp (TornadoV++)

A high-performance, native C++ rewrite of the TornadoV mod for Grand Theft Auto V. Marketed as **TornadoV++**, this project is a complete overhaul of the original C# implementation.

##  Key Features

- **Native Performance**: Built from the ground up in C++ to minimize frame time impact and maximize execution efficiency.
- **Seamless UI**: Built-in menu system for real-time configuration of vortex strength, movement, and visual styles.
- ...and all other features from TornadoV's C# version.

##  Technical Improvements (vs. C# Version)

- **Frame-Sync Logic**: Uses `GAMEPLAY::GET_FRAME_TIME()` and `GAMEPLAY::GET_GAME_TIMER()` to ensure consistent simulation regardless of frame rate.
- **Memory Management**: Utilizes modern C++ smart pointers and RAII principles to prevent memory leaks and script crashes.
- **Optimized Math**: Custom `MathEx` class for high-performance Vector3 and Quaternion operations, ensuring 1:1 parity with the original's movement logic while running at native speeds.

##  Installation

1. Ensure you have [ScriptHookV](http://www.dev-c.com/gtav/scripthookv/) installed.
2. Place `TornadoV.asi` into your GTA V main directory.
3. Launch the game once for the mod to create .ini file
4. (Optional) Customize settings in the `TornadoV.ini` file.

##  Controls

- **Spawn/Despawn hotkey(Default:F6)**: Toggle Tornado (Spawn/Despawn)
- **Menu Key (Default: F5)**: Open the TornadoV++ configuration menu.

##  Building from Source

- **Requirements**: Visual Studio 2022 (with C++ Desktop Development workload).
- **SDK**: ScriptHookV SDK.
- **Configuration**: Target `Release | x64` for the optimized ASI build.

## ⚖️ Credits

- **Dependencies**: Alexander Blade (ScriptHookV)

---
*Note: This project is intended for Single Player use only.*
