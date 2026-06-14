#include "fs.h"
// HW8
static void fs_lock(void);
static void fs_unlock(void);

// -------------------------- Initialize filesystem --------------------------
FILE *g_fs_file = NULL;
char g_fs_path[256] = {0};

// no file opened initially
uint32_t g_open_file_offset = 0;

// Helper to manage bits
static void set_bit(uint8_t *bitmap, int index) {
    bitmap[index / 8] |= (1 << (index % 8));
}

static void clear_bit(uint8_t *bitmap, int index) {
    bitmap[index / 8] &= ~(1 << (index % 8));
}

static bool get_bit(const uint8_t *bitmap, int index) {
    return (bitmap[index / 8] & (1 << (index % 8))) != 0;
}

bool load_superblock(superblock_t *sb) {
    fseek(g_fs_file, SUPERBLOCK_OFFSET, SEEK_SET);
    return fread(sb, 1, sizeof(superblock_t), g_fs_file) == sizeof(superblock_t);
}

bool save_superblock(const superblock_t *sb) {
    fseek(g_fs_file, SUPERBLOCK_OFFSET, SEEK_SET);
    size_t n = fwrite(sb, 1, sizeof(superblock_t), g_fs_file);
    fflush(g_fs_file);
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
    sb.file_count = 0;

    // Root user init
    sb.group_count = 1;
    sb.group_table[0].id = 0;
    strcpy(sb.group_table[0].name, "root");

    sb.user_count = 1;
    sb.user_table[0].id = 0;
    strcpy(sb.user_table[0].name, "root");
    sb.user_table[0].group_ids[0] = 0;
    sb.user_table[0].group_count = 1;

    if (!save_superblock(&sb)) return false;

    // 2. Init Bitmap (Block 1)
    // 32768 bits = 4096 bytes
    uint8_t bitmap[BLOCK_SIZE] = {0};
    
    // Mark Block 0 (Superblock) and Block 1 (Bitmap) as used
    set_bit(bitmap, 0);
    set_bit(bitmap, 1);

    fseek(g_fs_file, BITMAP_BLOCK_OFFSET, SEEK_SET);
    if (fwrite(bitmap, 1, BLOCK_SIZE, g_fs_file) != BLOCK_SIZE) return false;

    // 3. Set file size
    if (fseek(g_fs_file, FS_TOTAL_SIZE - 1, SEEK_SET) != 0) return false;
    if (fwrite("\0", 1, 1, g_fs_file) != 1) return false;
    fflush(g_fs_file);

    sync_users_from_disk();
    printf("Created new filesystem '%s' (128MB, Bitmap Allocator)\n", path);
    return true;
}

bool open_fs(const char *path) {
    g_fs_file = fopen(path, "r+b");
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
        printf("Error: invalid magic number.\n");
        return false;
    }
    if (sb.version != FS_VERSION) {
        printf("Error: version mismatch. Expected %d, got %d.\n", FS_VERSION, sb.version);
        return false;
    }

    printf("Loaded existing filesystem %s\n", path);
    sync_users_from_disk();
    return true;
}

bool init_fs(const char *path) {
    // Check if file exists
    struct stat st;
    bool exists = (stat(path, &st) == 0);

    bool result;
    if (!exists)
        result = create_fs(path);
    else    // File already exists -> open and verify
        result = open_fs(path);

    if (!result) {
        printf("failed to initialize the fs!\n");
        return false;
    }
    // path here is basically the name of the filesystem
    strncpy(g_fs_path, path, sizeof(g_fs_path));
    return true;
}

// -------------------------- Allocator (Bitmap) --------------------------
uint32_t fs_alloc() {
    uint8_t bitmap[BLOCK_SIZE];
    
    fseek(g_fs_file, BITMAP_BLOCK_OFFSET, SEEK_SET);
    if (fread(bitmap, 1, BLOCK_SIZE, g_fs_file) != BLOCK_SIZE) return 0;

    // Find first free bit starting from 2 (skip SB and Bitmap)
    for (int i = 2; i < TOTAL_BLOCKS; i++) {
        if (!get_bit(bitmap, i)) {
            // Found free block
            set_bit(bitmap, i);
            
            // Save updated bitmap
            fseek(g_fs_file, BITMAP_BLOCK_OFFSET, SEEK_SET);
            fwrite(bitmap, 1, BLOCK_SIZE, g_fs_file);
            fflush(g_fs_file);

            // Zero out the newly allocated block
            uint32_t offset = i * BLOCK_SIZE;
            uint8_t zeros[BLOCK_SIZE] = {0};
            fseek(g_fs_file, offset, SEEK_SET);
            fwrite(zeros, 1, BLOCK_SIZE, g_fs_file);
            
            return offset;
        }
    }

    printf("Error: Filesystem full!\n");
    return 0;
}

