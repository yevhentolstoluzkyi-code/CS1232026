#!/usr/bin/env python3
"""Перевірка ЛР 10: зовнішнє дерево Buildroot, зібраний образ і його запуск у QEMU.

    python3 tools/lab10-test.py КАТАЛОГ [--output O] [--buildroot BR] [--ssh-key KEY]

КАТАЛОГ  — ваш каталог submissions/ГРУПА_Прізвище (у ньому br2-external/).
--output — каталог збирання Buildroot (той, що передавали як O=...):
           без нього перевіряється лише дерево, без образу.
--buildroot — дерево Buildroot: тоді ще й utils/check-package.
--ssh-key — приватний ключ, чий публічний лежить в overlay:
           тоді перевіряється вхід через ssh (dropbear) з хоста.
--accel  — auto (типово), kvm або tcg: щоб порівняти час завантаження.

Образ запускається з -snapshot: файл rootfs.ext2 не змінюється.
Спілкування з системою йде через послідовну консоль (ttyS0), як у людини.
Кожен тест друкує OK, FAIL з поясненням або SKIP; наприкінці — N/M.
"""
import argparse
import os
import re
import select
import shutil
import subprocess
import sys
import time

ap = argparse.ArgumentParser()
ap.add_argument("dir")
ap.add_argument("--output")
ap.add_argument("--buildroot")
ap.add_argument("--ssh-key")
ap.add_argument("--accel", choices=("auto", "kvm", "tcg"), default="auto",
                help="auto: KVM, якщо /dev/kvm доступний; tcg — примусово без KVM")
ap.add_argument("--timeout", type=int, default=600,
                help="скільки секунд чекати на login: (TCG без KVM повільний)")
A = ap.parse_args()

D = os.path.abspath(A.dir)
EXT = os.path.join(D, "br2-external")
O = os.path.abspath(A.output) if A.output else None
results = []
INFO = {}


class Skip(Exception):
    pass


def test(fn):
    name = fn.__name__
    try:
        msg = fn()
    except Skip as e:
        print(f"SKIP {name}: {e}")
        return fn
    except subprocess.TimeoutExpired:
        msg = "завис (timeout)"
    except FileNotFoundError as e:
        msg = f"немає файла {e.filename}"
    except Exception as e:  # noqa: BLE001
        msg = f"виняток {type(e).__name__}: {e}"
    ok = msg is None
    results.append(ok)
    print(f"{'OK  ' if ok else 'FAIL'} {name}" + ("" if ok else f": {msg}"))
    return fn


def read(*parts):
    with open(os.path.join(*parts), encoding="utf-8") as fh:
        return fh.read()


def kv_lines(text):
    """Рядки KEY=VALUE з defconfig/.config без коментарів."""
    out = {}
    for line in text.splitlines():
        m = re.match(r"^(BR2_\w+)=(.*)$", line.strip())
        if m:
            out[m.group(1)] = m.group(2)
    return out


# ───────────────────────── дерево br2-external ─────────────────────────

DEFCONFIG = os.path.join(EXT, "configs", "lab10_qemu_x86_64_defconfig")


@test
def t_tree():
    desc = read(EXT, "external.desc")
    if not re.search(r"^name:\s*LAB10\s*$", desc, re.M):
        return "external.desc: має бути рядок 'name: LAB10'"
    if "BR2_EXTERNAL_LAB10_PATH" not in read(EXT, "external.mk"):
        return "external.mk має підключати package/*/*.mk через $(BR2_EXTERNAL_LAB10_PATH)"
    cfg = read(EXT, "Config.in")
    for p in ("lab10-sysinfo", "lab10-hello"):
        if f"package/{p}/Config.in" not in cfg:
            return f"Config.in не підключає package/{p}/Config.in"
    return None


