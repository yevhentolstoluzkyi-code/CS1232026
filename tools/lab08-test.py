#!/usr/bin/env python3
"""Перевірка ЛР 8: clocks, ticker, whenis, loc і таймер systemd.

    python3 tools/lab08-test.py КАТАЛОГ        # каталог із зібраними програмами

Тести t_loc_* потребують української локалі (sudo locale-gen uk_UA.UTF-8),
t_units — файла units/lab08-stamp.timer і, якщо є, команди systemd-analyze.
Ticker-тести міряють час: на завантаженій машині запустіть їх ще раз.
Кожен тест друкує OK або FAIL з поясненням; наприкінці — N/M.
"""
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time

D = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")
CLOCKS = os.path.join(D, "clocks")
TICKER = os.path.join(D, "ticker")
WHENIS = os.path.join(D, "whenis")
LOC = os.path.join(D, "loc")
UNITS = os.path.join(D, "units")
TMP = tempfile.mkdtemp(prefix="lab08-")
results = []


def run(args, env=None, timeout=60):
    e = dict(os.environ)
    if env:
        e.update(env)
    return subprocess.run(args, capture_output=True, text=True, timeout=timeout, env=e)


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


def fields(text, key):
    """Усі рядки, що починаються з key, як списки слів без першого."""
    return [line.split()[1:] for line in text.splitlines() if line.split()[:1] == [key]]


def ticker(*args):
    r = run([TICKER] + [str(a) for a in args], timeout=30)
    if r.returncode != 0:
        raise RuntimeError(f"ticker {' '.join(map(str, args))}: код {r.returncode} {r.stderr.strip()[:100]}")
    v = {}
    for line in r.stdout.splitlines():
        w = line.split()
        for i in range(0, len(w) - 1):
            try:
                v.setdefault(w[i], float(w[i + 1].replace(",", ".")))
            except ValueError:
                pass
    return v


# ---------------- частина 1: clocks ----------------

@test
def t_clocks_info():
    r = run([CLOCKS, "info"])
    if r.returncode != 0:
        return f"код {r.returncode}"
    got = {w[0]: w[1:] for w in fields(r.stdout, "clock")}
    for name in ("REALTIME", "MONOTONIC", "MONOTONIC_RAW", "BOOTTIME", "PROCESS_CPUTIME"):
        if name not in got or got[name][0] == "unsupported":
            return f"немає годинника {name}"
    mono, boot = float(got["MONOTONIC"][1]), float(got["BOOTTIME"][1])
    if boot + 0.001 < mono:
        return f"BOOTTIME {boot} менший за MONOTONIC {mono}"


def vdso_clocksource():
    try:
        with open("/sys/devices/system/clocksource/clocksource0/current_clocksource") as f:
            cs = f.read().strip()
    except OSError:
        return None
    return cs if cs in ("tsc", "arch_sys_counter", "kvm-clock", "hyperv_clocksource_tsc_page") else None


@test
def t_clocks_cost():
    r = run([CLOCKS, "cost", "20000"], timeout=120)
    if r.returncode != 0:
        return f"код {r.returncode}"
    cost = {w[0]: (float(w[1]), float(w[2])) for w in fields(r.stdout, "cost")}
    if "MONOTONIC" not in cost:
        return "немає рядка cost MONOTONIC"
    for name, (v, s) in cost.items():
        if not (0 < v < 1e5 and 0 < s < 1e5):
            return f"{name}: {v} і {s} нс — ціна має бути додатною і реалістичною (TODO 1)"
    v, s = cost["MONOTONIC"]
    if vdso_clocksource() and not v < s:
        return f"MONOTONIC через vDSO {v} нс, через syscall {s} нс — vDSO має бути дешевшим"
    if cost.get("PROCESS_CPUTIME", (0, 0))[0] < v:
        return "PROCESS_CPUTIME дешевший за MONOTONIC — перевірте, що міряєте саме годинник id"


# ---------------- частина 2: ticker ----------------

@test
def t_ticker_rel():
    v = ticker("rel", 10, 100, 2)
    if v["drift_ms"] < 150:
        return f"rel: дрейф {v['drift_ms']} мс, а мав би бути ≈ 100 × 2 мс роботи"


