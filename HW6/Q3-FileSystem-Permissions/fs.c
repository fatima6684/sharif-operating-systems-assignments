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
    g_fs_file = fopen(path, "w+b");
    if (!g_fs_file) return false;

    // 1. Init Superblock
    superblock_t sb = {0};
    sb.magic = MAGIC_VAL;
    sb.version = FS_VERSION;
    sb.block_size = BLOCK_SIZE;
    sb.last_block = sizeof(superblock_t);
    sb.file_count = 0;

    // Initialize root user & group
    sb.group_count = 1;
    sb.group_table[0].id = 0;
    strcpy(sb.group_table[0].name, "root");

    sb.user_count = 1;
    sb.user_table[0].id = 0;
    strcpy(sb.user_table[0].name, "root");
    sb.user_table[0].group_ids[0] = 0;
    sb.user_table[0].group_count = 1;

    // 2. Free list head right after superblock
    sb.free_block_head = sizeof(superblock_t);

    if (!save_superblock(&sb)) return false;

    // 3. Create initial huge free entry
    free_entry_t initial_free;
    initial_free.start = sizeof(superblock_t);
    initial_free.end = FS_TOTAL_SIZE;
    initial_free.next = 0;

    save_free_entry(initial_free.start, &initial_free);

    // 4. Set file size
    if (fseek(g_fs_file, FS_TOTAL_SIZE - 1, SEEK_SET) != 0) return false;
    if (fwrite("\0", 1, 1, g_fs_file) != 1) return false;
    fflush(g_fs_file);

    sync_users_from_disk();

    printf("Created new filesystem '%s'\n", path);
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
    sync_users_from_disk();
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

    uint32_t off = fs_alloc(sizeof(file_entry_t));
    if (off == 0) return 0;

    file_entry_t ent;
    memset(&ent, 0, sizeof(ent));

    strcpy(ent.name, name);
    ent.type = 0;          // normal file
    ent.perm = (permission_t){
        .owner_id = g_current_user_id,
        .group_id = get_primary_gid(g_current_user_id),
        .mode = 644
    };
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

    printf("\n------ Filesystem statistics: ------\n");
    printf("  Total size: %llu bytes (%.2f MB)\n",
           (unsigned long long)FS_TOTAL_SIZE,
           FS_TOTAL_SIZE / (1024.0 * 1024.0));

    printf("  Block size: %u bytes (%.2f KB)\n", sb.block_size, sb.block_size / 1024.0);
    printf("  File count: %u\n", sb.file_count);

    printf("  Used space: %llu bytes (%.2f KB)\n",
           (unsigned long long)used, used / 1024.0);

    printf("  Free space: %llu bytes\n",
           (unsigned long long)free_space);

    printf("------------------------------------\n\n");

}

// -------------------------- read/write --------------------------
void write_file(uint32_t pos, uint32_t nbytes) {
    if (g_open_file_offset == 0) { printf("No file open.\n"); return; }

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);

    // 2 is the bit for WRITE
    if (!check_permission(&ent.perm, 2)) {
        printf("Permission denied: cannot write.\n");
        return;
    }

    // Allocate initial block if empty
    if (ent.first_block == 0) {
        ent.first_block = fs_alloc(nbytes);
        if (ent.first_block == 0) return;
        ent.size = nbytes; // Set initial size
    }
    
    uint32_t end_of_file = ent.first_block + ent.size;
    uint32_t write_end = ent.first_block + pos + nbytes;

    if (write_end > end_of_file) {
        // need to extend
        uint32_t needed_total = pos + nbytes;
        uint32_t needed_extra = needed_total - ent.size;
        uint32_t new_space = fs_alloc(needed_extra);
        if (new_space == 0) {
            printf("No space left.\n");
            return;
        }
        ent.size = needed_total;
    }

    // Write data
    char *buffer = malloc(nbytes);
    if (buffer) {
        fread(buffer, 1, nbytes, stdin); 
        fseek(g_fs_file, ent.first_block + pos, SEEK_SET);
        fwrite(buffer, 1, nbytes, g_fs_file);
        free(buffer);
    }
    
    save_file_entry(g_open_file_offset, &ent);
    printf("Wrote %u bytes.\n", nbytes);
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

    // 4 is the bit for READ
    if (!check_permission(&ent.perm, 4)) {
        printf("Permission denied: cannot read.\n");
        return;
    }

    if (pos >= ent.size) {
        printf("ERROR: THE FILE IS EMPTY\n");
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
    if (g_open_file_offset == 0) return;

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);

    if (newsize >= ent.size) return;

    uint32_t old_end = ent.first_block + ent.size;
    uint32_t new_end = ent.first_block + newsize;

    // Return the difference to the free list
    fs_free(new_end, old_end);

    ent.size = newsize;
    save_file_entry(g_open_file_offset, &ent);
    printf("File shrunk.\n");
}

