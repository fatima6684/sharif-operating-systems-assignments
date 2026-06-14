#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "run.h"
#include "setup.h"

void container_from_config(struct config cfg, struct container *c) {
  strncpy(c->id, cfg.name, sizeof(c->id));
  strncpy(c->command, cfg.command, sizeof(c->command));
}

int run_container(struct container cont) {
  pid_t pid;

  if (unshare(CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWTIME) != 0) {
    return 1;
  }

  pid = fork();
  if (pid < 0) {
    return 1;
  }

  if (pid == 0) {
    char container_dir[256];
    if (setup_container_dir(cont.id, container_dir) != 0) {
      fprintf(stderr, "[ERR] Failed to setup container directory for %s\n",
              cont.id);
      return 1;
    }

    // Make mounts private
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
      fprintf(stderr, "[ERR] Failed to change mount to private: %s\n",
              strerror(errno));
      return 1;
    }


    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    // CHANGE ROOT
    if (chroot(container_dir) != 0) {
        fprintf(stderr, "[ERR] chroot failed: %s\n", strerror(errno));
        return 1;
    }

    // After chroot(), we MUST chdir("/") or paths break
    if (chdir("/") != 0) {
        fprintf(stderr, "[ERR] chdir('/') failed: %s\n", strerror(errno));
        return 1;
    }

    mkdir("/proc", 0555);

    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    // Now inside new root, mount proc
    if (mount(NULL, "/proc", "proc", 0, NULL) != 0) {
      fprintf(stderr, "[ERR] Failed to remount /proc: %s\n", strerror(errno));
      return 1;
    }

    if (sethostname(cont.id, 64) != 0) {
      fprintf(stderr, "[ERR] Failed to set hostname\n");
      return 1;
    }

    printf("Running child with pid: %d\n", getpid());
    if (execl("/bin/sh", "sh", "-c", cont.command, NULL) != 0) {
      fprintf(stderr, "[ERR] Failed to call create container process: %s\n",
              strerror(errno));
      return 1;
    }
  } else {
    sleep(2);
    waitpid(pid, NULL, 0);
    printf("[Parent] Stoping...\n");
  }
  return 0;
}
