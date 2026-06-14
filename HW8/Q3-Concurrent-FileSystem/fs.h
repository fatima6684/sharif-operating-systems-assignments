#ifndef FS_H
#define FS_H

#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

// -------------------------- Constants --------------------------
// -------------------------- HW7 --------------------------
#define MAGIC_VAL 0xDEADBEEF
#define FS_VERSION 2            // Version bump for new structure

#define BLOCK_SIZE 4096
#define TOTAL_BLOCKS 32768      // 128 MB / 4 KB
#define FS_TOTAL_SIZE (TOTAL_BLOCKS * BLOCK_SIZE)   // 128 MB

// Layout Offsets
#define SUPERBLOCK_OFFSET 0
#define BITMAP_BLOCK_OFFSET BLOCK_SIZE
#define DATA_BLOCK_START_OFFSET (2 * BLOCK_SIZE)

// -------------------------- Users & Permissions --------------------------
#define MAX_NAME 32
#define MAX_USERS 16
#define MAX_GROUPS 16

// Permission bits (standard Unix-style)
// Read: 4, Write: 2, Execute: 1
// Example: 755 (Owner: rwx, Group: r-x, Others: r-x)
typedef struct {
    uint32_t owner_id;
    uint32_t group_id;
    uint32_t mode;   // e.g., 755
} permission_t;

typedef struct {
    uint32_t id;
    char name[MAX_NAME];
    uint32_t group_ids[MAX_GROUPS];  // Groups this user belongs to
    uint32_t group_count;
} user_t;

typedef struct {
    uint32_t id;
    char name[MAX_NAME];
} group_t;

// Global state for the current logged-in user
extern uint32_t g_current_user_id;

// -------------------------- FS Structures --------------------------

// Superblock
typedef struct {
    uint32_t magic;                 // must be 0xDEADBEEF
    uint32_t version;               // FS version
    uint32_t block_size;            // should be 4096
    uint32_t file_count;            // number of files
    
    // User/Group Data
    uint32_t user_count;
    uint32_t group_count;
    user_t user_table[MAX_USERS];
    group_t group_table[MAX_GROUPS];

    // Padding to fill exactly 4096 bytes
    uint8_t reserved[BLOCK_SIZE - (7 * 4) - (MAX_USERS * sizeof(user_t)) - (MAX_GROUPS * sizeof(group_t))];
} superblock_t;

// File Entry
#define FILENAME_MAX_LEN 64

typedef struct file_entry {
    char name[FILENAME_MAX_LEN];    // file name (no '/')
    uint32_t type;                  // 0 = normal file (for now)
    permission_t perm;              // HW6
    uint32_t size;                  // Actual file size (<= 4096 - size(file_entry_t))
    
} file_entry_t;

// Globals
extern FILE *g_fs_file;
extern char g_fs_path[256];
extern uint32_t g_open_file_offset;

// -------------------------- API --------------------------
bool init_fs(const char *path);

// File Operations
uint32_t create_file(const char *name);
uint32_t find_file(const char *name, file_entry_t *out);
bool load_file_entry(uint32_t offset, file_entry_t *ent);
void save_file_entry(uint32_t offset, const file_entry_t *ent);

void write_file(uint32_t pos, uint32_t nbytes);
void read_file(uint32_t pos, uint32_t nbytes);
void shrink_file(uint32_t newsize);
void remove_file();
void close_file();

// Stats & Visuals
void file_stats();
void fs_stats();
void visualize_bitmap(); // Replaces visualize_freelist

// User Operations

int get_gid_by_name(const char *name, superblock_t *sb);
int get_uid_by_name(const char *name, superblock_t *sb);

// Allocator
uint32_t fs_alloc(); // No size arg needed, always 1 block
void fs_free(uint32_t offset); // Frees the block at this offset

// User & Permission Prototypes
void user_add(const char *username);
void group_add(const char *groupname);
void usermod_ag(const char *groupname, const char *username);
void chown_file(const char *user_name, const char *group_name);
void chmod_file(uint32_t mode);
void getfacl_file();
bool check_permission(permission_t *p, int required);
bool login_user(const char *username);
void sync_users_from_disk();
void user_del(const char *username);
void group_del(const char *groupname);
void chgrp_file(const char *group_name);
void list_users_and_groups();
uint32_t get_primary_gid(uint32_t uid);

// Stress Test
void stress_test();

#endif