#define _GNU_SOURCE

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "run.h"
#include "setup.h"

void container_from_config(struct config cfg, struct container *c) {
  strncpy(c->id, cfg.name, sizeof(c->id));
  c->id[sizeof(c->id) - 1] = '\0';

  strncpy(c->command, cfg.command, sizeof(c->command));
  c->command[sizeof(c->command) - 1] = '\0';

  strncpy(c->base_dir, cfg.base_dir, sizeof(c->base_dir));
  c->base_dir[sizeof(c->base_dir) - 1] = '\0';
}

static int ensure_proc(void) {
  if (mkdir("/proc", 0555) != 0 && errno != EEXIST) {
    fprintf(stderr, "[ERR] Failed to create /proc: %s\n", strerror(errno));
    return 1;
  }
  if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
    fprintf(stderr, "[ERR] Failed to mount /proc: %s\n", strerror(errno));
    return 1;
  }
  return 0;
}

int run_container(struct container cont) {
  pid_t pid;

  if (unshare(CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWTIME) != 0) {
    perror("[ERR] unshare failed");
    return 1;
  }

  pid = fork();
  if (pid < 0) {
    perror("[ERR] fork failed");
    return 1;
  }

  if (pid == 0) {
    char container_dir[256];

    if (cont.base_dir[0] != '\0') {
      snprintf(container_dir, sizeof(container_dir), "%s", cont.base_dir);
    } else {
      if (setup_container_dir(cont.id, container_dir) != 0) {
        fprintf(stderr, "[ERR] Failed to setup container directory for %s\n", cont.id);
        return 1;
      }
    }

    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
      fprintf(stderr, "[ERR] Failed to set mount private: %s\n", strerror(errno));
      return 1;
    }

    if (chroot(container_dir) != 0) {
      fprintf(stderr, "[ERR] Failed to chroot into %s: %s\n",
              container_dir, strerror(errno));
      return 1;
    }

    if (chdir("/") != 0) {
      fprintf(stderr, "[ERR] Failed to change directory to root: %s\n", strerror(errno));
      return 1;
    }

    if (ensure_proc() != 0) return 1;

    if (sethostname(cont.id, strnlen(cont.id, 64)) != 0) {
      fprintf(stderr, "[ERR] Failed to set hostname: %s\n", strerror(errno));
      return 1;
    }

    printf("Running container '%s' with PID: %d\n", cont.id, getpid());

    if (execl("/bin/sh", "sh", "-c", cont.command, NULL) != 0) {
      fprintf(stderr, "[ERR] Failed to execute command: %s\n", strerror(errno));
      return 1;
    }
    return 0;
  } else {
    waitpid(pid, NULL, 0);
    printf("[Parent] Container %s stopped\n", cont.id);
  }

  return 0;
}


