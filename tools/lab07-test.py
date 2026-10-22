#!/usr/bin/env python3
"""Перевірка ЛР 7: idinfo, permcheck, pwcheck і notes.

    python3 tools/lab07-test.py КАТАЛОГ        # каталог із зібраними програмами

Запускайте від звичайного користувача, не від root. Тести t_pam і t_notes_*
перевіряють програми, встановлені в /opt/lab07 скриптом tools/lab07-setup.sh,
і користуються sudo без пароля: перед запуском виконайте sudo -v.
Кожен тест друкує OK або FAIL з поясненням; наприкінці — N/M.
"""
import hashlib
import os
import pwd
import shutil
import stat
import subprocess
import sys
import tempfile
import time

D = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")
IDINFO = os.path.join(D, "idinfo")
PERM = os.path.join(D, "permcheck")
PW = os.path.join(D, "pwcheck")
NOTES = os.path.join(D, "notes")
OPT = "/opt/lab07"
TMP = tempfile.mkdtemp(prefix="lab07-")
ME = pwd.getpwuid(os.getuid()).pw_name
results = []


def run(args, stdin=None, timeout=60):
    return subprocess.run(args, input=stdin, capture_output=True, text=True, timeout=timeout)


def test(fn):
    name = fn.__name__
    try:
        msg = fn()
    except subprocess.TimeoutExpired:
        msg = "завис (timeout)"
    except Exception as e:  # noqa: BLE001
        msg = f"виняток {type(e).__name__}: {e}"
    ok = msg is None
    results.append(ok)
    print(f"{'OK  ' if ok else 'FAIL'} {name}" + ("" if ok else f": {msg}"))
    return fn


def kv(text):
    """'uid 1 2 3' -> {'uid': ['1', '2', '3']} для кожного рядка."""
    out = {}
    for line in text.splitlines():
        w = line.split()
        if w:
            out[w[0]] = w[1:]
    return out


def as_user(user, args, stdin=None, timeout=60):
    return run(["sudo", "-n", "-u", user, "--"] + args, stdin=stdin, timeout=timeout)


def need_setup():
    if os.geteuid() == 0:
        return "тести запускаються від звичайного користувача, не від root"
    if run(["sudo", "-n", "true"]).returncode != 0:
        return "sudo просить пароль: виконайте sudo -v і запустіть тести ще раз"
    for p in ("notes", "pwcheck"):
        if not os.path.exists(os.path.join(OPT, p)):
            return f"немає {OPT}/{p}: виконайте sudo sh ../../../tools/lab07-setup.sh ."
    return None


def same_build(name):
    def h(p):
        with open(p, "rb") as f:
            return hashlib.sha256(f.read()).hexdigest()
    return h(os.path.join(D, name)) == h(os.path.join(OPT, name))


if os.geteuid() == 0:
    print("Увага: тести запущено від root. Права на файли root не обмежують,\n"
          "тому частина перевірок втрачає сенс. Перейдіть до звичайного користувача.\n")

# ---------------- частина 1: idinfo ----------------


@test
def t_idinfo_self():
    r = run([IDINFO])
    if r.returncode != 0:
        return f"код {r.returncode}: {r.stderr.strip()[:200]}"
    f = kv(r.stdout)
    want_u = [str(os.getuid()), str(os.geteuid()), str(os.getresuid()[2])]
    want_g = [str(os.getgid()), str(os.getegid()), str(os.getresgid()[2])]
    if f.get("uid") != want_u:
        return f"uid {f.get('uid')}, очікувалося {want_u}"
    if f.get("gid") != want_g:
        return f"gid {f.get('gid')}, очікувалося {want_g}"
    if sorted(map(int, f.get("groups", []))) != sorted(os.getgroups()):
        return f"groups {f.get('groups')}, очікувалося {sorted(os.getgroups())}"


@test
def t_idinfo_user():
    for user in (ME, "root"):
        r = run([IDINFO, user])
        if r.returncode != 0:
            return f"idinfo {user}: код {r.returncode}"
        f = kv(r.stdout)
        got = sorted(int(x.split("(")[0]) for x in f.get("member", []))
        want = sorted(set(int(x) for x in run(["id", "-G", user]).stdout.split()))
        if got != want:
            return f"idinfo {user}: member {got}, а id -G дає {want}"
    r = run([IDINFO, "no-such-user-lab07"])
    if r.returncode != 1:
        return f"для неіснуючого користувача код {r.returncode}, очікувався 1"


# ---------------- частина 2: permcheck ----------------

