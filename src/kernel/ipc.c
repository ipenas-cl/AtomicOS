// AtomicOS IPC (Inter-Process Communication) Implementation
// Microkernel architecture message passing system

#include "ipc.h"

// Message queue structure
typedef struct message {
    uint32_t sender_pid;
    uint32_t receiver_pid;
    uint32_t type;
    uint32_t length;
    uint8_t data[IPC_MAX_MESSAGE_SIZE];
    struct message* next;
} message_t;

// IPC channel structure
typedef struct ipc_channel {
    uint32_t channel_id;
    uint32_t owner_pid;
    uint32_t permissions;
    message_t* message_queue_head;
    message_t* message_queue_tail;
    uint32_t message_count;
    uint32_t max_messages;
    struct ipc_channel* next;
} ipc_channel_t;

// Global IPC state
static ipc_channel_t* channel_list = NULL;
static uint32_t next_channel_id = 1;
static message_t message_pool[IPC_MAX_MESSAGES];
static uint32_t message_pool_bitmap = 0; // 32 messages max for simplicity

// Initialize IPC subsystem
void ipc_init(void) {
    // Clear channel list
    channel_list = NULL;
    next_channel_id = 1;
    
    // Initialize message pool
    for (int i = 0; i < IPC_MAX_MESSAGES; i++) {
        message_pool[i].sender_pid = 0;
        message_pool[i].receiver_pid = 0;
        message_pool[i].type = 0;
        message_pool[i].length = 0;
        message_pool[i].next = NULL;
    }
    message_pool_bitmap = 0;
}

// Allocate a message from the pool
static message_t* allocate_message(void) {
    for (int i = 0; i < IPC_MAX_MESSAGES; i++) {
        if (!(message_pool_bitmap & (1 << i))) {
            message_pool_bitmap |= (1 << i);
            return &message_pool[i];
        }
    }
    return NULL; // Pool exhausted
}

// Free a message back to the pool
static void free_message(message_t* msg) {
    int index = msg - message_pool;
    if (index >= 0 && index < IPC_MAX_MESSAGES) {
        message_pool_bitmap &= ~(1 << index);
        msg->next = NULL;
    }
}

// Create a new IPC channel
uint32_t ipc_create_channel(uint32_t owner_pid, uint32_t permissions) {
    // Allocate channel structure (static allocation in real implementation)
    static ipc_channel_t channels[IPC_MAX_CHANNELS];
    static int next_channel_slot = 0;
    
    if (next_channel_slot >= IPC_MAX_CHANNELS) {
        return 0; // No more channels available
    }
    
    ipc_channel_t* channel = &channels[next_channel_slot++];
    channel->channel_id = next_channel_id++;
    channel->owner_pid = owner_pid;
    channel->permissions = permissions;
    channel->message_queue_head = NULL;
    channel->message_queue_tail = NULL;
    channel->message_count = 0;
    channel->max_messages = IPC_CHANNEL_MAX_MESSAGES;
    
    // Add to channel list
    channel->next = channel_list;
    channel_list = channel;
    
    return channel->channel_id;
}

// Find a channel by ID
static ipc_channel_t* find_channel(uint32_t channel_id) {
    ipc_channel_t* current = channel_list;
    while (current) {
        if (current->channel_id == channel_id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Send a message through IPC
int ipc_send(uint32_t channel_id, uint32_t sender_pid, void* data, uint32_t length) {
    // Validate parameters
    if (!data || length == 0 || length > IPC_MAX_MESSAGE_SIZE) {
        return -1;
    }
    
    // Find channel
    ipc_channel_t* channel = find_channel(channel_id);
    if (!channel) {
        return -2;
    }
    
    // Check permissions (simplified)
    if (!(channel->permissions & IPC_PERM_WRITE)) {
        return -3;
    }
    
    // Check if queue is full
    if (channel->message_count >= channel->max_messages) {
        return -4;
    }
    
    // Allocate message
    message_t* msg = allocate_message();
    if (!msg) {
        return -5;
    }
    
    // Fill message
    msg->sender_pid = sender_pid;
    msg->receiver_pid = channel->owner_pid;
    msg->type = 0; // Default type
    msg->length = length;
    
    // Copy data (bounds checked)
    for (uint32_t i = 0; i < length; i++) {
        msg->data[i] = ((uint8_t*)data)[i];
    }
    
    // Add to queue
    msg->next = NULL;
    if (channel->message_queue_tail) {
        channel->message_queue_tail->next = msg;
    } else {
        channel->message_queue_head = msg;
    }
    channel->message_queue_tail = msg;
    channel->message_count++;
    
    // Wake up receiver if blocked
    // process_wakeup(channel->owner_pid);
    
    return 0;
}

// Receive a message from IPC
int ipc_receive(uint32_t channel_id, uint32_t receiver_pid, void* buffer, uint32_t buffer_size, uint32_t* actual_size) {
    // Find channel
    ipc_channel_t* channel = find_channel(channel_id);
    if (!channel) {
        return -1;
    }
    
    // Check ownership
    if (channel->owner_pid != receiver_pid) {
        return -2;
    }
    
    // Check if queue is empty
    if (!channel->message_queue_head) {
        return -3; // Would block in real implementation
    }
    
    // Get first message
    message_t* msg = channel->message_queue_head;
    channel->message_queue_head = msg->next;
    if (!channel->message_queue_head) {
        channel->message_queue_tail = NULL;
    }
    channel->message_count--;
    
    // Copy data to buffer
    uint32_t copy_size = msg->length;
    if (copy_size > buffer_size) {
        copy_size = buffer_size;
    }
    
    for (uint32_t i = 0; i < copy_size; i++) {
        ((uint8_t*)buffer)[i] = msg->data[i];
    }
    
    if (actual_size) {
        *actual_size = msg->length;
    }
    
    // Free message
    free_message(msg);
    
    return 0;
}

// Close an IPC channel
int ipc_close_channel(uint32_t channel_id, uint32_t owner_pid) {
    ipc_channel_t* prev = NULL;
    ipc_channel_t* current = channel_list;
    
    while (current) {
        if (current->channel_id == channel_id) {
            // Check ownership
            if (current->owner_pid != owner_pid) {
                return -1;
            }
            
            // Free all pending messages
            message_t* msg = current->message_queue_head;
            while (msg) {
                message_t* next = msg->next;
                free_message(msg);
                msg = next;
            }
            
            // Remove from list
            if (prev) {
                prev->next = current->next;
            } else {
                channel_list = current->next;
            }
            
            return 0;
        }
        prev = current;
        current = current->next;
    }
    
    return -2;
}

// Get channel statistics
int ipc_get_stats(uint32_t channel_id, ipc_stats_t* stats) {
    ipc_channel_t* channel = find_channel(channel_id);
    if (!channel || !stats) {
        return -1;
    }
    
    stats->channel_id = channel->channel_id;
    stats->owner_pid = channel->owner_pid;
    stats->message_count = channel->message_count;
    stats->max_messages = channel->max_messages;
    stats->total_channels = 0;
    
    // Count total channels
    ipc_channel_t* current = channel_list;
    while (current) {
        stats->total_channels++;
        current = current->next;
    }
    
    return 0;
}