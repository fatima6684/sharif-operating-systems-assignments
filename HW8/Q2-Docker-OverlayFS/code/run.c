#define _GNU_SOURCE

#include <ctype.h>
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

static void rstrip(char *s) {
  size_t n = strlen(s);
  while (n > 0 &&
    (s[n - 1] == '\n' || s[n - 1] == '\r' ||
    isspace((unsigned char)s[n - 1]) || s[n - 1] == '\'' ||
    s[n - 1] == '"')) {
    s[--n] = '\0';
    }
    while (s[0] == '\'' || s[0] == '"') {
      memmove(s, s + 1, strlen(s));
    }
}

static int get_image_lowerdir(const char *image, char *out, size_t out_sz) {
  char cmd[768];

 
  if (snprintf(cmd, sizeof(cmd),
    "docker inspect --format='{{.GraphDriver.Data.LowerDir}}' %s 2>/dev/null",
    image) < 0) {
    return 1;
    }

    FILE *fp = popen(cmd, "r");
  if (!fp) {
    fprintf(stderr, "[ERR] popen failed for docker inspect\n");
    return 1;
  }

  if (!fgets(out, (int)out_sz, fp)) {
    pclose(fp);
    fprintf(stderr, "[ERR] Failed to read docker inspect output for %s\n", image);
    return 1;
  }

  pclose(fp);

  rstrip(out);

  if (out[0] == '\0') {
    fprintf(stderr, "[ERR] Empty LowerDir for image %s (is it pulled?)\n", image);
    return 1;
  }

  return 0;
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

void container_from_config(struct config cfg, struct container *c) {
  strncpy(c->id, cfg.name, sizeof(c->id));
  c->id[sizeof(c->id) - 1] = '\0';

  strncpy(c->command, cfg.command, sizeof(c->command));
  c->command[sizeof(c->command) - 1] = '\0';

  strncpy(c->base_dir, cfg.base_dir, sizeof(c->base_dir));
  c->base_dir[sizeof(c->base_dir) - 1] = '\0';

  strncpy(c->base_image, cfg.base_image, sizeof(c->base_image));
  c->base_image[sizeof(c->base_image) - 1] = '\0';
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
    if (setup_container_dir(cont.id, container_dir) != 0) {
      fprintf(stderr, "[ERR] Failed to setup container directory for %s\n", cont.id);
      return 1;
    }
    char upper[512], work[512], merged[512];

    if (snprintf(upper, sizeof(upper), "%s/upper", container_dir) < 0) return 1;
    if (snprintf(work, sizeof(work), "%s/work", container_dir) < 0) return 1;
    if (snprintf(merged, sizeof(merged), "%s/merged", container_dir) < 0) return 1;

    // 3) lowerdir از docker inspect (base-image)
    if (cont.base_image[0] == '\0') {
      fprintf(stderr, "[ERR] Missing --base-image (required for overlay rootfs)\n");
      return 1;
    }

    char lowerdir[4096];
    if (get_image_lowerdir(cont.base_image, lowerdir, sizeof(lowerdir)) != 0) {
      fprintf(stderr, "[ERR] Could not get LowerDir for image: %s\n", cont.base_image);
      return 1;
    }

    // 4) mount private
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
      fprintf(stderr, "[ERR] Failed to set mount private: %s\n", strerror(errno));
      return 1;
    }

    // 5) mount overlay روی merged
    char opts[4096];
    if (snprintf(opts, sizeof(opts),
      "lowerdir=%s,upperdir=%s,workdir=%s",
      lowerdir, upper, work) < 0) {
      return 1;
      }

      if (mount("overlay", merged, "overlay", 0, opts) != 0) {
        fprintf(stderr, "[ERR] Failed to mount overlay at %s: %s\n",
                merged, strerror(errno));
        return 1;
      }

      // 6) chroot روی merged (root کانتینر)
      if (chroot(merged) != 0) {
        fprintf(stderr, "[ERR] Failed to chroot into %s: %s\n",
                merged, strerror(errno));
        return 1;
      }

      if (chdir("/") != 0) {
        fprintf(stderr, "[ERR] Failed to change directory to root: %s\n",
                strerror(errno));
        return 1;
      }

      if (ensure_proc() != 0) return 1;

      if (sethostname(cont.id, strnlen(cont.id, sizeof(cont.id))) != 0) {
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
