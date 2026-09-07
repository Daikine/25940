#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>

extern char **environ;

/* Вывод справки по использованию */
void usage(const char *progname) {
    fprintf(stderr, "Использование: %s [опции]\n", progname);
    fprintf(stderr, "Опции:\n");
    fprintf(stderr, "  -i           Реальные и эффективные UID/GID\n");
    fprintf(stderr, "  -s           Стать лидером группы процессов\n");
    fprintf(stderr, "  -p           PID, PPID, PGID\n");
    fprintf(stderr, "  -u           Текущий ulimit (размер файла)\n");
    fprintf(stderr, "  -U <value>   Установить ulimit\n");
    fprintf(stderr, "  -c           Размер core-файла\n");
    fprintf(stderr, "  -C <size>    Установить размер core-файла\n");
    fprintf(stderr, "  -d           Текущая рабочая директория\n");
    fprintf(stderr, "  -v           Переменные окружения\n");
    fprintf(stderr, "  -V name=val  Установить переменную окружения\n");
}

int main(int argc, char *argv[]) {
    int opt;
    int i, j;
    struct rlimit rl;

    /* --------------------------------------------------------
     * Обработка опций СПРАВА НАЛЕВО:
     * getopt обрабатывает аргументы слева направо, поэтому
     * мы переворачиваем массив argv[1..argc-1], чтобы
     * правые аргументы оказались первыми при обработке.
     * argv[0] (имя программы) оставляем на месте.
     * -------------------------------------------------------- */
    for (i = 1, j = argc - 1; i < j; i++, j--) {
        char *tmp = argv[i];
        argv[i] = argv[j];
        argv[j] = tmp;
    }

    /* Строка опций: после буквы ':' getopt ожидает аргумент */
    while ((opt = getopt(argc, argv, "ispuU:cC:dV:v")) != -1) {
        switch (opt) {

        /* ---- -i : реальные и эффективные UID и GID ---- */
        case 'i':
            printf("Real UID:      %d\n", getuid());
            printf("Effective UID: %d\n", geteuid());
            printf("Real GID:      %d\n", getgid());
            printf("Effective GID: %d\n", getegid());
            break;

        /* ---- -s : процесс становится лидером группы ---- */
        case 's':
            /* setpgid(0, 0) делает процесс лидером новой группы */
            if (setpgid(0, 0) == -1) {
                perror("setpgid");
            } else {
                printf("Процесс %d стал лидером группы процессов %d\n",
                       getpid(), getpgrp());
            }
            break;

        /* ---- -p : PID, PPID, PGID ---- */
        case 'p':
            printf("PID:  %d\n", getpid());
            printf("PPID: %d\n", getppid());
            printf("PGID: %d\n", getpgrp());
            break;

        /* ---- -u : печать текущего ulimit (RLIMIT_FSIZE) ---- */
        case 'u':
            if (getrlimit(RLIMIT_FSIZE, &rl) == -1) {
                perror("getrlimit(RLIMIT_FSIZE)");
            } else {
                if (rl.rlim_cur == RLIM_INFINITY)
                    printf("ulimit (soft): unlimited\n");
                else
                    printf("ulimit (soft): %ld байт\n", (long)rl.rlim_cur);

                if (rl.rlim_max == RLIM_INFINITY)
                    printf("ulimit (hard): unlimited\n");
                else
                    printf("ulimit (hard): %ld байт\n", (long)rl.rlim_max);
            }
            break;

        /* ---- -U <value> : изменить ulimit ---- */
        case 'U': {
            char *endptr;
            long val = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || val < 0) {
                fprintf(stderr, "Ошибка: недопустимое значение для -U: '%s'\n",
                        optarg);
                exit(EXIT_FAILURE);
            }
            rl.rlim_cur = (rlim_t)val;
            rl.rlim_max = (rlim_t)val;
            if (setrlimit(RLIMIT_FSIZE, &rl) == -1) {
                perror("setrlimit(RLIMIT_FSIZE)");
                exit(EXIT_FAILURE);
            }
            printf("ulimit установлен в %ld байт\n", val);
            break;
        }

        /* ---- -c : размер core-файла ---- */
        case 'c':
            if (getrlimit(RLIMIT_CORE, &rl) == -1) {
                perror("getrlimit(RLIMIT_CORE)");
            } else {
                if (rl.rlim_cur == RLIM_INFINITY)
                    printf("Core file size (soft): unlimited\n");
                else
                    printf("Core file size (soft): %ld байт\n",
                           (long)rl.rlim_cur);

                if (rl.rlim_max == RLIM_INFINITY)
                    printf("Core file size (hard): unlimited\n");
                else
                    printf("Core file size (hard): %ld байт\n",
                           (long)rl.rlim_max);
            }
            break;

        /* ---- -C <size> : изменить размер core-файла ---- */
        case 'C': {
            char *endptr;
            long val = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || val < 0) {
                fprintf(stderr,
                        "Ошибка: недопустимое значение для -C: '%s'\n",
                        optarg);
                exit(EXIT_FAILURE);
            }
            rl.rlim_cur = (rlim_t)val;
            rl.rlim_max = (rlim_t)val;
            if (setrlimit(RLIMIT_CORE, &rl) == -1) {
                perror("setrlimit(RLIMIT_CORE)");
                exit(EXIT_FAILURE);
            }
            printf("Core file size установлен в %ld байт\n", val);
            break;
        }

        /* ---- -d : текущая рабочая директория ---- */
        case 'd': {
            char cwd[4096];
            if (getcwd(cwd, sizeof(cwd)) == NULL) {
                perror("getcwd");
            } else {
                printf("Текущая директория: %s\n", cwd);
            }
            break;
        }

        /* ---- -v : печать переменных окружения ---- */
        case 'v': {
            char **env;
            printf("--- Переменные окружения ---\n");
            for (env = environ; *env != NULL; env++) {
                printf("%s\n", *env);
            }
            printf("--- Конец списка ---\n");
            break;
        }

        /* ---- -V name=value : установить переменную окружения ---- */
        case 'V': {
            char *eq = strchr(optarg, '=');
            if (eq == NULL) {
                fprintf(stderr,
                        "Ошибка: формат -V должен быть name=value, "
                        "получено: '%s'\n", optarg);
                exit(EXIT_FAILURE);
            }
            *eq = '\0';
            const char *name = optarg;
            const char *value = eq + 1;
            if (setenv(name, value, 1) == -1) {
                perror("setenv");
                exit(EXIT_FAILURE);
            }
            printf("Переменная окружения '%s'='%s' установлена\n",
                   name, value);
            break;
        }

        /* ---- неизвестная опция ---- */
        case '?':
        default:
            fprintf(stderr, "Недопустимая опция или отсутствует аргумент\n");
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
