# Asset Packer & VFS Guide (`packer.md`)

<p align="center">
  <b>English</b> • <a href="../ru/packer.md"><b>Русский</b></a>
</p>

The **DariVN** engine includes a built-in Virtual File System (**VFS**) and an official companion command-line utility called **`darivn-packer`**. It packages all visual novel assets (scripts, backgrounds, character sprites, audio tracks, fonts, and shaders) into a compact and secure `game.pak` archive featuring **DEFLATE** compression, **CRC32** integrity checksums, and optional stream encryption.

---

## Table of Contents

* [Why Use the Packer?](#why-use-the-packer)
* [Building `darivn-packer`](#building-darivn-packer)
* [Packer CLI Commands](#packer-cli-commands)
  * [1. Packaging a Directory (`pack`)](#1-packaging-a-directory-pack)
  * [2. Listing Contents (`list`)](#2-listing-contents-list)
  * [3. Verifying Integrity (`verify`)](#3-verifying-integrity-verify)
  * [4. Extracting Archives (`unpack`)](#4-extracting-archives-unpack)
* [Command-Line Options & Flags](#command-line-options--flags)
* [Engine Integration & Virtual File System (VFS)](#engine-integration--virtual-file-system-vfs)
  * [Automatic Mounting of `game.pak`](#automatic-mounting-of-gamepak)
  * [Developer Mode (`--dev`)](#developer-mode---dev)
* [Archive Binary Format (`DVNP`)](#archive-binary-format-dvnp)
* [Step-by-Step Tutorial: Preparing a Release Build](#step-by-step-tutorial-preparing-a-release-build)
* [Related Documentation](#related-documentation)

---

## Why Use the Packer?

1. **Single-File Distribution:** Thousands of small audio clips, scenario scripts, and sprite textures are unified into a single `game.pak` file. You only need to ship the native executable and `game.pak`.
2. **Data Compression (DEFLATE / zlib):** Narrative scripts (`.vn`), shader sources, translations, and uncompressed textures shrink significantly (typically saving 30–70% of space).
3. **Spoiler Protection:** Optional encryption with a key prevents players from casually browsing scenario files or discovering plot twists early.
4. **Data Integrity (CRC32):** Every packaged file is protected with a CRC32 checksum to detect corrupted downloads or unauthorized file tampering.
5. **Seamless In-Memory Streaming:** The `DariVN::VFS` subsystem loads assets directly from the mounted `.pak` into memory without extracting files to the user's hard drive.

---

## Building `darivn-packer`

`darivn-packer` is integrated into the CMake build system:

```bash
# Build the project (including darivn-packer)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target darivn-packer
```

The resulting binary will be located at:
* **macOS / Linux:** `build/darivn-packer`
* **Windows:** `build/Release/darivn-packer.exe`

---

## Packer CLI Commands

General syntax:
```bash
darivn-packer <command> [arguments] [options]
```

---

### 1. Packaging a Directory (`pack`)

Packs an entire asset folder into a `.pak` package:

```bash
# Standard packaging of assets/ into game.pak:
./build/darivn-packer pack assets game.pak

# Packaging with end-to-end key encryption:
./build/darivn-packer pack assets game.pak --key "MySecretVisualNovelKey2026"

# Packaging without compression (STORE mode, ideal for pre-compressed media):
./build/darivn-packer pack assets game.pak --no-compress
```

*Note:* When packing a folder named `assets`, the utility automatically prefixes inner entry paths with `assets/` (e.g. `assets/scripts/demo.vn`), guaranteeing that all existing paths in scripts and configs work out of the box!

---

### 2. Listing Contents (`list`)

Displays a table of all files inside the archive with their uncompressed/compressed sizes, compression method, and encryption flag:

```bash
./build/darivn-packer list game.pak

# If the archive is encrypted:
./build/darivn-packer list game.pak --key "MySecretVisualNovelKey2026"
```

Example output:
```text
Archive: game.pak
Files:   42
--------------------------------------------------------------------------------
Path                                      Orig Size    Comp Size  Method   Enc
--------------------------------------------------------------------------------
assets/config.vn                            1.04 KB       480 B  DEFLATE    NO
assets/scripts/demo.vn                     10.25 KB      3.12 KB  DEFLATE    NO
assets/textures/backgrounds/rooftop.jpg   450.20 KB    448.10 KB  DEFLATE    NO
assets/audio/bgm/theme.mp3                  3.20 MB      3.18 MB  DEFLATE    NO
--------------------------------------------------------------------------------
Total: 18.40 MB -> 15.20 MB
```

---

### 3. Verifying Integrity (`verify`)

Decompresses and validates every file against its recorded CRC32 checksum:

```bash
./build/darivn-packer verify game.pak

# For encrypted archives:
./build/darivn-packer verify game.pak --key "MySecretVisualNovelKey2026"
```

Output on success:
```text
[VERIFY] Verifying 42 files in 'game.pak'...
[VERIFY] OK! All files and CRC32 checksums are valid.
```

---

### 4. Extracting Archives (`unpack`)

Extracts all files from the package into a destination directory:

```bash
# Extract game.pak into unpacked_assets/ folder:
./build/darivn-packer unpack game.pak unpacked_assets/

# Verbose extraction:
./build/darivn-packer unpack game.pak unpacked_assets/ --verbose

# For encrypted archives:
./build/darivn-packer unpack game.pak unpacked_assets/ --key "MySecretVisualNovelKey2026"
```

---

## Command-Line Options & Flags

| Flag | Short | Description |
|---|---|---|
| `--key <string>` | `-k <string>` | Secret encryption key. Encrypts both the Table of Contents (TOC) and file payloads with a streaming XOR cipher. |
| `--no-compress` | — | Disables DEFLATE compression (`STORE` mode). Files are stored uncompressed for maximum decompression speed. |
| `--prefix <string>` | — | Explicitly sets a path prefix inside the archive (e.g. `--prefix assets`). |
| `--verbose` | `-v` | Outputs detailed progress information for each processed file. |

---

## Engine Integration & Virtual File System (VFS)

The `DariVN::VFS` subsystem is embedded directly in the engine and initializes at startup in `main()`.

### Automatic Mounting of `game.pak`
Upon launching, DariVN automatically searches for `game.pak` (or `assets.pak`) in:
1. The current working directory (`./game.pak`).
2. The directory containing the game executable.
3. The parent directory of the executable.

When found, the archive is mounted, and all engine loaders (`Texture2D` via `stbi_load_from_memory`, `TextRenderer` fonts, `AudioEngine`, `ScriptParser`, `Config`, `Shader`) seamlessly read resources from memory.

If no `.pak` file is found, the engine outputs a notice and falls back to loading loose files from the physical `assets/` folder.

---

### Developer Mode (`--dev`)

During active visual novel development, repacking `game.pak` after editing every dialogue line is inconvenient. The **Developer Mode** solves this:

```bash
# Launch game in developer mode:
./build/DariVN --dev
# (Aliases: -d, --developer)
```

**Behavior comparison:**
* **Release Mode (default):** VFS checks mounted `.pak` packages first, falling back to disk only if a file is not in the archive.
* **Developer Mode (`--dev`):** VFS **prioritizes physical files on disk**. This allows scriptwriters and artists to hot-reload `.vn` scripts, configs, and textures instantly without rebuilding archives!

---

## Archive Binary Format (`DVNP`)

DariVN packages use a layout designed for instant mounting and fast random access:

```text
+-------------------------------------------------------------------+
| ArchiveHeader (64 bytes):                                         |
|   magic: 'D', 'V', 'N', 'P' (0x504E5644)                          |
|   version: uint32 (Version 1)                                     |
|   flags: uint32 (Bits: FLAG_ENCRYPTED = 1)                        |
|   fileCount: uint32                                               |
|   tocOffset: uint64 (Offset to Table of Contents)                 |
|   tocCompressedSize: uint64                                       |
|   tocUncompressedSize: uint64                                     |
|   reserved: uint8[24] (Reserved for future extensions)            |
+-------------------------------------------------------------------+
| Binary File Payloads (1..N):                                      |
|   [DEFLATE compressed or uncompressed bytes for each file]        |
+-------------------------------------------------------------------+
| Table of Contents (TOC):                                          |
|   For each file entry:                                            |
|     pathLen: uint16, path: char[pathLen]                          |
|     offset: uint64, compressedSize: uint64, uncompressedSize: ... |
|     crc32: uint32, compression: uint16, flags: uint16             |
|   (The entire TOC is compressed with DEFLATE & optional cipher)   |
+-------------------------------------------------------------------+
```

Because the Table of Contents resides at the end of the file, archives can be mounted in fractions of a millisecond by reading only the header and the compressed TOC block.

---

## Step-by-Step Tutorial: Preparing a Release Build

When your visual novel is ready to ship to players:

### Step 1. Compile the Packer Utility
```bash
cmake --build build --config Release --target darivn-packer
```

### Step 2. Pack the `assets/` Directory
```bash
./build/darivn-packer pack assets game.pak
```

### Step 3. Verify Package Integrity
```bash
./build/darivn-packer verify game.pak
```

### Step 4. Test Release Run
Move the compiled `DariVN` binary and `game.pak` into a clean test directory (without an `assets/` folder) and run:
```bash
./DariVN
```
The game will mount `game.pak` instantly and run smoothly with all backgrounds, scripts, audio, and shaders intact!

---

## Related Documentation

* [Documentation Index](README.md)
* [Engine Configuration](configuration.md)
* [Saves & Engine Architecture](saves_and_architecture.md)
* [Writing Scenarios](scenario.md)
