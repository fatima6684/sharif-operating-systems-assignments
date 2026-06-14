#include "fs.h"
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
// -------------------------- Initialize filesystem --------------------------
FILE *g_fs_file = NULL;
char g_fs_path[256] = {0};

// no file opened initially
uint32_t g_open_file_offset = 0;

bool load_superblock(superblock_t *sb) {
    fseek(g_fs_file, 0, SEEK_SET);
    size_t n = fread(sb, 1, sizeof(superblock_t), g_fs_file);
    return (n == sizeof(superblock_t));
}

bool save_superblock(const superblock_t *sb) {
    fseek(g_fs_file, 0, SEEK_SET);  // point file pointer at the begining of the fs
    size_t n = fwrite(sb, 1, sizeof(superblock_t), g_fs_file); // write byte by byte
    fflush(g_fs_file); // force the write
    return (n == sizeof(superblock_t));
}

bool create_fs(const char *path) {
    // Create the filesystem
    g_fs_file = fopen(path, "w+b"); // write + binary
    if (!g_fs_file) {
        printf("Error: cannot create fs file.\n");
        return false;
    }
    // Create a superblock instance in memory
    superblock_t sb;
    memset(&sb, 0, sizeof(sb)); // set all bytes to 0

    sb.magic = MAGIC_VAL;
    sb.version = FS_VERSION;
    sb.block_size = BLOCK_SIZE;
    sb.last_block = 0;
    sb.file_list_head = 0;
    sb.file_count = 0;
    sb.free_block_head = 0;

    // Write initial superblock
    if (!save_superblock(&sb)) {
        printf("Error: failed writing superblock.\n");
        return false;
    }
    // Seek to last byte of desired FS size
    if (fseek(g_fs_file, FS_TOTAL_SIZE - 1, SEEK_SET) != 0) {
        printf("Error: cannot seek to allocate filesystem size.\n");
        return false;
    }
    // Write a single NULL byte to force allocation
    if (fwrite("\0", 1, 1, g_fs_file) != 1) {
        printf("Error: cannot finalize filesystem allocation.\n");
        return false;
    }
    fflush(g_fs_file);

    printf("Created new filesystem '%s' (size = %.2f MB)\n",
           path, FS_TOTAL_SIZE / (1024.0 * 1024.0));

    return true;
}

bool open_fs(const char *path) {
    g_fs_file = fopen(path, "r+b"); // read + binary
    if (!g_fs_file) {
        printf("Error: cannot open fs file.\n");
        return false;
    }

    superblock_t sb;
    if (!load_superblock(&sb)) {
        printf("Error: cannot read superblock.\n");
        return false;
    }

    if (sb.magic != MAGIC_VAL) {
        printf("Error: invalid filesystem magic number.\n");
        return false;
    }

    if (sb.version != FS_VERSION) {
        printf("Error: unsupported FS version.\n");
        return false;
    }

    printf("Loaded existing filesystem %s\n", path);
    return true;
}

// Creates file if missing, loads or writes superblock
bool init_fs(const char *path) {
    // path here is basically the name of the filesystem
    strncpy(g_fs_path, path, sizeof(g_fs_path));

    // Check if file exists
    struct stat st;
    bool exists = (stat(path, &st) == 0);

    if (!exists)
        return create_fs(path);

    // File already exists -> open and verify
    return open_fs(path);
}

// -------------------------- create file and entry --------------------------
bool load_file_entry(uint32_t offset, file_entry_t *ent) {
    fseek(g_fs_file, offset, SEEK_SET); // set the pointer at proper offset
    size_t n = fread(ent, 1, sizeof(file_entry_t), g_fs_file);
    return (n == sizeof(file_entry_t));
}

bool save_file_entry(uint32_t offset, const file_entry_t *ent) {
    fseek(g_fs_file, offset, SEEK_SET);
    size_t n = fwrite(ent, 1, sizeof(file_entry_t), g_fs_file);
    fflush(g_fs_file);
    return (n == sizeof(file_entry_t));
}