void remove_file() {
    if (g_open_file_offset == 0) { printf("There is no open file!"); return;}

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);
    
    if (!check_permission(&ent.perm, 2)) {
        printf("Permission denied: cannot remove file.\n");
        return;
    }

    // 1. Unlink from file list
    superblock_t sb;
    load_superblock(&sb);
    uint32_t target = g_open_file_offset;
    if (sb.file_list_head == target) {
        sb.file_list_head = ent.next;
    } else {
        uint32_t cur = sb.file_list_head;
        file_entry_t tmp;
        while (cur != 0) {
            load_file_entry(cur, &tmp);
            if (tmp.next == target) {
                tmp.next = ent.next;
                save_file_entry(cur, &tmp);
                break;
            }
            cur = tmp.next;
        }
    }
    sb.file_count--;
    save_superblock(&sb);

    // 2. Free Data
    if (ent.first_block != 0 && ent.size > 0) {
        fs_free(ent.first_block, ent.first_block + ent.size);
    }

    // 3. Free Metadata entry itself
    // treat the file_entry slot as a small memory block and free it too
    fs_free(g_open_file_offset, g_open_file_offset + sizeof(file_entry_t));

    g_open_file_offset = 0;
    printf("File removed.\n");
}

// -------------------------- HW5 --------------------------
// -------------------------- FreeList --------------------------
bool load_free_entry(uint32_t offset, free_entry_t *ent) {
    fseek(g_fs_file, offset, SEEK_SET);
    return fread(ent, 1, sizeof(free_entry_t), g_fs_file) == sizeof(free_entry_t);
}

bool save_free_entry(uint32_t offset, const free_entry_t *ent) {
    fseek(g_fs_file, offset, SEEK_SET);
    size_t n = fwrite(ent, 1, sizeof(free_entry_t), g_fs_file);
    fflush(g_fs_file);
    return n == sizeof(free_entry_t);
}

void visualize_freelist() {
    superblock_t sb;
    if (!load_superblock(&sb)) {
        printf("Error: cannot load superblock.\n");
        return;
    }

    if (sb.free_block_head == 0) {
        printf("[FreeList] No free regions.\n");
        return;
    }

    // --- Load all free entries ---
    uint32_t list_capacity = 64;
    uint32_t count = 0;
    free_entry_t *list = malloc(list_capacity * sizeof(free_entry_t));
    uint32_t cur = sb.free_block_head;

    while (cur != 0) {
        if (count >= list_capacity) {
            list_capacity *= 2;
            list = realloc(list, list_capacity * sizeof(free_entry_t));
        }

        free_entry_t f;
        load_free_entry(cur, &f);
        list[count++] = f;

        cur = f.next;
    }

    // --- Sort free entries by start offset ---
    for (uint32_t i = 0; i < count; i++) {
        for (uint32_t j = i + 1; j < count; j++) {
            if (list[j].start < list[i].start) {
                free_entry_t temp = list[i];
                list[i] = list[j];
                list[j] = temp;
            }
        }
    }

    // --- Print results ---
    printf("\n----- Free Space Visualization -----\n");
    printf("Total free blocks: %u\n\n", count);

    for (uint32_t i = 0; i < count; i++) {
        printf("Region %2u: start =%10u  end =%10u  size =%10u bytes\n",
               i,
               list[i].start,
               list[i].end,
               list[i].end - list[i].start
            );
    }

    printf("------------------------------------\n\n");

    free(list);
}