CASES = [
    # MODE, OWNER, GROUP, WANT, UID, GIDS, очікування
    ("100644", 1000, 1000, "r", 1000, "1000", "allow"),
    ("100644", 1000, 1000, "w", 1000, "1000", "allow"),
    ("100644", 1000, 1000, "w", 1001, "1001", "deny"),
    ("100640", 0, 42, "r", 1000, "1000,42", "allow"),       # додаткова група
    ("100640", 0, 42, "r", 1000, "1000", "deny"),
    ("100047", 1000, 1000, "r", 1000, "1000", "deny"),      # власник — лише біти власника
    ("100407", 1000, 100, "r", 1001, "100", "deny"),        # група — лише біти групи
    ("100407", 1000, 100, "r", 1001, "200", "allow"),       # інші
    ("100755", 0, 0, "rx", 1000, "1000", "allow"),
    ("100755", 0, 0, "w", 1000, "1000", "deny"),
    ("100600", 1000, 1000, "rw", 0, "0", "allow"),          # root
    ("100644", 1000, 1000, "x", 0, "0", "deny"),            # root без жодного x
    ("100700", 1000, 1000, "x", 0, "0", "allow"),           # root, x є в одного класу
    ("40700", 1000, 1000, "x", 0, "0", "allow"),            # каталог: root проходить завжди
    ("41777", 0, 0, "wx", 1000, "1000", "allow"),           # /tmp
    ("40751", 1000, 1000, "r", 1001, "1001", "deny"),       # чужий домашній каталог 0751
]


@test
def t_perm_table():
    for mode, owner, group, want, uid, gids, exp in CASES:
        r = run([PERM, "decide", mode, str(owner), str(group), want, str(uid), gids])
        got = r.stdout.strip()
        if got != exp:
            return (f"decide {mode} {owner} {group} {want} {uid} {gids}: "
                    f"{got or r.stderr.strip()[:80]!r}, очікувалося {exp}")


@test
def t_perm_scan():
    root = os.path.join(TMP, "tree")
    os.mkdir(root)
    for m in range(0o1000):                       # файли з усіма 512 режимами
        p = os.path.join(root, f"f{m:03o}")
        open(p, "w").close()
        os.chmod(p, m)
    for m in (0o700, 0o500, 0o300, 0o100, 0o600, 0o000, 0o1777, 0o711):
        d = os.path.join(root, f"d{m:04o}")
        os.mkdir(d)
        open(os.path.join(d, "inner"), "w").close()
        os.chmod(d, m)
    dirs = [root] + [x for x in ("/etc", "/usr/bin", "/var/log") if os.path.isdir(x)]
    bad = []
    for d in dirs:
        r = run([PERM, "scan", d], timeout=300)
        mism = [ln for ln in r.stdout.splitlines() if ln.startswith("mismatch")]
        summary = [ln for ln in r.stdout.splitlines() if ln.startswith("scan")]
        if not summary:
            bad.append(f"{d}: немає рядка scan (код {r.returncode})")
        bad += mism[:3]
    for dpath, _, _ in os.walk(root):
        os.chmod(dpath, 0o700)
    if bad:
        return "; ".join(bad[:4])


# ---------------- частина 3: pwcheck ----------------

VECTORS = [  # пароль "lab07"; $1$/$5$/$6$ — openssl passwd, $y$/$2b$ — mkpasswd
    ("$1$saltsalt$kWp0plNA1OoN08bEhmBl10", "lab07"),
    ("$5$saltsalt$uS7./vv146.bGFTSQ.AMGETwk7QNeBAsSTf19YKtGCD", "lab07"),
    ("$6$saltsalt$2zIKgNuHyHgxjvXeTjzlL8sOLxiQWDleTSGtDjTE4BEGo2vqSTLuO6eVVYUs7Cjau5DPiDank6Y0f8kwn7src/", "lab07"),
    ("$y$j9T$0ubYEHR8Iaiu5W3gP6YNP.$rbhbxsDPyySA6mIvu64G5fH/jsEFrPMashDvNrwKPN2", "lab07"),
    ("$2b$05$ITHCY4JNQwAFmG28oxEYO.aocFgofeI0Pe1dwW7kkgAE75GCSQoom", "lab07"),
]


@test
def t_pw_verify():
    for h, pw in VECTORS:
        r = run([PW, "verify", h], stdin=pw + "\n")
        if r.returncode != 0 or r.stdout.strip() != "ok":
            return f"правильний пароль до {h[:12]}…: код {r.returncode}, вивід {r.stdout.strip()!r}"
        r = run([PW, "verify", h], stdin=pw + "x\n")
        if r.returncode != 1 or r.stdout.strip() != "fail":
            return f"неправильний пароль до {h[:12]}…: код {r.returncode}"
        r = run([PW, "verify", h], stdin=pw[:-1] + "\n")
        if r.returncode != 1:
            return f"пароль без останнього символу до {h[:12]}… прийнято"
    r = run([PW, "verify", "$z$не-хеш"], stdin="lab07\n")
    if r.returncode != 2:
        return f"для нерозпізнаного хешу код {r.returncode}, очікувався 2"