uint32_t alloc_file_entry() {
    superblock_t sb;
    load_superblock(&sb);

    uint32_t offset = sb.last_block;
    if (offset < sizeof(superblock_t))
        offset = sizeof(superblock_t);

    uint32_t new_offset = offset + sizeof(file_entry_t);

    if (new_offset >= FS_TOTAL_SIZE) {
        printf("Error: filesystem full (no space for new metadata).\n");
        return 0;
    }

    sb.last_block = new_offset;
    save_superblock(&sb);

    return offset;
}

uint32_t find_file(const char *name, file_entry_t *out) {
    superblock_t sb;
    load_superblock(&sb);

    uint32_t cur = sb.file_list_head;

    while (cur != 0) {
        file_entry_t ent;
        if (!load_file_entry(cur, &ent))
            return 0;

        if (strcmp(ent.name, name) == 0) {
            if (out) *out = ent;
            return cur;
        }

        cur = ent.next;
    }

    return 0;
}

uint32_t create_file(const char *name) {
    if (strlen(name) >= FILENAME_MAX_LEN) {
        printf("Error: filename too long.\n");
        return 0;
    }

    // Check if file already exists
    if (find_file(name, NULL) != 0) {
        printf("Error: file already exists.\n");
        return 0;
    }

    uint32_t off = alloc_file_entry();
    if (off == 0) return 0;

    file_entry_t ent;
    memset(&ent, 0, sizeof(ent));

    strcpy(ent.name, name);
    ent.type = 0;          // normal file
    ent.permission = 0;    // ignore for now
    ent.size = 0;
    ent.first_block = 0;
    ent.next = 0;

    // Load superblock to link the file
    superblock_t sb;
    load_superblock(&sb);

    if (sb.file_list_head == 0) {
        sb.file_list_head = off;
    } else {
        // append to the end of linked list
        uint32_t cur = sb.file_list_head;
        file_entry_t temp;
        while (1) {
            load_file_entry(cur, &temp);
            if (temp.next == 0) break;
            cur = temp.next;
        }
        temp.next = off;
        save_file_entry(cur, &temp);
    }

    sb.file_count++;
    save_superblock(&sb);

    save_file_entry(off, &ent);

    return off;
}


// -------------------------- close file --------------------------
void close_file() {
    if (g_open_file_offset == 0) {
        printf("No file is open.\n");
        return;
    }

    g_open_file_offset = 0;
    printf("File closed.\n");
}

// -------------------------- show stats --------------------------
void file_stats() {
    if (g_open_file_offset == 0) {
        printf("No file is open.\n");
        return;
    }

    file_entry_t ent;
    if (!load_file_entry(g_open_file_offset, &ent)) {
        printf("Error: cannot load file metadata.\n");
        return;
    }

    printf("File name: %s\n", ent.name);
    printf("Size: %u bytes\n", ent.size);
    printf("First block: %u\n", ent.first_block);
    printf("Next entry: %u\n", ent.next);
}

void fs_stats() {
    superblock_t sb;
    if (!load_superblock(&sb)) {
        printf("Error: cannot load superblock.\n");
        return;
    }

    uint64_t used = sb.last_block;
    if (used < sizeof(superblock_t))
        used = sizeof(superblock_t);

    uint64_t free_space = FS_TOTAL_SIZE - used;

    printf("Filesystem statistics:\n");
    printf("  Total size: %llu bytes (%.2f MB)\n",
           (unsigned long long)FS_TOTAL_SIZE,
           FS_TOTAL_SIZE / (1024.0 * 1024.0));

    printf("  Block size: %u bytes (%.2f KB)\n", sb.block_size, sb.block_size / 1024.0);
    printf("  File count: %u\n", sb.file_count);

    printf("  Used space: %llu bytes (%.2f KB)\n",
           (unsigned long long)used, used / 1024.0);

    printf("  Free space: %llu bytes\n",
           (unsigned long long)free_space);
}

// -------------------------- read/write --------------------------
uint32_t alloc_data_block(uint32_t size_to_add) {
    superblock_t sb;
    load_superblock(&sb);

    uint32_t offset = sb.last_block;

    // ensure metadata does not overlap data
    if (offset < sizeof(superblock_t))
        offset = sizeof(superblock_t);

    uint32_t new_offset = offset + size_to_add;

    if (new_offset > FS_TOTAL_SIZE) {
        printf("Error: no space left for data.\n");
        return 0;
    }

    sb.last_block = new_offset;
    save_superblock(&sb);

    return offset;
}