@test
def t_defconfig():
    text = read(DEFCONFIG)
    c = kv_lines(text)
    need = {
        "BR2_x86_64": "y",
        "BR2_TOOLCHAIN_EXTERNAL": "y",
        "BR2_LINUX_KERNEL": "y",
        "BR2_PACKAGE_LAB10_SYSINFO": "y",
        "BR2_PACKAGE_LAB10_HELLO": "y",
        "BR2_PACKAGE_DROPBEAR": "y",
        "BR2_TARGET_ROOTFS_EXT2": "y",
    }
    for k, v in need.items():
        if c.get(k) != v:
            return f"немає {k}={v}"
    if not any(k.startswith("BR2_TOOLCHAIN_EXTERNAL_BOOTLIN_X86_64_GLIBC") for k in c):
        return "тулчейн: потрібен Bootlin x86-64 glibc (stable або bleeding-edge)"
    if c.get("BR2_PACKAGE_HOST_QEMU") == "y":
        return "host-qemu вимкніть: образ запускаємо системним QEMU, а збирання QEMU — зайва година"
    for k in ("BR2_ROOTFS_OVERLAY", "BR2_LINUX_KERNEL_CONFIG_FRAGMENT_FILES",
              "BR2_ROOTFS_POST_IMAGE_SCRIPT"):
        if "$(BR2_EXTERNAL_LAB10_PATH)" not in c.get(k, ""):
            return f"{k} має вказувати в дерево через $(BR2_EXTERNAL_LAB10_PATH)"
    if re.search(r"/(home|Users|mnt)/", text):
        return "абсолютний шлях до вашої машини: у когось іншого defconfig не збереться"
    host = c.get("BR2_TARGET_GENERIC_HOSTNAME", "").strip('"')
    if not re.fullmatch(r"lab10-[a-z][a-z0-9-]*", host):
        return "BR2_TARGET_GENERIC_HOSTNAME: lab10-прізвище латиницею"
    INFO["hostname"] = host
    return None


@test
def t_fragment():
    text = read(EXT, "board", "lab10", "linux.fragment")
    for line in ('CONFIG_LOCALVERSION="-lab10"', "CONFIG_IKCONFIG=y",
                 "CONFIG_IKCONFIG_PROC=y", "CONFIG_PRINTK_TIME=y"):
        if not re.search(rf"^{re.escape(line)}\s*$", text, re.M):
            return f"у linux.fragment немає {line}"
    return None


@test
def t_packages():
    mk = read(EXT, "package", "lab10-sysinfo", "lab10-sysinfo.mk")
    if "TARGET_CONFIGURE_OPTS" not in mk and "TARGET_CC" not in mk:
        return "lab10-sysinfo.mk: збирайте крос-компілятором ($(TARGET_CONFIGURE_OPTS) або $(TARGET_CC))"
    if "$(TARGET_DIR)" not in mk:
        return "lab10-sysinfo.mk: програма має потрапити в $(TARGET_DIR)"
    if not re.search(r"^\$\(eval \$\(generic-package\)\)\s*$", mk, re.M):
        return "lab10-sysinfo.mk: немає $(eval $(generic-package))"
    mk = read(EXT, "package", "lab10-hello", "lab10-hello.mk")
    km = mk.find("$(eval $(kernel-module))")
    gp = mk.find("$(eval $(generic-package))")
    if km < 0 or gp < 0 or km > gp:
        return "lab10-hello.mk: спочатку $(eval $(kernel-module)), потім $(eval $(generic-package))"
    for p in ("lab10-sysinfo", "lab10-hello"):
        up = p.upper().replace("-", "_")
        mk = read(EXT, "package", p, f"{p}.mk")
        if not re.search(rf"^{up}_LICENSE\s*=", mk, re.M):
            return f"{p}.mk: вкажіть {up}_LICENSE (make legal-info)"
    if "depends on BR2_LINUX_KERNEL" not in read(EXT, "package", "lab10-hello", "Config.in"):
        return "lab10-hello/Config.in: модуль без ядра не зібрати — depends on BR2_LINUX_KERNEL"
    return None


@test
def t_overlay():
    ov = os.path.join(EXT, "board", "lab10", "rootfs-overlay")
    s99 = os.path.join(ov, "etc", "init.d", "S99lab10")
    if not os.access(s99, os.X_OK):
        return "S99lab10 не виконуваний: rcS його пропустить (chmod +x і git add)"
    conf = read(ov, "etc", "modules-load.d", "lab10.conf")
    m = re.search(r"^lab10_hello\s+greeting=(\S+)", conf, re.M)
    if not m:
        return "modules-load.d/lab10.conf: рядок 'lab10_hello greeting=Прізвище'"
    INFO["greeting"] = m.group(1)
    if not os.access(os.path.join(EXT, "board", "lab10", "post-image.sh"), os.X_OK):
        return "post-image.sh не виконуваний"
    return None


@test
def t_check_package():
    if not A.buildroot:
        raise Skip("не вказано --buildroot")
    cp = os.path.join(A.buildroot, "utils", "check-package")
    files = []
    for root, _, names in os.walk(EXT):
        for n in names:
            if n.endswith((".mk", ".in", ".desc")) or n.startswith("S") and "init.d" in root:
                files.append(os.path.join(root, n))
    r = subprocess.run([cp, "-b"] + files, capture_output=True, text=True, timeout=120)
    warn = re.search(r"(\d+) warnings generated", r.stdout + r.stderr)
    if r.returncode != 0 or not warn or warn.group(1) != "0":
        tail = (r.stdout + r.stderr).strip().splitlines()[-3:]
        return "check-package: " + " | ".join(tail)
    return None