uint32_t fs_alloc(uint32_t size) {
    if (size == 0) return 0;

    superblock_t sb;
    load_superblock(&sb);

    uint32_t prev_ptr = 0;
    uint32_t curr_ptr = sb.free_block_head;
    free_entry_t curr;

    while (curr_ptr != 0) {
        load_free_entry(curr_ptr, &curr);
        
        uint32_t available = curr.end - curr.start;

        if (available >= size) {
            // Found a spot
            uint32_t alloc_start = curr.start;
            uint32_t remaining = available - size;

            // Update the link to point to the NEXT node
            uint32_t next_node = curr.next;

            if (remaining >= sizeof(free_entry_t)) {
                free_entry_t new_entry;
                new_entry.start = alloc_start + size;
                new_entry.end   = curr.end;
                new_entry.next  = curr.next;

                // Write new entry at the new split location
                save_free_entry(new_entry.start, &new_entry);
                
                next_node = new_entry.start;
            } else {
                // the remaining space is so small it's not usable
            }

            // Update previous pointer or head
            if (prev_ptr == 0) {
                sb.free_block_head = next_node;
                save_superblock(&sb);
            } else {
                free_entry_t prev;
                load_free_entry(prev_ptr, &prev);
                prev.next = next_node;
                save_free_entry(prev_ptr, &prev);
            }

            return alloc_start;
        }

        prev_ptr = curr_ptr;
        curr_ptr = curr.next;
    }

    printf("Error: No contiguous free block of size %u found.\n", size);
    return 0;
}

void fs_free(uint32_t start, uint32_t end) {
    if (start >= end) return;

    superblock_t sb;
    load_superblock(&sb);

    // 1. Create the new entry in memory
    free_entry_t new_ent;
    new_ent.start = start;
    new_ent.end = end;
    new_ent.next = 0;

    // 2. Find insertion spot
    uint32_t prev_ptr = 0;
    uint32_t curr_ptr = sb.free_block_head;
    free_entry_t curr;

    while (curr_ptr != 0) {
        load_free_entry(curr_ptr, &curr);
        if (curr.start > start) break; // found the one after us
        prev_ptr = curr_ptr;
        curr_ptr = curr.next;
    }

    // 3. Insert new_ent between prev_ptr and curr_ptr
    new_ent.next = curr_ptr;

    // Check merge with NEXT
    if (curr_ptr != 0 && new_ent.end == curr.start) {
        new_ent.end = curr.end;
        new_ent.next = curr.next; // absorb the next node
    }

    // Check merge with PREV
    if (prev_ptr != 0) {
        free_entry_t prev;
        load_free_entry(prev_ptr, &prev);
        
        if (prev.end == new_ent.start) {
            prev.end = new_ent.end;
            prev.next = new_ent.next; // absorb new node into prev
            save_free_entry(prev_ptr, &prev);
        } else {
            prev.next = start; // new node sits at 'start'
            save_free_entry(prev_ptr, &prev);
            save_free_entry(start, &new_ent);
        }
    } else {
        // Insert at head
        sb.free_block_head = start;
        save_superblock(&sb);
        save_free_entry(start, &new_ent);
    }
}

// -------------------------- HW6 --------------------------
// -------------------------- Users & Permissions --------------------------
user_t g_users[MAX_USERS];
group_t g_groups[MAX_GROUPS];
uint32_t g_user_count = 0;
uint32_t g_group_count = 0;
uint32_t g_current_user_id = 0; // Default to Root

void init_root() {
    // Add Root Group
    g_groups[0].id = 0;
    strcpy(g_groups[0].name, "root");
    g_group_count = 1;

    // Add Root User
    g_users[0].id = 0;
    strcpy(g_users[0].name, "root");
    g_users[0].group_ids[0] = 0;
    g_users[0].group_count = 1;
    g_user_count = 1;
}

