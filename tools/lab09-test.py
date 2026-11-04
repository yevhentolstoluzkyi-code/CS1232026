#!/usr/bin/env python3
"""Перевірка ЛР 9: vercmp, pkgaudit і пакет lab09-pkgtools.

    python3 tools/lab09-test.py КАТАЛОГ        # каталог із зібраними програмами

Потрібні dpkg і dpkg-dev (dpkg-parsechangelog, dpkg-shlibdeps,
dpkg-gencontrol): t_vercmp_dpkg порівнює vercmp з dpkg --compare-versions,
t_deb_* збирають пакет через make deb. Тести не потребують root і нічого
не встановлюють у систему. Кожен тест друкує OK або FAIL з поясненням;
наприкінці — N/M.
"""
import hashlib
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile
import time

D = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")
VERCMP = os.path.join(D, "vercmp")
AUDIT = os.path.join(D, "pkgaudit")
TMP = tempfile.mkdtemp(prefix="lab09-")
results = []


def run(args, env=None, timeout=60, input=None, cwd=None):
    e = dict(os.environ)
    e.pop("SOURCE_DATE_EPOCH", None)
    e["PKGAUDIT_CONF"] = os.path.join(TMP, "none.conf")
    if env:
        e.update(env)
    return subprocess.run(args, capture_output=True, text=True, timeout=timeout,
                          env=e, input=input, cwd=cwd)


def test(fn):
    name = fn.__name__
    try:
        msg = fn()
    except subprocess.TimeoutExpired:
        msg = "завис (timeout)"
    except FileNotFoundError as e:
        msg = f"немає файла {e.filename}: чи зібрано програми (make)?"
    except Exception as e:  # noqa: BLE001
        msg = f"виняток {type(e).__name__}: {e}"
    ok = msg is None
    results.append(ok)
    print(f"{'OK  ' if ok else 'FAIL'} {name}" + ("" if ok else f": {msg}"))
    return fn


def vc(a, op, b):
    return run([VERCMP, a, op, b]).returncode


def check_order(pairs):
    """pairs: (a, b, очікуване: -1, 0, 1). Повертає перше розходження."""
    for a, b, want in pairs:
        got = {0: None}
        lt, eq, gt = vc(a, "lt", b), vc(a, "eq", b), vc(a, "gt", b)
        if 2 in (lt, eq, gt):
            return f"vercmp відкинув коректну версію ('{a}' або '{b}')"
        got = -1 if lt == 0 else 0 if eq == 0 else 1 if gt == 0 else None
        if got != want:
            sign = {-1: "<", 0: "=", 1: ">"}
            return f"{a} {sign.get(got, '?')} {b}, а має бути {a} {sign[want]} {b}"
    return None


# ---------------- частина 1: vercmp ----------------

@test
def t_vercmp_basic():
    return check_order([
        ("1.0", "1.0", 0), ("1.0", "1.1", -1), ("1.10", "1.9", 1),
        ("1.0", "1.0.1", -1), ("2.0", "10.0", -1), ("1.001", "1.1", 0),
        ("1.0a", "1.0", 1), ("1.0a", "1.0b", -1), ("1.0+dfsg", "1.0", 1),
        ("1.0.a", "1.0+a", 1), ("20260101123456789", "20260101123456790", -1),
    ])


@test
def t_vercmp_tilde():
    return check_order([
        ("1.0~rc1", "1.0", -1), ("1.0~rc1", "1.0~rc2", -1), ("1.0~~", "1.0~", -1),
        ("1.0~~a", "1.0~", -1), ("1.0~", "1.0", -1), ("1.0", "1.0+", -1),
        ("2.0~beta", "1.9", 1),
    ])


