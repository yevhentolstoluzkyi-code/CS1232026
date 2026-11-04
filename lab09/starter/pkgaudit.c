/* pkgaudit — аудит встановлених пакетів за базою dpkg.
 * ЛР 9, «Системне програмне забезпечення».
 *
 *   pkgaudit [-s STATUS] stats             скільки пакетів і місця; 10 найбільших
 *   pkgaudit [-s STATUS] deps ПАКЕТ        прямі залежності пакета (D — Depends і
 *                                          Pre-Depends, R — Recommends)
 *   pkgaudit [-s STATUS] leaves            «листки»: пакети, від яких ніщо не залежить
 *   pkgaudit [-s STATUS] snapshot [ФАЙЛ]   зберегти список «назва версія»
 *   pkgaudit [-s STATUS] diff [ФАЙЛ]       що змінилося від знімка
 *
 * База — /var/lib/dpkg/status: абзаци «Поле: значення», розділені порожнім
 * рядком; рядок, що починається з пробілу, продовжує попереднє поле.
 * Налаштування — /etc/pkgaudit.conf (або файл зі змінної PKGAUDIT_CONF).
 */
#define _GNU_SOURCE
#include "debver.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#define DEFAULT_CONF     "/etc/pkgaudit.conf"
#define DEFAULT_STATUS   "/var/lib/dpkg/status"
#define DEFAULT_SNAPSHOT "/var/lib/pkgaudit/snapshot"

struct pkg {
    char *name, *version, *arch, *priority;
    char *depends, *predepends, *recommends, *provides;
    long size_kib;
    int essential;
    int used;                 /* для leaves: хтось від нього залежить */
};

static struct pkg *pkgs;
static size_t npkgs;

static char *keep[256];       /* з налаштувань: ніколи не вважати листками */
static size_t nkeep;
static char status_path[4096] = DEFAULT_STATUS;
static char snapshot_path[4096] = DEFAULT_SNAPSHOT;

static void *xmalloc(size_t n)
{
    void *p = malloc(n);
    if (!p) { perror("malloc"); exit(2); }
    return p;
}

static char *xstrdup(const char *s)
{
    char *p = strdup(s ? s : "");
    if (!p) { perror("strdup"); exit(2); }
    return p;
}

static char *trim(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = '\0';
    return s;
}

/* ---------- налаштування ---------- */

static void load_conf(void)
{
    const char *path = getenv("PKGAUDIT_CONF");
    if (!path)
        path = DEFAULT_CONF;
    FILE *f = fopen(path, "r");
    if (!f)
        return;                       /* немає файла — значення за замовчуванням */
    char line[4096];
    while (fgets(line, sizeof line, f)) {
        char *s = trim(line);
        if (*s == '#' || *s == '\0')
            continue;
        char *eq = strchr(s, '=');
        if (!eq)
            continue;
        *eq = '\0';
        char *key = trim(s), *val = trim(eq + 1);
        if (!strcmp(key, "status"))
            snprintf(status_path, sizeof status_path, "%s", val);
        else if (!strcmp(key, "snapshot"))
            snprintf(snapshot_path, sizeof snapshot_path, "%s", val);
        else if (!strcmp(key, "keep"))
            for (char *t = strtok(val, " \t,"); t && nkeep < 256; t = strtok(NULL, " \t,"))
                keep[nkeep++] = xstrdup(t);
    }
    fclose(f);
}

static int kept(const char *name)
{
    for (size_t i = 0; i < nkeep; i++)
        if (!strcmp(keep[i], name))
            return 1;
    return 0;
}

/* ---------- читання бази dpkg ---------- */

struct stanza {
    char *field[16];
    char *value[16];
    size_t n;
};

static const char *get(const struct stanza *st, const char *name)
{
    for (size_t i = 0; i < st->n; i++)
        if (!strcasecmp(st->field[i], name))
            return st->value[i];
    return NULL;
}

static const char *const wanted[] = {
    "Package", "Status", "Version", "Architecture", "Installed-Size", "Priority",
    "Essential", "Depends", "Pre-Depends", "Recommends", "Provides", NULL
};

static int is_wanted(const char *f)
{
    for (int i = 0; wanted[i]; i++)
        if (!strcasecmp(wanted[i], f))
            return 1;
    return 0;
}

