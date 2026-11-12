# Лабораторна робота № 10

**Buildroot і QEMU: власна система з пакетом і модулем ядра**

Повний текст роботи — у Google Classroom. Тут лише те, що потрібно для здачі.

## Що здається

```
lab10/submissions/ГРУПА_Прізвище/
├── br2-external/
│   ├── external.desc, external.mk, Config.in   ← скопіювати з lab10/starter/ без змін
│   ├── configs/lab10_qemu_x86_64_defconfig     — TODO 1 (після make savedefconfig)
│   ├── board/lab10/
│   │   ├── linux.fragment                      — TODO 2
│   │   ├── post-image.sh                       ← без змін
│   │   └── rootfs-overlay/
│   │       ├── etc/init.d/S99lab10             ← без змін, виконуваний
│   │       ├── etc/modules-load.d/lab10.conf   — TODO 6a
│   │       └── root/.ssh/authorized_keys       — TODO 6b: ваш ПУБЛІЧНИЙ ключ
│   └── package/
│       ├── lab10-sysinfo/                      — TODO 3 (.mk), TODO 4a і 4b (src/lab10-sysinfo.c)
│       └── lab10-hello/                        — TODO 5a (Config.in), TODO 5b (.mk)
└── REPORT.md
```

Buildroot (`git clone -b 2026.08.1 https://gitlab.com/buildroot.org/buildroot.git ~/buildroot`)
і каталоги збирання (`~/br-stock`, `~/br-lab10`) живуть поза репозиторієм і в коміт не потрапляють.
Так само — образи, `console.log`, `sbom.json` і приватні ключі.

Потрібен Linux на x86-64 або WSL 2 (не `/mnt/c`, а `~/`), близько 15 ГБ диска і пакети
`build-essential git file wget cpio unzip rsync bc libncurses-dev python3 qemu-system-x86
openssh-client kmod graphviz python3-matplotlib python3-magic python3-flake8 shellcheck`.
macOS і Windows на ARM не підходять. Перше збирання стандартної конфігурації — домашнє завдання до заняття.

## Збирання, перевірка й вимірювання

Команди виконуються з вашого каталогу `lab10/submissions/ГРУПА_Прізвище/`.

```bash
cd ~/buildroot
make O=~/br-lab10 BR2_EXTERNAL=$OLDPWD/br2-external lab10_qemu_x86_64_defconfig
make -C ~/br-lab10 menuconfig && make -C ~/br-lab10 savedefconfig
make -C ~/br-lab10
cd -

python3 ../../../tools/lab10-test.py . --output ~/br-lab10 \
    --buildroot ~/buildroot --ssh-key ~/.ssh/id_ed25519      # має бути 17/17
python3 ../../../tools/lab10-test.py . --output ~/br-lab10 --accel tcg   # для порівняння з KVM

~/br-lab10/images/start-qemu.sh                              # вихід: Ctrl-A, X
ssh -p 10022 root@127.0.0.1
```

Тест запускає образ із `-snapshot`, заходить через послідовну консоль і наприкінці друкує
виміряний час завантаження. Без `--output` перевіряється лише дерево (на каркасі проходить `t_tree`).

## Що має бути в REPORT.md

Середовище (система, процесор, `/dev/kvm`, `git describe` Buildroot) · вивід `lab10-test.py` ·
таблиця вимірювань (час збирання обох конфігурацій, пакети, розміри, завантаження KVM/TCG,
ліцензії й SBOM) · `modinfo` модуля і звіт `lab10-sysinfo` з гостя · `ssh` до і після
перезбирання · відповіді на десять питань для звіту · висновки · розділ «Використання ШІ»,
якщо користувалися.