@test
def t_ticker_abs():
    v = ticker("abs", 10, 100, 2)
    if abs(v["drift_ms"]) > 5:
        return f"abs: дрейф {v['drift_ms']} мс — дедлайн рахується від «зараз», а не від попереднього (TODO 2)"
    if v["missed"] > 5:
        return f"abs: пропущено {v['missed']} періодів при роботі 2 мс з 10"
    if v["med"] < 0:
        return "abs: медіана запізнення від'ємна — програма прокидається раніше за дедлайн"


@test
def t_ticker_overrun():
    v = ticker("abs", 10, 30, 15)
    if not 25 <= v["missed"] <= 35:
        return f"робота 15 мс при періоді 10 мс: missed {v['missed']}, очікувалося ≈ 30 (по одному на ітерацію)"
    if abs(v["drift_ms"]) > 5:
        return f"дрейф {v['drift_ms']} мс: після пропуску дедлайни мають лишатися на сітці періодів"


@test
def t_ticker_sigterm():
    p = subprocess.Popen([TICKER, "log", "100"], stdout=subprocess.PIPE, text=True)
    time.sleep(0.55)
    p.send_signal(signal.SIGTERM)
    try:
        out, _ = p.communicate(timeout=2)
    except subprocess.TimeoutExpired:
        p.kill()
        return "після SIGTERM програма не завершилася за 2 с"
    if p.returncode != 0:
        return f"після SIGTERM код {p.returncode}, очікувався 0"
    ticks = [line for line in out.splitlines() if line.startswith("tick ")]
    if len(ticks) < 4 or "stopped after" not in out:
        return f"за 0,55 с при періоді 100 мс лише {len(ticks)} рядків tick або немає 'stopped after'"


# ---------------- частина 3: whenis ----------------

def conv(wall, *zones):
    r = run([WHENIS, "conv", wall] + list(zones))
    if r.returncode != 0:
        raise RuntimeError(f"whenis conv '{wall}': код {r.returncode} {r.stderr.strip()[:100]}")
    st = fields(r.stdout, "status")
    return (st[0][0] if st else None), [w[0] for w in fields(r.stdout, "utc")], r.stdout


@test
def t_whenis_ok():
    st, utc, out = conv("2026-06-01 12:00", "Europe/Kyiv", "America/New_York")
    if st != "ok" or utc != ["2026-06-01T09:00:00Z"]:
        return f"12:00 у Києві 1 червня: {st} {utc}, очікувалося ok 2026-06-01T09:00:00Z"
    if "2026-06-01 05:00" not in out:
        return "немає рядка in America/New_York 2026-06-01 05:00"


@test
def t_whenis_gap():
    for wall, zone in (("2026-03-29 03:30", "Europe/Kyiv"), ("2026-03-08 02:30", "America/New_York")):
        st, utc, _ = conv(wall, zone)
        if st != "gap" or utc:
            return f"{wall} {zone} не існувало (годинник перескочив уперед), а отримано {st} {utc} (TODO 3)"


@test
def t_whenis_ambiguous():
    cases = [("2026-10-25 03:30", "Europe/Kyiv", ["2026-10-25T00:30:00Z", "2026-10-25T01:30:00Z"]),
             ("2026-11-01 01:30", "America/New_York", ["2026-11-01T05:30:00Z", "2026-11-01T06:30:00Z"])]
    for wall, zone, want in cases:
        st, utc, _ = conv(wall, zone)
        if st != "ambiguous" or utc != want:
            return f"{wall} {zone} було двічі: очікувалося ambiguous {want}, отримано {st} {utc} (TODO 3)"


@test
def t_whenis_daily():
    r = run([WHENIS, "daily", "09:00", "Europe/Kyiv", "2026-10-23", "5"])
    if r.returncode != 0:
        return f"код {r.returncode}"
    hours = [w[-1] for w in fields(r.stdout, "day") if "hours_since_prev" in w]
    if hours != ["24.0", "25.0", "24.0", "24.0"]:
        return f"години між подіями {hours}, очікувалося 24 25 24 24"


# ---------------- частина 4: loc ----------------

CFG = "# коефіцієнти регулятора\nkp = 3.5\nki=0.25\n  kd = 1.25e-1  \n\nlimit = -2.75\n"
WANT = ["kp 3.5", "ki 0.25", "kd 0.125", "limit -2.75", "total 1.125"]


def have_uk():
    out = run(["locale", "-a"]).stdout.lower().replace("-", "")
    return "uk_ua.utf8" in out


