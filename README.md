# Лабораторна робота №3: Побудова програми, ELF та організація пам'яті

## 1. Мета лабораторної роботи

У Lab 2 ми простежили перехід: C → Assembly → Machine Code. Але команда `gcc program.c -o program` приховує значно більше етапів. У цій лабораторній роботі ми експериментально дослідили повний шлях:

```
Source Code → Preprocessor → Compiler → Assembler → Object File
→ Linker → ELF Executable → Loader/OS → Process Memory
```

**Головні питання:** Як `program.c` перетворюється на executable? Де знаходяться код і різні типи даних програми?

---

## 2. Середовище

- **WSL 2**, Ubuntu 24.04 LTS (resolute)
- **gcc 15.2.0** (Ubuntu 15.2.0-16ubuntu1)
- **binutils 2.46** (readelf, objdump, nm)
- **file 5.46**
- Архітектура: **x86-64 (AMD64)**
- Хост: Windows 11 Pro, ноутбук x86_64

---

## 3. Таблиця етапів build pipeline

| Файл | Етап | Що містить | Чи може CPU виконати безпосередньо? |
|------|------|-----------|-------------------------------------|
| `program.c` | Source code | C-код з директивами препроцесора | Ні |
| `program.i` | Preprocessing | C-код з розгорнутими `#include`, `#define`, макросами | Ні |
| `program.s` | Compilation | Assembly-код (текст) для x86-64 | Ні (потрібен асемблер) |
| `program.o` | Assembly | Machine code + symbol table + relocation info (ELF relocatable) | Ні (потрібне лінкування) |
| `program` | Linking | Повністю слінкований ELF (PIE, dynamically linked) | Так (після завантаження) |

### Команди для кожного етапу

```bash
gcc -E program.c -o program.i                              # Preprocessing
gcc -S -O0 -masm=intel program.i -o program.s              # Compilation
gcc -c program.s -o program.o                              # Assembly
gcc program.o -o program                                   # Linking
```

### Результати `file` для кожного етапу

```
program.c: C source, ASCII text
program.i: C source, ASCII text
program.s: assembler source, ASCII text
program.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
program:   ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 3.2.0, not stripped
```

---

## 4. Завдання 1: Preprocessing

### Виконані команди

```bash
gcc -E program.c -o program.i
file program.i
wc -l program.c program.i
head -n 30 program.i
grep -n "int main" program.i
```

### Результати

```
program.i: C source, ASCII text
   18 program.c
  856 program.i
  874 total
849:int main(void)
```

### Відповіді на питання

**1. Чому `program.i` значно більший за `program.c`?**
`program.i` містить весь вміст заголовочного файлу `stdio.h` (а він, у свою чергу, включає десятки інших заголовків: `stddef.h`, `stdarg.h`, `bits/types.h` тощо). Препроцесор буквально вставляє весь цей текст у файл. `program.i` має 856 рядків проти 18 у `program.c`.

**2. Що сталося з `#include <stdio.h>`?**
Директива `#include` була замінена вмістом файлу `/usr/include/stdio.h` (рекурсивно з усіма вкладеними заголовками). У `program.i` видно оголошення `printf`, `FILE`, `size_t` та інших сутностей, які прийшли зі `stdio.h`.

**3. Чи є `program.i` машинним кодом?**
Ні. `program.i` — це все ще C source code (текст), але з обробленими директивами препроцесора. `file program.i` показує "C source, ASCII text".

**4. Чи може CPU виконувати `program.i`?**
Ні. CPU не розуміє C-код. Потрібні подальші етапи компіляції та асемблювання.

**Головний висновок:** Preprocessor — це текстова обробка. Він не компілює і не перевіряє синтаксис C.

---

## 5. Завдання 2: Compilation

### Виконані команди

```bash
gcc -S -O0 -masm=intel program.i -o program.s
file program.s
grep -n "add:" program.s
grep -n "main:" program.s
```

### Результати

```
program.s: assembler source, ASCII text
21:add:
47:main:
```

### Фрагмент отриманого Assembly (x86-64, Intel syntax)

```asm
add:
    push    rbp
    mov     rbp, rsp
    mov     DWORD PTR [rbp-4], edi
    mov     DWORD PTR [rbp-8], esi
    mov     eax, DWORD PTR [rbp-4]
    add     eax, DWORD PTR [rbp-8]
    pop     rbp
    ret

main:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 16
    mov     eax, DWORD PTR global_value[rip]
    mov     esi, 20
    mov     edi, eax
    call    add
    mov     DWORD PTR [rbp-4], eax
    mov     eax, DWORD PTR [rbp-4]
    mov     esi, eax
    lea     rax, .LC0[rip]
    mov     rdi, rax
    mov     eax, 0
    call    printf@PLT
    mov     eax, 0
    leave
    ret
```

### Відповіді на питання

**1. Якою мовою представлений `program.s`?**
Assembly (асемблер) для архітектури x86-64. Це текстовий файл з інструкціями процесора.