static void stanza_free(struct stanza *st)
{
    for (size_t i = 0; i < st->n; i++) {
        free(st->field[i]);
        free(st->value[i]);
    }
    st->n = 0;
}

static void stanza_commit(struct stanza *st)
{
    const char *name = get(st, "Package"), *status = get(st, "Status");
    if (name && status && !strcmp(status, "install ok installed")) {
        static size_t cap;
        if (npkgs == cap) {
            cap = cap ? cap * 2 : 1024;
            struct pkg *t = realloc(pkgs, cap * sizeof *t);
            if (!t) { perror("realloc"); exit(2); }
            pkgs = t;
        }
        struct pkg *p = &pkgs[npkgs++];
        memset(p, 0, sizeof *p);
        p->name = xstrdup(name);
        p->version = xstrdup(get(st, "Version"));
        p->arch = xstrdup(get(st, "Architecture"));
        p->priority = xstrdup(get(st, "Priority"));
        p->depends = xstrdup(get(st, "Depends"));
        p->predepends = xstrdup(get(st, "Pre-Depends"));
        p->recommends = xstrdup(get(st, "Recommends"));
        p->provides = xstrdup(get(st, "Provides"));
        const char *sz = get(st, "Installed-Size");
        p->size_kib = sz ? atol(sz) : 0;
        const char *ess = get(st, "Essential");
        p->essential = ess && !strcasecmp(ess, "yes");
    }
    stanza_free(st);
}

static int pkg_by_name(const void *x, const void *y)
{
    const struct pkg *a = x, *b = y;
    int r = strcmp(a->name, b->name);
    return r ? r : strcmp(a->arch, b->arch);
}

static void load_status(void)
{
    FILE *f = fopen(status_path, "r");
    if (!f) {
        fprintf(stderr, "pkgaudit: %s: %s\n", status_path, strerror(errno));
        exit(2);
    }
    struct stanza st = { .n = 0 };
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    int last_kept = 0;                /* попереднє поле збережено */
    while ((len = getline(&line, &cap, f)) != -1) {
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0') {        /* кінець абзацу */
            stanza_commit(&st);
            last_kept = 0;
            continue;
        }
        if (line[0] == ' ' || line[0] == '\t') {   /* продовження поля */
            if (last_kept) {
                char **v = &st.value[st.n - 1];
                char *t = xmalloc(strlen(*v) + strlen(line) + 2);
                sprintf(t, "%s %s", *v, trim(line));
                free(*v);
                *v = t;
            }
            continue;
        }
        char *colon = strchr(line, ':');
        last_kept = 0;
        if (!colon)
            continue;
        *colon = '\0';
        if (!is_wanted(line) || st.n == 16)
            continue;
        st.field[st.n] = xstrdup(line);
        st.value[st.n] = xstrdup(trim(colon + 1));
        st.n++;
        last_kept = 1;
    }
    stanza_commit(&st);
    free(line);
    fclose(f);
    qsort(pkgs, npkgs, sizeof *pkgs, pkg_by_name);
}

static struct pkg *find_pkg(const char *name)
{
    for (size_t i = 0; i < npkgs; i++)         /* масив відсортовано за назвою */
        if (!strcmp(pkgs[i].name, name))
            return &pkgs[i];
    return NULL;
}

/* ---------- поля залежностей ---------- */

typedef void (*dep_cb)(const char *name, void *ctx);

/* Розбирає поле залежностей і викликає cb для КОЖНОЇ назви пакета.
 * Синтаксис:  pkg1 (>= 1.2), pkg2:any | pkg3 [amd64] <!nocheck>, pkg4
 *   кома           — розділяє обов'язкові залежності;
 *   вертикальна риска — розділяє альтернативи (потрібна будь-яка);
 *   (…) — обмеження версії, [...] — архітектури, <...> — профілі збірки,
 *   :any / :amd64 — кваліфікатор архітектури.
 * Назва — лише сама назва: без пробілів, дужок і кваліфікатора. */
