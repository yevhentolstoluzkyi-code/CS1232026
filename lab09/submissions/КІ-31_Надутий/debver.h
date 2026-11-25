/* debver.h — версії пакетів Debian: розбір і порівняння.
 * ЛР 9, «Системне програмне забезпечення».
 *
 * Формат (Debian Policy, розділ 5.6.12):  [epoch:]upstream_version[-debian_revision]
 *   epoch     — невід'ємне ціле; немає двокрапки — епоха 0;
 *   upstream  — починається з цифри; літери, цифри, . + ~ (а також - і :,
 *               якщо є ревізія чи епоха відповідно);
 *   revision  — після ОСТАННЬОГО дефіса; літери, цифри, . + ~; немає — "".
 */
#ifndef DEBVER_H
#define DEBVER_H

#define DEBVER_MAX 256

struct debver {
    unsigned epoch;
    char upstream[DEBVER_MAX];
    char revision[DEBVER_MAX];
};

/* Розбирає рядок s у *v. Повертає 0, якщо версія коректна, інакше -1
 * і, якщо err != NULL, записує в *err коротку причину англійською. */
int debver_parse(const char *s, struct debver *v, const char **err);

/* Порівнює дві частини версії (upstream або revision) за алгоритмом dpkg.
 * Повертає від'ємне, 0 або додатне число, як strcmp. */
int debver_part_cmp(const char *a, const char *b);

/* Порівнює дві розібрані версії: епоха, потім upstream, потім revision. */
int debver_cmp(const struct debver *a, const struct debver *b);

/* Друкує версію в канонічному вигляді (епоха 0 не друкується). */
void debver_format(const struct debver *v, char *buf, unsigned long size);

#endif