**2. Чи є це machine code?**
Ні, це текстова мнемоніка інструкцій. Machine code — це двійкове представлення (байти), яке генерується на наступному етапі.

**3. Для якої ISA створено цей Assembly?**
Для x86-64 (AMD64). Це видно з використання регістрів `rax`, `rbp`, `rsp`, `edi`, `esi` та 64-бітних інструкцій.

**4. Чи бачите ви `main` та `add`?**
Так, обидві функції присутні як мітки (labels) у файлі. `add:` на рядку 21, `main:` на рядку 47.

---

## 6. Завдання 3: Assembly → Object File

### Виконані команди

```bash
gcc -c program.s -o program.o
file program.o
```

### Результат

```
program.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
```

### Відповіді на питання

**1. Чому результат виглядає зовсім інакше, ніж `program.s`?**
`program.o` — це **binary object file** (ELF relocatable). Він містить:
- машинні інструкції у двійковому вигляді;
- таблицю символів (symbol table);
- інформацію про релокації (relocation entries) — місця, які linker повинен заповнити адресами;
- секції `.text`, `.data`, `.bss`, `.rodata` тощо.

`cat program.o` покаже "сміття" (нечитабельні байти), бо це двійковий формат.

**2. Що містить `program.o`?**

| Компонент | Призначення |
|-----------|-------------|
| `.text` | Машинний код функцій `main` та `add` |
| `.data` | Ініціалізовані глобальні змінні (`global_value = 10`) |
| `.bss` | Неініціалізовані глобальні змінні (`global_empty`) |
| `.rodata` | Рядкові літерали ("Result: %d\n") |
| Symbol table | Імена та адреси символів (main, add, global_value, global_empty, printf) |
| Relocations | Записи про те, що адреса `printf` ще не відома |

---

## 7. Завдання 4: Symbols

### Виконана команда

```bash
nm program.o
```

### Результат

```
0000000000000000 T add
0000000000000000 B global_empty
0000000000000000 D global_value
0000000000000018 T main
                 U printf
```

### Розшифровка типів символів

| Літера | Значення |
|--------|----------|
| `T` | Text — визначена функція в `.text` |
| `D` | Data — ініціалізована глобальна змінна в `.data` |
| `B` | BSS — неініціалізована глобальна змінна в `.bss` |
| `U` | Undefined — символ, який використовується, але не визначений у цьому файлі |

### Відповіді на питання

**1. Чи містить `program.o` реалізацію `add()`?**
Так. `add` позначений як `T` (Text) — його машинний код знаходиться в `.text` цього об'єктного файлу.

**2. Чи містить він реалізацію `main()`?**
Так. `main` також `T` (адреса `0x18`).

**3. Чи містить він реалізацію `printf()`?**
Ні. `printf` позначений як `U` (Undefined). У `program.o` є лише виклик `printf@PLT`, але самого коду `printf` немає.

**4. Якщо ні — як програма зможе використати `printf()`?**
Саме для цього потрібен **linker**. Він знайде реалізацію `printf` у стандартній C-бібліотеці (`libc.so.6`) і зв'яже виклик з реальною адресою.

---

## 8. Завдання 5: Linking

### Виконані команди

```bash
gcc program.o -o program
file program
./program
```

### Результати

```
program: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=873b086..., for GNU/Linux 3.2.0, not stripped
Result: 30
```

### Порівняння `program.o` та `program`

| Властивість | `program.o` | `program` |
|-------------|-------------|-----------|
| Тип ELF | `REL` (relocatable) | `DYN` (PIE executable) |
| Символ `printf` | `U` (undefined) | Вирішено через PLT/GOT |
| Релокації | Присутні | Оброблені лінкером |
| Точка входу | Відсутня | Є `Entry point address` |
| Program headers | Немає | Є (для завантажувача) |
| Можна запустити? | Ні | Так |

---

## 9. Завдання 6–7: Linker error та її виправлення

### Створення помилки

```c
// main.c
#include <stdio.h>
int calculate(int a, int b);

int main(void) {
    int result = calculate(10, 20);
    printf("%d\n", result);
    return 0;
}
```

```bash
gcc -c main.c -o main.o    # ✅ Успішно
gcc main.o -o app          # ❌ Помилка
```

### Помилка

```
/usr/bin/x86_64-linux-gnu-ld.bfd: main.o: in function `main':
main.c:(.text+0x17): undefined reference to `calculate'
collect2: error: ld returned 1 exit status
```

### Відповідь на питання

**Чому компіляція пройшла успішно, а лінкування — ні?**

- **Компіляція** перевіряє лише синтаксис і семантику C. Оголошення `int calculate(int a, int b);` (прототип) достатньо, щоб компілятор згенерував виклик. Він не перевіряє, чи існує реалізація.
- **Лінкування** шукає реальне визначення символу `calculate`. У `main.o` він позначений як `U` (undefined). Linker переглядає всі надані `.o` файли та бібліотеки, але не знаходить `calculate` — тому помилка.

### Виправлення

```c
// calculate.c
int calculate(int a, int b) {
    return a + b;
}
```