void user_add(const char *username) {
    if (g_current_user_id != 0) { 
        printf("Permission denied: Only root can add users.\n"); 
        return; 
    }

    superblock_t sb;
    load_superblock(&sb);

    if (sb.user_count >= MAX_USERS) {
        printf("Error: Max users reached.\n");
        return;
    }

    // Find first unused UID
    int new_uid = 0;
    for (; new_uid < MAX_USERS; new_uid++) {
        bool used = false;
        for (uint32_t i = 0; i < sb.user_count; i++) {
            if (sb.user_table[i].id == (uint32_t)new_uid) {
                used = true;
                break;
            }
        }
        if (!used) break;
    }

    sb.user_table[sb.user_count].id = new_uid;
    strncpy(sb.user_table[sb.user_count].name, username, MAX_NAME);
    sb.user_table[sb.user_count].group_count = 0;
    sb.user_count++;

    save_superblock(&sb); // Save changes to the disk file
    sync_users_from_disk();

    printf("User '%s' added successfully.\n", username);
}

void usermod_ag(const char *groupname, const char *username) {
    if (g_current_user_id != 0) {
        printf("Permission denied: only root can modify users.\n");
        return;
    }

    superblock_t sb;
    load_superblock(&sb);

    int uid = get_uid_by_name(username, &sb);
    int gid = get_gid_by_name(groupname, &sb);

    if (uid == -1 || gid == -1) {
        printf("User or group not found.\n");
        return;
    }

    user_t *u = &sb.user_table[uid];

    // prevent duplicates
    for (uint32_t i = 0; i < u->group_count; i++) {
        if (u->group_ids[i] == (uint32_t)gid) {
            printf("User already in group.\n");
            return;
        }
    }

    if (u->group_count >= MAX_GROUPS) {
        printf("Error: user group limit reached.\n");
        return;
    }

    u->group_ids[u->group_count++] = gid;

    save_superblock(&sb);
    sync_users_from_disk();

    printf("User %s added to group %s.\n", username, groupname);
}

bool check_permission(permission_t *p, int required) {
    if (g_current_user_id == 0) return true; // root bypass

    int owner = (p->mode / 100) % 10;
    int group = (p->mode / 10) % 10;
    int other = p->mode % 10;

    if (p->owner_id == g_current_user_id)
        return (owner & required) == required;

    user_t *u = &g_users[g_current_user_id];
    for (uint32_t i = 0; i < u->group_count; i++) {
        if (u->group_ids[i] == p->group_id)
            return (group & required) == required;
    }

    return (other & required) == required;
}

void chmod_file(uint32_t mode) {
    if (g_open_file_offset == 0) return;
    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);
    
    if (g_current_user_id != 0 && ent.perm.owner_id != g_current_user_id) {
        printf("Permission denied.\n"); return;
    }
    
    ent.perm.mode = mode;
    save_file_entry(g_open_file_offset, &ent);
    printf("Permissions updated to %u\n", mode);
}

int get_uid_by_name(const char *name, superblock_t *sb) {
    for (uint32_t i = 0; i < sb->user_count; i++) {
        if (strcmp(sb->user_table[i].name, name) == 0) return i;
    }
    return -1;
}

int get_gid_by_name(const char *name, superblock_t *sb) {
    for (uint32_t i = 0; i < sb->group_count; i++) {
        if (strcmp(sb->group_table[i].name, name) == 0) return i;
    }
    return -1;
}

void chown_file(const char *user_name, const char *group_name) {
    if (g_open_file_offset == 0) { printf("No file open.\n"); return; }
    
    superblock_t sb;
    load_superblock(&sb);
    
    if (g_current_user_id != 0) { printf("Only root can chown.\n"); return; }

    int uid = get_uid_by_name(user_name, &sb);
    int gid = get_gid_by_name(group_name, &sb);

    if (uid == -1 || gid == -1) { printf("User or Group not found.\n"); return; }

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);
    ent.perm.owner_id = (uint32_t)uid;
    ent.perm.group_id = (uint32_t)gid;
    save_file_entry(g_open_file_offset, &ent);
    printf("Ownership updated to %s:%s\n", user_name, group_name);
}

