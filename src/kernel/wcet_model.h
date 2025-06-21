// AtomicOS WCET (Worst-Case Execution Time) Model
// Realistic cycle counts for x86 architecture

#ifndef WCET_MODEL_H
#define WCET_MODEL_H

#include "../include/types.h"

// Instruction categories with typical cycle counts
typedef enum {
    INST_ALU_REG,      // ADD, SUB, AND, OR, XOR reg,reg
    INST_ALU_MEM,      // ADD, SUB with memory operand
    INST_MUL,          // Integer multiplication
    INST_DIV,          // Integer division
    INST_BRANCH_TAKEN, // Conditional branch taken
    INST_BRANCH_NOT,   // Conditional branch not taken
    INST_CALL,         // Function call
    INST_RET,          // Function return
    INST_LOAD,         // Memory load
    INST_STORE,        // Memory store
    INST_PUSH,         // Stack push
    INST_POP,          // Stack pop
} instruction_type_t;

// Cache miss penalties
typedef struct {
    uint32_t l1_miss;    // L1 cache miss penalty (~4 cycles)
    uint32_t l2_miss;    // L2 cache miss penalty (~10 cycles)
    uint32_t l3_miss;    // L3 cache miss penalty (~40 cycles)
    uint32_t tlb_miss;   // TLB miss penalty (~100 cycles)
} cache_penalties_t;

// Pipeline penalties
typedef struct {
    uint32_t branch_mispredict;  // Branch misprediction (~15 cycles)
    uint32_t pipeline_stall;     // Data dependency stall (~1-3 cycles)
    uint32_t memory_fence;       // Memory fence/serialization (~30 cycles)
} pipeline_penalties_t;

// x86 instruction timing model (Pentium 4 / modern x86)
static const uint32_t instruction_cycles[] = {
    [INST_ALU_REG]      = 1,    // Basic ALU
    [INST_ALU_MEM]      = 3,    // ALU with memory
    [INST_MUL]          = 3,    // IMUL
    [INST_DIV]          = 40,   // IDIV (highly variable)
    [INST_BRANCH_TAKEN] = 3,    // Predicted correctly
    [INST_BRANCH_NOT]   = 1,    // Fall through
    [INST_CALL]         = 4,    // CALL instruction
    [INST_RET]          = 5,    // RET (return address prediction)
    [INST_LOAD]         = 3,    // MOV from memory
    [INST_STORE]        = 2,    // MOV to memory
    [INST_PUSH]         = 2,    // PUSH
    [INST_POP]          = 3,    // POP
};

// Default cache penalties for WCET analysis
static const cache_penalties_t default_cache_penalties = {
    .l1_miss = 4,
    .l2_miss = 10,
    .l3_miss = 40,
    .tlb_miss = 100
};

// Default pipeline penalties
static const pipeline_penalties_t default_pipeline_penalties = {
    .branch_mispredict = 15,
    .pipeline_stall = 2,
    .memory_fence = 30
};

// WCET calculation context
typedef struct {
    uint32_t base_cycles;           // Base instruction cycles
    uint32_t cache_misses;          // Estimated cache misses
    uint32_t branch_mispredicts;    // Estimated mispredictions
    uint32_t memory_accesses;       // Number of memory accesses
    uint32_t total_wcet;            // Total WCET in cycles
    
    // Configuration
    cache_penalties_t cache_config;
    pipeline_penalties_t pipeline_config;
    
    // Analysis flags
    bool assume_cold_cache;         // Assume all accesses miss
    bool assume_worst_branches;     // Assume all branches mispredict
    bool include_interrupt_delay;   // Include interrupt latency
} wcet_context_t;

// Function prototypes
void wcet_init_context(wcet_context_t* ctx);
void wcet_add_instruction(wcet_context_t* ctx, instruction_type_t type);
void wcet_add_memory_access(wcet_context_t* ctx, bool is_write);
void wcet_add_branch(wcet_context_t* ctx, bool is_loop);
uint32_t wcet_calculate_total(wcet_context_t* ctx);

// Helper macros for common patterns
#define WCET_FUNCTION_PROLOGUE(ctx) do { \
    wcet_add_instruction(ctx, INST_PUSH);     /* push ebp */ \
    wcet_add_instruction(ctx, INST_ALU_REG);  /* mov ebp,esp */ \
} while(0)

#define WCET_FUNCTION_EPILOGUE(ctx) do { \
    wcet_add_instruction(ctx, INST_ALU_REG);  /* mov esp,ebp */ \
    wcet_add_instruction(ctx, INST_POP);      /* pop ebp */ \
    wcet_add_instruction(ctx, INST_RET);      /* ret */ \
} while(0)

#define WCET_MEMORY_LOAD(ctx) do { \
    wcet_add_instruction(ctx, INST_LOAD); \
    wcet_add_memory_access(ctx, false); \
} while(0)

#define WCET_MEMORY_STORE(ctx) do { \
    wcet_add_instruction(ctx, INST_STORE); \
    wcet_add_memory_access(ctx, true); \
} while(0)

// Analysis utilities
typedef struct {
    const char* function_name;
    uint32_t wcet_cycles;
    uint32_t memory_accesses;
    uint32_t branches;
    bool validated;  // True if validated against hardware
} wcet_report_t;

void wcet_generate_report(wcet_context_t* ctx, wcet_report_t* report);
void wcet_print_report(const wcet_report_t* report);

#endif // WCET_MODEL_H