```bash
gcc -c calculate.c -o calculate.o
nm main.o        # U calculate
nm calculate.o   # T calculate
gcc main.o calculate.o -o app
./app            # 30
```

### Схема лінкування

```
main.o ────────┐
               │
calculate.o ───┼──→ Linker → app
               │
libc (printf) ─┘
```

---

## 10. Завдання 8: Дослідження ELF

### Виконані команди

```bash
file program
readelf -h program
```

### Заповнена таблиця ELF properties

| ELF property | Значення |
|--------------|----------|
| **Class** | `ELF64` |
| **Data** | `2's complement, little endian` |
| **Type** | `DYN (Position-Independent Executable file)` |
| **Machine** | `Advanced Micro Devices X86-64` |
| **Entry point address** | `0x1060` (це адреса `_start`, не `main`) |

### Відповіді на питання

**Що означає `ELF64`?**
Це 64-бітний формат ELF. Адреси в такому файлі займають 64 біти (8 байт), що дозволяє адресувати до 2⁶⁴ байт пам'яті.

**Яку архітектуру показує Machine?**
`Advanced Micro Devices X86-64` — архітектура x86-64 (AMD64).

**Важливо:** Entry point — це не `main`! Це адреса `_start` — точки входу з C runtime (CRT), яка потім викликає `__libc_start_main`, а та вже викликає `main`.

---

## 11. Завдання 9: ELF sections

### Виконана команда

```bash
readelf -S program
```

### Заповнена таблиця секцій

| Section | Address | Size | Flags | Що містить (у контексті нашої програми) |
|---------|---------|------|-------|------------------------------------------|
| `.text` | `0x1060` | `0x142` (322 байти) | AX | Машинний код функцій `main`, `add`, а також код CRT (`_start`, `__libc_csu_init` тощо) |
| `.rodata` | `0x2000` | `0x10` (16 байт) | A | Рядковий літерал `"Result: %d\n"` та інші константи (read-only) |
| `.data` | `0x4000` | `0x14` (20 байт) | WA | Ініціалізована глобальна змінна `global_value = 10` |
| `.bss` | `0x4014` | `0xc` (12 байт) | WA (NOBITS) | Неініціалізована глобальна змінна `global_empty` |

### Зв'язок із конкретною програмою

```c
int global_value = 10;   // → .data (має початкове значення)
int global_empty;        // → .bss  (нульова ініціалізація, не зберігається у файлі)
```

**Зверніть увагу:**
- `.rodata` має лише прапорець `A` (alloc), без `W` — тому read-only
- `.data` та `.bss` мають `WA` — записувані
- `.bss` має тип `NOBITS` — у файлі не займає місця (лише розмір у заголовку)
- `.text` має `AX` — виконуваний

---

## 12. Завдання 10: Перевірка `.data` та `.bss`

### Програма `sections.c`

```c
#include <stdio.h>

int initialized = 123;
int uninitialized;

static int static_initialized = 456;
static int static_uninitialized;

const char message[] = "Computer Systems";

int main(void) {
    printf("%d %d %d %d %s\n",
           initialized, uninitialized,
           static_initialized, static_uninitialized, message);
    return 0;
}
```

### Виконані команди

```bash
gcc -Wall -Wextra sections.c -o sections
nm sections
readelf -S sections
objdump -s -j .data sections
objdump -s -j .rodata sections
size sections
ls -lh sections
```

### Результати `nm sections` (ключові символи)

| Символ | Адреса | Тип | Секція |
|--------|--------|-----|--------|
| `initialized` | `0x4010` | `D` | `.data` |
| `uninitialized` | `0x401c` | `B` | `.bss` |
| `static_initialized` | `0x4014` | `d` (мала) | `.data` (локальний) |
| `static_uninitialized` | `0x4020` | `b` (мала) | `.bss` (локальний) |
| `message` | `0x2010` | `R` | `.rodata` |
| `main` | `0x1149` | `T` | `.text` |

**Важливо:** `static_initialized` та `static_uninitialized` позначені **малими** літерами `d`/`b` — це означає, що вони **локальні** для файлу (не видно з інших `.o`). Це наслідок `static`.

### Результат `objdump -s -j .data sections`

```
 4000 00000000 00000000 08400000 00000000
 4010 7b000000 c8010000
```

Розшифровка (little-endian):
- `0x4010`: `7b 00 00 00` = **123** → `initialized = 123` ✅
- `0x4014`: `c8 01 00 00` = **456** → `static_initialized = 456` ✅

### Результат `objdump -s -j .rodata sections`

```
 2010 436f6d70 75746572 20537973 74656d73  Computer Systems
 2020 00256420 25642025 64202564 2025730a  .%d %d %d %d %s.
```

- `0x2010`: `"Computer Systems"` — наш `message` ✅ (адреса збігається з `nm`)
- `0x2020`: `"%d %d %d %d %s\n"` — рядковий літерал формату для `printf`

### Заповнена таблиця розміщення