void getfacl_file() {
    if (g_open_file_offset == 0) {
        printf("Please open the file first.\n");
        return;
    }

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);

    superblock_t sb;
    load_superblock(&sb);

    int u = (ent.perm.mode / 100) % 10;
    int g = (ent.perm.mode / 10) % 10;
    int o = ent.perm.mode % 10;

    printf("\n# file: %s\n", ent.name);
    printf("owner: %s (id: %u)\n",
           sb.user_table[ent.perm.owner_id].name,
           ent.perm.owner_id);
    printf("group: %s (id: %u)\n",
           sb.group_table[ent.perm.group_id].name,
           ent.perm.group_id);

    printf("user: %c%c%c\n",
        (u & 4) ? 'r' : '-',
        (u & 2) ? 'w' : '-',
        (u & 1) ? 'x' : '-');

    printf("group: %c%c%c\n",
        (g & 4) ? 'r' : '-',
        (g & 2) ? 'w' : '-',
        (g & 1) ? 'x' : '-');

    printf("other: %c%c%c\n",
        (o & 4) ? 'r' : '-',
        (o & 2) ? 'w' : '-',
        (o & 1) ? 'x' : '-');
}

bool login_user(const char *username) {
    superblock_t sb;
    if (!load_superblock(&sb)) return false;

    for (uint32_t i = 0; i < sb.user_count; i++) {
        if (strcmp(sb.user_table[i].name, username) == 0) {
            g_current_user_id = sb.user_table[i].id;
            return true;
        }
    }
    return false;
}

void chgrp_file(const char *group_name) {
    if (g_open_file_offset == 0) return;
    
    superblock_t sb;
    load_superblock(&sb);
    
    int gid = get_gid_by_name(group_name, &sb);
    if (gid == -1) { printf("Group not found.\n"); return; }

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);
    
    // Only root or the owner can change the group
    if (g_current_user_id != 0 && ent.perm.owner_id != g_current_user_id) {
        printf("Permission denied.\n");
        return;
    }

    ent.perm.group_id = (uint32_t)gid;
    save_file_entry(g_open_file_offset, &ent);
    printf("Group changed to %s\n", group_name);
}

uint32_t get_primary_gid(uint32_t uid) {
    superblock_t sb;
    load_superblock(&sb);

    for (uint32_t i = 0; i < sb.user_count; i++) {
        if (sb.user_table[i].id == uid) {
            return sb.user_table[i].group_count
                   ? sb.user_table[i].group_ids[0]
                   : 0;
        }
    }
    return 0;
}

void sync_users_from_disk() {
    superblock_t sb;
    load_superblock(&sb);

    memcpy(g_users, sb.user_table, sizeof(sb.user_table));
    memcpy(g_groups, sb.group_table, sizeof(sb.group_table));

    g_user_count = sb.user_count;
    g_group_count = sb.group_count;
}

void user_del(const char *username) {
    if (g_current_user_id != 0) {
        printf("Permission denied: only root can delete users.\n");
        return;
    }

    if (strcmp(username, "root") == 0) {
        printf("Error: cannot delete root user.\n");
        return;
    }

    superblock_t sb;
    load_superblock(&sb);

    int uid = get_uid_by_name(username, &sb);
    if (uid == -1) {
        printf("User not found.\n");
        return;
    }

    /* ---- Fix file ownerships ---- */
    uint32_t cur = sb.file_list_head;
    file_entry_t ent;

    while (cur != 0) {
        load_file_entry(cur, &ent);
        if (ent.perm.owner_id == (uint32_t)uid) {
            ent.perm.owner_id = 0; // root
            ent.perm.group_id = 0;
            save_file_entry(cur, &ent);
        }
        cur = ent.next;
    }

    /* ---- Remove user by shifting table ---- */
    for (uint32_t i = uid; i < sb.user_count - 1; i++) {
        sb.user_table[i] = sb.user_table[i + 1];
    }

    sb.user_count--;

    save_superblock(&sb);
    sync_users_from_disk();

    printf("User '%s' deleted.\n", username);
}

