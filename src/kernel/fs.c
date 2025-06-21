// AtomicOS Simple Filesystem Implementation
// Deterministic filesystem with static allocation and bounded operations

#include "fs.h"

// Filesystem state - all statically allocated
static fs_superblock_t superblock;
static fs_inode_t inode_table[FS_MAX_FILES];
static uint8_t block_bitmap[FS_MAX_BLOCKS / 8];  // 1 bit per block
static uint8_t inode_bitmap[FS_MAX_FILES / 8];   // 1 bit per inode
static fs_file_t open_files[16];                 // Max 16 open files
static uint8_t data_blocks[FS_MAX_BLOCKS][FS_BLOCK_SIZE]; // Simulated disk

// Helper functions
static int allocate_block(void);
static void free_block(uint32_t block);
static int allocate_inode(void);
static void free_inode(uint32_t inode);
static int find_free_fd(void);

// Initialize filesystem
void fs_init(void) {
    // Clear all structures
    for (uint32_t i = 0; i < sizeof(superblock); i++) {
        ((uint8_t*)&superblock)[i] = 0;
    }
    
    for (int i = 0; i < FS_MAX_FILES; i++) {
        for (uint32_t j = 0; j < sizeof(fs_inode_t); j++) {
            ((uint8_t*)&inode_table[i])[j] = 0;
        }
    }
    
    for (uint32_t i = 0; i < sizeof(block_bitmap); i++) {
        block_bitmap[i] = 0;
    }
    
    for (uint32_t i = 0; i < sizeof(inode_bitmap); i++) {
        inode_bitmap[i] = 0;
    }
    
    for (int i = 0; i < 16; i++) {
        open_files[i].inode_num = 0;
        open_files[i].flags = 0;
    }
}

// Format filesystem
int fs_format(void) {
    fs_init();
    
    // Initialize superblock
    superblock.magic = FS_MAGIC;
    superblock.version = 1;
    superblock.total_blocks = FS_MAX_BLOCKS;
    superblock.free_blocks = FS_MAX_BLOCKS - 1; // Reserve block 0 for superblock
    superblock.total_inodes = FS_MAX_FILES;
    superblock.free_inodes = FS_MAX_FILES - 1;  // Reserve inode 0 for root
    superblock.root_inode = 0;
    superblock.block_size = FS_BLOCK_SIZE;
    
    // Mark block 0 as used (superblock)
    block_bitmap[0] |= 0x01;
    
    // Create root directory
    inode_bitmap[0] |= 0x01;  // Mark inode 0 as used
    inode_table[0].inode_num = 0;
    inode_table[0].type = FS_TYPE_DIRECTORY;
    inode_table[0].permissions = FS_PERM_READ | FS_PERM_WRITE | FS_PERM_EXEC;
    inode_table[0].size = 0;
    inode_table[0].link_count = 1;
    inode_table[0].owner_pid = 0;
    
    return 0;
}

// Mount filesystem
int fs_mount(void) {
    // In a real implementation, this would read from disk
    // For now, just verify the superblock
    if (superblock.magic != FS_MAGIC) {
        return FS_ERR_INVALID;
    }
    return 0;
}

// Unmount filesystem
int fs_unmount(void) {
    // Close all open files
    for (int i = 0; i < 16; i++) {
        if (open_files[i].flags != 0) {
            fs_close(i);
        }
    }
    return 0;
}

// Allocate a free block
static int allocate_block(void) {
    for (int i = 0; i < FS_MAX_BLOCKS; i++) {
        int byte = i / 8;
        int bit = i % 8;
        
        if (!(block_bitmap[byte] & (1 << bit))) {
            block_bitmap[byte] |= (1 << bit);
            superblock.free_blocks--;
            return i;
        }
    }
    return -1; // No free blocks
}

// Free a block
static void free_block(uint32_t block) {
    if (block < FS_MAX_BLOCKS) {
        int byte = block / 8;
        int bit = block % 8;
        
        if (block_bitmap[byte] & (1 << bit)) {
            block_bitmap[byte] &= ~(1 << bit);
            superblock.free_blocks++;
        }
    }
}

// Allocate a free inode
static int allocate_inode(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        int byte = i / 8;
        int bit = i % 8;
        
        if (!(inode_bitmap[byte] & (1 << bit))) {
            inode_bitmap[byte] |= (1 << bit);
            superblock.free_inodes--;
            return i;
        }
    }
    return -1; // No free inodes
}

// Free an inode
static void free_inode(uint32_t inode) {
    if (inode < FS_MAX_FILES) {
        int byte = inode / 8;
        int bit = inode % 8;
        
        if (inode_bitmap[byte] & (1 << bit)) {
            inode_bitmap[byte] &= ~(1 << bit);
            superblock.free_inodes++;
        }
    }
}

// Find a free file descriptor
static int find_free_fd(void) {
    for (int i = 0; i < 16; i++) {
        if (open_files[i].flags == 0) {
            return i;
        }
    }
    return -1;
}

