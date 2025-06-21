// AtomicOS IPC (Inter-Process Communication) Header
// Microkernel architecture message passing interface

#ifndef _IPC_H
#define _IPC_H

// Basic type definitions to avoid header conflicts
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

// IPC Constants
#define IPC_MAX_MESSAGE_SIZE    256     // Maximum message size in bytes
#define IPC_MAX_MESSAGES        32      // Maximum messages in pool
#define IPC_MAX_CHANNELS        16      // Maximum IPC channels
#define IPC_CHANNEL_MAX_MESSAGES 8      // Maximum messages per channel

// IPC Permissions
#define IPC_PERM_READ   0x01
#define IPC_PERM_WRITE  0x02
#define IPC_PERM_OWNER  0x04

// IPC Message Types
#define IPC_TYPE_DATA       0x00
#define IPC_TYPE_SIGNAL     0x01
#define IPC_TYPE_INTERRUPT  0x02
#define IPC_TYPE_SYSCALL    0x03

// IPC Statistics structure
typedef struct {
    uint32_t channel_id;
    uint32_t owner_pid;
    uint32_t message_count;
    uint32_t max_messages;
    uint32_t total_channels;
} ipc_stats_t;

// IPC Functions
void ipc_init(void);
uint32_t ipc_create_channel(uint32_t owner_pid, uint32_t permissions);
int ipc_send(uint32_t channel_id, uint32_t sender_pid, void* data, uint32_t length);
int ipc_receive(uint32_t channel_id, uint32_t receiver_pid, void* buffer, uint32_t buffer_size, uint32_t* actual_size);
int ipc_close_channel(uint32_t channel_id, uint32_t owner_pid);
int ipc_get_stats(uint32_t channel_id, ipc_stats_t* stats);

#endif // _IPC_H