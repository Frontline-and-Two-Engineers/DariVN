# Конфигурация персонажей (`characters.md`)

<p align="center">
  <b>Русский</b> • <a href="../en/characters.md"><b>English</b></a>
</p>

В движке **DariVN** персонажи являются самостоятельными модульными сущностями. Их свойства, спрайты эмоций, имена и визуальные эффекты вынесены в отдельные конфигурационные файлы формата `.vn`, расположенные в каталоге `assets/characters/`.

---

## Содержание

* [Структура файла конфигурации персонажа](#структура-файла-конфигурации-персонажа)
* [Параметры конфигурации](#параметры-конфигурации)
* [Эмоции и пути к спрайтам](#эмоции-и-пути-к-спрайтам)
* [Загрузка персонажа в сценарии](#загрузка-персонажа-в-сценарии)
* [Позиционирование и слои (Z-Ordering)](#позиционирование-и-слои-z-ordering)
* [Анимации и движения](#анимации-и-движения)
* [Шейдерные эффекты спрайта](#шейдерные-эффекты-спрайта)
* [Пошаговый пример: Создание нового персонажа](#пошаговый-пример-создание-нового-персонажа)
* [Связанные документы](#связанные-документы)

---

## Структура файла конфигурации персонажа

Файл персонажа создаётся с именем `<id>.vn` в каталоге `assets/characters/`. Например: `assets/characters/dari.vn`.

### Пример: `assets/characters/dari.vn`

```ini
# ==============================================================================
# Конфигурация персонажа: Дари (assets/characters/dari.vn)
# ==============================================================================

id = dari
name = @char.dari
scale = 1.0

# Эмоции персонажа и пути к текстурам спрайтов
expr.normal = "assets/textures/characters/dari/normal.png"
expr.happy = "assets/textures/characters/dari/happy.png"
expr.shock = "assets/textures/characters/dari/shock.png"
expr.sad = "assets/textures/characters/dari/sad.png"
expr.angry = "assets/textures/characters/dari/angry.png"
```

### Пример персонажа-силуэта: `assets/characters/shadow.vn`

```ini
# Таинственный персонаж-силуэт
id = shadow
name = @char.shadow
scale = 1.0
expr.normal = "assets/textures/characters/shadow/stand.png"
silhouette = true
```

---

## Параметры конфигурации

| Параметр | Описание | Пример |
|---|---|---|
| `id` | Уникальный строковый идентификатор персонажа, используемый в скриптах. | `id = alice` |
| `name` / `displayName` | Отображаемое имя в диалоговом окне. Может быть прямым текстом, ключом локализации (`@char.alice`) или содержать цветное форматирование. | `name = @char.alice`<br>`name = "{color=pink}Алиса{/color}"` |
| `scale` | Базовый множитель масштаба спрайта (по умолчанию `1.0`). Удобно, если спрайты персонажа нарисованы разного роста. | `scale = 0.9` |
| `silhouette` | Режим отображения в виде сплошного тёмного силуэта (`true` / `false`). | `silhouette = true` |
| `default_shader` | Идентификатор кастомного шейдера, применяемого к персонажу по умолчанию при загрузке. | `default_shader = glitch` |

---

## Эмоции и пути к спрайтам

Эмоции задаются с префиксом `expr.<имя>`:

```ini
expr.normal = "assets/textures/characters/dari/normal.png"
expr.smile = "assets/textures/characters/dari/smile.png"
expr.blush = "assets/textures/characters/dari/blush.png"
expr.thinking = "assets/textures/characters/dari/thinking.png"
```

Также поддерживаются стандартные имена эмоций без префикса `expr.`: `normal`, `happy`, `sad`, `angry`, `shock`.

В сценарии эмоция указывается после имени персонажа при вызове команды `show`:
```ini
show dari blush at center
dari "Ой... я не ожидала такого комплимента..."
```

---

## Загрузка персонажа в сценарии

В файле сюжета (`.vn`) перед использованием персонажа его необходимо подключить командой `load character`:

```ini
# Автоматический поиск в assets/characters/<id>.vn
load character dari
load character shadow

# Либо явное указание пути к файлу:
load character rival "assets/custom_chars/rival.vn"
```

---

## Позиционирование и слои (Z-Ordering)

### 1. Фиксированные слоты экрана
Движок предоставляет 3 стандартных позиции по горизонтали:
* `left` — левая треть экрана
* `center` — центр экрана
* `right` — правая треть экрана

```ini
show dari at center
show shadow at left
```

### 2. Свободные экранные координаты (X, Y)
Вы можете разместить спрайт в любых экранных пиксельных координатах:

```ini
# Координаты: X = 240, Y = 130
show shadow at 240 130
```

### 3. Слои отрисовки (Z-ordering)
Параметр `layer <N>` определяет глубину сцены. Персонажи на меньших слоях отрисовываются **позади** персонажей на больших слоях:

```ini
# Таинственный силуэт стоит позади на слое 0:
show shadow at 240 130 layer 0

# Дари стоит на переднем плане на слое 1:
show dari at center layer 1
```

### 4. Динамическое изменение масштаба
```ini
show dari at center scale 1.25
```

---

## Анимации и движения

DariVN поддерживает встроенные процедурные эффекты, оживляющие статичные спрайты:

### Плавное перемещение (`move`)
```ini
# Перемещение по горизонтали в пиксель X=680
move dari to 680

# Перемещение в слот
move dari to right

# Перемещение в координаты X Y
move dari to 400 150
```

### Подскок (`hop`)
Персонаж делает радостный или игривый подскок вверх с мягким приземлением:
```ini
# hop <id> [длительность=0.3] [высота=25.0]
hop dari 0.3 30.0
dari "Ура, каникулы начались!"
```

### Вздрагивание / испуг (`shake`)
Быстрое горизонтальное дрожание спрайта (испуг, шок, крик):
```ini
# shake <id> [длительность=0.4] [интенсивность=15.0]
shake dari 0.4 18.0
dari "Что это было?!"
```

### Вспышка белым (`flash`)
Спрайт мгновенно заливается ярким белым цветом и плавно возвращается к нормальному виду (удар, магический эффект, осознание):
```ini
# flash <id> [длительность=0.25]
flash dari 0.25
```

### Скрытие персонажа (`hide`)
```ini
hide dari
hide shadow
```

---

## Шейдерные эффекты спрайта

Внешний вид персонажа можно динамически менять командами `effect` и `shader`:

### Цветовая тонировка (`tint`)
Позволяет подогнать спрайт под освещение локации (например, ночной холодный свет или закат):

```ini
# effect <id> tint <R> <G> <B> [A]
# Ночное освещение (синеватый оттенок):
effect dari tint 0.6 0.6 0.95

# Закатное освещение (красно-оранжевый оттенок):
effect dari tint 1.1 0.8 0.7
```

### Теневой силуэт (`silhouette`)
Превращает спрайт персонажа в контурный силуэт без потери альфа-канала:

```ini
effect dari silhouette true
dari "Я стою в тени..."
effect dari silhouette false
```

### Сепия (`sepia`)
```ini
effect dari sepia 1.0  # Персонаж в сепии
```

### Сброс эффектов
```ini
effect dari clear
```

### Кастомные GLSL-шейдеры для персонажа
К персонажу можно привязать любой пользовательский шейдер (например, цифровой глитч, растворение, свечение):

```ini
load shader glitch

# Включение глитча на спрайте:
shader dari glitch
dari "Мой голос... искажается..."

# Сброс шейдера спрайта:
shader dari clear
```

Подробнее о создании кастомных шейдеров: [Руководство по шейдерам](shaders.md).

---

## Пошаговый пример: Создание нового персонажа

Предположим, мы хотим добавить новую героиню — **Мию**.

### Шаг 1. Подготовка изображений
Положите файлы спрайтов в каталог:
* `assets/textures/characters/miya/normal.png`
* `assets/textures/characters/miya/happy.png`
* `assets/textures/characters/miya/shock.png`

### Шаг 2. Создание файла конфигурации
Создайте файл `assets/characters/miya.vn`:

```ini
id = miya
name = @char.miya
scale = 0.95

expr.normal = "assets/textures/characters/miya/normal.png"
expr.happy = "assets/textures/characters/miya/happy.png"
expr.shock = "assets/textures/characters/miya/shock.png"
```

### Шаг 3. Добавление имени в локализацию
В файле `assets/locales/ru.ini`:
```ini
char.miya = {color=#f472b6}Мия{/color}
```

В файле `assets/locales/en.ini`:
```ini
char.miya = {color=#f472b6}Miya{/color}
```

### Шаг 4. Использование в сценарии `assets/scripts/demo.vn`
```ini
load character miya

scene "assets/textures/backgrounds/classroom.jpg" with fade 1.0
show miya happy at center layer 1

miya "Привет! Я новенькая в этом классе."
hop miya 0.3
miya "Надеюсь, мы подружимся!"
```

---

## Связанные документы

* [Главная страница документации](README.md)
* [Написание сценариев](scenario.md)
* [Шейдеры и визуальные спецэффекты](shaders.md)
* [Система локализации](localization.md)
* [Аудиосистема](audio.md)