| Змінна | Секція | Пояснення |
|--------|--------|-----------|
| `initialized` | `.data` | Ініціалізована (123) |
| `uninitialized` | `.bss` | Неініціалізована (0) |
| `static_initialized` | `.data` | Ініціалізована (456), локальна для файлу |
| `static_uninitialized` | `.bss` | Неініціалізована, локальна для файлу |
| `message` | `.rodata` | `const char[]` — read-only дані |

---

## 13. Завдання 11: Розмір областей executable

### Початковий стан (з `int uninitialized;`)

```bash
size sections
```

```
   text    data     bss     dec     hex filename
   1450     608      16    2074     81a sections
```

### Після заміни `int uninitialized;` на `int large_array[100000];`

```bash
gcc -Wall -Wextra sections.c -o sections
size sections
ls -lh sections
```

```
   text    data     bss     dec     hex filename
   1450     608  400040  402098   622b2 sections
```

```
-rwxr-xr-x 1 user user 16K Nov 21 12:41 sections
```

### Порівняння

| Секція | Було | Стало | Зміна |
|--------|------|-------|-------|
| `text` | 1450 | 1450 | 0 |
| `data` | 608 | 608 | 0 |
| `bss` | 16 | 400040 | **+400024** |
| **Розмір файлу** | **16K** | **16K** | **~0** |

### Відповідь на центральне питання

**Чому `large_array[100000]` (400 000 байт) суттєво збільшує `.bss`, але executable file не збільшується на ~400 000 байт?**

**Відповідь:**

`.bss` має тип **`NOBITS`** — у файлі зберігається лише **розмір секції** (у заголовку), а не самі дані. За стандартом C всі неініціалізовані глобальні змінні ініціалізуються нулем. Але **немає сенсу зберігати 400 000 нулів у файлі** — це марна трата дискового простору.

При завантаженні програми **loader/OS виділяє пам'ять під `.bss` і заповнює її нулями** (зазвичай через `mmap` з `MAP_ANONYMOUS`, який гарантує нулі).

**Формула:**
```
Розмір файлу ≈ text + data + rodata + headers
Розмір у пам'яті ≈ text + data + rodata + bss + headers
```

Саме тому `size` показує логічний розмір `.bss`, а `ls -lh` — фізичний розмір файлу.

---

## 14. Завдання 12: Dynamic libraries

### Виконана команда

```bash
ldd program
```

### Результат

```
linux-vdso.so.1 (0x0000757f08ebc000)
libc.so.6 => /usr/lib/x86_64-linux-gnu/libc.so.6 (0x0000757f08c00000)
/lib64/ld-linux-x86-64.so.2 (0x0000757f08ebe000)
```

### Відповіді на питання

**1. Чи знаходиться повна реалізація `printf()` безпосередньо у нашому executable?**
Ні. У executable є лише виклик через PLT (Procedure Linkage Table) та запис у GOT (Global Offset Table). Сама реалізація `printf` знаходиться в `libc.so.6`, яка завантажується динамічно під час запуску.

**2. Що означає `.so`?**
**Shared Object** — спільний об'єкт (динамічна бібліотека). Це ELF-файл типу `DYN`, який містить код і дані, що можуть бути завантажені в пам'ять і використані кількома процесами одночасно.

**3. Чому декілька програм можуть використовувати одну shared library?**
ОС завантажує `libc.so.6` в фізичну пам'ять **один раз**, а потім відображає її (memory-mapping) в адресний простір кожного процесу. Фізичні сторінки пам'яті спільні, а віртуальні адреси в кожному процесі можуть відрізнятися.

**4. Яку перевагу це дає порівняно з копіюванням усього library code в кожен executable?**

| Аспект | Static linking | Dynamic linking |
|--------|----------------|-----------------|
| Розмір executable | Великий (код libc всередині) | Малий |
| Використання RAM | Кожен процес має свою копію | Одна копія на всі процеси |
| Оновлення бібліотеки | Потрібна перекомпіляція | Достатньо оновити `.so` |
| Час запуску | Швидший | Повільніший (потрібне завантаження) |
| Залежності | Немає | Потрібні відповідні `.so` |

---

## 15. Завдання 13: Executable та process

### Програма `process.c`

```c
#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("PID: %d\n", getpid());
    printf("Process is running...\n");
    sleep(60);
    return 0;
}
```

### Виконані команди

```bash
gcc -Wall -Wextra process.c -o process
./process
# PID: 3811
# Process is running...

# У другому терміналі:
ps -p 3811
```

### Результат `ps -p 3811`

```
    PID TTY          TIME CMD
   3811 pts/4    00:00:00 process
```

### Пояснення різниці

| Поняття | `process` (файл) | `PID = 3811` (процес) |
|---------|------------------|------------------------|
| Що це | ELF executable на диску | Запущений екземпляр програми в пам'яті |
| Стан | Статичний файл | Активна сутність зі станом |
| Пам'ять | Займає місце на диску | Має власний address space |
| Ресурси | Немає | PID, file descriptors, threads, memory |
| Кількість | Один файл | Може бути багато процесів з одного файлу |