@test
def t_loc_parse():
    if not have_uk():
        return "немає локалі uk_UA.UTF-8: sudo locale-gen uk_UA.UTF-8 (див. розділ 2)"
    path = os.path.join(TMP, "cfg.txt")
    with open(path, "w") as f:
        f.write(CFG)
    for lc in ("C", "uk_UA.UTF-8"):
        r = run([LOC, "parse", path], env={"LC_ALL": lc})
        got = r.stdout.strip().splitlines()
        if r.returncode != 0 or got != WANT:
            return f"LC_ALL={lc}: код {r.returncode}, вивід {got}, очікувалося {WANT} (TODO 4)"


@test
def t_loc_reject():
    if not have_uk():
        return "немає локалі uk_UA.UTF-8: sudo locale-gen uk_UA.UTF-8 (див. розділ 2)"
    for bad in ("3,5", "3.5x", "", "kp"):
        path = os.path.join(TMP, "bad.txt")
        with open(path, "w") as f:
            f.write(f"ok = 1\nkp = {bad}\n")
        for lc in ("C", "uk_UA.UTF-8"):
            r = run([LOC, "parse", path], env={"LC_ALL": lc})
            if r.returncode != 2 or ":2:" not in r.stderr:
                return (f"LC_ALL={lc}, значення '{bad}': код {r.returncode}, stderr '{r.stderr.strip()[:80]}' — "
                        "очікувалися код 2 і номер рядка (TODO 4)")


# ---------------- частина 5: таймер systemd ----------------

def read_unit(path):
    sec, out = None, {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line[0] in "#;":
                continue
            m = re.match(r"\[(.+)\]$", line)
            if m:
                sec = m.group(1)
                continue
            if "=" in line and sec:
                k, v = line.split("=", 1)
                out.setdefault((sec, k.strip()), []).append(v.strip())
    return out


def seconds(span):
    total, units = 0.0, {"ms": 0.001, "s": 1, "sec": 1, "m": 60, "min": 60, "h": 3600, "": 1}
    for num, unit in re.findall(r"([\d.]+)\s*([a-z]*)", span):
        if unit not in units:
            return None
        total += float(num) * units[unit]
    return total


@test
def t_units():
    timer = os.path.join(UNITS, "lab08-stamp.timer")
    u = read_unit(timer)
    cal = u.get(("Timer", "OnCalendar"))
    if not cal:
        return "у lab08-stamp.timer немає OnCalendar у розділі [Timer] (TODO 5)"
    if shutil.which("systemd-analyze"):
        r = run(["systemd-analyze", "calendar", cal[-1]])
        if r.returncode != 0:
            return f"systemd-analyze не розуміє OnCalendar={cal[-1]}"
        norm = re.search(r"Normalized form:\s*(.+)", r.stdout)
        if not norm or norm.group(1).strip() != "*-*-* *:00/2:00":
            return f"OnCalendar={cal[-1]} нормалізується в '{norm.group(1).strip() if norm else '?'}', а потрібна кожна парна хвилина"
    acc = u.get(("Timer", "AccuracySec"))
    if not acc or seconds(acc[-1]) is None or seconds(acc[-1]) > 1:
        return f"AccuracySec={acc[-1] if acc else 'не задано'}: потрібна точність 1 с"
    if (u.get(("Timer", "Persistent")) or ["no"])[-1].lower() not in ("yes", "true", "1", "on"):
        return "немає Persistent=true"
    if "timers.target" not in " ".join(u.get(("Install", "WantedBy"), [])):
        return "у розділі [Install] немає WantedBy=timers.target"
    if shutil.which("systemd-analyze"):
        tmp = os.path.join(TMP, "units")
        os.makedirs(tmp, exist_ok=True)
        for name in ("lab08-stamp.timer", "lab08-stamp.service"):
            with open(os.path.join(UNITS, name)) as f:
                text = re.sub(r"(?m)^ExecStart=.*$", "ExecStart=/bin/true", f.read())
            with open(os.path.join(tmp, name), "w") as f:
                f.write(text)
        r = run(["systemd-analyze", "verify", os.path.join(tmp, "lab08-stamp.timer")])
        if r.returncode != 0:
            return f"systemd-analyze verify: {r.stderr.strip()[:160]}"


shutil.rmtree(TMP, ignore_errors=True)
print(f"\n{sum(results)}/{len(results)}")
sys.exit(0 if all(results) else 1)