static void parse_deps(const char *field, dep_cb cb, void *ctx)
{
    /* TODO 3. Розберіть поле й викличте cb(назва, ctx) для кожної назви —
     * і для обов'язкових залежностей, і для кожної з альтернатив.
     *   - поле може бути NULL або порожнім — тоді нічого не робити;
     *   - не псуйте field: розбирайте копію (xstrdup) і звільніть її;
     *   - strtok_r для ',' і всередині — ще один strtok_r для '|'
     *     (strtok без _r тут не годиться: два розбори вкладені);
     *   - з кожного шматка приберіть пробіли (trim) і відріжте все, що
     *     починається з пробілу, '(', '[', '<' або ':' (strcspn).
     *
     * Каркас не знаходить жодної залежності. */
    (void)field; (void)cb; (void)ctx;
}

/* ---------- stats ---------- */

static int by_size_desc(const void *x, const void *y)
{
    const struct pkg *a = *(struct pkg *const *)x, *b = *(struct pkg *const *)y;
    if (a->size_kib != b->size_kib)
        return a->size_kib < b->size_kib ? 1 : -1;
    return strcmp(a->name, b->name);
}

static int cmd_stats(void)
{
    long total = 0;
    struct pkg **v = xmalloc((npkgs + 1) * sizeof *v);
    for (size_t i = 0; i < npkgs; i++) {
        total += pkgs[i].size_kib;
        v[i] = &pkgs[i];
    }
    qsort(v, npkgs, sizeof *v, by_size_desc);
    printf("installed %zu\n", npkgs);
    printf("total_kib %ld\n", total);
    printf("total_mib %.1f\n", total / 1024.0);
    for (size_t i = 0; i < npkgs && i < 10; i++)
        printf("top %8ld %s %s\n", v[i]->size_kib, v[i]->name, v[i]->version);
    free(v);
    return 0;
}

/* ---------- deps ---------- */

static void print_d(const char *name, void *ctx) { (void)ctx; printf("D %s\n", name); }
static void print_r(const char *name, void *ctx) { (void)ctx; printf("R %s\n", name); }

static int cmd_deps(const char *name)
{
    struct pkg *p = find_pkg(name);
    if (!p) {
        fprintf(stderr, "pkgaudit: %s is not installed\n", name);
        return 1;
    }
    parse_deps(p->predepends, print_d, NULL);
    parse_deps(p->depends, print_d, NULL);
    parse_deps(p->recommends, print_r, NULL);
    return 0;
}

/* ---------- leaves ---------- */

/* Позначає як потрібні пакет з назвою name і всі встановлені пакети,
 * що надають (Provides) віртуальний пакет name. */
struct mark_ctx { const struct pkg *self; };

static void mark_used(const char *name, void *ctx)
{
    /* TODO 4. Пакет pkgs[i] потрібен (used = 1), якщо
     *   - його назва збігається з name, або
     *   - у його полі Provides є name (розберіть Provides тією самою
     *     parse_deps: синтаксис у Provides такий самий — «назва (= версія)»).
     * Залежність пакета від самого себе (m->self) не рахується.
     * Підказка: щоб дізнатися з колбека parse_deps, чи трапилася назва,
     * передайте через ctx невелику структуру або масив.
     *
     * Каркас нічого не позначає: усі пакети, крім обов'язкових, — «листки». */
    const struct mark_ctx *m = ctx;
    (void)name; (void)m;
}

static int cmd_leaves(void)
{
    for (size_t i = 0; i < npkgs; i++) {
        struct mark_ctx m = { &pkgs[i] };
        parse_deps(pkgs[i].predepends, mark_used, &m);
        parse_deps(pkgs[i].depends, mark_used, &m);
        parse_deps(pkgs[i].recommends, mark_used, &m);
    }
    struct pkg **v = xmalloc((npkgs + 1) * sizeof *v);
    size_t n = 0;
    long total = 0;
    for (size_t i = 0; i < npkgs; i++) {
        struct pkg *p = &pkgs[i];
        if (p->used || p->essential || kept(p->name) ||
            !strcmp(p->priority, "required") || !strcmp(p->priority, "important"))
            continue;
        v[n++] = p;
        total += p->size_kib;
    }
    qsort(v, n, sizeof *v, by_size_desc);
    for (size_t i = 0; i < n; i++)
        printf("leaf %8ld %s %s\n", v[i]->size_kib, v[i]->name, v[i]->version);
    printf("leaves %zu\n", n);
    printf("leaves_kib %ld\n", total);
    free(v);
    return 0;
}

/* ---------- snapshot і diff ---------- */

