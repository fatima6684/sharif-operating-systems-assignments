#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>


#include "setup.h"

static int ensure_dir(const char *path, mode_t mode) {
  struct stat st;
  if (stat(path, &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      fprintf(stderr, "[ERR] Path %s exists but is not a directory\n", path);
      return 1;
    }
    return 0;
  }
  if (errno != ENOENT) {
    fprintf(stderr, "[ERR] stat(%s) failed: %s\n", path, strerror(errno));
    return 1;
  }
  if (mkdir(path, mode) != 0) {
    fprintf(stderr, "[ERR] mkdir(%s) failed: %s\n", path, strerror(errno));
    return 1;
  }
  return 0;
}

int setup_zocker_dir(void) {
  if (ensure_dir(ZOCKER_PREFIX, 0755) != 0) return 1;
  return 0;
}

int setup_container_dir(const char id[64], char container_dir[256]) {
  const size_t buffer_size = 256;

  if (snprintf(container_dir, buffer_size, "%s/%s", ZOCKER_PREFIX, id) < 0) {
    return 1;
  }

  if (ensure_dir(container_dir, 0755) != 0) return 1;

  char upper[256];
  if (snprintf(upper, sizeof(upper), "%s/upper", container_dir) < 0) return 1;
  if (ensure_dir(upper, 0755) != 0) return 1;

  char work[256];
  if (snprintf(work, sizeof(work), "%s/work", container_dir) < 0) return 1;
  if (ensure_dir(work, 0755) != 0) return 1;

  char merged[256];
  if (snprintf(merged, sizeof(merged), "%s/merged", container_dir) < 0) return 1;
  if (ensure_dir(merged, 0755) != 0) return 1;

  return 0;
}