# ───────────────────────── результат збирання ─────────────────────────

def need_output():
    if not O:
        raise Skip("не вказано --output")


def elf_info(path):
    with open(path, "rb") as fh:
        head = fh.read(64)
    if head[:4] != b"\x7fELF":
        return None
    machine = int.from_bytes(head[18:20], "little")
    return {"class": head[4], "machine": machine}


@test
def t_output_config():
    need_output()
    c = kv_lines(read(O, ".config"))
    for k in ("BR2_PACKAGE_LAB10_SYSINFO", "BR2_PACKAGE_LAB10_HELLO"):
        if c.get(k) != "y":
            return f"у {O}/.config немає {k}=y: чи з тим defconfig зібрано?"
    for f in ("bzImage", "rootfs.ext2"):
        if not os.path.isfile(os.path.join(O, "images", f)):
            return f"немає images/{f}: make не дійшов до кінця"
    sq = os.path.join(O, "images", "start-qemu.sh")
    if not os.access(sq, os.X_OK):
        return "немає images/start-qemu.sh: post-image.sh не відпрацював"
    INFO["sizes"] = {f: os.path.getsize(os.path.join(O, "images", f))
                     for f in ("bzImage", "rootfs.ext2")}
    return None


@test
def t_output_cross():
    need_output()
    exe = os.path.join(O, "target", "usr", "bin", "lab10-sysinfo")
    info = elf_info(exe)
    if not info or info["class"] != 2 or info["machine"] != 62:
        return "target/usr/bin/lab10-sysinfo — не ELF64 x86-64"
    r = subprocess.run(["readelf", "-p", ".comment", exe], capture_output=True, text=True)
    if "Buildroot" not in r.stdout:
        return ("у .comment немає 'Buildroot': програму зібрано не тулчейном Buildroot "
                "(чи не скопіювали хостовий бінарник у overlay?)")
    return None


@test
def t_output_module():
    need_output()
    rel_files = [os.path.join(dp, "include", "config", "kernel.release")
                 for dp in (os.path.join(O, "build", d) for d in os.listdir(os.path.join(O, "build"))
                            if d.startswith("linux-") and not d.startswith("linux-headers"))]
    rel_files = [f for f in rel_files if os.path.isfile(f)]
    if not rel_files:
        return "немає build/linux-*/include/config/kernel.release"
    rel = read(rel_files[0]).strip()
    INFO["release"] = rel
    if not rel.endswith("-lab10"):
        return f"версія ядра {rel}: фрагмент LOCALVERSION не застосовано"
    ko = os.path.join(O, "target", "lib", "modules", rel, "updates", "lab10_hello.ko")
    if not os.path.isfile(ko):
        return f"немає /lib/modules/{rel}/updates/lab10_hello.ko"
    r = subprocess.run(["modinfo", "-F", "vermagic", ko], capture_output=True, text=True)
    if r.returncode == 0 and not r.stdout.startswith(rel + " "):
        return f"vermagic '{r.stdout.strip()}' не збігається з ядром {rel}"
    dep = read(O, "target", "lib", "modules", rel, "modules.dep")
    if "updates/lab10_hello.ko:" not in dep:
        return "lab10_hello немає в modules.dep: modprobe його не знайде"
    return None


# ───────────────────────── запуск у QEMU ─────────────────────────