@test
def t_vercmp_epoch_rev():
    r = check_order([
        ("1:0.9", "2.0", 1), ("0:1.0", "1.0", 0), ("2:1.0", "10:0.1", -1),
        ("1.0-1", "1.0-2", -1), ("1.0-10", "1.0-9", 1), ("1.0-1", "1.0", 1),
        ("1.0-0", "1.0", 0), ("1.0-1ubuntu1", "1.0-1", 1),
        ("1.0-1~bpo1", "1.0-1", -1), ("1.2-3-4", "1.2-3-5", -1),
        ("1:2.43.0-1ubuntu7", "1:2.43.0-.", -1), ("1.0-1+b1", "1.0-1", 1),
    ])
    if r:
        return r
    out = run([VERCMP, "show", "3:1.2-3-4ubuntu1"]).stdout
    want = {"epoch": "3", "upstream": "1.2-3", "revision": "4ubuntu1"}
    got = dict(line.split(None, 1) for line in out.splitlines() if " " in line)
    for k, v in want.items():
        if got.get(k, "").strip() != v:
            return f"vercmp show 3:1.2-3-4ubuntu1: {k} = '{got.get(k, '').strip()}', а має бути {v}"
    return None


@test
def t_vercmp_invalid():
    for bad in ["", "1.0 2", ":1.0", "x:1.0", "1:", "1.0-", "a1.0", "1_0",
                "1.0-a_b", "1.0-1:2", "1:-1", "99999999999:1.0"]:
        r = run([VERCMP, bad, "eq", "1.0"])
        if r.returncode != 2:
            return f"'{bad}' має бути помилкою (код 2), а vercmp повернув {r.returncode}"
    for good in ["0", "1:2:3", "1.0+dfsg~rc1-0.1", "0~git20260101.abcdef-1"]:
        if run([VERCMP, good, "eq", good]).returncode != 0:
            return f"коректну версію '{good}' відкинуто"
    return None


def rand_version(rng):
    parts = ["0", "1", "2", "9", "10", "01", "007", "a", "b", "z", "~", "~~", "~a",
             ".", "+", "A", "rc", "beta", "dfsg", "100", "~rc1", ".1", "+b1", "ubuntu"]
    s = rng.choice("0123456789")
    for _ in range(rng.randint(0, 4)):
        s += rng.choice(parts)
    if rng.random() < 0.25:
        s = f"{rng.randint(0, 3)}:{s}"
    if rng.random() < 0.6:
        s += "-" + rng.choice(["0", "1", "2", "1ubuntu1", "0.1", "1~bpo1", "a",
                               "1+b1", "~1", "10", "1.2.3", "9a"])
    return s


@test
def t_vercmp_dpkg():
    if not shutil.which("dpkg"):
        return "немає dpkg: тест можна виконати лише в Debian чи Ubuntu"
    rng = random.Random(2026)
    pairs = [(rand_version(rng), rand_version(rng)) for _ in range(400)]
    for a, b in pairs:
        for op in ("lt", "eq", "gt"):
            want = subprocess.run(["dpkg", "--compare-versions", a, op, b],
                                  capture_output=True).returncode
            got = vc(a, op, b)
            if got != want:
                return f"vercmp '{a}' {op} '{b}' → {got}, dpkg → {want}"
    return None


@test
def t_vercmp_sort():
    vs = ["1.10-1", "1.9-1", "1:0.1-1", "1.0~rc1-1", "1.0-1", "1.0+dfsg-1",
          "1.0-1ubuntu0.1", "1.0-1~bpo1", "0.99-3"]
    want = ["0.99-3", "1.0~rc1-1", "1.0-1~bpo1", "1.0-1", "1.0-1ubuntu0.1",
            "1.0+dfsg-1", "1.9-1", "1.10-1", "1:0.1-1"]
    r = run([VERCMP, "sort"], input="\n".join(vs) + "\n")
    if r.returncode != 0:
        return f"код {r.returncode}: {r.stderr.strip()[:100]}"
    got = r.stdout.split()
    if got != want:
        return f"порядок {' '.join(got)}"
    return None


# ---------------- частина 2: pkgaudit ----------------