void group_add(const char *groupname) {
    if (g_current_user_id != 0) {
        printf("Access denied: only root can add groups.\n");
        return;
    }

    superblock_t sb;
    load_superblock(&sb);

    if (get_gid_by_name(groupname, &sb) != -1) {
        printf("Group already exists.\n");
        return;
    }

    if (sb.group_count >= MAX_GROUPS) {
        printf("Error: max groups reached.\n");
        return;
    }

    sb.group_table[sb.group_count].id = sb.group_count;
    strncpy(sb.group_table[sb.group_count].name, groupname, MAX_NAME);
    sb.group_count++;

    save_superblock(&sb);
    sync_users_from_disk();

    printf("Group '%s' added.\n", groupname);
}

void group_del(const char *groupname) {
    if (g_current_user_id != 0) {
        printf("Permission denied: only root can delete groups.\n");
        return;
    }

    if (strcmp(groupname, "root") == 0) {
        printf("Error: cannot delete root group.\n");
        return;
    }

    superblock_t sb;
    load_superblock(&sb);

    int gid = get_gid_by_name(groupname, &sb);
    if (gid == -1) {
        printf("Group not found.\n");
        return;
    }

    /* ---- Fix files using this group ---- */
    uint32_t cur = sb.file_list_head;
    file_entry_t ent;

    while (cur != 0) {
        load_file_entry(cur, &ent);

        if (ent.perm.group_id == (uint32_t)gid) {
            ent.perm.group_id = 0;  // root group
        } else if (ent.perm.group_id > (uint32_t)gid) {
            ent.perm.group_id--;    // shift down
        }

        save_file_entry(cur, &ent);
        cur = ent.next;
    }

    /* ---- Remove group from all users ---- */
    for (uint32_t u = 0; u < sb.user_count; u++) {
        user_t *usr = &sb.user_table[u];

        for (uint32_t g = 0; g < usr->group_count; ) {
            if (usr->group_ids[g] == (uint32_t)gid) {
                for (uint32_t k = g; k < usr->group_count - 1; k++)
                    usr->group_ids[k] = usr->group_ids[k + 1];
                usr->group_count--;
            } else {
                g++;
            }
        }
    }

    /* ---- Remove group from table ---- */
    for (uint32_t i = gid; i < sb.group_count - 1; i++) {
        sb.group_table[i] = sb.group_table[i + 1];
    }
    sb.group_count--;

    /* ---- Fix group_ids in users after shifting groups ---- */
    for (uint32_t u = 0; u < sb.user_count; u++) {
        user_t *usr = &sb.user_table[u];
        for (uint32_t g = 0; g < usr->group_count; g++) {
            if (usr->group_ids[g] > (uint32_t)gid) {
                usr->group_ids[g]--;
            }
        }
    }

    save_superblock(&sb);
    sync_users_from_disk();

    printf("Group '%s' deleted.\n", groupname);
}

void list_users_and_groups() {
    superblock_t sb;
    if (!load_superblock(&sb)) {
        printf("Error: cannot load superblock.\n");
        return;
    }

    printf("\nUsers:\n");

    for (uint32_t i = 0; i < sb.user_count; i++) {
        user_t *u = &sb.user_table[i];

        printf("  %s (uid=%u): ", u->name, u->id);

        if (u->group_count == 0) {
            printf("(no groups)");
        } else {
            for (uint32_t g = 0; g < u->group_count; g++) {
                uint32_t gid = u->group_ids[g];
                if (gid < sb.group_count) {
                    printf("%s", sb.group_table[gid].name);
                } else {
                    printf("unknown");
                }

                if (g + 1 < u->group_count)
                    printf(", ");
            }
        }
        printf("\n");
    }

    printf("\nGroups:\n");

    for (uint32_t i = 0; i < sb.group_count; i++) {
        printf("  %s (gid=%u)\n",
               sb.group_table[i].name,
               sb.group_table[i].id);
    }

    printf("\n");
}