class Console:
    def __init__(self, images):
        self.buf = b""
        self.log = open(os.path.join(TMPLOG, "console.log"), "wb")
        cmd = ["qemu-system-x86_64", "-M", "pc", "-m", "256", "-smp", "2",
               "-kernel", os.path.join(images, "bzImage"),
               "-drive", f"file={os.path.join(images, 'rootfs.ext2')},if=virtio,format=raw",
               "-snapshot",
               "-append", "rootwait root=/dev/vda console=ttyS0",
               "-display", "none", "-serial", "stdio", "-monitor", "none",
               "-no-reboot",
               "-nic", "user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:10022-:22"]
        self.kvm = A.accel == "kvm" or (A.accel == "auto" and os.access("/dev/kvm", os.W_OK))
        if self.kvm:
            cmd[1:1] = ["-enable-kvm", "-cpu", "host"]
        self.t0 = time.monotonic()
        self.p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT)
        os.set_blocking(self.p.stdout.fileno(), False)

    def _pump(self, timeout):
        r, _, _ = select.select([self.p.stdout], [], [], timeout)
        if r:
            data = os.read(self.p.stdout.fileno(), 65536)
            if not data:
                raise EOFError("QEMU завершився")
            self.buf += data
            self.log.write(data)
            self.log.flush()

    def expect(self, pattern, timeout):
        rx = re.compile(pattern)
        end = time.monotonic() + timeout
        while True:
            m = rx.search(self.buf)
            if m:
                self.buf = self.buf[m.end():]
                return m
            left = end - time.monotonic()
            if left <= 0:
                raise subprocess.TimeoutExpired("expect", timeout)
            self._pump(min(left, 0.5))

    def send(self, text):
        self.p.stdin.write(text.encode())
        self.p.stdin.flush()

    def sh(self, cmd, timeout=60):
        """Виконати команду в оболонці гостя; повертає (код, вивід)."""
        # Маркери розірвано лапками, щоб луна набраної команди з ними не збіглася.
        self.send(f'echo "__B""EGIN__"; {cmd}; echo "__E""ND__$?"\n')
        self.expect(rb"__BEGIN__\r?\n", timeout)
        m = self.expect(rb"(?s)(.*?)__END__(\d+)", timeout)
        out = m.group(1).decode(errors="replace").replace("\r", "")
        return int(m.group(2)), out.strip()

    def close(self):
        if self.p.poll() is None:
            self.p.kill()
        self.p.wait()
        self.log.close()


TMPLOG = os.path.abspath(".")
VM = {}


def vm():
    need_output()
    if "con" in VM:
        return VM["con"]
    if "error" in VM:
        raise Skip("система не завантажилась (див. t_boot)")
    raise Skip("t_boot не запускався")


@test
def t_boot():
    need_output()
    if not shutil.which("qemu-system-x86_64"):
        return "немає qemu-system-x86_64: sudo apt install qemu-system-x86"
    images = os.path.join(O, "images")
    con = Console(images)
    try:
        con.expect(rb"login: ", A.timeout)
    except Exception as e:  # noqa: BLE001
        VM["error"] = str(e)
        con.close()
        if b"host forwarding" in con.buf:
            return "порт 10022 зайнятий: чи не працює ще один QEMU (pkill qemu-system)?"
        return f"не дочекалися login: ({type(e).__name__}); журнал консолі — console.log"
    INFO["login_s"] = time.monotonic() - con.t0
    INFO["accel"] = "KVM" if con.kvm else "TCG"
    try:
        con.send("root\n")
        m = con.expect(rb"Password: |# ", 60)
        if m.group(0).startswith(b"Password"):
            con.send("\n")          # BR2_TARGET_GENERIC_ROOT_PASSWD порожній
            con.expect(rb"# ", 60)
        con.send("stty -echo; dmesg -n 1; PS1='# '\n")
        con.expect(rb"# ", 30)
    except Exception as e:  # noqa: BLE001
        VM["error"] = str(e)
        con.close()
        return "login: є, але оболонки root немає (пароль root у defconfig?); див. console.log"
    VM["con"] = con
    return None


@test
def t_kernel():
    con = vm()
    _, rel = con.sh("uname -r")
    if not rel.endswith("-lab10"):
        return f"uname -r = {rel}"
    _, host = con.sh("hostname")
    if host != INFO.get("hostname", host):
        return f"hostname {host}, а в defconfig {INFO.get('hostname')}"
    code, n = con.sh("zcat /proc/config.gz | grep -c '^CONFIG_IKCONFIG_PROC=y'")
    if code != 0 or n != "1":
        return "немає /proc/config.gz з CONFIG_IKCONFIG_PROC=y"
    _, line = con.sh("dmesg | grep -m1 'Run /sbin/init'")
    m = re.match(r"\[\s*(\d+\.\d+)\]", line)
    if not m:
        return "у dmesg немає міток часу або рядка 'Run /sbin/init' (PRINTK_TIME?)"
    INFO["init_s"] = float(m.group(1))
    return None


@test
def t_module_autoload():
    con = vm()
    code, _ = con.sh("grep -q '^lab10_hello ' /proc/modules")
    if code != 0:
        return "lab10_hello не завантажено під час старту (S11modules + modules-load.d)"
    _, g = con.sh("cat /sys/module/lab10_hello/parameters/greeting")
    if g != INFO.get("greeting", g):
        return f"greeting = {g}, а в lab10.conf — {INFO.get('greeting')}"
    _, line = con.sh("dmesg | grep -m1 'lab10_hello: hello'")
    if INFO.get("release", "") not in line:
        return f"рядок привітання без версії ядра: {line!r}"
    return None


