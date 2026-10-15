# DariVN Documentation / Документация 🌸

Welcome to the documentation for **DariVN**, a modern, lightweight, high-performance 2D Visual Novel Engine built with **C++20** and **OpenGL 3.3 Core**.

Пожалуйста, выберите язык документации / Please select your preferred language:

---

### 🌐 [English Documentation (`en/`)](en/README.md)

* [**Overview & Table of Contents**](en/README.md)
* [**Writing Scenarios (`scenario.md`)**](en/scenario.md) — `.vn` script language, dialogue syntax, formatting tags, ADV/NVL modes, branching choices, procedural micro-animations.
* [**Character Configuration (`characters.md`)**](en/characters.md) — Character `.vn` configs, emotions, multi-layer rendering (Z-ordering), sprite effects, movement.
* [**Shaders & Special Effects (`shaders.md`)**](en/shaders.md) — GLSL 330 core shaders, custom shader configs, runtime uniform manipulation, screen post-processing, scene transitions.
* [**Audio System (`audio.md`)**](en/audio.md) — Multi-bus miniaudio architecture, BGM crossfading, SFX, voice acting, ambient loops, audio ducking.
* [**Engine Configuration (`configuration.md`)**](en/configuration.md) — `config.vn` settings, window options, typography, user preferences in `settings.vn`.
* [**Localization System (`localization.md`)**](en/localization.md) — UTF-8 `.ini` locale files, `@key` script resolution, dynamic language switching.
* [**Saves, Persistent Data & Architecture (`saves_and_architecture.md`)**](en/saves_and_architecture.md) — JSON save states with PNG screenshots, QuickSave/QuickLoad, persistent variables (`persistent.json`), backlog history, state machine architecture.
* [**Asset Packer & VFS (`packer.md`)**](en/packer.md) — The `darivn-packer` CLI utility, `.pak` archives with DEFLATE & encryption, developer mode (`--dev`), and virtual file system.

---

### 🌐 [Русскоязычная документация (`ru/`)](ru/README.md)

* [**Обзор и оглавление**](ru/README.md)
* [**Написание сценариев (`scenario.md`)**](ru/scenario.md) — Скриптовый язык `.vn`, синтаксис реплик, форматирование текста, режимы ADV/NVL, развилки `choice:`, анимации.
* [**Конфигурация персонажей (`characters.md`)**](ru/characters.md) — Файлы `.vn`, эмоции, слои Z-ordering, эффекты спрайтов, перемещения.
* [**Шейдеры и спецэффекты (`shaders.md`)**](ru/shaders.md) — GLSL шейдеры, конфигурация `.vn`, изменение uniform на лету, пост-процессинг, переходы между сценами.
* [**Аудиосистема (`audio.md`)**](ru/audio.md) — Архитектура miniaudio, кроссфейд музыки, SFX, озвучка, эмбиент, приглушение звука (Ducking).
* [**Конфигурация движка (`configuration.md`)**](ru/configuration.md) — Файл `config.vn`, параметры окна, шрифты, переопределения `settings.vn`.
* [**Система локализации (`localization.md`)**](ru/localization.md) — Файлы `.ini`, подстановка ключей `@key`, добавление новых языков.
* [**Сохранения, глобальные переменные и архитектура (`saves_and_architecture.md`)**](ru/saves_and_architecture.md) — Слоты сохранений со скриншотами PNG, QuickSave, переменные `$global.*`, бэклог, архитектура состояний.
* [**Упаковщик ресурсов и VFS (`packer.md`)**](ru/packer.md) — Консольная утилита `darivn-packer`, сборка `game.pak` со сжатием DEFLATE и шифрованием, VFS и режим разработчика (`--dev`).