void write_file(uint32_t pos, uint32_t nbytes) {
    if (g_open_file_offset == 0) {
        printf("No file is open.\n");
        return;
    }

    file_entry_t ent;
    if (!load_file_entry(g_open_file_offset, &ent)) {
        printf("Error: cannot load file.\n");
        return;
    }

    // allocate data region on first write
    if (ent.first_block == 0) {
        ent.first_block = alloc_data_block(nbytes);
        if (ent.first_block == 0) return;
    }

    // extend file data if needed
    uint32_t required = pos + nbytes;
    if (required > ent.size) {

        uint32_t extra = required - ent.size;
        // append extra space
        uint32_t new_block = alloc_data_block(extra);
        if (new_block == 0) return;

        ent.size = required;
    }

    // read data from stdin
    char *buffer = malloc(nbytes);
    if (!buffer) {
        printf("Memory error.\n");
        return;
    }

    fread(buffer, 1, nbytes, stdin);  // read raw bytes

    // write to fs
    fseek(g_fs_file, ent.first_block + pos, SEEK_SET);
    fwrite(buffer, 1, nbytes, g_fs_file);
    fflush(g_fs_file);

    free(buffer);

    save_file_entry(g_open_file_offset, &ent);

    printf("Wrote %u bytes at pos %u.\n", nbytes, pos);
}


void read_file(uint32_t pos, uint32_t nbytes) {
    if (g_open_file_offset == 0) {
        printf("No file is open.\n");
        return;
    }

    file_entry_t ent;
    if (!load_file_entry(g_open_file_offset, &ent)) {
        printf("Error: cannot load file.\n");
        return;
    }

    if (pos >= ent.size) {
        printf("EOF\n");
        return;
    }

    if (pos + nbytes > ent.size)
        nbytes = ent.size - pos;

    char *buffer = malloc(nbytes);
    if (!buffer) {
        printf("Memory error.\n");
        return;
    }

    fseek(g_fs_file, ent.first_block + pos, SEEK_SET);
    fread(buffer, 1, nbytes, g_fs_file);

    fwrite(buffer, 1, nbytes, stdout);  // print result
    printf("\n");

    free(buffer);
}

// -------------------------- shrink - rm --------------------------
void shrink_file(uint32_t newsize) {
    if (g_open_file_offset == 0) {
        printf("No file is open.\n");
        return;
    }

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);

    if (newsize > ent.size) {
        printf("Error: new size is bigger than old size.\n");
        return;
    }

    ent.size = newsize;

    save_file_entry(g_open_file_offset, &ent);

    printf("File truncated to %u bytes.\n", newsize);
}

void remove_file() {
    if (g_open_file_offset == 0) {
        printf("No file is open.\n");
        return;
    }

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);

    uint32_t file_start = ent.first_block;
    uint32_t file_end   = ent.first_block + ent.size;

    superblock_t sb;
    load_superblock(&sb);

    //    1. Remove from linked list
    uint32_t target = g_open_file_offset;

    if (sb.file_list_head == target) {
        // remove head
        sb.file_list_head = ent.next;
    } else {
        // find previous entry
        uint32_t cur = sb.file_list_head;
        file_entry_t tmp;
        while (cur != 0) {
            load_file_entry(cur, &tmp);
            if (tmp.next == target)
                break;
            cur = tmp.next;
        }
        if (cur != 0) {
            tmp.next = ent.next;
            save_file_entry(cur, &tmp);
        }
    }

    sb.file_count--;

    //    2. Reclaim space only if this file is the last allocated
    if (file_end == sb.last_block) {
        sb.last_block = file_start;
    }

    //    3. If NO files remain, reset storage to initial state!
    if (sb.file_count == 0) {
        sb.last_block = sizeof(superblock_t);
    }

    save_superblock(&sb);

    g_open_file_offset = 0;

    printf("File removed.\n");
}
