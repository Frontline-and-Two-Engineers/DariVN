# Writing Scenarios Guide (`scenario.md`)

<p align="center">
  <b>English</b> • <a href="../ru/scenario.md"><b>Русский</b></a>
</p>

Scenarios in **DariVN** are written in plain text files with the `.vn` extension (by default located in `assets/scripts/`). The scripting language is declarative, intuitive, and designed specifically for visual novel development. It features a complete expression evaluator, interactive branching, variable management, and tight integration with graphic shaders and audio.

---

## Table of Contents

* [Basic Syntax & Comments](#basic-syntax--comments)
* [Dialogue Lines & Narrator Speech](#dialogue-lines--narrator-speech)
* [Rich-Text Formatting & Typewriter Tags](#rich-text-formatting--typewriter-tags)
* [Dialogue Modes: ADV and NVL](#dialogue-modes-adv-and-nvl)
* [Variables & Expressions](#variables--expressions)
* [Conditional Statements (`if / else`)](#conditional-statements-if--else)
* [Scene Backgrounds & Transitions](#scene-backgrounds--transitions)
* [Character Display & Positioning](#character-display--positioning)
* [Procedural Micro-Animations](#procedural-micro-animations)
* [Sprite Visual Effects](#sprite-visual-effects)
* [Interactive Choices (`choice:`)](#interactive-choices-choice)
* [Labels & Flow Control (`label`, `jump`)](#labels--flow-control-label-jump)
* [Audio & Music Playback](#audio--music-playback)
* [Applying Custom Shaders](#applying-custom-shaders)
* [Related Documentation](#related-documentation)

---

## Basic Syntax & Comments

Each line in a `.vn` script represents either a dialogue line, a command, or a variable assignment. Blank lines are ignored.

Comments start with a hash `#` or double slashes `//`:

```ini
# This is a single-line comment
// Double slashes are also supported
$score = 10 # Trailing comment
```

---

## Dialogue Lines & Narrator Speech

### Character Dialogue
To have a character speak, write their identifier followed by the dialogue line in quotes:

```ini
dari "Hello! Welcome to our school rooftop."
```

### Narrator Text (Anonymous Speech)
Lines in quotes without a character identifier are displayed as narrator commentary without a speaker nameplate:

```ini
"Snow was gently falling outside the classroom window..."
"I took a deep breath and opened the heavy wooden door."
```

### Localized Lines
Dialogue can reference translation keys directly using the `@` prefix:

```ini
dari @story.welcome
@story.intro_narration
```

See [Localization Guide](localization.md) for more information.

---

## Rich-Text Formatting & Typewriter Tags

DariVN includes an inline rich-text markup system enclosed in curly braces `{...}`:

| Tag | Description | Example |
|---|---|---|
| `{b}...{/b}` | Bold text | `{b}Important Warning!{/b}` |
| `{i}...{/i}` | Italic text | `{i}He murmured to himself...{/i}` |
| `{color=#HEX}...{/color}` | Custom text color in hex format | `{color=#38bdf8}Sky blue text{/color}` |
| `{color=name}...{/color}` | Named color (`red`, `green`, `blue`, `yellow`, `orange`, `gold`, `cyan`, `magenta`, `purple`, `white`, `gray`) | `{color=gold}Legendary Relic{/color}` |
| `{scale=X}...{/scale}` or `{size=X}` | Font scale multiplier | `{scale=1.3}Loud shout!{/scale}` |
| `{alpha=X}...{/alpha}` | Text opacity / alpha (0.0 to 1.0) | `{alpha=0.5}A faint whisper...{/alpha}` |
| `{w=seconds}` or `{w}` | **Typewriter pause** (defaults to 0.4s) | `"One... {w=1.0}two... {w=1.0}three!"` |
| `{$var_name}` | **Variable interpolation** directly in text | `"Welcome, {b}{$player_name}{/b}! Points: {$points}"` |
| `{loc=key}` | Inline localized string substitution | `"Speaker: {loc=char.dari}"` |
| `{{` and `}}` | Escaped literal curly braces | `{{This is not a tag}}` |

### Formatting Example:
```ini
dari "Greetings, {color=#22c55e}{b}{$player_name}{/b}{/color}! We have waited for you {w=0.8}for so long..."
```

---

## Dialogue Modes: ADV and NVL

DariVN supports both classic visual novel layout paradigms:

1. **ADV (Adventure)** — A dialogue box located at the bottom of the screen with a character nameplate.
2. **NVL (Novel)** — Fullscreen text mode (reminiscent of *Everlasting Summer* and Type-Moon novels). Dialogue lines accumulate on screen sequentially.

### Mode Commands:

```ini
# Switch to fullscreen NVL mode
nvl on
# (Aliases: mode nvl, window fullscreen, nvl show)

"Memories suddenly rushed back..."
"It was a quiet, unassuming winter evening..."
dari "And then Dari appeared before me!"

# Clear the accumulated text on screen (start a fresh page)
nvl clear

"A new page of memories unfolds..."

# Switch back to the standard bottom ADV dialogue box
nvl off
# (Aliases: mode adv, window bottom, window normal)
```

---

## Variables & Expressions

Script variables are prefixed with `$`. They can store numbers (integers and floating-point), strings, booleans, and null values.

### Assignments & Arithmetic Operators

```ini
# Base assignments
$player_name = "Alex"
$points = 0
$visited_roof = false

# Compound arithmetic operators
$points += 10
$points -= 5
$points *= 2
$points /= 2
```

### Supported Data Types
* **Numbers:** `10`, `-5`, `3.1415`
* **Strings:** `"Hello"`, `'World'`
* **Booleans:** `true`, `false`
* **Null:** `null`, `none`, `nil`

### Global (Persistent) Variables
Standard variables are stored inside the player's specific save slot. If you need variables that **persist across all playthroughs and game restarts** (e.g. game run counters, unlocked endings, CG gallery flags), use the `$global.` or `$persistent.` prefix:

```ini
# This counter is never reset when starting a New Game!
$global.started_counter += 1
$persistent.true_ending_unlocked = true
```

Persistent variables are automatically written to `saves/persistent.json`.

---

## Conditional Statements (`if / else`)

You can execute any script command conditionally:

```ini
# Format: if <condition> then <command>
if $points >= 50 then dari "You did an incredible job!"

# Colon separator syntax:
if $visited_roof: scene "assets/textures/backgrounds/rooftop.jpg"

# Direct conditional jump:
if $points < 10 jump bad_ending_label

# Conditional jump with an else branch:
if $points >= 100 jump secret_scene else jump common_scene
```

### Operators:
* Comparisons: `==`, `!=`, `<`, `<=`, `>`, `>=`
* Logic: `and`, `or`, `not`
* Grouping: `if ($points > 10 and not $visited_roof) or $global.started_counter > 5 jump bonus_label`

---

## Scene Backgrounds & Transitions

### Background Switching
Background images are set using the `scene` (or `bg`) command. Hardware-accelerated shader transitions are supported:

```ini
# Instant background change
scene "assets/textures/backgrounds/classroom.jpg"

# Fade to black and back (duration in seconds)
scene "assets/textures/backgrounds/classroom.jpg" with fade 1.5

# Smooth crossfade between frames
scene "assets/textures/backgrounds/rooftop.jpg" with crossfade 2.0

# Procedural noise dissolve
scene "assets/textures/backgrounds/rooftop.jpg" with dissolve 1.5
```

### Standalone Transitions
```ini
transition fade 1.0
transition dissolve 1.5
```

---

## Character Display & Positioning

Before displaying a character, load their configuration:

```ini
load character dari
load character shadow
```
*(Loaded automatically from `assets/characters/<id>.vn`. See [Character Configuration Guide](characters.md))*

### Displaying a Character (`show`)
```ini
# Show at a preset screen slot (left, center, right)
show dari at center

# Specify a facial expression
show dari happy at right

# Explicit rendering layer (Z-ordering)
show shadow at left layer 0
show dari at center layer 1

# Free absolute screen coordinates (X, Y)
show dari at 640 100 layer 1

# Scale multiplier override
show dari at center scale 1.25
```

### Moving Characters (`move`)
Smoothly interpolates a character's position to a new location:

```ini
# Move horizontally to coordinate X=680
move dari to 680

# Move to a slot
move dari to right

# Move to absolute coordinates X and Y
move dari to 300 120
```

### Hiding Characters (`hide`)
```ini
hide dari
hide shadow
```

---

## Procedural Micro-Animations

DariVN features procedural procedural animations that bring 2D sprites to life without requiring frame-by-frame sprite sheets:

### 1. Joyful Bounce (`hop`)
```ini
# hop <character> [duration=0.3] [height=25.0]
hop dari 0.3
dari "Yay! School is finally out for summer!"
```

### 2. Shock / Tremble (`shake`)
Rapid horizontal vibration for scares, fear, or explosions:
```ini
# shake <character> [duration=0.4] [intensity=15.0]
shake dari 0.4
dari "Eek! What was that terrifying sound?!"
```

### 3. Flash (`flash`)
Flashes the sprite in solid white and smoothly fades back to normal:
```ini
# flash <character> [duration=0.25]
flash dari 0.25
```

---

## Sprite Visual Effects

Dynamically modify character appearance using `effect <character>`:

```ini
# Color tinting (R G B [A]) for nocturnal/sunset lighting:
effect dari tint 0.6 0.6 0.95

# Convert character into an anonymous silhouette:
effect dari silhouette true

# Disable silhouette:
effect dari silhouette false

# Character-specific sepia filter (0.0 to 1.0):
effect dari sepia 1.0

# Reset all sprite effects:
effect dari clear
```

---

## Interactive Choices (`choice:`)

A branching choice begins with the `choice:` keyword. Options are indented and specify a target label using `->`:

```ini
choice:
    "Head to the rooftop" -> rooftop_scene
    "Stay in the classroom" -> classroom_scene
```

### Availability Conditions
You can append conditions to individual options:

```ini
choice:
    # Option is hidden if condition is false
    "Inspect the rooftop" if not $visited_roof -> rooftop_scene
    
    # Option appears grayed-out and disabled (locked) if condition is false
    "Crack the safe" if $points >= 100 else disable -> safe_scene
    
    # Alternate lock syntax:
    "Secret archive" if $has_key [lock] -> secret_room
```

### Navigating to Another Script File
Choices can jump directly into another script:

```ini
choice:
    "Begin Chapter 2" -> script "assets/scripts/chapter2.vn"
    "Secret Prologue" -> script "assets/scripts/prologue.vn" secret_label
```

---

## Labels & Flow Control (`label`, `jump`)

```ini
label rooftop_scene
$visited_roof = true
scene "assets/textures/backgrounds/rooftop.jpg" with fade 1.0
dari "The view from here is breathtaking!"
jump common_ending

label classroom_scene
dari "Staying inside was a cozy choice."
jump common_ending

label common_ending
dari "Thanks for playing the demo!"
```

### Jumping to Another Script File
```ini
# Jump to the beginning of another file:
jump script "assets/scripts/chapter2.vn"

# Jump to a specific label in another file:
jump script "assets/scripts/chapter2.vn" rooftop_scene
```

---

## Audio & Music Playback

```ini
# Background music with loop, fade-in, and volume
play music "assets/audio/bgm/theme.mp3" loop fade 1.5 volume 0.8
stop music fade 2.0

# Sound effects (SFX)
play sound "assets/audio/sfx/bell.wav"

# Voice acting line
play voice "assets/audio/voice/line_001.mp3"

# Environmental ambient loops (wind, rain)
play ambient "assets/audio/ambient/wind.mp3" loop fade 2.0 volume 0.7
stop ambient fade 1.5
```

See [Audio System Guide](audio.md) for complete details.

---

## Applying Custom Shaders

### Screen Post-Processing Effects:
```ini
effect vignette 0.8  # Cinematic edge darkening
effect sepia 1.0     # Warm nostalgic sepia
effect blur 0.6      # Gaussian blur (dizziness, fainting)
effect clear         # Reset all screen post-processing effects
```

### Custom GLSL Shaders:
```ini
load shader wave
load shader glitch

# Apply wave distortion to the background scene:
shader scene wave
shader scene set u_Speed 4.0
shader scene clear

# Apply chromatic glitch to a character sprite:
shader dari glitch
shader dari clear
```

See [Shaders Guide](shaders.md) for custom shader authoring.

---

## Related Documentation

* [Documentation Index](README.md)
* [Character Configuration Guide](characters.md)
* [Shaders & Special Effects Guide](shaders.md)
* [Audio System Guide](audio.md)
* [Localization Guide](localization.md)
* [Saves & Architecture Guide](saves_and_architecture.md)
