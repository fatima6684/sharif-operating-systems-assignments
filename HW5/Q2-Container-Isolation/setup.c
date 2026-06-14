// to support 'popen'
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

// newly added
#include <stdlib.h>
#include <string.h>

#include "setup.h"

static int setup_bin_dir(const char container_dir[256]);
static int setup_lib_dir(const char container_dir[256]);

int setup_zocker_dir(void) {
  struct stat st;
  char prefix[64];

  if (snprintf(prefix, sizeof(prefix), "%s", ZOCKER_PREFIX) < 0) {
    return 1;
  }

  if (stat(prefix, &st) == -1) {
    if (errno == ENOENT) {
      fprintf(stderr, "[ERR] ZOCKER_PREFIX %s does not exists\n",
              ZOCKER_PREFIX);
    }
    return 1;
  }

  if (!S_ISDIR(st.st_mode)) {
    fprintf(stderr, "[ERR] ZOCKER_PREFIX %s is not a directory\n",
            ZOCKER_PREFIX);
    return 1;
  }

  return 0;
}

int setup_container_dir(const char id[64], char container_dir[256]) {
  struct stat st;
  const size_t buffer_size = 256;

  if (snprintf(container_dir, buffer_size, "%s/%s", ZOCKER_PREFIX, id) < 0) {
    return 1;
  }

  if (stat(container_dir, &st) == -1) {
    if (errno != ENOENT) {
      return 1;
    }

    if (mkdir(container_dir, 0755) == -1) {
      fprintf(stderr, "[ERR] Failed to create container directory %s\n",
              container_dir);
      return 1;
    }
  } else if (!S_ISDIR(st.st_mode)) {
    fprintf(stderr, "[ERR] Path %s is not a directory\n", container_dir);
    return 1;
  }

  if (setup_bin_dir(container_dir) != 0) {
    fprintf(stderr, "[ERR] Failed to setup bin directory for container %s\n",
            id);
    return 1;
  }

  if (setup_lib_dir(container_dir) != 0) {
    fprintf(stderr, "[ERR] Failed to setup lib directory for container %s\n",
            id);
    return 1;
  }

  return 0;
}

// previous function
/*
static int setup_bin_dir(const char container_dir[256]) {
  char bin_dir[256];
  if (snprintf(bin_dir, sizeof(bin_dir), "%s/bin", container_dir) < 0) {
    return 1;
  }
  if (mkdir(bin_dir, 0755) == -1) {
    fprintf(stderr, "[ERR] Failed to create bin directory %s\n", bin_dir);
    return 1;
  }

  return 0;
}
*/

// updated function
// setup.c

// updated function
static int setup_bin_dir(const char container_dir[256]) {
    char bin_dir[256];
    char cmd[512];

    if (snprintf(bin_dir, sizeof(bin_dir), "%s/bin", container_dir) < 0) {
        return 1;
    }

    mkdir(bin_dir, 0755);

    // Copy /bin/sh into container/bin/sh
    snprintf(cmd, sizeof(cmd), "cp /bin/sh %s/sh", bin_dir);
    if (system(cmd) != 0) {
        fprintf(stderr, "[ERR] Failed to copy /bin/sh into container\n");
        return 1;
    }

    // Copy /bin/ls into container/bin/ls
    snprintf(cmd, sizeof(cmd), "cp /bin/ls %s/ls", bin_dir);
    if (system(cmd) != 0) {
        fprintf(stderr, "[ERR] Failed to copy /bin/ls into container\n");
    }
    
    // Copy other common commands (e.g., cat, echo, etc.)
    snprintf(cmd, sizeof(cmd), "cp /bin/cat %s/cat", bin_dir);
    system(cmd);
    
    snprintf(cmd, sizeof(cmd), "cp /bin/pwd %s/pwd", bin_dir);
    system(cmd);

    return 0;
}

// previous function
/*
static int setup_lib_dir(const char container_dir[256]) {
  char lib_dir[256];
  char lib32_dir[256];
  char lib64_dir[256];

  if (snprintf(lib_dir, sizeof(lib_dir), "%s/lib", container_dir) < 0) {
    return 1;
  }
  if (mkdir(lib_dir, 0755) == -1) {
    fprintf(stderr, "[ERR] Failed to create lib directory %s\n", lib_dir);
    return 1;
  }

  if (snprintf(lib32_dir, sizeof(lib32_dir), "%s/lib32", container_dir) < 0) {
    return 1;
  }
  if (mkdir(lib32_dir, 0755) == -1) {
    fprintf(stderr, "[ERR] Failed to create lib32 directory %s\n", lib32_dir);
    return 1;
  }

  if (snprintf(lib64_dir, sizeof(lib64_dir), "%s/lib64", container_dir) < 0) {
    return 1;
  }

  if (mkdir(lib64_dir, 0755) == -1) {
    fprintf(stderr, "[ERR] Failed to create lib64 directory %s\n", lib64_dir);
    return 1;
  }

  return 0;
}
*/

// updated function
static int setup_lib_dir(const char container_dir[256]) {
    char lib_dir[256];
    char lib64_dir[256];
    char cmd[512];
    FILE *fp;
    char line[512];

    snprintf(lib_dir, sizeof(lib_dir), "%s/lib", container_dir);
    mkdir(lib_dir, 0755);

    snprintf(lib64_dir, sizeof(lib64_dir), "%s/lib64", container_dir);
    mkdir(lib64_dir, 0755);

    // EXPLICITLY COPY DYNAMIC LINKER
    snprintf(cmd, sizeof(cmd), "cp -L /lib64/ld-linux-x86-64.so.2 %s/", lib64_dir);
    if (system(cmd) != 0) {
        fprintf(stderr, "[ERR] Failed to copy dynamic linker into container\n");
        return 1;
    }

    // Run ldd on all essential binaries copied in setup_bin_dir
    fp = popen("ldd /bin/sh /bin/ls /bin/cat /bin/pwd", "r");
    if (!fp) {
        fprintf(stderr, "[ERR] Failed to run ldd\n");
        return 1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *path_start = line;
        
        while (*path_start == ' ' || *path_start == '\t') {
            path_start++;
        }
        
        size_t len = strlen(path_start);
        if (len > 1 && path_start[len - 2] == ':') { // -2 accounts for potential newline
            continue; // Skip the line entirely
        }
        
        char *path = strstr(line, "=>");
        if (!path) {
            // some lines have format: "/lib64/ld-linux-x86-64.so.2 (0x...)"
            if (line[0] == '/') {
                path = strtok(line, " ");   // strip address
            } else {
                continue;
            }
        } else {
            // standard format: "library.so => /path/to/library.so (0x...)"
            path = strstr(line, "/");
        }

        if (!path) continue;

        path[strcspn(path, " \n")] = '\0'; // trim newline/spaces

        // Decide if it goes to lib or lib64 (Prioritize lib64 for 64-bit libs)
        const char *dest = strstr(path, "lib64") ? lib64_dir : lib_dir;

        // Use -L to dereference symlinks and copy the actual file
        snprintf(cmd, sizeof(cmd), "cp -L %s %s/", path, dest);
        
        system(cmd); 
    }

    pclose(fp);
    return 0;
}