STATUS = """\
Package: base-files
Essential: yes
Status: install ok installed
Priority: required
Installed-Size: 400
Architecture: amd64
Version: 13ubuntu10

Package: libc6
Status: install ok installed
Priority: optional
Installed-Size: 13000
Architecture: amd64
Version: 2.39-0ubuntu8.4
Depends: libgcc-s1
Description: GNU C Library
 Contains the standard libraries.
 .
 Depends: this-is-description-not-a-field

Package: libgcc-s1
Status: install ok installed
Priority: optional
Installed-Size: 150
Architecture: amd64
Version: 14-20240412-0ubuntu1
Depends: gcc-14-base (= 14-20240412-0ubuntu1), libc6 (>= 2.35)

Package: gcc-14-base
Status: install ok installed
Priority: optional
Installed-Size: 270
Architecture: amd64
Version: 14-20240412-0ubuntu1

Package: app-a
Status: install ok installed
Priority: optional
Installed-Size: 900
Architecture: amd64
Version: 1.9-1
Depends: libc6 (>= 2.34), libfoo-virt [amd64] | libfoo-alt, mail-transport-agent
Recommends: helper-r

Package: libfoo1
Status: install ok installed
Priority: optional
Installed-Size: 120
Architecture: amd64
Version: 3.1-2
Provides: libfoo-virt (= 3.1)

Package: libfoo-alt
Status: install ok installed
Priority: optional
Installed-Size: 80
Architecture: amd64
Version: 1.0-1

Package: postfix
Status: install ok installed
Priority: optional
Installed-Size: 4500
Architecture: amd64
Version: 3.8.6-1build2
Provides: default-mta, mail-transport-agent

Package: helper-r
Status: install ok installed
Priority: optional
Installed-Size: 60
Architecture: all
Version: 0.5-1

Package: app-b
Status: install ok installed
Priority: optional
Installed-Size: 700
Architecture: amd64
Version: 2.0-1
Pre-Depends: init-system-helpers:any (>= 1.54~)
Depends: libbar2,
 libbaz3:amd64 (>= 1.0) <!nocheck>,
 app-b

Package: init-system-helpers
Status: install ok installed
Priority: optional
Installed-Size: 130
Architecture: all
Version: 1.66ubuntu1

Package: libbar2
Status: install ok installed
Priority: optional
Installed-Size: 210
Architecture: amd64
Version: 1.0-1

Package: libbaz3
Status: install ok installed
Priority: optional
Installed-Size: 95
Architecture: amd64
Version: 1.2-1

Package: orphan-lib
Status: install ok installed
Priority: optional
Installed-Size: 2000
Architecture: amd64
Version: 0.1-1

Package: tool-z
Status: install ok installed
Priority: optional
Installed-Size: 300
Architecture: amd64
Version: 1:0.9-1

Package: kept-tool
Status: install ok installed
Priority: optional
Installed-Size: 50
Architecture: amd64
Version: 1.0-1

Package: removed-pkg
Status: deinstall ok config-files
Priority: optional
Installed-Size: 99999
Architecture: amd64
Version: 1.0-1
"""

SF = os.path.join(TMP, "status")
with open(SF, "w") as f:
    f.write(STATUS)


def audit(*args, env=None):
    r = run([AUDIT, "-s", SF] + list(args), env=env)
    if r.returncode != 0:
        raise RuntimeError(f"pkgaudit {' '.join(args)}: код {r.returncode} {r.stderr.strip()[:100]}")
    return r.stdout


@test
def t_audit_stats():
    out = audit("stats")
    if "installed 16" not in out:
        return "має бути installed 16 (removed-pkg лише з налаштуваннями — не рахується)"
    if "total_kib 22965" not in out:
        return "має бути total_kib 22965"
    tops = [line.split()[2] for line in out.splitlines() if line.startswith("top ")]
    if tops[:3] != ["libc6", "postfix", "orphan-lib"]:
        return f"top: {' '.join(tops[:3])}"
    return None


def deps(name):
    return [tuple(line.split()) for line in audit("deps", name).splitlines()]