**Ключова ідея:** executable — це "рецепт", process — це "приготована страва". Один executable може породити багато процесів (наприклад, запустивши `./process` кілька разів).

---

## 16. Завдання 14: Де знаходяться дані програми?

### Програма `memory.c`

```c
#include <stdio.h>
#include <stdlib.h>

int global_initialized = 10;
int global_uninitialized;

static int static_initialized = 20;
static int static_uninitialized;

int main(void) {
    int local = 30;
    int *dynamic = malloc(sizeof(int));
    if (dynamic == NULL) return 1;
    *dynamic = 40;

    printf("Function main:         %p\n", (void *)main);
    printf("Global initialized:    %p\n", (void *)&global_initialized);
    printf("Global uninitialized:  %p\n", (void *)&global_uninitialized);
    printf("Static initialized:    %p\n", (void *)&static_initialized);
    printf("Static uninitialized:  %p\n", (void *)&static_uninitialized);
    printf("Local variable:        %p\n", (void *)&local);
    printf("Dynamic allocation:    %p\n", (void *)dynamic);

    free(dynamic);
    return 0;
}
```

### Виконані команди

```bash
gcc -Wall -Wextra -O0 memory.c -o memory
./memory
./memory
./memory
```

### Прогноз (до запуску)

| Об'єкт | Очікувана область |
|--------|-------------------|
| `main` | **code** (`.text`) |
| `global_initialized` | **data** (`.data`) |
| `global_uninitialized` | **bss** (`.bss`) |
| `static_initialized` | **data** (`.data`) |
| `static_uninitialized` | **bss** (`.bss`) |
| `local` | **stack** |
| `dynamic allocation` | **heap** |

### Реальні результати (3 запуски)

| Об'єкт | Запуск 1 | Запуск 2 | Запуск 3 | Область |
|--------|----------|----------|----------|---------|
| `main` | `0x5fca253811a9` | `0x5ebf847d11a9` | `0x6268c76c91a9` | `.text` |
| `global_initialized` | `0x5fca25384010` | `0x5ebf847d4010` | `0x6268c76cc010` | `.data` |
| `global_uninitialized` | `0x5fca2538401c` | `0x5ebf847d401c` | `0x6268c76cc01c` | `.bss` |
| `static_initialized` | `0x5fca25384014` | `0x5ebf847d4014` | `0x6268c76cc014` | `.data` |
| `static_uninitialized` | `0x5fca25384020` | `0x5ebf847d4020` | `0x6268c76cc020` | `.bss` |
| `local` | `0x7fffa840c46c` | `0x7fff1b8149fc` | `0x7ffe958f21fc` | stack |
| `dynamic allocation` | `0x5fca4e3d5010` | `0x5ebfb3184010` | `0x6268ccb22010` | heap |

### Інтерпретація адрес

**1. Які адреси знаходяться близько одна до одної?**
`main`, `global_*`, `static_*` — усі в діапазоні `0x5fca...` (низькі адреси).

**2. Чи знаходяться `global_initialized` і `global_uninitialized` поруч?**
Так, `0x...4010` та `0x...401c` — різниця 12 байт. Це сусідні комірки в сегменті даних. `static_initialized` та `static_uninitialized` також поруч.

**3. Чи знаходиться `local` поруч із global variables?**
Ні. `local` на stack (`0x7fff...`), глобальні — в сегменті даних (`0x5fca...`). Це різні області address space.

**4. Чи знаходиться пам'ять, отримана через `malloc`, поруч із `local`?**
Ні. `malloc` повертає адресу з heap (`0x5fca...`), а `local` — зі stack (`0x7fff...`). Вони в різних кінцях address space.

**5. Що це говорить про організацію address space?**
Address space процесу розділений на логічні області:
- **нижні адреси**: `.text` (код), `.rodata`, `.data`, `.bss`, далі heap (зростає вгору);
- **верхні адреси**: stack (зростає вниз);
- між ними — велика "порожня" область.

### Відповіді на питання розділу 22 (ASLR)

**Чи однакові адреси при кожному запуску?** Ні.

**Чому?** ASLR (Address Space Layout Randomization) — сучасні ОС рандомізують базові адреси сегментів при кожному запуску для захисту від exploitation. Але **відносне розташування** об'єктів залишається тим самим.

---

## 17. Завдання 15: Stack

### Програма `stack.c`

```c
#include <stdio.h>

void level3(void) {
    int c = 30;
    printf("level3: &c = %p\n", (void *)&c);
}

void level2(void) {
    int b = 20;
    printf("level2: &b = %p\n", (void *)&b);
    level3();
}

void level1(void) {
    int a = 10;
    printf("level1: &a = %p\n", (void *)&a);
    level2();
}

int main(void) {
    level1();
    return 0;
}
```

### Виконані команди

```bash
gcc -Wall -Wextra -O0 stack.c -o stack
./stack
./stack
```

### Результати (2 запуски)

**Запуск 1:**
```
level1: &a = 0x7ffefca3aea4
level2: &b = 0x7ffefca3ae84
level3: &c = 0x7ffefca3ae64
```

