#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

// -------------------------- initialize fs --------------------------
#define MAGIC_VAL 0xDEADBEEF
#define FS_VERSION 1

#define FS_TOTAL_SIZE (256ULL * 1024 * 1024)    // 256 MB
#define SUPERBLOCK_SIZE 4096    // 4 MB
#define BLOCK_SIZE 4096

// Superblock structure
typedef struct {
    uint32_t magic;               // must be 0xDEADBEEF
    uint32_t version;             // FS version
    uint32_t block_size;          // should be 4096
    uint32_t last_block;          // last allocated block index
    uint32_t file_list_head;      // offset of first file entry
    uint32_t file_count;          // number of files
    uint32_t free_block_head;     // head of free block list (0 = none)
    uint8_t reserved[SUPERBLOCK_SIZE - 7 * 4];
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
    uint32_t permission;           // ignore or set constant
    uint32_t size;                 // file size in bytes

    // where file data begins (offset in the 256MB filesystem)
    uint32_t first_block;

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
#endif
