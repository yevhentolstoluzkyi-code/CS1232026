/* launch — запуск програми в контрольованому оточенні (ЛР 5, частина 2–3).
 *
 *   launch [-e ІМ'Я=ЗНАЧЕННЯ]... [-u ІМ'Я]... [-C КАТАЛОГ] [-o ФАЙЛ]
 *          [-l ЖУРНАЛ] [-d [-p PIDФАЙЛ]] [--] ПРОГРАМА [АРГ...]
 *
 * Специфікація — у lab05/README.md. Завдання позначено TODO 2 і TODO 3.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXENV 64

/* Що дитина повідомляє батькові через канал звіту */
enum stage { ST_PID = 1, ST_CHDIR, ST_OPEN, ST_EXEC };
struct report { int stage; int value; };   /* value: PID або errno */

struct opts {
    char *set[MAXENV]; int nset;
    char *unset[MAXENV]; int nunset;
    const char *dir, *out, *log, *pidfile;
    int daemon;
    char **argv;
};

static int logfd = -1;

static void logmsg(const char *fmt, ...)
{
    if (logfd < 0) return;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n >= sizeof buf) n = sizeof buf - 1;
    if (write(logfd, buf, (size_t)n) < 0) { /* журнал — не критично */ }
}

static void usage(void)
{
    fprintf(stderr, "usage: launch [-e NAME=VALUE]... [-u NAME]... [-C DIR] [-o FILE]\n"
                    "              [-l LOG] [-d [-p PIDFILE]] [--] PROGRAM [ARG...]\n");
    exit(2);
}

/* Записати повідомлення в канал і завершити дитину. Лише async-signal-safe
 * виклики: після fork у дитині не можна покладатися на stdio чи malloc. */
static void child_fail(int wfd, int stage, int err)
{
    struct report r = { stage, err };
    ssize_t n = write(wfd, &r, sizeof r);
    (void)n;
    _exit(stage == ST_EXEC ? (err == ENOENT ? 127 : 126) : 1);
}

/* Усе, що дитина робить між fork і exec. Не повертається.
 * wfd — кінець каналу звіту для запису (з O_CLOEXEC). */