**Запуск 2:**
```
level1: &a = 0x7ffe12c368d4
level2: &b = 0x7ffe12c368b4
level3: &c = 0x7ffe12c36894
```

### Відповіді на питання

**1. Чи змінюються адреси при вкладених function calls?**
Так, кожен виклик функції створює новий **stack frame**, і локальні змінні розміщуються в ньому.

**2. У якому напрямку змінюються адреси у вашому експерименті?**
Адреси **зменшуються** при заглибленні: `&a > &b > &c`. Це означає, що stack зростає **вниз** (від вищих адрес до нижчих).

**3. Чи узгоджується це з моделлю stack для x86-64?**
Так. На x86-64 stack росте вниз: `push` зменшує `rsp`, `call` зменшує `rsp` для збереження return address. Кожен новий frame розміщується за нижчими адресами, ніж попередній.

**Важливо:** це правило для конкретної ISA/ABI (x86-64 System V). На інших архітектурах напрямок може відрізнятися.

**Спостереження:** різниця між сусідніми фреймами — 32 байти (`0x20`) в обох запусках.

---

## 18. Завдання 16: Heap

### Програма `heap.c`

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *a = malloc(sizeof(int));
    int *b = malloc(sizeof(int));
    int *c = malloc(sizeof(int));
    if (a == NULL || b == NULL || c == NULL) {
        free(a); free(b); free(c);
        return 1;
    }
    printf("a: %p\n", (void *)a);
    printf("b: %p\n", (void *)b);
    printf("c: %p\n", (void *)c);
    free(a); free(b); free(c);
    return 0;
}
```

### Прогноз

**Чи повинні адреси `a`, `b`, `c` бути послідовними?**
Не обов'язково. Allocator (glibc malloc) може виділяти блоки з різних причин не послідовно: вирівнювання, метадані, повторне використання звільнених блоків.

### Виконані команди

```bash
gcc -Wall -Wextra -O0 heap.c -o heap
./heap
./heap
```

### Результати (2 запуски)

**Запуск 1:**
```
a: 0x6050cafdd010
b: 0x6050cafdd030
c: 0x6050cafdd050
```

**Запуск 2:**
```
a: 0x5cb15b000010
b: 0x5cb15b000030
c: 0x5cb15b000050
```

### Відповіді на питання

**Чи повинні адреси `a`, `b`, `c` бути послідовними?**
У цьому простому випадку glibc malloc виділив послідовні chunk'и (різниця 0x20 = 32 байти — типовий розмір chunk). Але **не робіть висновку "malloc завжди повертає послідовні адреси"**. Allocator може організовувати heap різними способами.

**Що показує експеримент?**
- Адреси в діапазоні `0x6050...` / `0x5cb1...` — це **heap** (низькі адреси, але далеко від stack `0x7fff...`)
- **ASLR** змінює базу heap між запусками (`0x6050...` → `0x5cb1...`), але відносне розташування `a < b < c` зберігається
- Мета експерименту — побачити, що dynamic allocations належать до іншої частини address space, ніж local stack variables

---

## 19. Фінальне завдання — `memory_map.c`

### Програма `memory_map.c`

```c
#include <stdio.h>
#include <stdlib.h>

/* 1. Функція */
int my_function(void) {
    return 42;
}

/* 2. Ініціалізована глобальна змінна */
int global_initialized = 10;

/* 3. Неініціалізована глобальна змінна */
int global_uninitialized;

/* 4. Ініціалізована статична змінна */
static int static_initialized = 20;

/* 5. Неініціалізована статична змінна */
static int static_uninitialized;

/* 6. Рядковий літерал */
const char *string_literal = "Computer Systems";