static int cmd_snapshot(const char *path)
{
    if (!strcmp(path, DEFAULT_SNAPSHOT) || !strcmp(path, snapshot_path)) {
        char dir[4096];
        snprintf(dir, sizeof dir, "%s", path);
        char *slash = strrchr(dir, '/');
        if (slash && slash != dir) {
            *slash = '\0';
            if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
                fprintf(stderr, "pkgaudit: mkdir %s: %s\n", dir, strerror(errno));
                return 2;
            }
        }
    }
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "pkgaudit: %s: %s\n", path, strerror(errno));
        return 2;
    }
    for (size_t i = 0; i < npkgs; i++)
        fprintf(f, "%s %s\n", pkgs[i].name, pkgs[i].version);
    if (fclose(f) != 0) {
        fprintf(stderr, "pkgaudit: %s: %s\n", path, strerror(errno));
        return 2;
    }
    printf("snapshot %zu %s\n", npkgs, path);
    return 0;
}

struct old { char name[256]; char version[2 * DEBVER_MAX]; };

static int old_cmp(const void *x, const void *y)
{
    return strcmp(((const struct old *)x)->name, ((const struct old *)y)->name);
}

static int vcmp_str(const char *a, const char *b)
{
    struct debver va, vb;
    if (debver_parse(a, &va, NULL) || debver_parse(b, &vb, NULL))
        return strcmp(a, b);          /* некоректна версія — хоч якийсь порядок */
    return debver_cmp(&va, &vb);
}

static int cmd_diff(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "pkgaudit: %s: %s\n", path, strerror(errno));
        return 2;
    }
    size_t n = 0, cap = 1024;
    struct old *o = xmalloc(cap * sizeof *o);
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        if (n == cap) {
            cap *= 2;
            struct old *t = realloc(o, cap * sizeof *t);
            if (!t) { perror("realloc"); exit(2); }
            o = t;
        }
        if (sscanf(line, "%255s %511s", o[n].name, o[n].version) == 2)
            n++;
    }
    fclose(f);
    qsort(o, n, sizeof *o, old_cmp);

    /* Злиття двох відсортованих списків. */
    size_t i = 0, j = 0;
    int added = 0, removed = 0, up = 0, down = 0;
    while (i < n || j < npkgs) {
        int c = i == n ? 1 : j == npkgs ? -1 : strcmp(o[i].name, pkgs[j].name);
        if (c < 0) {
            printf("- %s %s\n", o[i].name, o[i].version);
            removed++, i++;
        } else if (c > 0) {
            printf("+ %s %s\n", pkgs[j].name, pkgs[j].version);
            added++, j++;
        } else {
            int r = vcmp_str(o[i].version, pkgs[j].version);
            if (r < 0)
                printf("^ %s %s -> %s\n", o[i].name, o[i].version, pkgs[j].version), up++;
            else if (r > 0)
                printf("v %s %s -> %s\n", o[i].name, o[i].version, pkgs[j].version), down++;
            i++, j++;
        }
    }
    printf("summary added %d removed %d upgraded %d downgraded %d\n",
           added, removed, up, down);
    free(o);
    return 0;
}

/* ---------- main ---------- */

static void usage(void)
{
    fprintf(stderr,
        "usage: pkgaudit [-s STATUS] stats | deps PKG | leaves | "
        "snapshot [FILE] | diff [FILE]\n");
    exit(2);
}

int main(int argc, char **argv)
{
    load_conf();
    int k = 1;
    if (k + 1 < argc && !strcmp(argv[k], "-s")) {
        snprintf(status_path, sizeof status_path, "%s", argv[k + 1]);
        k += 2;
    }
    if (k >= argc)
        usage();
    const char *cmd = argv[k];
    const char *arg = k + 1 < argc ? argv[k + 1] : NULL;
    load_status();
    if (!strcmp(cmd, "stats"))
        return cmd_stats();
    if (!strcmp(cmd, "deps") && arg)
        return cmd_deps(arg);
    if (!strcmp(cmd, "leaves"))
        return cmd_leaves();
    if (!strcmp(cmd, "snapshot"))
        return cmd_snapshot(arg ? arg : snapshot_path);
    if (!strcmp(cmd, "diff"))
        return cmd_diff(arg ? arg : snapshot_path);
    usage();
    return 2;
}
