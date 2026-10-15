# Character Configuration Guide (`characters.md`)

<p align="center">
  <b>English</b> • <a href="../ru/characters.md"><b>Русский</b></a>
</p>

In the **DariVN** engine, characters are modular entities. Their identifiers, localized names, sprite emotions, scales, and default shaders are defined in dedicated `.vn` configuration files located in the `assets/characters/` folder.

---

## Table of Contents

* [Configuration File Structure](#configuration-file-structure)
* [Configuration Parameters](#configuration-parameters)
* [Declaring Expressions & Sprite Paths](#declaring-expressions--sprite-paths)
* [Loading Characters in Script](#loading-characters-in-script)
* [Screen Positioning & Z-Ordering Layers](#screen-positioning--z-ordering-layers)
* [Procedural Animations & Movement](#procedural-animations--movement)
* [Sprite Shader Effects](#sprite-shader-effects)
* [Step-by-Step Tutorial: Adding a New Character](#step-by-step-tutorial-adding-a-new-character)
* [Related Documentation](#related-documentation)

---

## Configuration File Structure

Each character has its own `.vn` declaration file named `<id>.vn` placed in `assets/characters/` (e.g., `assets/characters/dari.vn`).

### Example: `assets/characters/dari.vn`

```ini
# ==============================================================================
# Character Configuration: Dari (assets/characters/dari.vn)
# ==============================================================================

id = dari
name = @char.dari
scale = 1.0

# Expressions and corresponding sprite image textures
expr.normal = "assets/textures/characters/dari/normal.png"
expr.happy = "assets/textures/characters/dari/happy.png"
expr.shock = "assets/textures/characters/dari/shock.png"
expr.sad = "assets/textures/characters/dari/sad.png"
expr.angry = "assets/textures/characters/dari/angry.png"
```

### Example of a Silhouette Character: `assets/characters/shadow.vn`

```ini
# Mysterious shadow figure
id = shadow
name = @char.shadow
scale = 1.0
expr.normal = "assets/textures/characters/shadow/stand.png"
silhouette = true
```

---

## Configuration Parameters

| Parameter | Description | Example |
|---|---|---|
| `id` | Unique string identifier used in narrative scripts. | `id = alice` |
| `name` / `displayName` | Display name shown in dialogue boxes. Can be plain text, a localized string key (`@char.alice`), or styled with color markup. | `name = @char.alice`<br>`name = "{color=pink}Alice{/color}"` |
| `scale` | Base sprite scale multiplier (default: `1.0`). Useful for balancing characters with different original sprite heights. | `scale = 0.9` |
| `silhouette` | Renders the character as a solid silhouette preserving alpha transparency (`true` / `false`). | `silhouette = true` |
| `default_shader` | Identifier of a custom GLSL shader automatically attached to the character upon loading. | `default_shader = glitch` |

---

## Declaring Expressions & Sprite Paths

Expressions are registered using the `expr.<name>` prefix:

```ini
expr.normal = "assets/textures/characters/dari/normal.png"
expr.smile = "assets/textures/characters/dari/smile.png"
expr.blush = "assets/textures/characters/dari/blush.png"
expr.thinking = "assets/textures/characters/dari/thinking.png"
```

Common expression names can also be written without the `expr.` prefix: `normal`, `happy`, `sad`, `angry`, `shock`.

In scenarios, the expression is specified after the character's identifier when calling `show`:
```ini
show dari blush at center
dari "Oh... I wasn't expecting such a kind compliment..."
```

---

## Loading Characters in Script

Load the character before using them in a narrative script:

```ini
# Automatically searches for assets/characters/<id>.vn
load character dari
load character shadow

# Or provide an explicit file path:
load character rival "assets/custom_chars/rival.vn"
```

---

## Screen Positioning & Z-Ordering Layers

### 1. Preset Screen Slots
DariVN provides 3 horizontal slot positions:
* `left` — Left third of the screen
* `center` — Horizontal center
* `right` — Right third of the screen

```ini
show dari at center
show shadow at left
```

### 2. Free Pixel Coordinates (X, Y)
Position sprites precisely on screen in virtual resolution units (1280x720):

```ini
# Coordinates: X = 240, Y = 130
show shadow at 240 130
```

### 3. Rendering Layers (Z-Ordering)
The `layer <N>` attribute specifies sprite depth. Sprites on lower layers render **behind** sprites on higher layers:

```ini
# Shadow figure stands in the background on layer 0:
show shadow at 240 130 layer 0

# Dari stands in the foreground on layer 1:
show dari at center layer 1
```

### 4. Dynamic Scale Override
```ini
show dari at center scale 1.25
```

---

## Procedural Animations & Movement

### Smooth Translation (`move`)
```ini
# Move horizontally to coordinate X=680
move dari to 680

# Move to a named slot
move dari to right

# Move to coordinates X and Y
move dari to 400 150
```

### Joyful Hop (`hop`)
Causes the sprite to bounce vertically with a gentle landing:
```ini
# hop <id> [duration=0.3] [height=25.0]
hop dari 0.3 30.0
dari "Hurray, summer break is finally here!"
```

### Shock / Tremble (`shake`)
Rapid horizontal vibration for fright, surprise, or loud noises:
```ini
# shake <id> [duration=0.4] [intensity=15.0]
shake dari 0.4 18.0
dari "What in the world was that sound?!"
```

### White Flash (`flash`)
Flashes the sprite in solid white and smoothly fades back:
```ini
# flash <id> [duration=0.25]
flash dari 0.25
```

### Hiding Characters (`hide`)
```ini
hide dari
hide shadow
```

---

## Sprite Shader Effects

### Color Tinting (`tint`)
Adjust sprite lighting to match the background mood (e.g., cool night tones or warm sunsets):

```ini
# effect <id> tint <R> <G> <B> [A]
# Cool moonlight (blue tint):
effect dari tint 0.6 0.6 0.95

# Sunset glow (warm red-orange tint):
effect dari tint 1.1 0.8 0.7
```

### Silhouette Mode (`silhouette`)
Turns the character into a mysterious outline silhouette:

```ini
effect dari silhouette true
dari "I'm standing in the shadows..."
effect dari silhouette false
```

### Sepia Filter (`sepia`)
```ini
effect dari sepia 1.0  # Character in vintage sepia tones
```

### Resetting Effects
```ini
effect dari clear
```

### Custom GLSL Shaders on Sprites
Attach any custom shader (such as digital glitch, dissolve, or aura glow) to an individual sprite:

```ini
load shader glitch

# Enable glitch on Dari's sprite:
shader dari glitch
dari "My voice... it's breaking apart..."

# Return sprite to standard rendering:
shader dari clear
```

See [Shaders Guide](shaders.md) for more details.

---

## Step-by-Step Tutorial: Adding a New Character

Let's add a new heroine named **Miya**.

### Step 1. Prepare Texture Files
Place PNG sprite files in the texture folder:
* `assets/textures/characters/miya/normal.png`
* `assets/textures/characters/miya/happy.png`
* `assets/textures/characters/miya/shock.png`

### Step 2. Create the Configuration File
Create `assets/characters/miya.vn`:

```ini
id = miya
name = @char.miya
scale = 0.95

expr.normal = "assets/textures/characters/miya/normal.png"
expr.happy = "assets/textures/characters/miya/happy.png"
expr.shock = "assets/textures/characters/miya/shock.png"
```

### Step 3. Add Localized Names
In `assets/locales/en.ini`:
```ini
char.miya = {color=#f472b6}Miya{/color}
```

In `assets/locales/ru.ini`:
```ini
char.miya = {color=#f472b6}Мия{/color}
```

### Step 4. Use in Script `assets/scripts/demo.vn`
```ini
load character miya

scene "assets/textures/backgrounds/classroom.jpg" with fade 1.0
show miya happy at center layer 1

miya "Hello everyone! I'm the new transfer student."
hop miya 0.3
miya "I hope we can become great friends!"
```

---

## Related Documentation

* [Documentation Index](README.md)
* [Writing Scenarios](scenario.md)
* [Shaders & Special Effects](shaders.md)
* [Localization System](localization.md)
* [Audio System](audio.md)
