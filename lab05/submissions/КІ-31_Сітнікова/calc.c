/* calc — калькулятор цілих виразів з відновленням після помилок через
 * setjmp/longjmp (ЛР 5, частина 5).
 *
 * Читає вирази з stdin по одному на рядок і друкує результат або
 * «error: опис». Помилка в рядку не зупиняє роботу: калькулятор
 * переходить до наступного рядка. Наприкінці — «lines=N errors=M»;
 * код завершення 0, якщо помилок не було, інакше 1.
 *
 *   вираз   := доданок (('+' | '-') доданок)*
 *   доданок := множник (('*' | '/') множник)*
 *   множник := ЧИСЛО | '-' множник | '(' вираз ')'
 *
 * У цьому файлі дві навмисні вади (шукайте їх у частині 5 роботи) і TODO 5.
 */
#define _POSIX_C_SOURCE 200809L   /* strdup */
#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_DEPTH 200

static jmp_buf on_error;            /* куди повертатися після помилки   */
static const char *err_msg;         /* що сталося                       */
static const char *p;               /* поточна позиція в розбираному рядку */

/* Не повертається: стрибає в main, через усі кадри парсера одразу. */
static void fail(const char *msg)
{
    err_msg = msg;
    longjmp(on_error, 1);
}

static void skip_spaces(void)
{
    while (*p == ' ' || *p == '\t') p++;
}

static long long expr(int depth);

static long long factor(int depth)
{
    if (depth > MAX_DEPTH) fail("too deep");
    skip_spaces();
    if (*p == '(') {
        p++;
        long long v = expr(depth + 1);
        skip_spaces();
        if (*p != ')') fail("expected )");
        p++;
        return v;
    }
    if (*p == '-') {
        p++;
        long long v = factor(depth + 1);
        /* TODO 5: -v переповнюється для одного значення v. Якого? */
        /*return -v;*/
        long long result;
        if (__builtin_sub_overflow(0LL, v, &result))
           fail("overflow");
        return result;
    }
    if (!isdigit((unsigned char)*p)) fail("syntax error");
    long long v = 0;
    while (isdigit((unsigned char)*p)) {
        /* TODO 5: v * 10 + цифра може не вміститися в long long.
         * Перевірка — через __builtin_mul_overflow / __builtin_add_overflow,
         * повідомлення — fail("overflow"). */
        long long tmp;

        if (__builtin_mul_overflow(v, 10LL, &tmp))
            fail("overflow");

        if (__builtin_add_overflow(tmp, (long long)(*p - '0'), &v))
            fail("overflow");
        p++;
    }
    return v;
}

static long long term(int depth)
{
    long long v = factor(depth);
    for (;;) {
        skip_spaces();
        char op = *p;
        if (op != '*' && op != '/') return v;
        p++;
        long long r = factor(depth);
        if (op == '*') {
            /*v *= r;*/                 /* TODO 5: переповнення */
            long long result;
            if (__builtin_mul_overflow(v, r, &result))
                fail("overflow");
            v = result;
        } else {
            if (r == 0) fail("division by zero");

            if (v == LLONG_MIN && r == -1)
               fail("overflow");

            v /= r;                 /* TODO 5: є одна пара, для якої і це переповнення */
        }
    }
}

static long long expr(int depth)
{
    long long v = term(depth);
    for (;;) {
        skip_spaces();
        char op = *p;
        if (op != '+' && op != '-') return v;
        p++;
        long long r = term(depth);
        long long result;

        if (op == '+') 
        {
            if (__builtin_add_overflow(v, r, &result))
                fail("overflow");
        } 
        else 
        {
            if (__builtin_sub_overflow(v, r, &result))
                fail("overflow");
        }

        v = result;
        /*v = (op == '+') ? v + r : v - r;*/   /* TODO 5: переповнення */
    }
}

int main(void)
{
    char buf[1024];
    volatile int lines = 0, errors = 0;
    char *volatile text = NULL;

    if (setjmp(on_error) != 0) {
        /* Сюди потрапляємо з fail(): другий «вихід» із setjmp */
        errors++;
        printf("error: %s\n", err_msg);
        free((void *)text);
        text = NULL;
    }

    while (fgets(buf, sizeof buf, stdin)) {
        lines++;
        buf[strcspn(buf, "\n")] = '\0';
        text = strdup(buf);         /* робоча копія рядка для парсера */
        if (!text) {
            perror("calc");
            return 2;
        }
        p = (const char *)text;
        long long v = expr(0);
        skip_spaces();
        if (*p != '\0') fail("trailing garbage");
        printf("%lld\n", v);
        free((void *)text);
        text = NULL;
    }
    printf("lines=%d errors=%d\n", lines, errors);
    return errors ? 1 : 0;
}