int main(void) {
    /* 7. Локальна змінна */
    int local = 30;

    /* 8. Динамічне виділення */
    int *heap_allocation = malloc(sizeof(int));
    if (heap_allocation == NULL) return 1;
    *heap_allocation = 40;

    printf("Code/function:            %p\n", (void *)my_function);
    printf("Global initialized:       %p\n", (void *)&global_initialized);
    printf("Global uninitialized:     %p\n", (void *)&global_uninitialized);
    printf("Static initialized:       %p\n", (void *)&static_initialized);
    printf("Static uninitialized:     %p\n", (void *)&static_uninitialized);
    printf("String literal:           %p\n", (void *)string_literal);
    printf("Heap allocation:          %p\n", (void *)heap_allocation);
    printf("Stack/local:              %p\n", (void *)&local);

    free(heap_allocation);
    return 0;
}
```

### Виконані команди

```bash
gcc -Wall -Wextra -O0 memory_map.c -o memory_map
./memory_map
./memory_map
```

### Результати (2 запуски)

**Запуск 1:**
```
Code/function:            0x619fa28f31a9
Global initialized:       0x619fa28f6010
Global uninitialized:     0x619fa28f6024
Static initialized:       0x619fa28f6014
Static uninitialized:     0x619fa28f6028
String literal:           0x619fa28f4004
Heap allocation:          0x619fa78b2010
Stack/local:              0x7ffcc2092d3c
```

**Запуск 2:**
```
Code/function:            0x64d631fe41a9
Global initialized:       0x64d631fe7010
Global uninitialized:     0x64d631fe7024
Static initialized:       0x64d631fe7014
Static uninitialized:     0x64d631fe7028
String literal:           0x64d631fe5004
Heap allocation:          0x64d666f66010
Stack/local:              0x7ffe61d85c8c
```

### Концептуальна карта пам'яті (запуск 1)

```
Higher addresses
┌──────────────────────────────────────────────┐
│        Stack                                 │
│ local:              0x7ffcc2092d3c           │
├──────────────────────────────────────────────┤
│          ...                                 │
├──────────────────────────────────────────────┤
│        Heap                                  │
│ heap_allocation:    0x619fa78b2010           │
├──────────────────────────────────────────────┤
│        .bss                                  │
│ global_uninitialized: 0x619fa28f6024         │
│ static_uninitialized: 0x619fa28f6028         │
├──────────────────────────────────────────────┤
│        .data                                 │
│ global_initialized: 0x619fa28f6010           │
│ static_initialized: 0x619fa28f6014           │
├──────────────────────────────────────────────┤
│       .rodata                                │
│ string_literal:     0x619fa28f4004           │
├──────────────────────────────────────────────┤
│        .text                                 │
│ my_function:        0x619fa28f31a9           │
└──────────────────────────────────────────────┘
Lower addresses
```

### Ключові спостереження

**1. Групування адрес:**

| Область | Діапазон (запуск 1) | Об'єкти |
|---------|---------------------|---------|
| `.text` | `0x619fa28f31a9` | `my_function` (код) |
| `.rodata` | `0x619fa28f4004` | `string_literal` |
| `.data` | `0x619fa28f6010–6014` | `global_initialized`, `static_initialized` |
| `.bss` | `0x619fa28f6024–6028` | `global_uninitialized`, `static_uninitialized` |
| Heap | `0x619fa78b2010` | `malloc` |
| Stack | `0x7ffcc2092d3c` | `local` |

**2. Відносне розташування всередині областей:**
- `.data`: `global_initialized` (`...6010`) → `static_initialized` (`...6014`) — різниця 4 байти
- `.bss`: `global_uninitialized` (`...6024`) → `static_uninitialized` (`...6028`) — різниця 4 байти
- `.data` та `.bss` йдуть підряд: `.data` закінчується на `...6014`, `.bss` починається з `...6024` (між ними 16 байт вирівнювання)

**3. ASLR між запусками:**

| Об'єкт | Запуск 1 | Запуск 2 | Змінилася? |
|--------|----------|----------|------------|
| Code/function | `0x619fa28f31a9` | `0x64d631fe41a9` | Так (база executable) |
| Global initialized | `0x619fa28f6010` | `0x64d631fe7010` | Так (разом з executable) |
| String literal | `0x619fa28f4004` | `0x64d631fe5004` | Так (разом з executable) |
| Heap allocation | `0x619fa78b2010` | `0x64d666f66010` | Так (окремо) |
| Stack/local | `0x7ffcc2092d3c` | `0x7ffe61d85c8c` | Так (окремо) |

**Відносне розташування** всередині кожної області зберігається: `global_initialized` завжди на `+0x10` від бази `.data`, `static_initialized` — на `+0x14`, тощо.

---

## 20. Фінальний аналіз build pipeline для `memory_map.c`

### Виконані команди

```bash
gcc -E memory_map.c -o memory_map.i
gcc -S -O0 -masm=intel memory_map.i -o memory_map.s
gcc -c memory_map.s -o memory_map.o
gcc memory_map.o -o memory_map
```

### Результати `file` для кожного етапу

```
memory_map.c: C source, Unicode text, UTF-8 text
memory_map.i: C source, ASCII text
memory_map.s: assembler source, ASCII text
memory_map.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
memory_map:   ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=b968614d7b9a504366dc65f38ef52f28c5eee68e, for GNU/Linux 3.2.0, not stripped
```

### Фінальна таблиця build pipeline

| Файл | Етап | Що містить | Чи може CPU виконати безпосередньо? |
|------|------|-----------|-------------------------------------|
| `memory_map.c` | Source code | C-код з коментарями (UTF-8) | Ні |
| `memory_map.i` | Preprocessing | C-код з розгорнутими директивами (ASCII) | Ні |
| `memory_map.s` | Compilation | Assembly (текст) | Ні |
| `memory_map.o` | Assembly | Machine code + symbols + relocations (ELF relocatable) | Ні (потрібне лінкування) |
| `memory_map` | Linking | Повністю слінкований ELF (PIE, dynamically linked) | Так (після завантаження) |

**Зверніть увагу:**
- `.c` → `Unicode text, UTF-8` (бо містить символи UTF-8, наприклад, українські коментарі)
- `.i` → `ASCII text` (препроцесор нормалізував)
- `.o` → `relocatable` (ще не слінкований)
- executable → `pie executable, dynamically linked` (готовий до запуску)

---

## 21. Відповіді на контрольні питання

1. **Які основні етапи проходить C-програма до створення executable?**
   Preprocessing → Compilation → Assembly → Linking.

2. **Що робить preprocessor?**
   Обробляє директиви `#include`, `#define`, `#ifdef` тощо. Розгортає заголовні файли, замінює макроси. Результат — `.i` файл (все ще C-код).