void fs_free(uint32_t offset) {
    if (offset == 0) return;
    
    uint32_t block_idx = offset / BLOCK_SIZE;
    if (block_idx < 2 || block_idx >= TOTAL_BLOCKS) return;

    uint8_t bitmap[BLOCK_SIZE];
    fseek(g_fs_file, BITMAP_BLOCK_OFFSET, SEEK_SET);
    fread(bitmap, 1, BLOCK_SIZE, g_fs_file);

    clear_bit(bitmap, block_idx);

    fseek(g_fs_file, BITMAP_BLOCK_OFFSET, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, g_fs_file);
    fflush(g_fs_file);
}

void visualize_bitmap() {
    uint8_t bitmap[BLOCK_SIZE];
    fseek(g_fs_file, BITMAP_BLOCK_OFFSET, SEEK_SET);
    fread(bitmap, 1, BLOCK_SIZE, g_fs_file);

    int used = 0;
    for(int i=0; i<TOTAL_BLOCKS; i++) {
        if(get_bit(bitmap, i)) used++;
    }

    printf("\n----- Bitmap Visualization -----\n");
    printf("Total Blocks: %d\n", TOTAL_BLOCKS);
    printf("Used Blocks:  %d\n", used);
    printf("Free Blocks:  %d\n", TOTAL_BLOCKS - used);
    printf("Block Size:   %d bytes\n", BLOCK_SIZE);
    
    printf("Map (first 64 blocks): ");
    for(int i=0; i<64; i++) {
        printf("%c", get_bit(bitmap, i) ? '1' : '0');
    }
    printf("...\n--------------------------------\n");
}

// -------------------------- File Entry Ops --------------------------
bool load_file_entry(uint32_t offset, file_entry_t *ent) {
    fseek(g_fs_file, offset, SEEK_SET);
    return fread(ent, 1, sizeof(file_entry_t), g_fs_file) == sizeof(file_entry_t);
}

void save_file_entry(uint32_t offset, const file_entry_t *ent) {
    fseek(g_fs_file, offset, SEEK_SET);
    fwrite(ent, 1, sizeof(file_entry_t), g_fs_file);
    fflush(g_fs_file);
}

uint32_t find_file(const char *name, file_entry_t *out) {
    if (!g_fs_file) return 0;
    // Load Superblock to get the total file count
    superblock_t sb;
    if (!load_superblock(&sb))
        return 0;
    if (sb.file_count == 0)
        return 0;

    // Load Bitmap to determine which blocks are "alive"
    uint8_t bitmap[BLOCK_SIZE];
    fseek(g_fs_file, BITMAP_BLOCK_OFFSET, SEEK_SET);
    if (fread(bitmap, 1, BLOCK_SIZE, g_fs_file) != BLOCK_SIZE)
        return 0;

    uint32_t checked_files = 0;
    // Iterate over blocks starting from 2 (Block 0=SB, Block 1=Bitmap)
    for (int i = 2; i < TOTAL_BLOCKS; i++) {
        if (checked_files >= sb.file_count)
            break;

        if (get_bit(bitmap, i)) {
            checked_files++;

            uint32_t offset = i * BLOCK_SIZE;
            file_entry_t ent;
            load_file_entry(offset, &ent);

            if (strcmp(ent.name, name) == 0) {
                if (out) *out = ent;
                return offset;
            }
        }
    }
    return 0;   // File not found
}

uint32_t create_file(const char *name) {
    fs_lock();   // 🔒 START

    if (strlen(name) >= FILENAME_MAX_LEN) {
        printf("Error: filename too long.\n");
        goto fail;}
    if (find_file(name, NULL) != 0) {
        printf("Error: file already exists.\n");
        goto fail;}

    uint32_t off = fs_alloc();
    if (off == 0) {
        printf("Error: failed to allocate memory.\n");
        goto fail;}

    file_entry_t ent = {0};
    strcpy(ent.name, name);
    ent.perm = (permission_t){
        .owner_id = g_current_user_id,
        .group_id = get_primary_gid(g_current_user_id),
        .mode = 644};
    ent.size = 0;
    save_file_entry(off, &ent);

    superblock_t sb;
    load_superblock(&sb);
    sleep(10);       // wait for 10 seconds (HW8)
    sb.file_count++;
    save_superblock(&sb);

    fs_unlock();           // 🔓 END
    return off;

    fail:
        fs_unlock();
        return 0;
}


