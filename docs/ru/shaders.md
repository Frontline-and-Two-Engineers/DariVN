# Шейдеры и визуальные спецэффекты (`shaders.md`)

<p align="center">
  <b>Русский</b> • <a href="../en/shaders.md"><b>English</b></a>
</p>

Движок **DariVN** использует аппаратное ускорение **OpenGL 3.3 Core Profile** и язык шейдеров **GLSL 330 core**. Графический конвейер построен вокруг двухпроходного рендеринга через фреймбуферы (FBO), что даёт разработчикам визуальных новелл неограниченные возможности создания атмосферных эффектов: процедурные волны, глитчи, стилизация фонов под аниме-арт, размытия, виньетирование и плавные переходы.

---

## Содержание

* [Архитектура графического пайплайна](#архитектура-графического-пайплайна)
* [Структура конфигурации шейдера (`.vn`)](#структура-конфигурации-шейдера-vn)
* [Стандартные входные данные и переменные Uniform](#стандартные-входные-данные-и-переменные-uniform)
* [Разбор готовых шейдеров](#разбор-готовых-шейдеров)
  * [1. Волновое искажение (`wave.frag`)](#1-волновое-искажение-wavefrag)
  * [2. Цифровой глитч и аберрация (`glitch.frag`)](#2-цифровой-глитч-и-аберрация-glitchfrag)
  * [3. Художественная стилизация фонов (`art.frag`)](#3-художественная-стилизация-фонов-artfrag)
* [Управление шейдерами из сценария](#управление-шейдерами-из-сценария)
  * [Шейдеры сцены (фона)](#шейдеры-сцены-фона)
  * [Шейдеры спрайтов персонажей](#шейдеры-спрайтов-персонажей)
  * [Изменение параметров Uniform на лету](#изменение-параметров-uniform-на-лету)
* [Встроенные экранные пост-эффекты](#встроенные-экранные-пост-эффекты)
* [Шейдерные переходы между сценами](#шейдерные-переходы-между-сценами)
* [Пошаговый пример: Создание собственного шейдера](#пошаговый-пример-создание-собственного-шейдера)
* [Связанные документы](#связанные-документы)

---

## Архитектура графического пайплайна

Рендеринг в DariVN организован следующим образом:
1. **Сцена и спрайты** рисуются в отдельный полноэкранный FBO (Framebuffer Object).
2. К фону сцены и каждому персонажу может быть применён индивидуальный шейдер.
3. Содержимое FBO передаётся в конвейер пост-процессинга (`postprocess.frag`), где накладываются экранные эффекты (виньетка, размытие, сепия, глобальный оттенок).
4. Во время смены локаций шейдер перехода (`transition.frag`) выполняет переход между предыдущим и новым кадром.

---

## Структура конфигурации шейдера (`.vn`)

Конфигурация шейдера описывается в файле `assets/shaders/<id>.vn`:

```ini
# assets/shaders/wave.vn
id = wave
vertex = assets/shaders/sprite.vert
fragment = assets/shaders/custom/wave.frag

# Дефолтные параметры (Uniforms)
u_Speed = 2.5
u_Frequency = 14.0
u_Amplitude = 0.02
```

### Доступные директивы:

| Директива | Назначение |
|---|---|
| `id` | Уникальный строковый ID шейдера для сценария (`load shader <id>`). |
| `vertex` (или `vert`) | Путь к вершинному шейдеру (обычно `assets/shaders/sprite.vert`). |
| `fragment` (или `frag`) | Путь к фрагментному GLSL-шейдеру. |
| `<имя_uniform> = <значения>` | Начальные значения переменных `uniform` (число `float`, либо вектор из 2, 3 или 4 чисел). |

*Совет: можно также использовать префиксы `uniform.u_Speed` или `param.u_Speed`.*

---

## Стандартные входные данные и переменные Uniform

### Вершинный шейдер (`assets/shaders/sprite.vert`)
Движок передаёт вершины спрайтов через единый атрибут:
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

### Автоматически передаваемые движком Uniforms
В любой фрагментный шейдер спрайта или фона движок автоматически передаёт следующие параметры:

| Uniform | Тип | Описание |
|---|---|---|
| `u_Texture` | `sampler2D` | Текстура спрайта или фона сцены (текстурный юнит 0). |
| `u_Color` | `vec4` | Множитель цвета (включает альфа-прозрачность). |
| `u_Time` | `float` | Непрерывно растущее время работы игры в секундах. |
| `u_Resolution` | `vec2` | Разрешение окна/экрана в пикселях (например, `1280.0, 720.0`). |

Любые дополнительные uniform-переменные объявляются в шейдере и могут настраиваться из конфигурации или скрипта новеллы!

---

## Разбор готовых шейдеров

### 1. Волновое искажение (`wave.frag`)
Идеально подходит для эффектов подводного мира, миражей в пустыне, транса или искажения реальности:

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
    // Вычисляем синусоидальное смещение по горизонтали
    float wave = sin(v_TexCoords.y * u_Frequency + u_Time * u_Speed) * u_Amplitude;
    vec2 uv = vec2(v_TexCoords.x + wave, v_TexCoords.y);
    
    vec4 tex = texture(u_Texture, uv);
    if (tex.a < 0.005) {
        discard;
    }
    FragColor = tex * u_Color;
}
```

### 2. Цифровой глитч и аберрация (`glitch.frag`)
Используется при системных сбоях, психологических сценах, цифровых помехах или появлении мистических сущностей:

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

    // Горизонтальные сдвиги полос по шуму
    float sliceY = floor(uv.y * 28.0);
    float noise = rand(vec2(sliceY, floor(u_Time * u_Speed * 8.0)));
    if (noise > 0.75) {
        uv.x += (rand(vec2(u_Time * 5.0, sliceY)) - 0.5) * u_Intensity;
    }

    // Хроматическая аберрация (разделение R и B каналов)
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

### 3. Художественная стилизация фонов (`art.frag`)
Мягкий фильтр стилизации реальных фотографий под аниме-фон: выделение контуров по оператору Собеля, мягкая пастеризация, регулировка насыщенности и контраста:

Параметры:
* `u_EdgeThreshold`: чувствительность контуров.
* `u_EdgeStrength`: контрастность контурных линий.
* `u_Saturation`: насыщенность красок.
* `u_Contrast`: контрастность изображения.

---

## Управление шейдерами из сценария

### 1. Загрузка конфигурации шейдера
Шейдер необходимо загрузить перед использованием:
```ini
load shader wave
load shader glitch
load shader art
```
*(Ищет файл `assets/shaders/<id>.vn`)*

### Шейдеры сцены (фона)
```ini
# Применение шейдера art к текущему фону
shader scene art

# Применение шейдера волн
shader scene wave

# Отключение шейдера фона (возврат к обычному виду)
shader scene clear
# (Альтернативы: shader scene off, shader bg clear)
```

### Шейдеры спрайтов персонажей
```ini
# Наложение глитча на спрайт Дари
shader dari glitch
dari "Что со мной происходит?!"

# Возврат спрайта к исходному виду
shader dari clear
```

### Изменение параметров Uniform на лету
Вы можете менять параметры шейдера прямо во время сюжета без перезагрузки:

```ini
# Формат для фона: shader scene set <имя_параметра> <значения...>
shader scene set u_Speed 4.0
shader scene set u_Amplitude 0.05

# Формат для персонажа: shader <персонаж> set <имя_параметра> <значения...>
shader dari set u_Intensity 0.08
shader dari set u_ColorOffset 0.02
```

---

## Встроенные экранные пост-эффекты

Команда `effect <тип>` применяет глобальные шейдерные эффекты ко всему экрану:

### 1. Кинематографическая виньетка (`vignette`)
Затемняет края экрана, концентрируя внимание игрока на центре:
```ini
# Интенсивность от 0.0 до 1.5
effect vignette 0.8
```

### 2. Сепия / Воспоминания (`sepia`)
Переводит экран в тёплые винтажные тона воспоминаний:
```ini
# Сила эффекта от 0.0 (выкл) до 1.0 (полная сепия)
effect sepia 1.0
```

### 3. Размытие экрана (`blur`)
Мягкое кинематографичное 9-точечное гауссово размытие (сон, обморок, потеря сознания, воспоминания):
```ini
effect blur 0.6
```

### 4. Экранная цветовая тонировка (`tint`)
Окрашивает всю сцену в заданный цвет (RGB или RGBA):
```ini
# Холодная лунная ночь:
effect tint 0.5 0.5 0.8

# Тревожный багровый свет:
effect tint 1.0 0.4 0.4
```

### 5. Сброс всех пост-эффектов
```ini
effect clear
```

---

## Шейдерные переходы между сценами

При выполнении команды `scene <путь> with <тип> <время>` задействуется шейдер `assets/shaders/transition.frag`.

### Доступные типы переходов:
1. **`fade`** — классическое затемнение экрана в чёрный цвет с последующим проявлением нового фона.
2. **`crossfade`** — мягкое прямое линейное перетекание между предыдущим и новым кадром.
3. **`dissolve`** — пиксельное растворение кадра через процедурную карту шума.

### Примеры:
```ini
scene "assets/textures/backgrounds/rooftop.jpg" with fade 1.5
scene "assets/textures/backgrounds/classroom.jpg" with dissolve 2.0
scene "assets/textures/backgrounds/corridor.jpg" with crossfade 1.0
```

---

## Пошаговый пример: Создание собственного шейдера

Создадим шейдер **пульсирующего негатива** `assets/shaders/custom/invert.frag`.

### Шаг 1. Пишем фрагментный шейдер
Создайте файл `assets/shaders/custom/invert.frag`:
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

    // Вычисляем пульсирующий фактор инверсии от 0.0 до 1.0
    float factor = (sin(u_Time * u_PulseSpeed) + 1.0) * 0.5;
    vec3 inverted = vec3(1.0) - tex.rgb;
    vec3 finalRgb = mix(tex.rgb, inverted, factor);

    FragColor = vec4(finalRgb, tex.a) * u_Color;
}
```

### Шаг 2. Создаем файл конфигурации `.vn`
Создайте файл `assets/shaders/invert.vn`:
```ini
id = invert
vertex = assets/shaders/sprite.vert
fragment = assets/shaders/custom/invert.frag

u_PulseSpeed = 3.0
```

### Шаг 3. Используем в сценарии
```ini
load shader invert

# Применим к фону в кульминационный момент:
shader scene invert
"Реальность вокруг начала неестественно пульсировать..."

# Увеличим частоту пульсации:
shader scene set u_PulseSpeed 8.0
"Пульсация ускорилась!"

# Отключим шейдер:
shader scene clear
```

---

## Связанные документы

* [Главная страница документации](README.md)
* [Написание сценариев](scenario.md)
* [Конфигурация персонажей](characters.md)
* [Конфигурация движка](configuration.md)