static void child_setup_and_exec(const struct opts *o, int wfd)
{
    /* TODO 2a, у такому порядку:
     *   1) -C: chdir; помилка → child_fail(wfd, ST_CHDIR, errno);
     *   2) -u: unsetenv для кожного імені, потім -e: putenv для кожної пари;
     *   3) -o: open(ФАЙЛ, O_WRONLY | O_CREAT | O_TRUNC | ..., 0644) і dup2 на 1;
     *      у режимі -d ще й stdin ← /dev/null, stdout і stderr ← ФАЙЛ або
     *      /dev/null; будь-яка помилка → child_fail(wfd, ST_OPEN, errno);
     *   4) execvp; якщо повернувся → child_fail(wfd, ST_EXEC, errno).
     * Після fork у дитині — лише async-signal-safe виклики (man 7 signal-safety):
     * жодного printf, жодного exit. Чому? */
    if (o->dir)  
    {
       if (chdir(o->dir) == -1)
           child_fail(wfd, ST_CHDIR, errno);
    }

    for (int i = 0; i < o->nunset; i++)
        unsetenv(o->unset[i]);

    for (int i = 0; i < o->nset; i++)
        putenv(o->set[i]);

    if (o->out && !o->daemon) 
    {
       int fd = open(o->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
       if (fd == -1)
           child_fail(wfd, ST_OPEN, errno);

       if (dup2(fd, STDOUT_FILENO) == -1)
           child_fail(wfd, ST_OPEN, errno);

     close(fd);
     }

     if (o->daemon && o->dir)
     {
         int nullfd = open("/dev/null", O_RDWR);
         if (nullfd == -1)
             child_fail(wfd, ST_OPEN, errno);

         int outfd = nullfd;

         if (o->out) 
         {
             outfd = open(o->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
             if (outfd == -1)
                 child_fail(wfd, ST_OPEN, errno);
         }

         if (dup2(nullfd, STDIN_FILENO) == -1 ||
             dup2(outfd, STDOUT_FILENO) == -1 ||
             dup2(outfd, STDERR_FILENO) == -1)
             child_fail(wfd, ST_OPEN, errno);

         if (outfd != nullfd)
             close(outfd);
         close(nullfd);
     }

    execvp(o->argv[0], o->argv);
    child_fail(wfd, ST_EXEC, errno);
}

static const char *stage_name(const struct opts *o, int stage)
{
    switch (stage) {
    case ST_CHDIR: return o->dir;
    case ST_OPEN:  return o->out ? o->out : "/dev/null";
    default:       return o->argv[0];
    }
}

/* Прочитати звіт дитини. 0 — канал закрився без помилки (exec вдався);
 * 1 — у *r помилка; -1 — збій самого read. */
static int read_report(int rfd, struct report *r)
{
    for (;;) {
        ssize_t n = read(rfd, r, sizeof *r);
        if (n == (ssize_t)sizeof *r) return 1;
        if (n == 0) return 0;
        if (n == -1 && errno == EINTR) continue;
        return -1;
    }
}

static pid_t wait_child(pid_t pid, int *status)
{
    pid_t w;
    while ((w = waitpid(pid, status, 0)) == -1 && errno == EINTR)
        ;
    return w;
}

static int run_foreground(const struct opts *o)
{
    /* TODO 2b:
     *   1) канал звіту: pipe2(..., O_CLOEXEC) — чому саме з цим прапорцем?
     *   2) fflush(NULL), fork; у дитині закрити кінець для читання і викликати
     *      child_setup_and_exec;
     *   3) у батька закрити кінець для запису, logmsg("start pid=%d cmd=%s\n", ...),
     *      read_report: 1 → дитина повідомила про помилку, 0 → exec вдався;
     *   4) wait_child; розібрати статус макросами WIFEXITED / WEXITSTATUS /
     *      WIFSIGNALED / WTERMSIG і повернути код за специфікацією;
     *      у журнал — "exit pid=%d code=%d\n" або "signal pid=%d sig=%d\n". */
    
    int pfd[2];

    if (pipe2(pfd, O_CLOEXEC) == -1) 
    {
        fprintf(stderr, "launch: pipe: %s\n", strerror(errno));
        return 1;
    }

    fflush(NULL);

    pid_t pid = fork();

    if (pid == -1) 
    {
        fprintf(stderr, "launch: fork: %s\n", strerror(errno));
        close(pfd[0]);
        close(pfd[1]);
        return 1;
    }

    if (pid == 0) 
    {
        close(pfd[0]);
        child_setup_and_exec(o, pfd[1]);
    }

    close(pfd[1]);

    logmsg("start pid=%d cmd=%s\n", (int)pid, o->argv[0]);

    struct report r;
    int rr = read_report(pfd[0], &r);
    close(pfd[0]);

    if (rr == 1) 
    {
        fprintf(stderr, "launch: %s: %s\n",
            stage_name(o, r.stage), strerror(r.value));
    }
    else if (rr == -1) 
    {
    fprintf(stderr, "launch: pipe: %s\n", strerror(errno));
    }

    int status;

    if (wait_child(pid, &status) == -1) 
    {
        fprintf(stderr, "launch: waitpid: %s\n", strerror(errno));
        return 1;
    }

    if (WIFEXITED(status)) 
    {
        int code = WEXITSTATUS(status);
        logmsg("exit pid=%d code=%d\n", (int)pid, code);
        return code;
    }

    if (WIFSIGNALED(status)) 
    {
        int sig = WTERMSIG(status);
        fprintf(stderr, "launch: %s: killed by signal %d\n",
                         o->argv[0], sig);
        logmsg("signal pid=%d sig=%d\n", (int)pid, sig);
        return 128 + sig; 
    }

    return 1;
}

static int run_daemon(const struct opts *o)
{
    /* TODO 3: подвійний fork, setsid, umask(022), chdir("/") (якщо немає -C),
     * PID онука — першим повідомленням ST_PID у канал звіту, потім
     * child_setup_and_exec. Батько: дочекатися проміжного процесу, прочитати
     * PID демона, потім дочекатися EOF або помилки; у разі успіху — PID-файл
     * і "daemon pid=%d cmd=%s\n" у журнал. Покроково — у частині 3 роботи. */
      
     int pfd[2];

     if (pipe2(pfd, O_CLOEXEC) == -1) 
     {
         fprintf(stderr, "launch: pipe: %s\n", strerror(errno));
         return 1;
     }

     fflush(NULL);

     pid_t pid = fork();

     if (pid == -1) 
     {
         fprintf(stderr, "launch: fork: %s\n", strerror(errno));
         close(pfd[0]);
         close(pfd[1]);
         return 1;
     }

     if (pid == 0) 
     {
         close(pfd[0]);

         if (setsid() == -1)
             child_fail(pfd[1], ST_EXEC, errno);

         pid_t pid2 = fork();

         if (pid2 == -1)
             child_fail(pfd[1], ST_EXEC, errno);

         if (pid2 > 0)
             _exit(0);

         struct report pr = { ST_PID, (int)getpid() };

         if (write(pfd[1], &pr, sizeof pr) != (ssize_t)sizeof pr)
             _exit(1);

         umask(022);

         if (!o->dir)
         {
             int nullfd = open("/dev/null", O_RDWR);
             if (nullfd == -1)
                 child_fail(pfd[1], ST_OPEN, errno);

             int outfd = nullfd;

             if (o->out)
             {
                 outfd = open(o->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                 if (outfd == -1)
                     child_fail(pfd[1], ST_OPEN, errno);
             }

             if (dup2(nullfd, STDIN_FILENO) == -1 || dup2(outfd, STDOUT_FILENO) == -1 ||
                 dup2(outfd, STDERR_FILENO) == -1)
                 child_fail(pfd[1], ST_OPEN, errno);

             if (outfd != nullfd)
                 close(outfd);
             close(nullfd);

             if (chdir("/") == -1)
                 child_fail(pfd[1], ST_CHDIR, errno);
         }

         child_setup_and_exec(o, pfd[1]);
     }

     close(pfd[1]);

     int status;
     if (wait_child(pid, &status) == -1) 
     {
         fprintf(stderr, "launch: waitpid: %s\n", strerror(errno));
         close(pfd[0]);
         return 1;
     }

     struct report r;
     int rr = read_report(pfd[0], &r);

     if (rr != 1 || r.stage != ST_PID) 
     {
         close(pfd[0]);
         return 1;
     }

     pid_t daemon_pid = (pid_t)r.value;

     rr = read_report(pfd[0], &r);
     close(pfd[0]);

     if (rr == 1) 
     {
         fprintf(stderr, "launch: %s: %s\n",
                 stage_name(o, r.stage), strerror(r.value));
         return r.stage == ST_EXEC
                ? (r.value == ENOENT ? 127 : 126)
                : 1;
     }

     if (rr == -1) 
     {
         fprintf(stderr, "launch: pipe: %s\n", strerror(errno));
         return 1;
     }

     if (o->pidfile) 
     {
         int fd = open(o->pidfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

         if (fd == -1) 
         {
             fprintf(stderr, "launch: %s: %s\n",
                     o->pidfile, strerror(errno));
             return 1;
         }

         char buf[64];
         int n = snprintf(buf, sizeof buf, "%d\n", (int)daemon_pid);

         if (write(fd, buf, (size_t)n) != n) 
         {
             fprintf(stderr, "launch: %s: %s\n",
                     o->pidfile, strerror(errno));
             close(fd);
             return 1;
         }
 
         close(fd);
     }

     logmsg("daemon pid=%d cmd=%s\n",
            (int)daemon_pid, o->argv[0]);

     return 0;
}

int main(int argc, char *argv[])
{
    struct opts o = {0};
    int opt;
    while ((opt = getopt(argc, argv, "+e:u:C:o:l:dp:")) != -1) {
        switch (opt) {
        case 'e':
            if (!strchr(optarg, '=') || optarg[0] == '=' || o.nset == MAXENV) usage();
            o.set[o.nset++] = optarg;
            break;
        case 'u':
            if (!*optarg || strchr(optarg, '=') || o.nunset == MAXENV) usage();
            o.unset[o.nunset++] = optarg;
            break;
        case 'C': o.dir = optarg; break;
        case 'o': o.out = optarg; break;
        case 'l': o.log = optarg; break;
        case 'd': o.daemon = 1; break;
        case 'p': o.pidfile = optarg; break;
        default: usage();
        }
    }
    if (optind >= argc || (o.pidfile && !o.daemon)) usage();
    o.argv = argv + optind;

    if (o.log) {
        logfd = open(o.log, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
        if (logfd == -1) {
            fprintf(stderr, "launch: %s: %s\n", o.log, strerror(errno));
            return 1;
        }
    }
    int rc = o.daemon ? run_daemon(&o) : run_foreground(&o);
    if (logfd != -1) close(logfd);
    return rc;
}
