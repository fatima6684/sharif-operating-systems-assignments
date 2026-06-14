#ifndef __RUN_H__
#define __RUN_H__

#include "config.h"

struct container {
  char id[64];
  char command[256];
  char base_dir[512];
  char base_image[256];
};

int run_container(struct container cont);
void container_from_config(struct config cfg, struct container *c);

#endif /* __RUN_H__ */
