# ЛР 11. Надутий Денис, КІ-31

## Середовище виконання

- Студент: Надутий Денис, група КІ-31.
- Хост: Ubuntu 22.04.5 LTS у WSL2.
- Архітектура: x86-64.
- Buildroot: 2026.08.1.
- Clang: Ubuntu clang 14.0.0-1ubuntu1.1.
- `/dev/kvm`: доступний.
- Каталог Buildroot: `~/buildroot`.
- Каталог збирання: `~/br-lab11`.
- Спільний каталог завантажень: `~/br-dl`.
- Вільне місце перед роботою: приблизно 928 ГБ.

## Частина 0. Підготовка і конфігурація

Створено гілку `lab11-nadutyi` та зовнішнє дерево
`br2-external` на основі каркаса ЛР11.

До overlay додано публічний ключ Ed25519.
Приватний ключ у репозиторій не копіювався.

У `linux.fragment` збережено базові параметри
ядра з суфіксом `-lab11` і додано шість параметрів:

- `CONFIG_BPF_SYSCALL=y`;
- `CONFIG_BPF_JIT=y`;
- `CONFIG_FTRACE_SYSCALLS=y`;
- `CONFIG_KPROBES=y`;
- `CONFIG_BPF_EVENTS=y`;
- `CONFIG_DYNAMIC_DEBUG=y`.

У defconfig встановлено hostname `lab11-nadutyi`
та банер `Welcome to LAB11 (KI-31_Nadutyi)`.

Конфігурацію завантажено в `~/br-lab11`
та збережено командою `make savedefconfig`.

Фактичну наявність усіх шести можливостей
у зібраному ядрі перевіримо після `make`.

### Результати першого збирання

Buildroot 2026.08.1 зібрано для x86-64 з ядром Linux 6.18.7.

Під час першого збирання виникли проблеми із залежностями
та конфігурацією, які було усунено:

1. Для збирання bpftool були відсутні заголовки
   `gelf.h` і `libelf.h`. Встановлено `libelf-dev`.
2. Збірка очікувала команду `llvm-strip`, тоді як
   встановлено `llvm-strip-14`. Створено локальне
   посилання `~/.local/bin/llvm-strip`.
3. Перевірка архітектури Buildroot відхиляла
   об'єктні файли `Linux BPF`. У `lab11-bpf.mk`
   додано вузький виняток `LAB11_BPF_BIN_ARCH_EXCLUDE`
   для файлів `.bpf.o`.
4. Для `host-libffi` виникло повторне застосування
   патча. Очищено лише каталог цього пакета
   командою `make host-libffi-dirclean`.
5. У початковій конфігурації ядра були вимкнені
   FTRACE та залежні можливості трасування.
   До `linux.fragment` додано `CONFIG_FTRACE=y`
   і `CONFIG_KPROBE_EVENTS=y`. Після
   `make linux-reconfigure` усі вісім перевірених
   параметрів отримали значення `y`.

**Фактичні результати:**

- `make linux-reconfigure`: код завершення 0.
- Обов'язкові шість параметрів TODO 1: 6/6.
- Разом із додатковими залежностями: 8/8.
- Завершальний `make`: код завершення 0.
- Тривалість завершального `make`: 14,029 с.
- `images/bzImage`: приблизно 7,8 МіБ.
- `images/rootfs.ext2`: 60 МіБ.
- `images/start-qemu.sh`: створений і виконуваний.

Образ створено. Завантаження в QEMU і можливості
ядра в гостьовій системі перевіряються далі.

## Частина 1. Можливості ядра та трасування

Дослідження виконано у QEMU з KVM, Linux 6.18.7-lab11.
Гостьова система: `lab11-nadutyi`, 2 віртуальні процесори.

Підтверджено роботу `CONFIG_BPF_SYSCALL=y`,
`CONFIG_BPF_JIT=y`, `CONFIG_FTRACE_SYSCALLS=y`,
`CONFIG_KPROBES=y`, `CONFIG_BPF_EVENTS=y`
і `CONFIG_DYNAMIC_DEBUG=y`.

`bpftool feature probe kernel` підтвердив доступність
системного виклику `bpf()` та увімкнений JIT-компілятор.
Використання `bpf()` непривілейованими процесами обмежене:
`unprivileged_bpf_disabled = 2`.
До дослідів з модулями `kernel/tainted = 0`.

### Питання для звіту №1

Порівняння Linux 6.18.7 у ЛР10 та ЛР11:

| Показник | ЛР10 | ЛР11 | Різниця |
|---|---:|---:|---:|
| Рядки `.config` | 3756 | 3830 | +74 |
| `bzImage`, байт | 6616064 | 8074240 | +1458176 |

Розмір `bzImage` збільшився приблизно на 22,04%.
Зростання пов'язане з увімкненням додаткових можливостей
трасування та eBPF. Кількість рядків `.config` показує
різницю конфігурацій, а не кількість увімкнених опцій.

У tracefs виявлено 92 підсистеми, 1453 події та
335 подій `sys_enter_*`.

Для події `sys_enter_openat`:
- ID події: 626;
- `filename`: offset = 24 байти, size = 8 байтів;
- поле містить вказівник на рядок у пам'яті процесу,
  тому для читання шляху потрібен безпечний доступ
  до пам'яті користувача.

### Питання для звіту №2

У `/proc/kallsyms` виявлено 80913 рядків з інформацією
про символи ядра.

У `Module.symvers` виявлено 9453 експортовані символи:
- `EXPORT_SYMBOL`: 5070;
- `EXPORT_SYMBOL_GPL`: 4383.

`/proc/kallsyms` описує ширший набір символів,
ніж `Module.symvers`. Не кожен символ ядра доступний
для прямого виклику із завантажуваного модуля.

