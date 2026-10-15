# Engine Configuration Guide (`configuration.md`)

<p align="center">
  <b>English</b> • <a href="../ru/configuration.md"><b>Русский</b></a>
</p>

The **DariVN** engine is designed to be fully configurable without recompiling C++ source code. Core settings — including window resolution, VSync, initial story scripts, font paths, and initial sound levels — are defined in `assets/config.vn`.

User customizations made in the in-game Settings menu are saved separately to `saves/settings.vn` and automatically override default values.

---

## Table of Contents

* [Structure of `assets/config.vn`](#structure-of-assetsconfigvn)
* [Window & Display Parameters](#window--display-parameters)
* [Main Menu Parameters](#main-menu-parameters)
* [Fonts & Typography](#fonts--typography)
* [Localization & Language Setup](#localization--language-setup)
* [Gameplay & Story Settings](#gameplay--story-settings)
* [Audio Parameters](#audio-parameters)
* [User Settings Overrides (`saves/settings.vn`)](#user-settings-overrides-savessettingsvn)
* [Related Documentation](#related-documentation)

---

## Structure of `assets/config.vn`

Configuration entries use simple `key = value` syntax:

```ini
# ==============================================================================
# DariVN Engine Configuration File (config.vn)
# ==============================================================================

window.title = "DariVN - Visual Novel Engine"
window.width = 1280
window.height = 720
window.vsync = true

menu.background = "assets/textures/backgrounds/classroom.jpg"
menu.music = "assets/audio/bgm/menu_theme.mp3"
menu.shader = ""

font.path = "assets/fonts/roboto.ttf"
font.size = 34.0

locale.default = "en"
locale.ru = "assets/locales/ru.ini"
locale.en = "assets/locales/en.ini"
locale.uk = "assets/locales/uk.ini"

story.start_script = "assets/scripts/demo.vn"
story.text_speed = 0.020

audio.master_volume = 1.0
audio.bgm_volume = 0.8
audio.sfx_volume = 1.0
audio.voice_volume = 1.0
audio.ambient_volume = 0.7
audio.bgm_crossfade = 1.5

ui.click_sound = "assets/audio/sfx/click.wav"
```

---

## Window & Display Parameters

| Parameter | Type | Default | Description |
|---|---|---|---|
| `window.title` | `string` | `"DariVN"` | Operating system window title. |
| `window.width` | `int` | `1280` | Virtual rendering width in pixels (16:9 aspect ratio). |
| `window.height` | `int` | `720` | Virtual rendering height in pixels. |
| `window.vsync` | `bool` | `true` | Enables vertical synchronization to eliminate screen tearing. |
| `window.fullscreen` | `bool` | `false` | Launches game in borderless/fullscreen mode. |

---

## Main Menu Parameters

| Parameter | Type | Description |
|---|---|---|
| `menu.background` | `string` | Path to the background image for the title screen (`.jpg` or `.png`). |
| `menu.music` | `string` | Background music track played in the main menu. |
| `menu.shader` | `string` | Optional shader ID applied to the title background (e.g. `art` or `wave`). Leave empty `""` for standard display. |

---

## Fonts & Typography

| Parameter | Type | Description |
|---|---|---|
| `font.path` | `string` | Path to a TrueType font (`.ttf`). Font glyphs are rasterized into an OpenGL texture atlas via `stb_truetype`. |
| `font.size` | `float` | Base rasterized glyph font size in pixels (recommended: `32.0` - `36.0`). |

*Make sure your font contains both Latin and Cyrillic character ranges if you plan to support multilingual visual novels.*

---

## Localization & Language Setup

| Parameter | Type | Description |
|---|---|---|
| `locale.default` | `string` | Default fallback language code (e.g. `"en"`). |
| `locale.<code_lang>` | `string` | Path to the `.ini` translation file for the language. |

Example:
```ini
locale.default = "en"
locale.en = "assets/locales/en.ini"
locale.ru = "assets/locales/ru.ini"
locale.uk = "assets/locales/uk.ini"
```

See [Localization Guide](localization.md) for more details.

---

## Gameplay & Story Settings

| Parameter | Type | Description |
|---|---|---|
| `story.start_script` | `string` | Entry-point script loaded when the player clicks **"New Game"**. |
| `story.text_speed` | `float` | Typewriter delay in seconds per character (e.g. `0.020` = 50 characters per second). Set to `0.0` for instant text display. |
| `gameplay.auto_dim` | `bool` | Automatically dims inactive characters when another character is speaking. |

---

## Audio Parameters

| Parameter | Type | Default | Description |
|---|---|---|---|
| `audio.master_volume` | `float` | `1.0` | Global master audio volume (0.0 — 1.0). |
| `audio.bgm_volume` | `float` | `0.8` | Background music volume. |
| `audio.sfx_volume` | `float` | `1.0` | Sound effects volume. |
| `audio.voice_volume` | `float` | `1.0` | Character voice acting volume. |
| `audio.ambient_volume` | `float` | `0.7` | Environmental ambient loop volume. |
| `audio.bgm_crossfade` | `float` | `1.5` | Default music crossfade duration in seconds. |
| `ui.click_sound` | `string` | `"assets/audio/sfx/click.wav"` | Audio effect played on button clicks. |

See [Audio System Guide](audio.md) for more details.

---

## User Settings Overrides (`saves/settings.vn`)

When the player modifies settings in the in-game options menu (`SettingsState`), the engine:
1. Leaves the original `assets/config.vn` unchanged.
2. Writes user overrides to `saves/settings.vn`.
3. Loads `assets/config.vn` first upon startup, then overlays values from `saves/settings.vn`.

Clicking **"Reset to Defaults"** removes custom overrides.

---

## Related Documentation

* [Documentation Index](README.md)
* [Writing Scenarios](scenario.md)
* [Audio System](audio.md)
* [Localization System](localization.md)
* [Saves & Architecture](saves_and_architecture.md)
