#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>

enum COMMAND {
    NONE = 0,
    RUN = 10,
    EXEC = 11,
};


struct config {
    enum COMMAND subcommand;
    char name[64];
    char command[256];
};

int validate_config(struct config cfg) {
    if (cfg.subcommand == NONE) {
        fprintf(stderr, "[ERR] Mssing subcommand (run|exec)\n");
        return 1;
    }

    if (strcmp(cfg.name, "") == 0) {
        strncpy(cfg.name, "bib", sizeof(cfg.name)-1);
    }

    if (strcmp(cfg.command, "") == 0) {
        fprintf(stderr, "[ERR] Mssing command (e.g. 'sleep 1000')\n");
        return 1;
    }
    return 0;
}

int run_container(struct config cfg) {
    pid_t pid;
    // ایجاد فضای نام جدید (PID + mount
    if (unshare(CLONE_NEWPID | CLONE_NEWNS) < 0) {
        perror("unshare");
        return 1;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // فرزند داخل فضای نام جدید
        printf("Child PID in new namespace: %d\n", getpid());
        execl("/bin/sh", "sh", "-c", cfg.command, NULL);
        perror("execl"); // اگر execl شکست خورد
        exit(1);
    } else {
        // والد
        waitpid(pid, NULL, 0); // منتظر پایان فرزند می‌ماند
        printf("[Parent] Child finished, PID in parent namespace: %d\n", pid);
    }

    return 0;
}

int main(int argc, char **argv) {
    struct config cfg = {
        .subcommand = NONE,
        .name = "",
        .command = "",
    };

    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "run") == 0) {
            cfg.subcommand = RUN;
            i++;
        } else if (strcmp(argv[i], "exec") == 0) {
            cfg.subcommand = EXEC;
            i++;
        } else if (strcmp(argv[i], "--name") == 0) {
            if (i+1 >= argc) {
                fprintf(stderr, "[ERR] Missing --name value (e.g. [--name bib]).\n");
                return 1;
            }
            strncpy(cfg.name, argv[++i], sizeof(cfg.name) - 1);
            i++;
        } else {
            strncpy(cfg.command, argv[i], sizeof(cfg.command) - 1);
            i++;
        }
    }

    if (validate_config(cfg) != 0) {
        return 1;
    }

    switch (cfg.subcommand) {
        case RUN:
            if (run_container(cfg) != 0) {
                fprintf(stderr, "[ERR] Running container failed due to some internal errors.\n");
                return 1;
            }
            break;
        case EXEC:
            printf("EXEC subcommand have not implemented yet...\n");
            break;
        case NONE:
        default:
            break;
    }
    return 0;
}
