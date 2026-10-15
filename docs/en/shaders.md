# Shaders & Special Effects Guide (`shaders.md`)

<p align="center">
  <b>English</b> • <a href="../ru/shaders.md"><b>Русский</b></a>
</p>

The **DariVN** visual novel engine leverages hardware acceleration through **OpenGL 3.3 Core Profile** and **GLSL 330 core**. Its rendering pipeline features a multi-pass architecture using Framebuffer Objects (FBO), giving developers powerful tools for creating cinematic atmospheres: procedural water waves, CRT/digital glitch distortions, painterly anime-style background filters, blur, vignettes, and smooth transitions.

---

## Table of Contents

* [Graphics Pipeline Architecture](#graphics-pipeline-architecture)
* [Shader Configuration File Structure (`.vn`)](#shader-configuration-file-structure-vn)
* [Standard Inputs & Uniform Variables](#standard-inputs--uniform-variables)
* [Walkthrough of Built-in Custom Shaders](#walkthrough-of-built-in-custom-shaders)
  * [1. Wave Distortion (`wave.frag`)](#1-wave-distortion-wavefrag)
  * [2. Digital Glitch & Aberration (`glitch.frag`)](#2-digital-glitch--aberration-glitchfrag)
  * [3. Painterly Anime Background Filter (`art.frag`)](#3-painterly-anime-background-filter-artfrag)
* [Controlling Shaders from Scenarios](#controlling-shaders-from-scenarios)
  * [Scene Background Shaders](#scene-background-shaders)
  * [Character Sprite Shaders](#character-sprite-shaders)
  * [Modifying Uniform Parameters at Runtime](#modifying-uniform-parameters-at-runtime)
* [Built-in Screen Post-Processing Effects](#built-in-screen-post-processing-effects)
* [Hardware-Accelerated Scene Transitions](#hardware-accelerated-scene-transitions)
* [Step-by-Step Tutorial: Writing a Custom Shader](#step-by-step-tutorial-writing-a-custom-shader)
* [Related Documentation](#related-documentation)

---

## Graphics Pipeline Architecture

Rendering in DariVN follows a modular multi-pass structure:
1. **Scene & Sprites Pass:** The scene background and active character sprites are drawn into an offscreen full-resolution Framebuffer Object (FBO).
2. Custom shaders can be bound independently to the background and to individual character sprites.
3. The resulting FBO texture is fed into the post-processing pipeline (`postprocess.frag`), where screen-wide effects (vignette, blur, sepia, global color tinting) are applied.
4. During scene background changes, the transition shader (`transition.frag`) blends between the previous and upcoming frames.

---

## Shader Configuration File Structure (`.vn`)

Shaders are registered via configuration files located in `assets/shaders/<id>.vn`:

```ini
# assets/shaders/wave.vn
id = wave
vertex = assets/shaders/sprite.vert
fragment = assets/shaders/custom/wave.frag

# Default uniform parameter values
u_Speed = 2.5
u_Frequency = 14.0
u_Amplitude = 0.02
```

### Directives:

| Directive | Description |
|---|---|
| `id` | Unique string identifier used in narrative scripts (`load shader <id>`). |
| `vertex` (or `vert`) | Path to the GLSL vertex shader (usually `assets/shaders/sprite.vert`). |
| `fragment` (or `frag`) | Path to the GLSL fragment shader source file. |
| `<uniform_name> = <values>` | Default initial values for uniform variables (`float`, or vectors of 2, 3, or 4 numbers). |

*Prefixes like `uniform.u_Speed` or `param.u_Speed` are also supported.*

---

## Standard Inputs & Uniform Variables

### Vertex Shader (`assets/shaders/sprite.vert`)
Sprites and backgrounds share a standard quad vertex shader:
```glsl
#version 330 core
layout (location = 0) in vec4 a_Vertex; // <vec2 position (xy), vec2 texCoords (zw)>

out vec2 v_TexCoords;

uniform mat4 u_Projection;
uniform mat4 u_Model;

void main() {
    v_TexCoords = a_Vertex.zw;
    gl_Position = u_Projection * u_Model * vec4(a_Vertex.xy, 0.0, 1.0);
}
```

### Automatically Provided Uniforms
The engine automatically passes these uniforms to every fragment shader:

| Uniform | Type | Description |
|---|---|---|
| `u_Texture` | `sampler2D` | Texture of the sprite or background (bound to texture unit 0). |
| `u_Color` | `vec4` | Color tint multiplier (including alpha transparency). |
| `u_Time` | `float` | Continuous game runtime elapsed in seconds. |
| `u_Resolution` | `vec2` | Screen resolution in pixels (e.g. `1280.0, 720.0`). |

Any additional custom uniforms declared in your fragment shader can be populated through your `.vn` config and tweaked on the fly from script commands!

---

## Walkthrough of Built-in Custom Shaders

### 1. Wave Distortion (`wave.frag`)
Ideal for underwater visuals, desert mirages, dreams, or disorientation:

```glsl
#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform float u_Time;
uniform vec2 u_Resolution;

uniform float u_Speed;
uniform float u_Frequency;
uniform float u_Amplitude;

void main() {
    // Calculate horizontal sinusoidal displacement
    float wave = sin(v_TexCoords.y * u_Frequency + u_Time * u_Speed) * u_Amplitude;
    vec2 uv = vec2(v_TexCoords.x + wave, v_TexCoords.y);
    
    vec4 tex = texture(u_Texture, uv);
    if (tex.a < 0.005) {
        discard;
    }
    FragColor = tex * u_Color;
}
```

### 2. Digital Glitch & Aberration (`glitch.frag`)
Designed for psychological horror, digital errors, sci-fi anomalies, or psychic events:

```glsl
#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform float u_Time;
uniform vec2 u_Resolution;

uniform float u_Speed;
uniform float u_Intensity;
uniform float u_ColorOffset;

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 uv = v_TexCoords;

    // Horizontal strip displacement based on pseudo-random noise
    float sliceY = floor(uv.y * 28.0);
    float noise = rand(vec2(sliceY, floor(u_Time * u_Speed * 8.0)));
    if (noise > 0.75) {
        uv.x += (rand(vec2(u_Time * 5.0, sliceY)) - 0.5) * u_Intensity;
    }

    // Chromatic aberration (splitting R and B channels)
    float r = texture(u_Texture, uv + vec2(u_ColorOffset, 0.0)).r;
    float g = texture(u_Texture, uv).g;
    float b = texture(u_Texture, uv - vec2(u_ColorOffset, 0.0)).b;
    float a = texture(u_Texture, uv).a;

    if (a < 0.005) {
        discard;
    }

    FragColor = vec4(r, g, b, a) * u_Color;
}
```

### 3. Painterly Anime Background Filter (`art.frag`)
Transforms real-world photographs into stylized visual novel backgrounds using Sobel edge detection, soft posterization, saturation boosting, and contrast enhancement.

Configured parameters:
* `u_EdgeThreshold`: Edge detection sensitivity.
* `u_EdgeStrength`: Contrast of dark outline contours.
* `u_Saturation`: Color vibrancy boost.
* `u_Contrast`: Image contrast curve.

---

## Controlling Shaders from Scenarios

### 1. Loading a Shader Configuration
Before applying a shader, load its `.vn` configuration:
```ini
load shader wave
load shader glitch
load shader art
```
*(Searches for `assets/shaders/<id>.vn`)*

### Scene Background Shaders
```ini
# Apply the painterly filter to the active background:
shader scene art

# Apply wave distortion:
shader scene wave

# Clear background shader (returns to standard rendering):
shader scene clear
# (Aliases: shader scene off, shader bg clear)
```

### Character Sprite Shaders
```ini
# Apply glitch effect to Dari's sprite:
shader dari glitch
dari "What is happening to me?!"

# Clear shader from sprite:
shader dari clear
```

### Modifying Uniform Parameters at Runtime
You can fine-tune any uniform variable dynamically in the script:

```ini
# Format for scene: shader scene set <uniform_name> <values...>
shader scene set u_Speed 4.0
shader scene set u_Amplitude 0.05

# Format for characters: shader <character> set <uniform_name> <values...>
shader dari set u_Intensity 0.08
shader dari set u_ColorOffset 0.02
```

---

## Built-in Screen Post-Processing Effects

The `effect <type>` command applies full-screen post-processing through `postprocess.frag`:

### 1. Vignette (`vignette`)
Darkens screen borders to focus the player's eye on the center:
```ini
# Intensity range from 0.0 to 1.5
effect vignette 0.8
```

### 2. Sepia / Flashback (`sepia`)
Applies a warm vintage sepia tone:
```ini
# Tone strength from 0.0 (off) to 1.0 (full sepia)
effect sepia 1.0
```

### 3. Screen Blur (`blur`)
A 9-tap Gaussian blur effect for fainting, waking up, or dream sequences:
```ini
effect blur 0.6
```

### 4. Global Color Tint (`tint`)
Tints the entire screen with an RGB or RGBA color multiplier:
```ini
# Cool moonlit night:
effect tint 0.5 0.5 0.8

# Crimson danger lighting:
effect tint 1.0 0.4 0.4
```

### 5. Clearing Screen Effects
```ini
effect clear
```

---

## Hardware-Accelerated Scene Transitions

When using `scene <path> with <type> <duration>`, the engine invokes `assets/shaders/transition.frag`.

### Supported Transition Types:
1. **`fade`** — Fades smoothly to black and emerges on the new scene.
2. **`crossfade`** — Soft linear alpha cross-dissolve between the previous and new frame.
3. **`dissolve`** — Organic pixel dissolution powered by a procedural noise map.

### Examples:
```ini
scene "assets/textures/backgrounds/rooftop.jpg" with fade 1.5
scene "assets/textures/backgrounds/classroom.jpg" with dissolve 2.0
scene "assets/textures/backgrounds/corridor.jpg" with crossfade 1.0
```

---

## Step-by-Step Tutorial: Writing a Custom Shader

Let's build a **pulsating color inversion** shader: `assets/shaders/custom/invert.frag`.

### Step 1. Write the Fragment Shader
Create `assets/shaders/custom/invert.frag`:
```glsl
#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform float u_Time;

uniform float u_PulseSpeed;

void main() {
    vec4 tex = texture(u_Texture, v_TexCoords);
    if (tex.a < 0.005) {
        discard;
    }

    // Compute pulsating factor between 0.0 and 1.0
    float factor = (sin(u_Time * u_PulseSpeed) + 1.0) * 0.5;
    vec3 inverted = vec3(1.0) - tex.rgb;
    vec3 finalRgb = mix(tex.rgb, inverted, factor);

    FragColor = vec4(finalRgb, tex.a) * u_Color;
}
```

### Step 2. Create the Configuration File
Create `assets/shaders/invert.vn`:
```ini
id = invert
vertex = assets/shaders/sprite.vert
fragment = assets/shaders/custom/invert.frag

u_PulseSpeed = 3.0
```

### Step 3. Use It in Your Script
```ini
load shader invert

# Apply to the background during a climactic event:
shader scene invert
"The world around me began to invert erratically..."

# Accelerate pulsation speed:
shader scene set u_PulseSpeed 8.0
"The distortion is speeding up!"

# Disable shader:
shader scene clear
```

---

## Related Documentation

* [Documentation Index](README.md)
* [Writing Scenarios](scenario.md)
* [Character Configuration](characters.md)
* [Engine Configuration](configuration.md)