// Simple string comparison
static int strcmp_simple(const char* s1, const char* s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

// Simple string copy
static void strcpy_simple(char* dst, const char* src) {
    while (*src) {
        *dst++ = *src++;
    }
    *dst = '\0';
}

// Convert path to inode number
int fs_path_to_inode(const char* path) {
    // For now, only support root directory and direct children
    if (path[0] == '/' && path[1] == '\0') {
        return 0; // Root directory
    }
    
    // Skip leading slash
    if (path[0] == '/') {
        path++;
    }
    
    // Search in root directory
    fs_inode_t* root = &inode_table[0];
    if (root->type != FS_TYPE_DIRECTORY) {
        return FS_ERR_INVALID;
    }
    
    // Read directory entries from root's blocks
    for (int i = 0; i < 12 && root->blocks[i] != 0; i++) {
        fs_dirent_t* entries = (fs_dirent_t*)data_blocks[root->blocks[i]];
        int entries_per_block = FS_BLOCK_SIZE / sizeof(fs_dirent_t);
        
        for (int j = 0; j < entries_per_block; j++) {
            if (entries[j].inode_num != 0 && 
                strcmp_simple(entries[j].name, path) == 0) {
                return entries[j].inode_num;
            }
        }
    }
    
    return FS_ERR_NOT_FOUND;
}

// Create a file or directory
int fs_create(const char* path, uint32_t type, uint32_t permissions) {
    // Allocate inode
    int inode_num = allocate_inode();
    if (inode_num < 0) {
        return FS_ERR_NO_SPACE;
    }
    
    // Initialize inode
    fs_inode_t* inode = &inode_table[inode_num];
    inode->inode_num = inode_num;
    inode->type = type;
    inode->permissions = permissions;
    inode->size = 0;
    inode->link_count = 1;
    inode->owner_pid = 0; // Would get from current process
    
    // Clear block pointers
    for (int i = 0; i < 12; i++) {
        inode->blocks[i] = 0;
    }
    
    // Add directory entry to parent (root for now)
    fs_inode_t* parent = &inode_table[0];
    
    // Find space in parent directory
    for (int i = 0; i < 12; i++) {
        if (parent->blocks[i] == 0) {
            // Allocate new block for directory
            int block = allocate_block();
            if (block < 0) {
                free_inode(inode_num);
                return FS_ERR_NO_SPACE;
            }
            parent->blocks[i] = block;
        }
        
        fs_dirent_t* entries = (fs_dirent_t*)data_blocks[parent->blocks[i]];
        int entries_per_block = FS_BLOCK_SIZE / sizeof(fs_dirent_t);
        
        for (int j = 0; j < entries_per_block; j++) {
            if (entries[j].inode_num == 0) {
                // Found free entry
                entries[j].inode_num = inode_num;
                
                // Extract filename from path
                const char* name = path;
                if (name[0] == '/') {
                    name++;
                }
                
                strcpy_simple(entries[j].name, name);
                parent->size += sizeof(fs_dirent_t);
                return inode_num;
            }
        }
    }
    
    // No space in directory
    free_inode(inode_num);
    return FS_ERR_NO_SPACE;
}

// Open a file
int fs_open(const char* path, uint32_t flags) {
    // Find the inode
    int inode_num = fs_path_to_inode(path);
    if (inode_num < 0) {
        if (flags & FS_OPEN_CREATE) {
            // Create the file
            inode_num = fs_create(path, FS_TYPE_FILE, 
                                 FS_PERM_READ | FS_PERM_WRITE);
            if (inode_num < 0) {
                return inode_num;
            }
        } else {
            return FS_ERR_NOT_FOUND;
        }
    }
    
    // Find free file descriptor
    int fd = find_free_fd();
    if (fd < 0) {
        return FS_ERR_TOO_MANY;
    }
    
    // Initialize file descriptor
    open_files[fd].inode_num = inode_num;
    open_files[fd].flags = flags;
    open_files[fd].offset = 0;
    open_files[fd].owner_pid = 0; // Would get from current process
    
    return fd;
}

// Close a file
int fs_close(int fd) {
    if (fd < 0 || fd >= 16 || open_files[fd].flags == 0) {
        return FS_ERR_INVALID;
    }
    
    open_files[fd].flags = 0;
    open_files[fd].inode_num = 0;
    return 0;
}

// Read from a file
int fs_read(int fd, void* buffer, uint32_t size) {
    if (fd < 0 || fd >= 16 || open_files[fd].flags == 0) {
        return FS_ERR_INVALID;
    }
    
    if (!(open_files[fd].flags & FS_OPEN_READ)) {
        return FS_ERR_PERM_DENIED;
    }
    
    fs_inode_t* inode = &inode_table[open_files[fd].inode_num];
    uint32_t offset = open_files[fd].offset;
    
    // Check if we're at EOF
    if (offset >= inode->size) {
        return 0;
    }
    
    // Limit read size
    if (offset + size > inode->size) {
        size = inode->size - offset;
    }
    
    uint32_t bytes_read = 0;
    uint8_t* buf = (uint8_t*)buffer;
    
    while (bytes_read < size) {
        uint32_t block_num = offset / FS_BLOCK_SIZE;
        uint32_t block_offset = offset % FS_BLOCK_SIZE;
        
        if (block_num >= 12 || inode->blocks[block_num] == 0) {
            break; // No more blocks
        }
        
        uint32_t to_read = FS_BLOCK_SIZE - block_offset;
        if (to_read > size - bytes_read) {
            to_read = size - bytes_read;
        }
        
        // Copy data from block
        uint8_t* block_data = data_blocks[inode->blocks[block_num]];
        for (uint32_t i = 0; i < to_read; i++) {
            buf[bytes_read + i] = block_data[block_offset + i];
        }
        
        bytes_read += to_read;
        offset += to_read;
    }
    
    open_files[fd].offset = offset;
    return bytes_read;
}

// Write to a file
int fs_write(int fd, const void* buffer, uint32_t size) {
    if (fd < 0 || fd >= 16 || open_files[fd].flags == 0) {
        return FS_ERR_INVALID;
    }
    
    if (!(open_files[fd].flags & FS_OPEN_WRITE)) {
        return FS_ERR_PERM_DENIED;
    }
    
    fs_inode_t* inode = &inode_table[open_files[fd].inode_num];
    uint32_t offset = open_files[fd].offset;
    
    // Check size limit
    if (offset + size > FS_MAX_FILE_SIZE) {
        size = FS_MAX_FILE_SIZE - offset;
    }
    
    uint32_t bytes_written = 0;
    const uint8_t* buf = (const uint8_t*)buffer;
    
    while (bytes_written < size) {
        uint32_t block_num = offset / FS_BLOCK_SIZE;
        uint32_t block_offset = offset % FS_BLOCK_SIZE;
        
        if (block_num >= 12) {
            break; // No indirect blocks supported
        }
        
        // Allocate block if needed
        if (inode->blocks[block_num] == 0) {
            int new_block = allocate_block();
            if (new_block < 0) {
                break; // No space
            }
            inode->blocks[block_num] = new_block;
        }
        
        uint32_t to_write = FS_BLOCK_SIZE - block_offset;
        if (to_write > size - bytes_written) {
            to_write = size - bytes_written;
        }
        
        // Copy data to block
        uint8_t* block_data = data_blocks[inode->blocks[block_num]];
        for (uint32_t i = 0; i < to_write; i++) {
            block_data[block_offset + i] = buf[bytes_written + i];
        }
        
        bytes_written += to_write;
        offset += to_write;
    }
    
    // Update file size if needed
    if (offset > inode->size) {
        inode->size = offset;
    }
    
    open_files[fd].offset = offset;
    return bytes_written;
}

// Delete a file
int fs_delete(const char* path) {
    // Find the inode
    int inode_num = fs_path_to_inode(path);
    if (inode_num < 0) {
        return inode_num;
    }
    
    if (inode_num == 0) {
        return FS_ERR_PERM_DENIED; // Cannot delete root
    }
    
    fs_inode_t* inode = &inode_table[inode_num];
    
    // Free all blocks
    for (int i = 0; i < 12 && inode->blocks[i] != 0; i++) {
        free_block(inode->blocks[i]);
    }
    
    // Free inode
    free_inode(inode_num);
    
    // Remove from parent directory (root)
    fs_inode_t* parent = &inode_table[0];
    
    // Extract filename
    const char* name = path;
    if (name[0] == '/') {
        name++;
    }
    
    // Find and remove directory entry
    for (int i = 0; i < 12 && parent->blocks[i] != 0; i++) {
        fs_dirent_t* entries = (fs_dirent_t*)data_blocks[parent->blocks[i]];
        int entries_per_block = FS_BLOCK_SIZE / sizeof(fs_dirent_t);
        
        for (int j = 0; j < entries_per_block; j++) {
            if (entries[j].inode_num == (uint32_t)inode_num) {
                entries[j].inode_num = 0;
                entries[j].name[0] = '\0';
                parent->size -= sizeof(fs_dirent_t);
                return 0;
            }
        }
    }
    
    return 0;
}

// Get file statistics
int fs_stat(const char* path, fs_inode_t* stat_buf) {
    int inode_num = fs_path_to_inode(path);
    if (inode_num < 0) {
        return inode_num;
    }
    
    // Copy inode data
    fs_inode_t* inode = &inode_table[inode_num];
    for (uint32_t i = 0; i < sizeof(fs_inode_t); i++) {
        ((uint8_t*)stat_buf)[i] = ((uint8_t*)inode)[i];
    }
    
    return 0;
}

// Get filesystem statistics
int fs_get_stats(fs_stats_t* stats) {
    if (!stats) {
        return FS_ERR_INVALID;
    }
    
    stats->total_blocks = superblock.total_blocks;
    stats->free_blocks = superblock.free_blocks;
    stats->total_files = superblock.total_inodes - superblock.free_inodes;
    
    // Count open files
    stats->open_files = 0;
    for (int i = 0; i < 16; i++) {
        if (open_files[i].flags != 0) {
            stats->open_files++;
        }
    }
    
    stats->block_size = FS_BLOCK_SIZE;
    return 0;
}