// AtomicOS IPC Test Suite
// Tests inter-process communication functionality

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/kernel/ipc.h"

// Mock process ID for testing
static uint32_t test_pid = 100;

// Test basic channel creation
void test_ipc_create_channel() {
    printf("Testing IPC channel creation...\n");
    
    ipc_init();
    
    // Create a channel with read/write permissions
    uint32_t channel_id = ipc_create_channel(test_pid, IPC_PERM_READ | IPC_PERM_WRITE);
    assert(channel_id > 0);
    printf("  ✓ Created channel with ID: %u\n", channel_id);
    
    // Create multiple channels
    for (int i = 0; i < 5; i++) {
        uint32_t ch = ipc_create_channel(test_pid + i + 1, IPC_PERM_READ);
        assert(ch > channel_id);
        channel_id = ch;
    }
    printf("  ✓ Created multiple channels successfully\n");
}

// Test message sending and receiving
void test_ipc_send_receive() {
    printf("Testing IPC send/receive...\n");
    
    ipc_init();
    
    // Create a channel
    uint32_t channel = ipc_create_channel(test_pid, IPC_PERM_READ | IPC_PERM_WRITE);
    assert(channel > 0);
    
    // Test data
    char test_msg[] = "Hello, IPC!";
    uint32_t msg_len = strlen(test_msg) + 1;
    
    // Send message
    int result = ipc_send(channel, test_pid + 1, test_msg, msg_len);
    assert(result == 0);
    printf("  ✓ Sent message successfully\n");
    
    // Receive message
    char buffer[256];
    uint32_t actual_size = 0;
    result = ipc_receive(channel, test_pid, buffer, sizeof(buffer), &actual_size);
    assert(result == 0);
    assert(actual_size == msg_len);
    assert(strcmp(buffer, test_msg) == 0);
    printf("  ✓ Received message correctly: '%s'\n", buffer);
}

// Test channel permissions
void test_ipc_permissions() {
    printf("Testing IPC permissions...\n");
    
    ipc_init();
    
    // Create read-only channel
    uint32_t ro_channel = ipc_create_channel(test_pid, IPC_PERM_READ);
    assert(ro_channel > 0);
    
    // Try to send to read-only channel (should fail)
    char msg[] = "test";
    int result = ipc_send(ro_channel, test_pid + 1, msg, sizeof(msg));
    assert(result == -3); // Permission denied
    printf("  ✓ Write to read-only channel correctly denied\n");
}

// Test message queue limits
void test_ipc_queue_limits() {
    printf("Testing IPC queue limits...\n");
    
    ipc_init();
    
    uint32_t channel = ipc_create_channel(test_pid, IPC_PERM_READ | IPC_PERM_WRITE);
    
    // Fill the queue
    char msg[10];
    int sent = 0;
    for (int i = 0; i < IPC_CHANNEL_MAX_MESSAGES + 2; i++) {
        sprintf(msg, "msg%d", i);
        int result = ipc_send(channel, test_pid + 1, msg, strlen(msg) + 1);
        if (result == 0) {
            sent++;
        }
    }
    
    assert(sent == IPC_CHANNEL_MAX_MESSAGES);
    printf("  ✓ Queue correctly limited to %d messages\n", IPC_CHANNEL_MAX_MESSAGES);
}

// Test channel closing
void test_ipc_close() {
    printf("Testing IPC channel closing...\n");
    
    ipc_init();
    
    uint32_t channel = ipc_create_channel(test_pid, IPC_PERM_READ | IPC_PERM_WRITE);
    
    // Send a message
    char msg[] = "test";
    ipc_send(channel, test_pid + 1, msg, sizeof(msg));
    
    // Close the channel
    int result = ipc_close_channel(channel, test_pid);
    assert(result == 0);
    printf("  ✓ Channel closed successfully\n");
    
    // Try to send after close (should fail)
    result = ipc_send(channel, test_pid + 1, msg, sizeof(msg));
    assert(result == -2); // Channel not found
    printf("  ✓ Send to closed channel correctly failed\n");
}

// Test statistics
void test_ipc_stats() {
    printf("Testing IPC statistics...\n");
    
    ipc_init();
    
    uint32_t channel = ipc_create_channel(test_pid, IPC_PERM_READ | IPC_PERM_WRITE);
    
    // Send some messages
    char msg[] = "test";
    for (int i = 0; i < 3; i++) {
        ipc_send(channel, test_pid + 1, msg, sizeof(msg));
    }
    
    // Get stats
    ipc_stats_t stats;
    int result = ipc_get_stats(channel, &stats);
    assert(result == 0);
    assert(stats.channel_id == channel);
    assert(stats.owner_pid == test_pid);
    assert(stats.message_count == 3);
    printf("  ✓ Statistics: Channel %u has %u messages\n", 
           stats.channel_id, stats.message_count);
}

int main() {
    printf("=== AtomicOS IPC Test Suite ===\n\n");
    
    test_ipc_create_channel();
    test_ipc_send_receive();
    test_ipc_permissions();
    test_ipc_queue_limits();
    test_ipc_close();
    test_ipc_stats();
    
    printf("\n✅ All IPC tests passed!\n");
    return 0;
}