# DariVN Engine Documentation 🌸

<p align="center">
  <b>English</b> • <a href="../ru/README.md"><b>Русский</b></a>
</p>

Welcome to the official technical documentation for the **DariVN** 2D visual novel engine!

**DariVN** is a modern, lightweight, and high-performance native 2D visual novel engine built with **C++20** and **OpenGL 3.3 Core Profile**, leveraging **miniaudio**, **GLFW**, and **stb** libraries.

All visual novel narrative logic, character configurations, and engine settings are declarative and stored in plain text files with the `.vn` extension, complemented by custom GLSL shaders.

---

## 📚 Documentation Modules

The documentation is organized into modular guides with cross-references:

1. [**Writing Scenarios (`scenario.md`)**](scenario.md)
   * Syntax of the `.vn` narrative scripting language, character speech, and narrator lines.
   * Rich-text formatting (colors, styles, fonts, typewriter pauses).
   * Dialogue presentation styles: classic bottom dialogue box (**ADV**) and full-screen novel mode (**NVL**).
   * Script variables, math/logical expressions, and conditional branches (`if / else`).
   * Interactive choices (`choice:`) with condition checks, lock, and disable modes.
   * Scene management, shader transitions (`with fade / dissolve / crossfade`), and jumps (`label`, `jump`).
   * Procedural sprite micro-animations (`hop`, `shake`, `flash`) and smooth movement (`move`).

2. [**Character Configuration (`characters.md`)**](characters.md)
   * Structure and syntax of character declaration files in `assets/characters/<id>.vn`.
   * Declaring expressions (`expr.<name>`) and sprite paths.
   * Scale overrides, Z-ordering (rendering layers), and screen positioning (slots vs. absolute coordinates).
   * Built-in sprite shader effects: silhouettes, color tinting (`tint`), sepia, and flashes.
   * Attaching custom GLSL shaders directly to character sprites.

3. [**Shaders & Special Effects (`shaders.md`)**](shaders.md)
   * Graphics pipeline architecture (OpenGL 3.3 Core, Framebuffers, GLSL 330 core).
   * Shader configuration files in `assets/shaders/<id>.vn`.
   * Writing custom GLSL fragment shaders for scene backgrounds and sprites (detailed walkthrough of `wave`, `glitch`, and `art` shaders).
   * Passing and modifying uniform parameters dynamically from scripts in real time.
   * Built-in screen post-processing effects: vignette (`vignette`), blur (`blur`), sepia (`sepia`), tint (`tint`).
   * Hardware-accelerated scene transitions (`fade`, `dissolve`, `crossfade`).

4. [**Audio System (`audio.md`)**](audio.md)
   * Multi-bus audio architecture using miniaudio: `Master`, `BGM`, `SFX`, `Voice`, `Ambient`.
   * Script commands: `play/stop music`, `play sound`, `play voice`, `play/stop ambient`.
   * Smooth volume transitions (`fade`), volume levels (`volume`), and looping (`loop / noloop`).
   * Automatic crossfading between background music tracks.
   * Cinematic Audio Ducking during pause menus and dialogue backlog inspection.

5. [**Engine Configuration (`configuration.md`)**](configuration.md)
   * Global configuration file `assets/config.vn`.
   * Window and graphics parameters (resolution, window title, VSync, fullscreen).
   * Vector TrueType font loading and typography settings.
   * Default audio levels, music crossfade duration, and UI sound effects.
   * User override settings stored in `saves/settings.vn`.

6. [**Localization System (`localization.md`)**](localization.md)
   * Structure of `.ini` locale files in `assets/locales/` (UTF-8).
   * Referencing translation keys in scripts (`@story.key` and `{loc=char.name}`).
   * Dynamic in-game language switching.

7. [**Saves, Persistent Data & Architecture (`saves_and_architecture.md`)**](saves_and_architecture.md)
   * Save game serialization (JSON) and automatic preview thumbnail generation (PNG).
   * Save slots and instant QuickSave / QuickLoad (`F5` / `F9`).
   * Global persistent variables (`$global.*` and `$persistent.*`) stored in `persistent.json`.
   * Dialogue history window (Backlog).
   * Finite State Machine architecture (`StateManager`, `GameplayState`, `MainMenuState`, etc.).

8. [**Asset Packer & VFS (`packer.md`)**](packer.md)
   * Official `darivn-packer` CLI utility for packaging assets into `game.pak`.
   * DEFLATE compression, end-to-end XOR encryption, and CRC32 integrity validation.
   * Virtual File System (VFS) integration and automatic package mounting.
   * Developer Mode (`--dev`) for hot-reloading loose disk assets during production.

---

## 🗺️ Project Architecture Overview

```text
DariVN/
├── assets/                  # Game assets and configurations
│   ├── audio/              # Music (bgm), sound effects (sfx), ambient tracks
│   │   ├── ambient/
│   │   ├── bgm/
│   │   └── sfx/
│   ├── characters/         # Character declarations (.vn)
│   ├── fonts/              # Vector TrueType fonts (.ttf)
│   ├── locales/            # Localization files (.ini: ru.ini, en.ini, uk.ini)
│   ├── scripts/            # Story chapters and dialogue scripts (.vn)
│   ├── shaders/            # Shader configs (.vn) and GLSL source files (.vert, .frag)
│   │   └── custom/         # Custom user-defined fragment shaders
│   ├── textures/           # Backgrounds (.jpg, .png), character sprites, UI graphics
│   └── config.vn           # Global engine configuration file
├── docs/                    # Complete project documentation
│   ├── en/                 # English documentation
│   └── ru/                 # Russian documentation
├── saves/                   # Player save files, screenshots, and persistent.json
├── src/                     # C++20 engine source code
├── tools/                   # Engine companion utilities
│   └── darivn-packer/       # Source code for asset packaging CLI tool
└── CMakeLists.txt          # CMake build configuration
```

---

## ⚡ Quick Start

1. Tweak your game parameters in [assets/config.vn](configuration.md).
2. Define characters in [assets/characters/](characters.md).
3. Write your first story chapter in [assets/scripts/](scenario.md).
4. Add atmospheric post-processing and shaders in [assets/shaders/](shaders.md).
5. Build your standalone release archive using [darivn-packer](packer.md)!