3. **Що створює compiler?**
   Перетворює C-код (`.i`) в Assembly (`.s`) для конкретної ISA.

4. **Що робить assembler?**
   Перетворює Assembly (`.s`) в object file (`.o`) — machine code + symbol table + relocations.

5. **Що містить object file?**
   Machine code (`.text`), дані (`.data`, `.bss`, `.rodata`), symbol table, relocation entries, debug info (якщо є).

6. **Навіщо потрібен linker?**
   Об'єднує кілька `.o` файлів та бібліотек в один executable, вирішує символи (relocations), розміщує секції за адресами.

7. **Що означає `undefined reference`?**
   Лінкер не знайшов визначення символу, який використовується в програмі.

8. **Що таке symbol?**
   Ім'я (функції або змінної) з адресою та типом (`T`, `D`, `B`, `U` тощо). Використовується лінкером для зв'язування.

9. **Чим header file відрізняється від library?**
   Header (`.h`) містить оголошення (прототипи, макроси, типи), вставляється препроцесором. Library (`.a`/`.so`) містить реалізації (machine code), лінкується лінкером.

10. **Що таке ELF?**
    Executable and Linkable Format — стандартний формат бінарних файлів у Linux (executable, object, shared library, core dump).

11. **Що зазвичай містить `.text`?**
    Машинний код (інструкції) програм.

12. **Що зазвичай містить `.rodata`?**
    Read-only дані: рядкові літерали, `const`-константи, таблиці переходів.

13. **У чому різниця між `.data` та `.bss`?**
    `.data` — ініціалізовані глобальні/статичні змінні (зберігаються у файлі). `.bss` — неініціалізовані (у файлі лише розмір, при завантаженні заповнюються нулями).

14. **Чому велика `.bss` не обов'язково означає такий самий приріст розміру executable?**
    `.bss` має тип `NOBITS` — у файлі зберігається лише розмір, а не дані. При завантаженні OS виділяє пам'ять і заповнює нулями.

15. **Що показує `ldd`?**
    Список динамічних бібліотек, від яких залежить executable, та шляхи до них.

16. **Чим executable відрізняється від process?**
    Executable — файл на диску. Process — запущений екземпляр з власним address space, PID, file descriptors тощо.

17. **Де концептуально розташовуються local variables?**
    На stack (високі адреси, росте вниз).

18. **Для чого використовується heap?**
    Для динамічного виділення пам'яті (`malloc`/`free`), розмір якої невідомий на етапі компіляції або змінюється під час виконання.

19. **Яка різниця між stack та heap?**
    Stack — автоматичне виділення/звільнення (при вході/виході з функції), росте вниз, обмежений розмір. Heap — ручне виділення/звільнення (`malloc`/`free`), росте вгору, більший розмір, повільніший.

20. **Чому адреси можуть змінюватися між запусками програми?**
    ASLR (Address Space Layout Randomization) — захист від exploitation, рандомізує базові адреси сегментів.

21. **Що таке temporal locality?**
    Властивість програм: якщо до даних звернулися, ймовірно, до них звернуться знову найближчим часом. Використовується в кешах.

22. **Що таке spatial locality?**
    Властивість програм: якщо до даних звернулися, ймовірно, до сусідніх даних теж звернуться. Використовується в кешах (cache lines).

---

## 22. Висновок

Лабораторна робота експериментально підтвердила:

- **Повний build pipeline:** `.c → .i → .s → .o → executable`
- **Роль preprocessor, compiler, assembler, linker** — кожен етап виконує свою функцію
- **Механізм linker error** та його виправлення: компіляція перевіряє лише синтаксис, лінкер шукає реальні визначення символів
- **Структуру ELF:** `.text`, `.rodata`, `.data`, `.bss` — кожна секція має своє призначення та прапорці
- **Різницю між `.data` та `.bss`:** `.data` зберігається у файлі, `.bss` — лише розмір (NOBITS)
- **Динамічне лінкування:** `printf` знаходиться в `libc.so.6`, а не в executable; переваги спільного використання бібліотек
- **Різницю між executable та process:** файл на диску vs запущений екземпляр з PID
- **Розташування об'єктів у address space:** `.text` (низькі) → `.rodata` → `.data` → `.bss` → heap → stack (високі)
- **Напрямок росту stack (вниз) та heap (вгору)** на x86-64
- **ASLR:** адреси змінюються між запусками, але відносне розташування зберігається

Дослідження показало, що команда `gcc program.c -o program` приховує складний багатоетапний процес, кожен етап якого можна дослідити окремо за допомогою інструментів `gcc -E/-S/-c`, `readelf`, `objdump`, `nm`, `ldd`, `size`.
