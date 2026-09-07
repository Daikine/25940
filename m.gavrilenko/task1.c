#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

extern char **environ;

void handle_sigint(int sig) {
    (void)sig;
    fprintf(stderr, "\nПрограмма прервана пользователем\n");
    exit(EXIT_FAILURE);
}

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

long parse_long(const char *str, const char *optname) {
    char *endptr;
    long val;
    
    if (str == NULL || *str == '\0') {
        fprintf(stderr, "Ошибка: пустое значение для опции -%s\n", optname);
        exit(EXIT_FAILURE);
    }
    
    errno = 0;
    val = strtol(str, &endptr, 10);
    
    if (*endptr != '\0') {
        fprintf(stderr, "Ошибка: '%s' не является корректным числом для -%s\n", 
                str, optname);
        exit(EXIT_FAILURE);
    }
    
    if (errno == ERANGE) {
        fprintf(stderr, "Ошибка: значение '%s' вне допустимого диапазона для -%s\n", 
                str, optname);
        exit(EXIT_FAILURE);
    }
    
    if (val < 0) {
        fprintf(stderr, "Ошибка: значение не может быть отрицательным для -%s\n", 
                optname);
        exit(EXIT_FAILURE);
    }
    
    return val;
}

int main(int argc, char *argv[]) {
    int opt;
    int i, j;
    struct rlimit rl;
    
    signal(SIGINT, handle_sigint);

    if (argc == 1) {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }

    for (i = 1, j = argc - 1; i < j; i++, j--) {
        char *tmp = argv[i];
        argv[i] = argv[j];
        argv[j] = tmp;
    }

    while ((opt = getopt(argc, argv, ":ispuU:cC:dV:v")) != -1) {
        switch (opt) {

        case 'i':
            printf("Real UID:      %d\n", getuid());
            printf("Effective UID: %d\n", geteuid());
            printf("Real GID:      %d\n", getgid());
            printf("Effective GID: %d\n", getegid());
            break;

        case 's':
            if (setpgid(0, 0) == -1) {
                perror("setpgid");
            } else {
                printf("Процесс %d стал лидером группы процессов %d\n",
                       getpid(), getpgrp());
            }
            break;

        case 'p':
            printf("PID:  %d\n", getpid());
            printf("PPID: %d\n", getppid());
            printf("PGID: %d\n", getpgrp());
            break;

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

        case 'U': {
            long val = parse_long(optarg, "U");
            rl.rlim_cur = (rlim_t)val;
            rl.rlim_max = (rlim_t)val;
            if (setrlimit(RLIMIT_FSIZE, &rl) == -1) {
                perror("setrlimit(RLIMIT_FSIZE)");
                exit(EXIT_FAILURE);
            }
            printf("ulimit установлен в %ld байт\n", val);
            break;
        }

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

        case 'C': {
            long val = parse_long(optarg, "C");
            rl.rlim_cur = (rlim_t)val;
            rl.rlim_max = (rlim_t)val;
            if (setrlimit(RLIMIT_CORE, &rl) == -1) {
                perror("setrlimit(RLIMIT_CORE)");
                exit(EXIT_FAILURE);
            }
            printf("Core file size установлен в %ld байт\n", val);
            break;
        }

        case 'd': {
            char cwd[4096];
            if (getcwd(cwd, sizeof(cwd)) == NULL) {
                perror("getcwd");
            } else {
                printf("Текущая директория: %s\n", cwd);
            }
            break;
        }

        case 'v': {
            char **env;
            printf("--- Переменные окружения ---\n");
            for (env = environ; *env != NULL; env++) {
                printf("%s\n", *env);
            }
            printf("--- Конец списка ---\n");
            break;
        }

        case 'V': {
            char *eq = strchr(optarg, '=');
            if (eq == NULL) {
                fprintf(stderr,
                        "Ошибка: формат -V должен быть name=value, "
                        "получено: '%s'\n", optarg);
                exit(EXIT_FAILURE);
            }
            if (eq == optarg) {
                fprintf(stderr, "Ошибка: имя переменной не может быть пустым\n");
                exit(EXIT_FAILURE);
            }
            if (*(eq + 1) == '\0') {
                fprintf(stderr, "Ошибка: значение переменной не может быть пустым\n");
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

        case '?':
            fprintf(stderr, "Недопустимая опция: -%c\n", optopt);
            usage(argv[0]);
            exit(EXIT_FAILURE);
        
        case ':':
            fprintf(stderr, "Ошибка: после -%c требуется аргумент\n", optopt);
            usage(argv[0]);
            exit(EXIT_FAILURE);

        default:
            fprintf(stderr, "Неизвестная ошибка при обработке опций\n");
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}
