/* debver.c — розбір і порівняння версій Debian. ЛР 9, «Системне програмне забезпечення».
 * TODO 1 — debver_parse, TODO 2 — debver_part_cmp. */
#include "debver.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int fail(const char **err, const char *why)
{
    if (err)
        *err = why;
    return -1;
}

static int allowed(const char *s, const char *extra)
{
    for (; *s; s++)
        if (!isalnum((unsigned char)*s) && !strchr(extra, *s))
            return 0;
    return 1;
}

int debver_parse(const char *s, struct debver *v, const char **err)
{
    /* TODO 1. Розберіть рядок s за форматом [epoch:]upstream[-revision]
     * (коментар у debver.h; повна таблиця правил — у розділі 4.1 ЛР).
     *   1. Порожній рядок і рядок із пробільними символами — помилка.
     *   2. Епоха — усе до ПЕРШОЇ двокрапки: непорожня, лише цифри,
     *      не більше INT_MAX. Немає двокрапки — епоха 0.
     *   3. Ревізія — усе після ОСТАННЬОГО дефіса в залишку: непорожня,
     *      лише літери, цифри та . + ~. Немає дефіса — ревізія "".
     *   4. Upstream — те, що між ними: непорожній, починається з цифри,
     *      лише літери, цифри та . + ~ - :.
     *   5. Задовгі частини (>= DEBVER_MAX) — помилка, а не переповнення.
     * Для помилки — return fail(err, "коротка причина"); допоміжна allowed()
     * перевіряє, що всі символи рядка — літери, цифри або з набору extra.
     *
     * Каркас кладе весь рядок в upstream: епох і ревізій він не бачить. */
    memset(v, 0, sizeof *v);
    if (s == NULL || *s == '\0')
        return fail(err, "empty version");
    snprintf(v->upstream, sizeof v->upstream, "%s", s);
    (void)allowed;
    return 0;
}

/* Вага символу в нецифровій частині: ~ менша за все, навіть за кінець
 * рядка; кінець рядка — 0; літери — за кодом; решта — після літер. */
static int order(int c)
{
    if (c == '~')
        return -1;
    if (c == '\0' || isdigit(c))
        return 0;
    if (isalpha(c))
        return c;
    return c + 256;
}

int debver_part_cmp(const char *a, const char *b)
{
    /* TODO 2. Порівняйте a і b так, як це робить dpkg.
     * Обидва рядки ділимо на чергування «нецифровий шматок, цифровий шматок»
     * і порівнюємо шматки попарно зліва направо, доки рядки не скінчаться:
     *   1. Нецифрові шматки — посимвольно, але не за ASCII, а за вагою
     *      order(): '~' менша за все, навіть за кінець рядка; кінець — 0;
     *      літери — за кодом; інші символи (. + -) — після всіх літер.
     *      Короткий шматок доповнюється «кінцем рядка» (вага 0).
     *   2. Цифрові шматки — як числа: провідні нулі не важать, порожній
     *      шматок — це 0. Не перетворюйте на int: "20260101123456789" не
     *      влізе в жоден тип. Довше число (після нулів) — більше; однакової
     *      довжини — вирішує перша відмінна цифра.
     * Перша відмінність і визначає знак результату.
     *
     * Каркас порівнює байти, як strcmp: для нього 1.10 < 1.9 і 1.0~rc1 > 1.0. */
    (void)order;
    return strcmp(a, b);
}

int debver_cmp(const struct debver *a, const struct debver *b)
{
    if (a->epoch != b->epoch)
        return a->epoch < b->epoch ? -1 : 1;
    int r = debver_part_cmp(a->upstream, b->upstream);
    if (r)
        return r;
    return debver_part_cmp(a->revision, b->revision);
}

void debver_format(const struct debver *v, char *buf, unsigned long size)
{
    if (v->epoch)
        snprintf(buf, size, "%u:%s%s%s", v->epoch, v->upstream,
                 v->revision[0] ? "-" : "", v->revision);
    else
        snprintf(buf, size, "%s%s%s", v->upstream,
                 v->revision[0] ? "-" : "", v->revision);
}
