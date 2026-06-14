// helper: compute block file offset
off_t block_offset(uint32_t block_index, off_t blocks_base) {
    return blocks_base + (off_t)(block_index - 1) * BLOCK_SIZE;
}

// allocate block (try free list else increment last_block)
uint32_t allocate_block(FILE *db, superblock_t *sb, off_t blocks_base) {
    if (sb->free_block_head != 0) {
        uint32_t b = sb->free_block_head;
        // read the block to get next in free-list
        data_block_t blk;
        fseek(db, block_offset(b, blocks_base), SEEK_SET);
        fread(&blk, sizeof(blk), 1, db);
        sb->free_block_head = blk.next_block; // pop
        // zero the block and set next_block = 0
        blk.next_block = 0;
        memset(blk.data, 0, sizeof(blk.data));
        fseek(db, block_offset(b, blocks_base), SEEK_SET);
        fwrite(&blk, sizeof(blk), 1, db);
        return b;
    } else {
        sb->last_block++;
        uint32_t b = sb->last_block;
        data_block_t blk;
        blk.next_block = 0;
        memset(blk.data, 0, sizeof(blk.data));
        fseek(db, block_offset(b, blocks_base), SEEK_SET);
        fwrite(&blk, sizeof(blk), 1, db);
        return b;
    }
}

// free chain of blocks: add each to free list
void free_block_chain(FILE *db, superblock_t *sb, uint32_t head, off_t blocks_base) {
    uint32_t cur = head;
    while (cur != 0) {
        data_block_t blk;
        fseek(db, block_offset(cur, blocks_base), SEEK_SET);
        fread(&blk, sizeof(blk), 1, db);
        uint32_t next = blk.next_block;
        // push cur onto free list
        blk.next_block = sb->free_block_head;
        fseek(db, block_offset(cur, blocks_base), SEEK_SET);
        fwrite(&blk, sizeof(blk), 1, db);
        sb->free_block_head = cur;
        cur = next;
    }
}

// read bytes from file's block chain starting at position pos
size_t read_from_file(FILE *db, file_entry_on_disk_t *entry, superblock_t *sb, off_t blocks_base,
                      uint64_t pos, size_t nbytes, uint8_t *out) {
    if (pos >= entry->size) return 0;
    size_t toread = (size_t) ((pos + nbytes > entry->size) ? (entry->size - pos) : nbytes);
    uint32_t block_idx = entry->head_block;
    size_t payload = BLOCK_SIZE - 4;
    uint64_t cur_pos = 0;
    // walk to block that contains 'pos'
    while (block_idx != 0 && cur_pos + payload <= pos) {
        // read block.next_block
        data_block_t blk;
        fseek(db, block_offset(block_idx, blocks_base), SEEK_SET);
        fread(&blk, sizeof(blk), 1, db);
        block_idx = blk.next_block;
        cur_pos += payload;
    }
    size_t read_total = 0;
    uint64_t offset_within_block = pos - cur_pos;
    while (toread > 0 && block_idx != 0) {
        data_block_t blk;
        fseek(db, block_offset(block_idx, blocks_base), SEEK_SET);
        fread(&blk, sizeof(blk), 1, db);
        size_t canread = payload - offset_within_block;
        if (canread > toread) canread = toread;
        memcpy(out + read_total, blk.data + offset_within_block, canread);
        read_total += canread;
        toread -= canread;
        offset_within_block = 0;
        block_idx = blk.next_block;
    }
    return read_total;
}
