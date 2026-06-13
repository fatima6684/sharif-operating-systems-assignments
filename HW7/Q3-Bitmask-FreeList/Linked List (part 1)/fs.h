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



// -------------------------- HW6 --------------------------
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
    uint32_t mode; // e.g., 755
} permission_t;

typedef struct {
    uint32_t id;
    char name[MAX_NAME];
    uint32_t group_ids[MAX_GROUPS]; // Groups this user belongs to
    uint32_t group_count;
} user_t;

typedef struct {
    uint32_t id;
    char name[MAX_NAME];
} group_t;

// Global state for the current logged-in user
extern uint32_t g_current_user_id;

// Management Function Prototypes
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

// -------------------------- initialize fs --------------------------
#define MAGIC_VAL 0xDEADBEEF
#define FS_VERSION 1

#define FS_TOTAL_SIZE (128ULL * 1024 * 1024)    // 128 MB
#define SUPERBLOCK_SIZE 4096    // 4 KB
#define BLOCK_SIZE 4096     // we're not using BLOCK structure for this version

// Superblock structure
typedef struct {
    uint32_t magic;               // must be 0xDEADBEEF
    uint32_t version;             // FS version
    uint32_t block_size;          // should be 4096
    uint32_t last_block;          // last allocated block index
    uint32_t file_list_head;      // offset of first file entry
    uint32_t file_count;          // number of files
    uint32_t free_block_head;     // head of free block list (0 = none)
    uint32_t user_count;
    uint32_t group_count;
    user_t user_table[MAX_USERS];
    group_t group_table[MAX_GROUPS];

    uint8_t reserved[SUPERBLOCK_SIZE - (9 * 4) - (MAX_USERS * sizeof(user_t)) - (MAX_GROUPS * sizeof(group_t))];
} superblock_t;

// global FILE* for our file system
extern FILE *g_fs_file;
extern char g_fs_path[256];

// currently opened file:
extern uint32_t g_open_file_offset;   // offset of file_entry_on_disk_t (0 if none opened)

// API
bool init_fs(const char *path);


// -------------------------- define file entries --------------------------
#define FILENAME_MAX_LEN 64

typedef struct file_entry {
    char name[FILENAME_MAX_LEN];   // file name (no '/')
    uint32_t type;                 // 0 = normal file (for now)
    permission_t perm;             // HW6
    uint32_t size;                 // file size in bytes

    // where file data begins (offset in the 128MB filesystem)
    uint32_t data_offset; // not an actual block! just the offset where data begins.

    // linked list
    uint32_t next;                 // offset of next file_entry (or 0)

} file_entry_t;

uint32_t create_file(const char *name);
uint32_t find_file(const char *name, file_entry_t *out);
bool load_file_entry(uint32_t offset, file_entry_t *ent);

// -------------------------- show stats --------------------------
void file_stats();
void fs_stats();
void close_file();

// -------------------------- read/write/shrink/rm --------------------------
void write_file(uint32_t pos, uint32_t nbytes);
void read_file(uint32_t pos, uint32_t nbytes);
void shrink_file(uint32_t newsize);
void remove_file();



// -------------------------- HW5 --------------------------
// -------------------------- FreeList --------------------------
#define FREELIST_ENTRY_SIZE sizeof(free_entry_t)

typedef struct free_entry {
    uint32_t start;      // start offset of free region
    uint32_t end;        // end offset (exclusive)
    uint32_t next;       // next free block in the list
} free_entry_t;

void visualize_freelist();
bool load_free_entry(uint32_t offset, free_entry_t *ent);
bool save_free_entry(const free_entry_t *ent);

uint32_t fs_alloc(uint32_t size);
void fs_free(uint32_t start, uint32_t end);

int get_gid_by_name(const char *name, superblock_t *sb);
int get_uid_by_name(const char *name, superblock_t *sb);

// -------------------------- HW7 --------------------------
void stress_test();

#endif