@test
def t_module_cycle():
    con = vm()
    code, out = con.sh("rmmod lab10_hello 2>/dev/null; modprobe lab10_hello greeting=review && "
                       "cat /sys/module/lab10_hello/parameters/greeting")
    if code != 0 or out.splitlines()[-1:] != ["review"]:
        return f"modprobe з параметром не спрацював: {out!r}"
    code, out = con.sh("rmmod lab10_hello && dmesg | grep -c 'lab10_hello: bye, review'")
    con.sh("modprobe lab10_hello greeting=review")
    if code != 0 or int((out.splitlines() or ["0"])[-1] or 0) < 1:
        return f"rmmod не спрацював або немає 'bye, review' у dmesg: {out!r}"
    return None


@test
def t_sysinfo():
    con = vm()
    code, out = con.sh("lab10-sysinfo")
    if code != 0:
        return f"lab10-sysinfo повернула {code}"
    m = re.search(r"^procs:\s+(\d+)$", out, re.M)
    if not m:
        return "у звіті немає рядка 'procs: N'"
    _, real = con.sh("ls -d /proc/[0-9]* | wc -l")
    if abs(int(m.group(1)) - int(real)) > 3:
        return f"procs: {m.group(1)}, а каталогів /proc/<PID> — {real}"
    if not re.search(r"^modules:\s+\d+ loaded, lab10_hello: yes$", out, re.M):
        return "рядок modules: має показати 'lab10_hello: yes'"
    if not re.search(r"^kernel:\s+Linux \S+-lab10 x86_64$", out, re.M):
        return "рядок kernel: не той"
    code, _ = con.sh("lab10-sysinfo -m lab10_hello")
    code2, _ = con.sh("lab10-sysinfo -m no_such_module")
    if (code, code2) != (0, 1):
        return f"-m: коди виходу {code} і {code2}, а треба 0 і 1"
    m = re.search(r"^memory:\s+total (\d+) MiB", out, re.M)
    if m:
        INFO["mem_total"] = int(m.group(1))
    m = re.search(r"^rootfs:\s+used ([\d.]+) of", out, re.M)
    if m:
        INFO["rootfs_used"] = float(m.group(1))
    INFO["procs"] = int(real)
    return None


@test
def t_boot_script():
    con = vm()
    code, line = con.sh("dmesg | grep -m1 'LAB10 boot done in'")
    m = re.search(r"LAB10 boot done in ([\d.]+) s", line)
    if code != 0 or not m:
        return "S99lab10 не відпрацював (немає 'LAB10 boot done' у dmesg)"
    INFO["s99_s"] = float(m.group(1))
    return None


@test
def t_ssh():
    vm()
    if not A.ssh_key:
        raise Skip("не вказано --ssh-key")
    if not shutil.which("ssh"):
        return "немає ssh на хості"
    cmd = ["ssh", "-i", A.ssh_key, "-p", "10022", "-o", "BatchMode=yes",
           "-o", "StrictHostKeyChecking=no", "-o", "UserKnownHostsFile=/dev/null",
           "-o", "ConnectTimeout=20", "-o", "LogLevel=ERROR", "root@127.0.0.1", "uname -r"]
    for _ in range(6):
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        if r.returncode == 0:
            break
        time.sleep(5)
    if r.returncode != 0 or not r.stdout.strip().endswith("-lab10"):
        return f"ssh -p 10022 root@127.0.0.1: {r.stderr.strip() or r.stdout.strip()}"
    return None


@test
def t_poweroff():
    con = vm()
    con.send("poweroff\n")
    try:
        con.p.wait(timeout=120)
    except subprocess.TimeoutExpired:
        con.close()
        return "після poweroff QEMU не завершився за 120 с"
    con.close()
    VM.pop("con", None)
    return None


if "con" in VM:
    VM["con"].close()

if INFO:
    print("\nВиміряно:")
    for k in ("accel", "login_s", "init_s", "s99_s", "release", "procs", "mem_total",
              "rootfs_used", "sizes"):
        if k in INFO:
            v = INFO[k]
            print(f"  {k:12} {v:.2f}" if isinstance(v, float) else f"  {k:12} {v}")
print(f"\n{sum(results)}/{len(results)}")
sys.exit(0 if results and all(results) else 1)
