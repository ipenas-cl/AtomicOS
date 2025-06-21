// AtomicOS Filesystem Test Suite
// Tests basic filesystem functionality

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/kernel/fs.h"

// Test filesystem initialization and formatting
void test_fs_init_format() {
    printf("Testing filesystem init and format...\n");
    
    fs_init();
    int result = fs_format();
    assert(result == 0);
    printf("  ✓ Filesystem formatted successfully\n");
    
    // Verify mount works
    result = fs_mount();
    assert(result == 0);
    printf("  ✓ Filesystem mounted successfully\n");
}

// Test file creation
void test_fs_create_file() {
    printf("Testing file creation...\n");
    
    fs_init();
    fs_format();
    
    // Create a file
    int result = fs_create("/test.txt", FS_TYPE_FILE, FS_PERM_READ | FS_PERM_WRITE);
    assert(result > 0);
    printf("  ✓ Created file with inode %d\n", result);
    
    // Try to create same file (should succeed, returning existing inode)
    int inode2 = fs_path_to_inode("/test.txt");
    assert(inode2 == result);
    printf("  ✓ File lookup works correctly\n");
}

// Test file operations
void test_fs_file_operations() {
    printf("Testing file read/write operations...\n");
    
    fs_init();
    fs_format();
    
    // Create and open a file
    int fd = fs_open("/data.bin", FS_OPEN_CREATE | FS_OPEN_WRITE);
    assert(fd >= 0);
    printf("  ✓ Opened file with fd %d\n", fd);
    
    // Write data
    char write_data[] = "Hello, AtomicOS filesystem!";
    int written = fs_write(fd, write_data, strlen(write_data) + 1);
    assert(written == strlen(write_data) + 1);
    printf("  ✓ Wrote %d bytes\n", written);
    
    // Close and reopen for reading
    fs_close(fd);
    
    fd = fs_open("/data.bin", FS_OPEN_READ);
    assert(fd >= 0);
    
    // Read data back
    char read_buffer[100];
    memset(read_buffer, 0, sizeof(read_buffer));
    int bytes_read = fs_read(fd, read_buffer, sizeof(read_buffer));
    assert(bytes_read == written);
    assert(strcmp(read_buffer, write_data) == 0);
    printf("  ✓ Read back data correctly: '%s'\n", read_buffer);
    
    fs_close(fd);
}

// Test multiple files
void test_fs_multiple_files() {
    printf("Testing multiple files...\n");
    
    fs_init();
    fs_format();
    
    // Create multiple files
    char filename[32];
    for (int i = 0; i < 5; i++) {
        sprintf(filename, "/file%d.dat", i);
        int result = fs_create(filename, FS_TYPE_FILE, FS_PERM_READ | FS_PERM_WRITE);
        assert(result > 0);
    }
    printf("  ✓ Created 5 files successfully\n");
    
    // Write different data to each
    for (int i = 0; i < 5; i++) {
        sprintf(filename, "/file%d.dat", i);
        int fd = fs_open(filename, FS_OPEN_WRITE);
        assert(fd >= 0);
        
        char data[16];
        sprintf(data, "File %d data", i);
        fs_write(fd, data, strlen(data) + 1);
        fs_close(fd);
    }
    printf("  ✓ Wrote data to all files\n");
    
    // Verify each file
    for (int i = 0; i < 5; i++) {
        sprintf(filename, "/file%d.dat", i);
        int fd = fs_open(filename, FS_OPEN_READ);
        
        char buffer[32];
        fs_read(fd, buffer, sizeof(buffer));
        
        char expected[16];
        sprintf(expected, "File %d data", i);
        assert(strcmp(buffer, expected) == 0);
        fs_close(fd);
    }
    printf("  ✓ Verified all file contents\n");
}

// Test file deletion
void test_fs_delete() {
    printf("Testing file deletion...\n");
    
    fs_init();
    fs_format();
    
    // Create a file
    fs_create("/deleteme.txt", FS_TYPE_FILE, FS_PERM_READ | FS_PERM_WRITE);
    
    // Verify it exists
    int inode = fs_path_to_inode("/deleteme.txt");
    assert(inode > 0);
    
    // Delete it
    int result = fs_delete("/deleteme.txt");
    assert(result == 0);
    printf("  ✓ File deleted successfully\n");
    
    // Verify it's gone
    inode = fs_path_to_inode("/deleteme.txt");
    assert(inode == FS_ERR_NOT_FOUND);
    printf("  ✓ File no longer exists\n");
}

// Test filesystem statistics
void test_fs_stats() {
    printf("Testing filesystem statistics...\n");
    
    fs_init();
    fs_format();
    
    fs_stats_t stats;
    fs_get_stats(&stats);
    
    printf("  ✓ Initial stats: %u/%u blocks free, %u files\n",
           stats.free_blocks, stats.total_blocks, stats.total_files);
    
    // Create some files and check stats change
    fs_create("/file1.txt", FS_TYPE_FILE, FS_PERM_READ);
    fs_create("/file2.txt", FS_TYPE_FILE, FS_PERM_READ);
    
    fs_get_stats(&stats);
    assert(stats.total_files == 2);
    printf("  ✓ Stats updated correctly: %u files\n", stats.total_files);
}

// Test large file handling
void test_fs_large_file() {
    printf("Testing large file operations...\n");
    
    fs_init();
    fs_format();
    
    int fd = fs_open("/large.dat", FS_OPEN_CREATE | FS_OPEN_WRITE);
    assert(fd >= 0);
    
    // Write 10KB of data
    uint8_t pattern[512];
    for (int i = 0; i < 512; i++) {
        pattern[i] = i & 0xFF;
    }
    
    int total_written = 0;
    for (int i = 0; i < 20; i++) {
        int written = fs_write(fd, pattern, sizeof(pattern));
        total_written += written;
    }
    
    printf("  ✓ Wrote %d bytes to large file\n", total_written);
    
    // Verify file size through stat
    fs_close(fd);
    
    fs_inode_t stat;
    fs_stat("/large.dat", &stat);
    assert(stat.size == total_written);
    printf("  ✓ File size correctly reported as %u bytes\n", stat.size);
}

int main() {
    printf("=== AtomicOS Filesystem Test Suite ===\n\n");
    
    test_fs_init_format();
    test_fs_create_file();
    test_fs_file_operations();
    test_fs_multiple_files();
    test_fs_delete();
    test_fs_stats();
    test_fs_large_file();
    
    printf("\n✅ All filesystem tests passed!\n");
    return 0;
}