@test
def t_audit_deps():
    got = deps("app-a")
    want = [("D", "libc6"), ("D", "libfoo-virt"), ("D", "libfoo-alt"),
            ("D", "mail-transport-agent"), ("R", "helper-r")]
    if got != want:
        return f"deps app-a: {got}"
    got = deps("app-b")
    want = [("D", "init-system-helpers"), ("D", "libbar2"), ("D", "libbaz3"), ("D", "app-b")]
    if got != want:
        return f"deps app-b (Pre-Depends, продовження рядка, :any, <профіль>): {got}"
    if deps("orphan-lib"):
        return "у orphan-lib немає залежностей"
    return None


def leaves(env=None):
    out = audit("leaves", env=env)
    return sorted(line.split()[2] for line in out.splitlines() if line.startswith("leaf "))


@test
def t_audit_leaves():
    got = leaves()
    want = ["app-a", "app-b", "kept-tool", "orphan-lib", "tool-z"]
    if got != want:
        return f"leaves: {' '.join(got)}; очікувалось {' '.join(want)}"
    conf = os.path.join(TMP, "keep.conf")
    with open(conf, "w") as f:
        f.write("# test\nkeep = kept-tool other\n")
    got = leaves(env={"PKGAUDIT_CONF": conf})
    if "kept-tool" in got:
        return "kept-tool у keep, але показаний як листок"
    out = audit("leaves")
    order = [line.split()[2] for line in out.splitlines() if line.startswith("leaf ")]
    if order[0] != "orphan-lib":
        return "листки мають бути впорядковані за розміром, найбільший перший"
    return None


@test
def t_audit_provides():
    got = leaves()
    for name, why in (("postfix", "надає mail-transport-agent, від якого залежить app-a"),
                      ("libfoo1", "надає libfoo-virt (з версією в Provides)")):
        if name in got:
            return f"{name} не листок: {why}"
    if "app-b" not in got:
        return "app-b залежить сам від себе — це не робить його потрібним"
    return None


@test
def t_audit_diff():
    snap = os.path.join(TMP, "snap")
    with open(snap, "w") as f:
        f.write("libc6 2.39-0ubuntu8\napp-a 1.10-1\ntool-z 2.0-1\nlibbar2 1.0~rc1-1\n"
                "removed-old 0.1\nlibbaz3 1.2-1\n")
    out = audit("diff", snap)
    want = {
        "^ libc6": "оновлено 2.39-0ubuntu8 -> 2.39-0ubuntu8.4",
        "v app-a": "1.10 > 1.9: це зниження версії",
        "^ tool-z": "епоха 1 > 0: це оновлення",
        "^ libbar2": "1.0~rc1 < 1.0: це оновлення",
        "- removed-old": "пакет видалено",
        "+ orphan-lib": "пакет додано",
    }
    lines = out.splitlines()
    for prefix, why in want.items():
        if not any(line.startswith(prefix + " ") for line in lines):
            return f"немає рядка «{prefix} …» ({why})"
    if any(line.split()[1:2] == ["libbaz3"] for line in lines if line[:1] in "^v+-"):
        return "libbaz3 не змінився — його не має бути у diff"
    if "summary added 11 removed 1 upgraded 3 downgraded 1" not in out:
        return "підсумок має бути: added 11 removed 1 upgraded 3 downgraded 1"
    return None


# ---------------- частина 3: пакет ----------------

DEB = {}


def build_deb(env=None):
    for f in os.listdir(D):
        if f.endswith(".deb"):
            os.unlink(os.path.join(D, f))
    r = run(["make", "-s", "deb"] + ([f"SOURCE_DATE_EPOCH={env}"] if env is not None else []),
            cwd=D, timeout=180)
    debs = [f for f in os.listdir(D) if f.endswith(".deb")]
    if r.returncode != 0 or len(debs) != 1:
        raise RuntimeError(f"make deb: код {r.returncode} {r.stderr.strip()[-200:]}")
    return os.path.join(D, debs[0])


def get_deb():
    if "path" not in DEB:
        if not shutil.which("dpkg-gencontrol"):
            raise RuntimeError("немає dpkg-gencontrol: sudo apt install dpkg-dev")
        DEB["path"] = build_deb()
    return DEB["path"]