// -------------------------- IO Operations --------------------------
void write_file(uint32_t pos, uint32_t nbytes) {
    if (g_open_file_offset == 0) { printf("No file open.\n"); return; }

    fs_lock();   // 🔒

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);

    if (!check_permission(&ent.perm, 2)) {
        fs_unlock(); printf("Permission denied.\n"); return;
    }

    // Constraint: File cannot exceed 1 block (4096 bytes)
    if (pos + nbytes > BLOCK_SIZE - sizeof(file_entry_t)) {
        printf("Error: Write exceeds block size limit (4KB).\n");
        return;
    }

    // Read input
    char *buffer = malloc(nbytes);
    if (buffer) {
        // Simple stdin read
        printf("Enter data (%u bytes): ", nbytes);
        fread(buffer, 1, nbytes, stdin);
        
        // Consume newline
        int c; while ((c = getchar()) != '\n' && c != EOF);

        fseek(g_fs_file, g_open_file_offset + sizeof(file_entry_t) + pos, SEEK_SET);
        fwrite(buffer, 1, nbytes, g_fs_file);
        free(buffer);

        if (pos + nbytes > ent.size) {
            ent.size = pos + nbytes;
            save_file_entry(g_open_file_offset, &ent);
        }
        printf("Wrote %u bytes.\n", nbytes);
    }
    fs_unlock();           // 🔓
}

void read_file(uint32_t pos, uint32_t nbytes) {
    if (g_open_file_offset == 0) { printf("No file open.\n"); return; }

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);

    // 4 is the bit for READ
    if (!check_permission(&ent.perm, 4)) {
        printf("Permission denied.\n"); return;
    }

    if (ent.size == 0) {
        printf("File is empty.\n");
        return;
    }

    if (pos >= ent.size) return;
    if (pos + nbytes > ent.size) nbytes = ent.size - pos;

    char *buffer = malloc(nbytes);
    fseek(g_fs_file, g_open_file_offset + sizeof(file_entry_t) + pos, SEEK_SET);
    fread(buffer, 1, nbytes, g_fs_file);
    fwrite(buffer, 1, nbytes, stdout);
    printf("\n");
    free(buffer);
}

void shrink_file(uint32_t newsize) {
    if (g_open_file_offset == 0) return;
    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);

    if (newsize >= ent.size) return;
    ent.size = newsize;
        
    save_file_entry(g_open_file_offset, &ent);
    // printf("File shrunk.\n");
}

void remove_file() {
    if (g_open_file_offset == 0) return;

    fs_lock();   // 🔒
    
    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);

    if (!check_permission(&ent.perm, 2)) {
        fs_unlock(); printf("Permission denied.\n"); return;
    }

    // FreeBlock
    fs_free(g_open_file_offset);

    g_open_file_offset = 0;

    // file_count--;
    superblock_t sb;
    load_superblock(&sb);
    sb.file_count--;
    save_superblock(&sb);
    printf("File removed.\n");
    
    fs_unlock();           // 🔓
}

void file_stats() {
    if (g_open_file_offset == 0) { printf("No file open.\n"); return; }
    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);
    printf("File name: %s\nSize: %u bytes\nLocated at Block: %u\n", 
            ent.name, ent.size, g_open_file_offset / BLOCK_SIZE);
}

void fs_stats() {
    if (g_fs_file == NULL) {
        printf("Please first initialize a fs!\n");
        return;
    }
    superblock_t sb;
    if (!load_superblock(&sb)) {
        printf("Error: cannot load superblock.\n");
        return;
    }
    int used = sb.file_count * BLOCK_SIZE;
    int free = FS_TOTAL_SIZE - used;
    printf("\n------ Filesystem statistics: ------\n");
    printf("  Total size: %llu bytes (%.2f MB)\n",
        (unsigned long long)FS_TOTAL_SIZE,
        FS_TOTAL_SIZE / (1024.0 * 1024.0));
    printf("  Block size: %u bytes (%.2f KB)\n", BLOCK_SIZE, BLOCK_SIZE / 1024.0);
    printf("  File count: %u\n", sb.file_count);
    printf("  Used space: %llu bytes (%.2f KB)\n",
        (unsigned long long)used, used / 1024.0);
    printf("  Free space: %llu bytes (%.2f MB)\n",
        (unsigned long long)free,
            free / (1024.0 * 1024.0));
    printf("------------------------------------\n\n");
}