Для використання модулем символ має бути експортований.
Символи `EXPORT_SYMBOL_GPL` доступні модулям
із сумісною з GPL ліцензією. Наявність назви функції
у `/proc/kallsyms` сама собою не означає, що модуль
може з нею компонуватися.

Kprobe дозволяє спостерігати за виконанням деяких
неекспортованих функцій, якщо їх можна пробувати,
але не перетворює такі функції на доступний API модуля.

Після правильного ввімкнення трасування символ
`for_each_kernel_tracepoint` з'явився у `Module.symvers`
як `EXPORT_SYMBOL_GPL`.

## Частина 2. Модуль ядра lab11_opencount

### Реалізація та перевірка

TODO 2–4 виконано: параметри target (0444) і verbose (0644),
атомарні лічильники, tracepoint sys_enter для openat,
безпечне копіювання шляху, реєстрація й зняття пробника.

Збирання: `make lab11-opencount-rebuild all` — код 0.
Середовище: QEMU/KVM, Linux 6.18.7-lab11.

`modinfo`:
- license: GPL
- vermagic: 6.18.7-lab11 SMP preempt mod_unload
- target: Process comm to count (read-only), string
- verbose: Log matching openat paths when enabled, bool

`modprobe lab11_opencount target=openbench` — успішно.
`openbench -n 1000 -r 1` — 898.6 ns.

`/proc/lab11_opencount` після тесту:
- target: openbench
- verbose: 0
- total: 1040
- matched: 1004

При verbose=1 журнал показав відкриття:
`/etc/ld.so.cache`, `/lib64/libc.so.6`, `/etc/hostname`,
а також спроби в каталогах `glibc-hwcaps`.

Після ввімкнення dynamic debug отримано повідомлення:
`openbench[146] openat #1012`, `#1013`, `#1014`.

Запис у `/sys/module/lab11_opencount/parameters/target`
повернув `Permission denied`; target залишився openbench.

Перед вивантаженням: total=1089, matched=1017.
`rmmod lab11_opencount` — код 0.
Повідомлення: `unloaded: total=1108 matched=1017`.
Після rmmod файл `/proc/lab11_opencount` відсутній.
`kernel/tainted` до та після rmmod: 4096.

### Питання для звіту №3

Після 1000 операцій open+close модуль нарахував 1004
виклики openat процесу openbench. Додаткові чотири
виклики пов'язані із запуском динамічно скомпонованої
програми: відкриттям кешу завантажувача та бібліотек.
У verbose видно `/etc/ld.so.cache` і `libc.so.6`.
Загальний total=1040 також враховує інші процеси.

Значення taint модуля — O, загальне tainted — 4096.
O означає out-of-tree module, його біт — 2^12.
Ліцензія GPL не скасовує ознаки зовнішнього модуля.
Після rmmod цей прапорець залишається.

Запис у target повернув Permission denied, оскільки
в коді `module_param_string(..., 0444)` — лише читання.

### Питання для звіту №4

`tracepoint_probe_unregister` від'єднує пробник,
але обробник на іншому процесорі ще може виконуватися.
`tracepoint_synchronize_unregister` чекає його завершення,
щоб не допустити виконання вже вивантаженого коду.

Звичайний тест рідко виявляє відсутність синхронізації,
бо гонка потребує збігу виконання пробника з rmmod.

`READ_ONCE(verbose)` запобігає небажаній оптимізації
читання прапорця, який змінюється через sysfs.

`pr_info_ratelimited` обмежує частоту повідомлень,
щоб часті openat не переповнювали журнал ядра й не
створювали надмірне навантаження.

## Частина 3. Дослідження lab11_oops

Досліди виконано у QEMU/KVM з -snapshot. Код lab11_oops не змінювався.

### mode=1

До досліду tainted=0. Отримано BUG: kernel NULL pointer dereference, address 0x0. RIP: lab11_oops_init+0x3a/0xff0. CR2=0x0. У стеку: do_one_initcall, do_init_module, load_module, init_module_from_file, __x64_sys_finit_module, do_syscall_64. Процес insmod (PID 137) завершився Killed, rc=137. Після Oops tainted=4224. lsmod: lab11_oops, Used by=1. rmmod: Device or resource busy, rc=1. Повторний insmod спричинив другий Oops у __x64_sys_finit_module+0x112/0x3c0, rc=137.

### mode=2

Після нового запуску QEMU tainted=0. Повідомлення: mode=2: spinning forever, try kill -9. insmod PID 136 залишився після kill -9; State: R (running). loadavg: 0.24 0.05 0.02. rmmod: Device or resource busy. Після досліду tainted=4096.

### Відповідь на питання №5

У mode=1 модуль записав дані за NULL-адресою, що викликало помилку сторінки в режимі ядра. RIP вказав на lab11_oops_init+0x3a/0xff0; CR2=0 підтвердив адресу помилки. Стек показав шлях від системного виклику finit_module через load_module і do_init_module до ініціалізації модуля. insmod був примусово завершений (Killed, rc=137), але система продовжила працювати. Це Oops, а не загальна паніка ядра.

Після Oops модуль залишився в незавершеному стані: rmmod повернув Device or resource busy. Повторне завантаження спричинило другий Oops, тому після пошкодження стану ядра потрібен перезапуск QEMU.

Tainted=4224 означає O (4096, зовнішній модуль) і D (128, ядро пережило Oops). У mode=2 tainted=4096: зовнішній модуль завантажено, але Oops не відбулося.

У mode=2 процес insmod нескінченно виконував код ядра і залишився у стані R після kill -9. Сигнал SIGKILL не може перервати такий цикл, доки виконання не повернеться до місця обробки сигналів. rmmod не зміг вивантажити модуль, ініціалізація якого не завершилася.