@test
def t_deb_meta():
    deb = get_deb()
    f = lambda name: run(["dpkg-deb", "-f", deb, name]).stdout.strip()  # noqa: E731
    arch = run(["dpkg", "--print-architecture"]).stdout.strip()
    if f("Architecture") != arch:
        return f"Architecture: {f('Architecture')} — у пакеті скомпільовані програми для {arch}"
    if not re.search(r"(^|, )libc6 \(>= 2\.\d+\)", f("Depends")):
        return f"Depends: '{f('Depends')}' — має бути libc6 з мінімальною версією від dpkg-shlibdeps"
    m = f("Maintainer")
    if not re.fullmatch(r"[^<>]+ <[^@<>\s]+@[^@<>\s]+>", m) or "you@example.org" in m:
        return f"Maintainer: '{m}' — ваші ім'я та пошта"
    desc = run(["dpkg-deb", "-f", deb, "Description"]).stdout.rstrip("\n").split("\n")
    if len(desc) < 3 or desc[0].strip() in ("", "TODO") or not any(l.strip() == "." for l in desc):
        return "Description: короткий опис, довгий опис і хоча б один порожній рядок « .»"
    for field in ("Section", "Priority"):
        if not f(field):
            return f"немає поля {field}"
    conff = run(["sh", "-c", f"dpkg-deb --ctrl-tarfile '{deb}' | tar -xOf - ./conffiles"]).stdout.split()
    if conff != ["/etc/pkgaudit.conf"]:
        return f"conffiles: {conff} — має бути /etc/pkgaudit.conf"
    listing = run(["dpkg-deb", "-c", deb]).stdout
    if any(line.split()[1] != "root/root" for line in listing.splitlines()):
        return "файли в пакеті мають належати root/root"
    if "./etc/pkgaudit.conf" not in listing or "./usr/bin/pkgaudit" not in listing:
        return "у пакеті немає /etc/pkgaudit.conf або /usr/bin/pkgaudit"
    return None


@test
def t_deb_postrm():
    deb = get_deb()
    ctl = os.path.join(TMP, "ctl")
    run(["dpkg-deb", "-e", deb, ctl])
    pr = os.path.join(ctl, "postrm")
    if not os.access(pr, os.X_OK):
        return "у пакеті немає виконуваного postrm"
    root = os.path.join(TMP, "root")
    state = os.path.join(root, "var/lib/pkgaudit")
    os.makedirs(state, exist_ok=True)
    open(os.path.join(state, "snapshot"), "w").close()
    env = {"DPKG_ROOT": root}
    for arg in ("remove", "upgrade"):
        r = run([pr, arg, "1.0-1"], env=env)
        if r.returncode != 0 or not os.path.isdir(state):
            return f"postrm {arg}: код {r.returncode}; каталог знімків має лишитися"
    r = run([pr, "purge"], env=env)
    if r.returncode != 0 or os.path.exists(state):
        return f"postrm purge: код {r.returncode}; ${{DPKG_ROOT}}/var/lib/pkgaudit має зникнути"
    if run([pr, "purge"], env=env).returncode != 0:
        return "другий postrm purge має завершитися успішно (ідемпотентність)"
    if run([pr, "frobnicate"], env=env).returncode == 0:
        return "postrm з невідомим аргументом має повертати помилку"
    return None


@test
def t_deb_reproducible():
    if not shutil.which("dpkg-gencontrol"):
        return "немає dpkg-gencontrol: sudo apt install dpkg-dev"
    hashes = []
    for _ in range(2):
        for name in ("vercmp.c", "pkgaudit.c", "pkgaudit.conf"):
            os.utime(os.path.join(D, name))
        deb = build_deb()
        with open(deb, "rb") as fh:
            hashes.append(hashlib.sha256(fh.read()).hexdigest())
        time.sleep(1.2)
    DEB["path"] = deb
    if hashes[0] != hashes[1]:
        return "дві збірки з однаковим SOURCE_DATE_EPOCH дали різні .deb"
    return None


shutil.rmtree(TMP, ignore_errors=True)
print(f"\n{sum(results)}/{len(results)}")
sys.exit(0 if all(results) else 1)