void close_file() {
    if (g_open_file_offset == 0) {
        printf("No file is open.\n");
        return;
    }
    g_open_file_offset = 0;
    printf("File closed.\n");
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
    // uint32_t cur = sb.file_list_head;
    // file_entry_t ent;

    // while (cur != 0) {
    //     load_file_entry(cur, &ent);
    //     if (ent.perm.owner_id == (uint32_t)uid) {
    //         ent.perm.owner_id = 0; // root
    //         ent.perm.group_id = 0;
    //         save_file_entry(cur, &ent);
    //     }
    //     cur = ent.next;
    // }

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
    // uint32_t cur = sb.file_list_head;
    // file_entry_t ent;

    // while (cur != 0) {
    //     load_file_entry(cur, &ent);

    //     if (ent.perm.group_id == (uint32_t)gid) {
    //         ent.perm.group_id = 0;  // root group
    //     } else if (ent.perm.group_id > (uint32_t)gid) {
    //         ent.perm.group_id--;    // shift down
    //     }

    //     save_file_entry(cur, &ent);
    //     cur = ent.next;
    // }

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

// -------------------------- HW7 --------------------------
// -------- stress test and performance evaluation ---------
void write_file_random(uint32_t pos, uint32_t nbytes) {
    if (g_open_file_offset == 0) { printf("No file open.\n"); return; }

    file_entry_t ent;
    load_file_entry(g_open_file_offset, &ent);

    // Constraint: File cannot exceed 1 block (4096 bytes)
    if (pos + nbytes > BLOCK_SIZE - sizeof(file_entry_t)) {
        printf("Error: Write exceeds block size limit (4KB).\n");
        return;
    }

    // Write data
    char *buffer = malloc(nbytes);
    if (!buffer) {
        perror("failed to allocate buffer!");
        return;
    }
    for (uint32_t i = 0; i < nbytes; i++) {
        buffer[i] = 'A' + (rand() % 26);
    }

    fseek(g_fs_file, g_open_file_offset + sizeof(file_entry_t) + pos, SEEK_SET);
    fwrite(buffer, 1, nbytes, g_fs_file);
    free(buffer);

    if (pos + nbytes > ent.size) {
        ent.size = pos + nbytes;
        save_file_entry(g_open_file_offset, &ent);
    }
    // printf("Wrote %u bytes.\n", nbytes);
}

void stress_test() {
    printf("--- Starting Stress Test (Linked List Baseline) ---\n");

    // create new fs (fully destroys any existing fs with the same name)
    if (!create_fs("stress_test.bin")) {
        printf("Error: Could not create test file.\n");
        return;
    }

    const int num_files = 2000;
    const int num_operations = 10000;
    char filenames[num_files][16];
    
    // create initial files
    for (int i = 0; i < num_files; i++) {
        snprintf(filenames[i], sizeof(filenames[i]), "file_%d", i);
        create_file(filenames[i]);
    }
    
    // start timers (Linux / WSL2)
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    srand(time(NULL));

    for (int i = 0; i < num_operations; i++) {
        // if (i % 1000 == 0)
		//     printf("iter: %d\n", i);

        int target = rand() % num_files;

        file_entry_t ent;
        uint32_t offset = find_file(filenames[target], &ent);
        if (offset == 0) {
            printf("couldn't load file %s!\n", filenames[target]);
            return;
        }
        g_open_file_offset = offset;

        // printf("write...\n");
        write_file_random(0, 10);
        // printf("read...\n");
        read_file(0, 100);
        // printf("shrink...\n");
        shrink_file(ent.size / 2);
        // printf("remove...\n");
        remove_file();
        // printf("recreate...\n");
        create_file(filenames[target]);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double duration =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\nStress Test Finished in: %.4f seconds\n", duration);
}

// -------------------------- HW8 --------------------------
// ------------------------ locking ------------------------
static void fs_lock(void) {
    struct flock fl = {0};
    fl.l_type   = F_WRLCK;      // What kind of lock
    fl.l_whence = SEEK_SET;     // write lock
    fl.l_start  = 0;            // Offset in file
    fl.l_len    = 0;            // Length (0 = entire file)
    // F_SETLKW: Wait until lock becomes available
    fcntl(fileno(g_fs_file), F_SETLKW, &fl);
}

static void fs_unlock(void) {
    struct flock fl = {0};
    fl.l_type   = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;
    fcntl(fileno(g_fs_file), F_SETLK, &fl);
}
