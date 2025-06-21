// Tempo Compiler Optimizer
// Reduces code overhead while maintaining determinism

#ifndef TEMPO_OPTIMIZER_H
#define TEMPO_OPTIMIZER_H

#include <stdbool.h>
#include <stdint.h>

// Forward declaration
struct ast_node;

// Optimization levels
typedef enum {
    OPT_NONE = 0,      // No optimization
    OPT_SIZE = 1,      // Optimize for size
    OPT_SPEED = 2,     // Optimize for speed (maintains WCET)
    OPT_BALANCED = 3   // Balance size and speed
} optimization_level_t;

// Optimization flags
typedef struct {
    bool remove_frame_pointer;      // Omit EBP in leaf functions
    bool inline_small_functions;    // Inline functions < N instructions
    bool constant_folding;          // Evaluate constants at compile time
    bool dead_code_elimination;     // Remove unreachable code
    bool peephole_optimization;     // Local instruction optimizations
    bool register_allocation;       // Better register usage
    bool tail_call_optimization;    // Convert tail calls to jumps
    
    // Thresholds
    uint32_t inline_threshold;      // Max instructions to inline
    uint32_t unroll_threshold;      // Max iterations to unroll
    
    // Constraints
    bool maintain_wcet_bounds;      // Never exceed WCET estimates
    bool preserve_security;         // Keep all security checks
} optimization_flags_t;

// Function analysis results
typedef struct {
    bool is_leaf;                   // No function calls
    bool is_tail_recursive;         // Last operation is recursive call
    bool has_loops;                 // Contains loops
    bool uses_local_vars;           // Uses stack variables
    uint32_t instruction_count;     // Estimated instructions
    uint32_t stack_usage;           // Bytes of stack used
    uint32_t register_pressure;     // Number of live variables
} function_analysis_t;

// Peephole patterns
typedef struct {
    const char* pattern[3];         // Up to 3 instruction pattern
    const char* replacement[3];     // Replacement instructions
    uint32_t pattern_length;
    uint32_t replacement_length;
    int32_t cycle_saving;          // Cycles saved (negative = slower)
} peephole_pattern_t;

// Common x86 peephole optimizations
static const peephole_pattern_t peephole_patterns[] = {
    // Push/pop elimination
    {
        {"push eax", "pop eax", NULL},
        {NULL, NULL, NULL},
        2, 0, 5  // Save 5 cycles
    },
    
    // Redundant move elimination
    {
        {"mov eax, ebx", "mov ebx, eax", NULL},
        {"mov eax, ebx", NULL, NULL},
        2, 1, 1
    },
    
    // Zero register optimization
    {
        {"mov eax, 0", NULL, NULL},
        {"xor eax, eax", NULL, NULL},
        1, 1, 1  // XOR is 1 byte vs 5 bytes
    },
    
    // Compare with zero
    {
        {"cmp eax, 0", NULL, NULL},
        {"test eax, eax", NULL, NULL},
        1, 1, 0  // Same speed, smaller encoding
    },
    
    // Increment optimization
    {
        {"add eax, 1", NULL, NULL},
        {"inc eax", NULL, NULL},
        1, 1, 0  // Smaller encoding
    },
    
    // LEA for arithmetic
    {
        {"mov edx, eax", "add edx, ebx", NULL},
        {"lea edx, [eax + ebx]", NULL, NULL},
        2, 1, 1  // One instruction instead of two
    }
};

// Optimization API
optimization_flags_t optimizer_get_default_flags(optimization_level_t level);
function_analysis_t optimizer_analyze_function(struct ast_node* func);
void optimizer_optimize_function(struct ast_node* func, 
                               const optimization_flags_t* flags);

// Specific optimizations
bool optimizer_can_omit_frame_pointer(const function_analysis_t* analysis);
bool optimizer_should_inline(const function_analysis_t* caller,
                           const function_analysis_t* callee,
                           const optimization_flags_t* flags);
void optimizer_constant_fold(struct ast_node* node);
void optimizer_eliminate_dead_code(struct ast_node* node);
void optimizer_apply_peephole(char** instructions, uint32_t count);

// Register allocation
typedef struct {
    const char* var_name;
    uint32_t live_start;    // First use
    uint32_t live_end;      // Last use
    uint8_t assigned_reg;   // EAX=0, EBX=1, ECX=2, EDX=3
    bool spilled;           // Spilled to stack
} register_allocation_t;

void optimizer_allocate_registers(struct ast_node* func,
                                register_allocation_t* allocation,
                                uint32_t* num_allocations);

// WCET-aware optimization
typedef struct {
    uint32_t original_wcet;
    uint32_t optimized_wcet;
    bool wcet_preserved;     // True if WCET not increased
} wcet_optimization_result_t;

wcet_optimization_result_t optimizer_validate_wcet(struct ast_node* original,
                                                 struct ast_node* optimized);

// Code generation helpers
void optimizer_emit_optimized_prologue(FILE* output, 
                                     const function_analysis_t* analysis);
void optimizer_emit_optimized_epilogue(FILE* output,
                                     const function_analysis_t* analysis);

// Statistics
typedef struct {
    uint32_t functions_analyzed;
    uint32_t frame_pointers_removed;
    uint32_t functions_inlined;
    uint32_t constants_folded;
    uint32_t dead_code_removed;
    uint32_t peephole_applied;
    uint32_t cycles_saved;
    uint32_t bytes_saved;
} optimization_stats_t;

void optimizer_print_stats(const optimization_stats_t* stats);

#endif // TEMPO_OPTIMIZER_H