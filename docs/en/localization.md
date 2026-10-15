# Localization System Guide (`localization.md`)

<p align="center">
  <b>English</b> • <a href="../ru/localization.md"><b>Русский</b></a>
</p>

The **DariVN** engine features a built-in internationalization (i18n) system that allows you to translate visual novels into multiple languages without duplicating narrative scripts.

Translations are stored in simple **UTF-8** `.ini` files inside `assets/locales/`. Scenarios, choice menus, and character definitions can directly reference these translation keys.

---

## Table of Contents

* [Localization File Format (`.ini`)](#localization-file-format-ini)
* [Using Translation Keys in Scenarios](#using-translation-keys-in-scenarios)
  * [1. Localized Dialogue Lines](#1-localized-dialogue-lines)
  * [2. Localized Choice Options](#2-localized-choice-options)
  * [3. Character Names](#3-character-names)
  * [4. Inline Key Interpolation `{loc=...}`](#4-inline-key-interpolation-loc)
* [Registering Languages in `config.vn`](#registering-languages-in-configvn)
* [Dynamic In-Game Language Switching](#dynamic-in-game-language-switching)
* [Step-by-Step Tutorial: Adding a New Language](#step-by-step-tutorial-adding-a-new-language)
* [Related Documentation](#related-documentation)

---

## Localization File Format (`.ini`)

Translation files must be encoded in **UTF-8** and placed in `assets/locales/` (e.g. `en.ini`, `ru.ini`, `uk.ini`).

Key-value pairs are declared using `key = value`:

```ini
# assets/locales/en.ini

# Character names
char.dari = {color=cyan}Dari{/color}
char.shadow = {color=#9ca3af}Mysterious Shadow{/color}

# User Interface
menu.new_game = New Game
menu.load = Load Game
menu.settings = Settings
menu.exit = Exit

# Narrative Dialogue
story.welcome = Hello! Glad to meet you in DariVN!
story.features = We support dynamic {color=red}c{/color}{color=yellow}o{/color}{color=green}l{/color}{color=cyan}o{/color}{color=blue}r{/color} and {b}bold styling{/b}!

# Branching choices
choice.roof_chapter2 = Head to the school rooftop at sunset
choice.stay_class = Stay inside the warm classroom
```

*Values can contain any markup formatting tags: `{b}`, `{i}`, `{color=...}`, `{scale=...}`, `{w=...}`.*

---

## Using Translation Keys in Scenarios

### 1. Localized Dialogue Lines
Instead of hardcoding text into one language, prefix the key with `@`:

```ini
# Character speech referencing a key:
dari @story.welcome

# Narrator speech referencing a key:
@story.narration_intro
```

### 2. Localized Choice Options
In `choice:` blocks, keys can be used directly for option text:

```ini
choice:
    @choice.roof_chapter2 -> script "assets/scripts/chapter2.vn"
    @choice.stay_class -> classroom_scene
```

### 3. Character Names
In character declarations (`assets/characters/dari.vn`):
```ini
id = dari
name = @char.dari
```
When the player switches between English, Russian, or Ukrainian, character names in dialogue boxes update automatically!

### 4. Inline Key Interpolation `{loc=...}`
To insert a localized word within a sentence:

```ini
dari "You have met two people: {loc=char.dari} and {loc=char.shadow}!"
```

---

## Registering Languages in `config.vn`

In `assets/config.vn`, define the default language and paths to `.ini` files:

```ini
locale.default = "en"
locale.en = "assets/locales/en.ini"
locale.ru = "assets/locales/ru.ini"
locale.uk = "assets/locales/uk.ini"
```

If a translation key is missing in a specific language, DariVN automatically falls back to the string in `locale.default`, preventing missing text or crashes.

---

## Dynamic In-Game Language Switching

* **Title Screen:** The language toggle button cycles through available languages instantly.
* **Settings Menu (`SettingsState`):** The language selector updates the UI in real time.
* Selected language preferences are saved in `saves/settings.vn`.

---

## Step-by-Step Tutorial: Adding a New Language

Let's add support for **French (`fr`)**.

### Step 1. Create `assets/locales/fr.ini`
Copy `assets/locales/en.ini` to `assets/locales/fr.ini` and translate:
```ini
char.dari = {color=cyan}Dari{/color}
menu.new_game = Nouvelle Partie
menu.load = Charger
menu.settings = Paramètres
menu.exit = Quitter
story.welcome = Bonjour ! Bienvenue sur DariVN !
```

### Step 2. Register in `assets/config.vn`
Add the following line to `assets/config.vn`:
```ini
locale.fr = "assets/locales/fr.ini"
```

The engine will immediately include French in all in-game menus!

---

## Related Documentation

* [Documentation Index](README.md)
* [Writing Scenarios](scenario.md)
* [Character Configuration](characters.md)
* [Engine Configuration](configuration.md)
