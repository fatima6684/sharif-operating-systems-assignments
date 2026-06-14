#include <stdio.h>
#include <string.h>

#include "config.h"

int validate_config(struct config cfg) {
  if (cfg.subcommand == NONE) {
    fprintf(stderr, "[ERR] Missing subcommand (run | exec)\n");
    return 1;
  }

  if (cfg.name[0] == '\0') {
    strncpy(cfg.name, DEFAULT_NAME, sizeof(cfg.name) - 1);
    cfg.name[sizeof(cfg.name) - 1] = '\0';
  }

  if (cfg.command[0] == '\0') {
    fprintf(stderr, "[ERR] Missing command (e.g. '/bin/sh' or 'sleep 1000')\n");
    return 1;
  }

  if (cfg.subcommand == RUN) {
    if (cfg.base_image[0] == '\0' && cfg.base_dir[0] == '\0') {
      fprintf(stderr,
              "[ERR] Missing base filesystem. "
              "Use either --base-image or --base-dir\n");
      return 1;
    }
  }

  return 0;
}
