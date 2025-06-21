// AtomicOS Simple Filesystem Header
// Deterministic filesystem with fixed-size blocks and static allocation

#ifndef _FS_H
#define _FS_H

// Basic type definitions
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef int int32_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

// Filesystem constants
#define FS_BLOCK_SIZE       512         // Block size in bytes
#define FS_MAX_BLOCKS       1024        // Maximum blocks in filesystem
#define FS_MAX_FILES        64          // Maximum number of files
#define FS_MAX_NAME_LEN     32          // Maximum filename length
#define FS_MAX_FILE_SIZE    65536       // Maximum file size (128 blocks)
#define FS_MAGIC            0x41544F53  // "ATOS" magic number

// File types
#define FS_TYPE_FREE        0x00
#define FS_TYPE_FILE        0x01
#define FS_TYPE_DIRECTORY   0x02

// File permissions
#define FS_PERM_READ        0x04
#define FS_PERM_WRITE       0x02
#define FS_PERM_EXEC        0x01

// File operations
#define FS_OPEN_READ        0x01
#define FS_OPEN_WRITE       0x02
#define FS_OPEN_CREATE      0x04
#define FS_OPEN_APPEND      0x08

// Error codes
#define FS_ERR_NOT_FOUND    -1
#define FS_ERR_NO_SPACE     -2
#define FS_ERR_INVALID      -3
#define FS_ERR_PERM_DENIED  -4
#define FS_ERR_TOO_MANY     -5

// Superblock structure
typedef struct {
    uint32_t magic;             // Filesystem magic number
    uint32_t version;           // Filesystem version
    uint32_t total_blocks;      // Total blocks in filesystem
    uint32_t free_blocks;       // Number of free blocks
    uint32_t total_inodes;      // Total inodes
    uint32_t free_inodes;       // Number of free inodes
    uint32_t root_inode;        // Root directory inode
    uint32_t block_size;        // Block size
    uint8_t  reserved[480];     // Reserved for future use
} fs_superblock_t;

// Inode structure (64 bytes)
typedef struct {
    uint32_t inode_num;         // Inode number
    uint32_t type;              // File type
    uint32_t permissions;       // File permissions
    uint32_t size;              // File size in bytes
    uint32_t blocks[12];        // Direct block pointers
    uint32_t created_time;      // Creation timestamp
    uint32_t modified_time;     // Last modification timestamp
    uint32_t link_count;        // Number of hard links
    uint32_t owner_pid;         // Owner process ID
} fs_inode_t;

// Directory entry structure (64 bytes)
typedef struct {
    uint32_t inode_num;         // Inode number
    char name[FS_MAX_NAME_LEN]; // Filename
    uint8_t reserved[28];       // Reserved for alignment
} fs_dirent_t;

// File descriptor structure
typedef struct {
    uint32_t inode_num;         // Inode number
    uint32_t flags;             // Open flags
    uint32_t offset;            // Current file offset
    uint32_t owner_pid;         // Process that opened the file
} fs_file_t;

// Filesystem statistics
typedef struct {
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t total_files;
    uint32_t open_files;
    uint32_t block_size;
} fs_stats_t;

// Filesystem functions
void fs_init(void);
int fs_format(void);
int fs_mount(void);
int fs_unmount(void);

// File operations
int fs_create(const char* path, uint32_t type, uint32_t permissions);
int fs_open(const char* path, uint32_t flags);
int fs_close(int fd);
int fs_read(int fd, void* buffer, uint32_t size);
int fs_write(int fd, const void* buffer, uint32_t size);
int fs_seek(int fd, int32_t offset, uint32_t whence);
int fs_delete(const char* path);

// Directory operations
int fs_mkdir(const char* path);
int fs_rmdir(const char* path);
int fs_readdir(const char* path, fs_dirent_t* entries, uint32_t max_entries);

// Utility functions
int fs_stat(const char* path, fs_inode_t* inode);
int fs_get_stats(fs_stats_t* stats);
int fs_path_to_inode(const char* path);

#endif // _FS_H