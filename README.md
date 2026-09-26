# DariVN (Visual Novel Engine) 🌸

<p align="center">
  <a href="#english"><b>English</b></a> •
  <a href="#українська"><b>Українська</b></a> •
  <a href="#русский"><b>Русский</b></a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg?logo=c%2B%2B" alt="C++20" />
  <img src="https://img.shields.io/badge/OpenGL-3.3%20Core-green.svg?logo=opengl" alt="OpenGL" />
  <img src="https://img.shields.io/badge/License-AGPL_v3-red.svg" alt="AGPLv3" />
  <img src="https://img.shields.io/badge/License-Commercial-orange.svg" alt="Commercial" />
  <img src="https://img.shields.io/badge/Platform-macOS%20%7C%20Linux%20%7C%20Windows-lightgrey.svg" alt="Platform" />
</p>

---

## English

**DariVN** is a modern, lightweight, and high-performance native 2D visual novel engine built with **C++20** and **OpenGL 3.3 Core**.

Unlike engines built on top of interpreted scripting runtimes (such as Python in Ren'Py) or web frameworks (Electron), DariVN compiles directly into a single native binary. It features instant startup, minimal RAM footprint, and smooth hardware-accelerated rendering.

### 🌟 Key Features

* 🚀 **Native Performance:** Core written in C++20 with hardware-accelerated OpenGL 3.3 Core rendering for silky-smooth framerates and minimal resource overhead.
* 📜 **Declarative Scripting Language (`.vn`):**
  * Intuitive syntax for dialogues, character speech, and comments.
  * Interactive branching (`choice:`) with conditional availability.
  * Comprehensive state management with variables (`$player_name`, `$points`, `$flag`) and expressions (`if / else`).
* 🎭 **Advanced Character & Animation System:**
  * Free sprite positioning and multi-layer rendering (Z-ordering).
  * Built-in procedural micro-animations: jumps (`hop`), shocks/scares (`shake`), white flashes (`flash`), and smooth translations (`move`).
  * Sprite shader effects: custom color tinting (`tint`), outlines, and silhouettes.
* 🎨 **Shader Transitions:**
  * Post-processing scene transitions (Fade, Dissolve, and custom GLSL transitions rendered via Framebuffers).
* 💬 **Rich Typography & Text Rendering:**
  * Vector TrueType font rendering powered by `stb_truetype`.
  * Inline markup tags: `{b}bold{/b}`, `{i}italic{/i}`, `{color=#HEX}custom colors{/color}`, `{size=...}`.
  * Automatic word wrapping and configurable typewriter text speed.
* 🎵 **Multi-Channel Audio Engine (miniaudio):**
  * Dedicated audio buses: `Master`, `BGM`, `SFX`, `Ambient`, and `Voice`.
  * Smooth volume fades (Fade In / Fade Out) and seamless background music crossfades.
* 🌍 **Out-of-the-Box Localization (i18n):**
  * Locale strings loaded from `.ini` files (e.g., English, Ukrainian, Russian).
  * Direct substitution of localized keys in scripts (`@key` and `{loc=key}`).
* 💾 **Save & Load System:**
  * Full serialization of story progress, script variables, active scene background, and audio track state.
* ⚙️ **Flexible Configuration (`config.vn`):**
  * Tweak window resolution, VSync, initial script paths, asset directories, and sound levels without recompilation.

### 📖 Script Example (`.vn`)

```ini
# Variable initialization
$player_name = "Player"
$points = 10
$visited_roof = false

# Load characters and play music
load character dari
play music "assets/audio/bgm/theme.mp3" loop fade 1.5

# Scene background with transition
scene "assets/textures/backgrounds/classroom.jpg" with fade 1.0

# Display character on specific layer
show dari at center layer 1
dari "Hello, {b}{$player_name}{/b}! Glad to meet you."

# Procedural animations and effects
hop dari 0.3
dari "Look! I can hop happily!"
shake dari 0.4
dari "{color=#ef4444}Whoa!{/color} Someone slammed the door shut..."

# Interactive choices with conditions
choice:
    "Head to the rooftop" if not $visited_roof -> rooftop_scene
    "Stay in the classroom" -> classroom_scene
```

### 🛠️ Technology Stack & Libraries

DariVN relies on lightweight, battle-tested libraries:
* **GLFW 3.4** — Window management, context creation, and cross-platform input (zlib License).
* **GLAD** — OpenGL 3.3 Core function loader (MIT / Public Domain).
* **GLM** — Mathematics library tailored for graphics (MIT License).
* **miniaudio** — Lightweight single-header multiplatform audio engine (MIT-0 / Public Domain).
* **stb libraries** (`stb_image`, `stb_image_write`, `stb_truetype`) — Image decoding and font rasterization (MIT / Public Domain).

### 📁 Project Structure

```text
DariVN/
├── assets/                  # Game assets and configurations
│   ├── audio/              # Music (bgm), sounds (sfx), ambient tracks
│   ├── characters/         # Character declarations (.vn)
│   ├── fonts/              # Vector fonts (.ttf)
│   ├── locales/            # Localization files (.ini)
│   ├── scripts/            # Story chapters and scripts (.vn)
│   ├── shaders/            # GLSL shader files
│   ├── textures/           # Backgrounds, UI elements, sprites
│   └── config.vn           # Global engine configuration
├── external/               # Third-party libraries (GLAD, GLM, stb, miniaudio)
├── src/                    # Engine source code
│   ├── audio/              # Audio playback & mixer subsystem
│   ├── core/               # Window, events, config, main loop, save manager
│   ├── renderer/           # Shaders, textures, sprites, text renderer, pipeline
│   ├── resource/           # Asset manager and localization system
│   ├── scene/              # Character sprites, dialogue box, choice menu
│   ├── script/             # Script parser, AST, evaluator, story state machine
│   └── main.cpp            # Entry point
└── CMakeLists.txt          # CMake build configuration
```

### 🚀 Building & Running

#### Prerequisites:
* C++ compiler with **C++20** support (GCC 11+, Clang 13+, MSVC 2019+).
* **CMake 3.24** or newer.
* Graphics hardware supporting **OpenGL 3.3 Core Profile**.

#### Build Instructions:

```bash
# 1. Clone the repository
git clone https://github.com/username/DariVN.git
cd DariVN

# 2. Configure build with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compile the executable
cmake --build build --config Release
```

#### Running:
Ensure the working directory contains the `assets/` directory (or run directly from the project root):
```bash
# macOS / Linux
./build/DariVN

# Windows
.\build\Release\DariVN.exe
```

### ⚖️ Dual Licensing

**DariVN** is available under a dual-licensing scheme:

1. **GNU AGPLv3 (Open-Source):**
   Free for community, hobbyist, and open-source projects. If you modify the engine or build products with it (including network/cloud-distributed versions), your source code and modifications must be made public under the AGPLv3. See [`LICENSE`](LICENSE) (AGPLv3).
2. **Commercial License:**
   For studios and indie developers looking to release commercial games on Steam, App Store, Google Play, or consoles without disclosing their proprietary source code, narrative scripts, or engine changes. See [`LICENSE-COMMERCIAL.md`](LICENSE-COMMERCIAL.md) or contact [arsenii.soloviov.02@gmail.com](mailto:arsenii.soloviov.02@gmail.com).


---

## Українська

**DariVN** — сучасний, легковажний та високопродуктивний нативний 2D-рушій для створення візуальних новел на **C++20** та **OpenGL 3.3 Core**.

На відміну від рішень на базі інтерпретованого коду (Ren'Py на Python) або веб-технологій (Electron), DariVN компілюється в єдиний нативний бінарний файл: миттєво запускається, ощадливо використовує оперативну пам'ять і забезпечує плавний рендеринг з апаратним прискоренням.

### 🌟 Ключові можливості

* 🚀 **Нативна продуктивність:** Ядро на C++20, апаратний рендеринг через OpenGL 3.3 Core, мінімальні накладні витрати та стабільний FPS.
* 📜 **Власна декларативна скриптова мова (`.vn`):**
  * Інтуїтивний синтаксис діалогів, реплік персонажів та коментарів.
  * Інтерактивні розгалуження (`choice:`) з перевіркою умов доступності варіантів.
  * Повноцінна підтримка змінних (`$player_name`, `$points`, `$flag`) та виразів (`if / else`).
* 🎭 **Розвинена система персонажів та анімацій:**
  * Шари рендерингу (Z-ordering / layers) та вільне позиціонування спрайтів на екрані.
  * Вбудовані процедурні мікроанімації: стрибки (`hop`), тремтіння від переляку (`shake`), білі спалахи (`flash`), плавні переміщення (`move`).
  * Шейдерні ефекти спрайтів: колірне тонування (`tint`), контури та силуети.
* 🎨 **Шейдерні переходи між сценами:**
  * Плавна зміна фонів (Fade, Dissolve та кастомні переходи через Framebuffer).
* 💬 **Багата типографіка та рендеринг тексту:**
  * Завантаження та растеризація векторних шрифтів TrueType (`stb_truetype`).
  * Теги форматування на льоту: `{b}жирний{/b}`, `{i}курсив{/i}`, `{color=#HEX}кольори{/color}`, `{size=...}`.
  * Автоматичний перенос рядків по межах слів (Word Wrapping) і налаштування швидкості друку тексту (ефект друкарської машинки).
* 🎵 **Багатоканальний аудіорушій (miniaudio):**
  * Окремі аудіошини: `Master`, `BGM`, `SFX`, `Ambient`, `Voice`.
  * Плавне наростання й згасання звуку (Fade In / Fade Out), автоматичний кросфейд фонової музики.
* 🌍 **Багатомовність із коробки (i18n):**
  * Збереження локалізованих рядків у файлах `.ini` (українська, англійська, тощо).
  * Підстановка ключів безпосередньо в сценарій (`@key` та `{loc=key}`).
* 💾 **Система збережень (Save / Load):**
  * Повна серіалізація стану сюжету, змінних, поточного фону та музичного треку.
* ⚙️ **Гнучка конфігурація (`config.vn`):**
  * Налаштування роздільної здатності вікна, VSync, стартових скриптів, шляхів до ресурсів та гучності без перекомпіляції.

### 📖 Приклад сценарію (`.vn`)

```ini
# Ініціалізація змінних
$player_name = "Гравець"
$points = 10
$visited_roof = false

# Завантаження персонажів і запуск музики
load character dari
play music "assets/audio/bgm/theme.mp3" loop fade 1.5

# Встановлення фону з плавним переходом
scene "assets/textures/backgrounds/classroom.jpg" with fade 1.0

# Відображення персонажа на потрібному шарі
show dari at center layer 1
dari "Привіт, {b}{$player_name}{/b}! Рада тебе бачити."

# Анімації та візуальні ефекти
hop dari 0.3
dari "Дивись, як я вмію підстрибувати!"
shake dari 0.4
dari "{color=#ef4444}Ой!{/color} Хтось гучно грюкнув дверима..."

# Інтерактивний вибір із перевіркою умов
choice:
    "Піти на дах" if not $visited_roof -> rooftop_scene
    "Залишитися в класі" -> classroom_scene
```

### 🛠️ Стек технологій та бібліотеки

* **GLFW 3.4** — створення вікон, контексту та кросплатформний ввід (zlib License).
* **GLAD** — завантажувач функцій OpenGL 3.3 Core (MIT / Public Domain).
* **GLM** — математична бібліотека для графіки (MIT License).
* **miniaudio** — легковажний аудіобекенд (MIT-0 / Public Domain).
* **stb libraries** (`stb_image`, `stb_image_write`, `stb_truetype`) — робота із зображеннями та шрифтами TTF (MIT / Public Domain).

### 📁 Структура проєкту

```text
DariVN/
├── assets/                  # Ресурси гри
│   ├── audio/              # Музика (bgm), звукові ефекти (sfx), ембієнт
│   ├── characters/         # Конфігурації персонажів (.vn)
│   ├── fonts/              # Векторні шрифти (.ttf)
│   ├── locales/            # Файли локалізації (.ini)
│   ├── scripts/            # Сценарії розділів і діалогів (.vn)
│   ├── shaders/            # Шейдери GLSL
│   ├── textures/           # Фони, спрайти, графіка UI
│   └── config.vn           # Головний конфігураційний файл
├── external/               # Сторонні бібліотеки (GLAD, GLM, stb, miniaudio)
├── src/                    # Вихідний код рушія
│   ├── audio/              # Підсистема аудіо
│   ├── core/               # Вікно, ввід, конфіг, цикл оновлення, збереження
│   ├── renderer/           # Шейдери, текстури, спрайти, текст, конвеєр рендерингу
│   ├── resource/           # Менеджер ресурсів і локалізація
│   ├── scene/              # Спрайти персонажів, діалогове вікно, меню
│   ├── script/             # Парсер сценаріїв, обчислювач виразів, стан новели
│   └── main.cpp            # Точка входу
└── CMakeLists.txt          # Скрипт збірки CMake
```

### 🚀 Збірка та запуск

#### Системні вимоги:
* Компілятор C++ із підтримкою стандарту **C++20** (GCC 11+, Clang 13+, MSVC 2019+).
* **CMake 3.24** або новіший.
* Графічний адаптер із підтримкою **OpenGL 3.3 Core Profile**.

#### Інструкція зі збірки:

```bash
# 1. Клонуйте репозиторій
git clone https://github.com/username/DariVN.git
cd DariVN

# 2. Сконфігуруйте проєкт за допомогою CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Скомпілюйте рушій
cmake --build build --config Release
```

#### Запуск:
Переконайтеся, що робоча директорія містить каталог `assets/`:
```bash
# macOS / Linux
./build/DariVN

# Windows
.\build\Release\DariVN.exe
```

### ⚖️ Подвійне ліцензування (Dual License)

Рушій **DariVN** доступний за двома моделями ліцензування:

1. **GNU AGPLv3 (Open-Source):**
   Безкоштовно для некомерційного використання, навчання та проєктів із відкритим вихідним кодом. Якщо ви модифікуєте рушій або створюєте похідні продукти, ви зобов'язані відкривати весь вихідний код на умовах AGPLv3. Див. [`LICENSE`](LICENSE).
2. **Комерційна ліцензія:**
   Для студій та інді-розробників, які планують комерційний реліз гри (в Steam, App Store, Google Play тощо) без необхідності відкривати вихідний код своєї новели та внесених змін до рушія. Див. [`LICENSE-COMMERCIAL.md`](LICENSE-COMMERCIAL.md) або пишіть на [arsenii.soloviov.02@gmail.com](mailto:arsenii.soloviov.02@gmail.com).


---

## Русский

**DariVN** — современный, легковесный и высокопроизводительный нативный 2D-движок для создания визуальных новелл на **C++20** и **OpenGL 3.3 Core**.

В отличие от решений на базе Python (Ren'Py) или браузерных рантаймов (Electron), DariVN скомпилирован в единый нативный бинарный файл: мгновенно запускается, бережно расходует оперативную память и обеспечивает плавный рендеринг с аппаратным ускорением.

### 🌟 Ключевые возможности

* 🚀 **Нативная производительность:** Ядро на C++20, аппаратный рендеринг через OpenGL 3.3 Core, минимальный overhead и высокий FPS.
* 📜 **Собственный декларативный скриптовый язык (`.vn`):**
  * Простой синтаксис диалогов, реплик персонажей и комментариев.
  * Интерактивные развилки (`choice:`) с условиями доступности вариантов.
  * Полноценная поддержка переменных (`$player_name`, `$points`, `$flag`) и выражений (`if / else`).
* 🎭 **Продвинутая система персонажей и анимаций:**
  * Слои рендеринга (Z-ordering / layers) и свободное позиционирование спрайтов на экране.
  * Встроенные процедурные микро-анимации: прыжки (`hop`), вздрагивание от испуга (`shake`), белые вспышки (`flash`), плавные перемещения (`move`).
  * Шейдерные эффекты спрайтов: цветовая тонировка (`tint`), контуры и силуэты.
* 🎨 **Шейдерные переходы между сценами:**
  * Плавные переходы смены фонов (Fade, Dissolve и кастомные шейдерные переходы через Framebuffer).
* 💬 **Продвинутая типографика и текст:**
  * Загрузка и рендеринг векторных TrueType-шрифтов (`stb_truetype`).
  * Теги форматирования на лету: `{b}жирный{/b}`, `{i}курсив{/i}`, `{color=#HEX}кастомные цвета{/color}`, `{size=...}`.
  * Автоматический перенос строк по границам слов (Word Wrapping) с настраиваемой скоростью вывода текста (Typewriter effect).
* 🎵 **Многоканальный аудиодвижок (miniaudio):**
  * Отдельные шины громкости: `Master`, `BGM`, `SFX`, `Ambient`, `Voice`.
  * Плавное нарастание и затухание (Fade In / Fade Out), автоматический кроссфейд при смене фоновой музыки.
* 🌍 **Мультиязычность из коробки (i18n):**
  * Хранение строк в формате `.ini` (например, русский, украинский, английский).
  * Подстановка локализованных ключей прямо в текст сценария (`@key` и `{loc=key}`).
* 💾 **Сохранения и состояние (Save / Load):**
  * Сохранение состояния истории, значений скриптовых переменных, текущего фона и музыки.
* ⚙️ **Гибкая конфигурация (`config.vn`):**
  * Настройка разрешения окна, VSync, стартовых скриптов, путей к ресурсам и громкости без перекомпиляции.

### 📖 Пример сценария (`.vn`)

```ini
# Инициализация переменных
$player_name = "Игрок"
$points = 10
$visited_roof = false

# Загрузка персонажей и музыки
load character dari
play music "assets/audio/bgm/theme.mp3" loop fade 1.5

# Смена фона с плавным переходом
scene "assets/textures/backgrounds/classroom.jpg" with fade 1.0

# Отображение персонажа на нужном слое
show dari at center layer 1
dari "Привет, {b}{$player_name}{/b}! Рада тебя видеть."

# Анимации и визуальные эффекты
hop dari 0.3
dari "Смотри, как я умею подпрыгивать!"
shake dari 0.4
dari "{color=#ef4444}Ой!{/color} Кто-то громко захлопнул дверь..."

# Интерактивный выбор с условиями
choice:
    "Пойти на крышу" if not $visited_roof -> rooftop_scene
    "Остаться в классе" -> classroom_scene
```

### 🛠️ Стек технологий и сторонние библиотеки

* **GLFW 3.4** — создание окон и обработка кроссплатформенного ввода (zlib License).
* **GLAD** — генератор OpenGL загрузчика (MIT / Public Domain).
* **GLM** — математическая библиотека для графики (MIT License).
* **miniaudio** — легковесный кроссплатформенный аудио-бэкенд (MIT-0 / Public Domain).
* **stb libraries** (`stb_image`, `stb_image_write`, `stb_truetype`) — декодирование изображений и растеризация TTF-шрифтов (MIT / Public Domain).

### 📁 Структура проекта

```text
DariVN/
├── assets/                  # Игровые ресурсы
│   ├── audio/              # Музыка (bgm), звуки (sfx), эмбиент
│   ├── characters/         # Конфигурации персонажей (.vn)
│   ├── fonts/              # Шрифты (.ttf)
│   ├── locales/            # Файлы локализации (.ini)
│   ├── scripts/            # Сценарии глав и диалогов (.vn)
│   ├── shaders/            # GLSL шейдеры
│   ├── textures/           # Фоны, элементы интерфейса, спрайты
│   └── config.vn           # Главный конфигурационный файл игры
├── external/               # Внешние зависимости (GLAD, GLM, stb, miniaudio)
├── src/                    # Исходный код движка
│   ├── audio/              # Аудиодвижок
│   ├── core/               # Окно, ввод, конфиг, игровой цикл, сохранения
│   ├── renderer/           # Шейдеры, текстуры, спрайты, текст, пайплайн
│   ├── resource/           # Менеджер ресурсов и локализация
│   ├── scene/              # Спрайты персонажей, диалоговые окна, меню
│   ├── script/             # Парсер сценариев, выражений и машина состояний новеллы
│   └── main.cpp            # Точка входа
└── CMakeLists.txt          # Скрипт сборки CMake
```

### 🚀 Сборка и запуск

#### Системные требования:
* Компилятор C++ с поддержкой стандарта **C++20** (GCC 11+, Clang 13+, MSVC 2019+).
* **CMake 3.24** или новее.
* Поддержка **OpenGL 3.3 Core Profile**.

#### Инструкция по сборке:

```bash
# 1. Клонируйте репозиторий
git clone https://github.com/username/DariVN.git
cd DariVN

# 2. Создайте каталог сборки и сконфигурируйте проект
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Скомпилируйте проект
cmake --build build --config Release
```

#### Запуск:
Убедитесь, что рабочая директория при запуске содержит каталог `assets/`:
```bash
# macOS / Linux
./build/DariVN

# Windows
.\build\Release\DariVN.exe
```

### ⚖️ Лицензирование (Dual License)

Движок **DariVN** распространяется по модели двойного лицензирования:

1. **AGPLv3 (GNU Affero General Public License v3.0):**
   Бесплатно для сообщества, образовательных целей и проектов с открытым исходным кодом. Если вы модифицируете движок или распространяете производные работы (в том числе по сети/облаку), вы обязаны открывать исходный код на условиях AGPLv3. См. [`LICENSE`](LICENSE).
2. **Коммерческая лицензия:**
   Для студий и инди-разработчиков, планирующих коммерческий релиз игры в Steam, App Store, Google Play или на других площадках без необходимости раскрывать исходный код своей игры и модификаций движка. См. [`LICENSE-COMMERCIAL.md`](LICENSE-COMMERCIAL.md) или напишите на [arsenii.soloviov.02@gmail.com](mailto:arsenii.soloviov.02@gmail.com).


*Все используемые сторонние библиотеки (GLFW, GLM, GLAD, miniaudio, stb) распространяются под разрешительными лицензиями (MIT, zlib, Public Domain) и полностью совместимы с обеими моделями распространения.*