@test
def t_pw_hash():
    for prefix in ("$y$", "$6$", "$2b$"):
        r1 = run([PW, "hash", prefix], stdin="Пароль-123\n")
        r2 = run([PW, "hash", prefix], stdin="Пароль-123\n")
        h1, h2 = r1.stdout.strip(), r2.stdout.strip()
        if r1.returncode != 0 or not h1.startswith(prefix):
            return f"hash {prefix}: код {r1.returncode}, вивід {h1!r}"
        if h1 == h2:
            return f"hash {prefix}: два хеші одного пароля однакові — де сіль?"
        v = run([PW, "verify", h1], stdin="Пароль-123\n")
        if v.returncode != 0:
            return f"hash {prefix}: власний хеш не пройшов verify"


@test
def t_pam():
    why = need_setup()
    if why:
        return why
    if not same_build("pwcheck"):
        return f"{OPT}/pwcheck відрізняється від зібраного: запустіть lab07-setup.sh ще раз"
    pc = os.path.join(OPT, "pwcheck")
    r = as_user("lab7user", [pc, "pam", "lab7user"], stdin="lab7-Pass!\n")
    if r.returncode != 0:
        return f"свій правильний пароль не прийнято: {r.stdout.strip()} {r.stderr.strip()[:120]}"
    t = time.monotonic()
    r = as_user("lab7user", [pc, "pam", "lab7user"], stdin="wrong\n")
    dt = time.monotonic() - t
    if r.returncode != 1:
        return f"неправильний пароль: код {r.returncode}"
    if dt < 1.0:
        return f"відмова за {dt:.2f} с: очікувалася затримка pam_unix близько 2 с"
    r = as_user("lab7user", [pc, "pam", ME], stdin="lab7-Pass!\n")
    if r.returncode == 0:
        return "пароль lab7user прийнято для іншого користувача"


# ---------------- частина 4: notes ----------------

def notes(args, user=None):
    cmd = [os.path.join(OPT, "notes")] + args
    return as_user(user, cmd) if user else run(cmd)


@test
def t_notes_install():
    why = need_setup()
    if why:
        return why
    st = os.stat(os.path.join(OPT, "notes"))
    if not st.st_mode & stat.S_ISUID:
        return "на /opt/lab07/notes немає біта SUID"
    if pwd.getpwuid(st.st_uid).pw_name != "lab7notes":
        return "власник /opt/lab07/notes — не lab7notes"
    if not same_build("notes"):
        return f"{OPT}/notes відрізняється від зібраного: запустіть lab07-setup.sh ще раз"


@test
def t_notes_drop():
    why = need_setup()
    if why:
        return why
    r = notes(["id"])
    f = kv(r.stdout)
    me_u, me_g = str(os.getuid()), str(os.getgid())
    if f.get("uid") != [me_u] * 3:
        return f"після старту uid {f.get('uid')}: очікувалося {[me_u] * 3}"
    if f.get("gid") != [me_g] * 3:
        return f"після старту gid {f.get('gid')}: очікувалося {[me_g] * 3}"


@test
def t_notes_isolation():
    why = need_setup()
    if why:
        return why
    mark = f"lab07-test-{os.getpid()}-{time.time_ns()}"
    r = notes(["add", mark])
    if r.returncode != 0:
        return f"add: код {r.returncode}: {r.stderr.strip()[:120]}"
    if mark not in notes(["list"]).stdout:
        return "своєї нотатки немає в list"
    if mark in notes(["list"], user="lab7user").stdout:
        return "lab7user бачить чужу нотатку"
    out = os.path.join(TMP, "export.txt")
    r = notes(["export", out])
    if r.returncode != 0 or mark not in open(out).read():
        return (f"export у свій файл: код {r.returncode} {r.stderr.strip()[:80]} — "
                    "чи не відкрито файл з правами lab7notes?")
    if os.stat(out).st_uid != os.getuid():
        return "файл експорту належить не вам: його відкрито з правами lab7notes"
    target = "/var/lib/lab07/export-test"
    r = notes(["export", target])
    exists = run(["sudo", "-n", "test", "-e", target]).returncode == 0
    if exists:
        run(["sudo", "-n", "rm", "-f", target])
    if r.returncode == 0 or exists:
        return "export записав файл у /var/lib/lab07: шлях користувача відкрито з правами lab7notes"


shutil.rmtree(TMP, ignore_errors=True)
print(f"\n{sum(results)}/{len(results)}")
sys.exit(0 if all(results) else 1)
