// Tempo Compiler Optimizer Implementation
// Reduces overhead while maintaining determinism and WCET bounds

#include "tempo_optimizer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Get default optimization flags for a given level
optimization_flags_t optimizer_get_default_flags(optimization_level_t level) {
    optimization_flags_t flags = {0};
    
    switch (level) {
        case OPT_NONE:
            // All optimizations disabled
            flags.maintain_wcet_bounds = true;
            flags.preserve_security = true;
            break;
            
        case OPT_SIZE:
            // Optimize for size
            flags.remove_frame_pointer = true;
            flags.dead_code_elimination = true;
            flags.peephole_optimization = true;
            flags.inline_threshold = 5;  // Only tiny functions
            flags.maintain_wcet_bounds = true;
            flags.preserve_security = true;
            break;
            
        case OPT_SPEED:
            // Optimize for speed
            flags.remove_frame_pointer = true;
            flags.inline_small_functions = true;
            flags.constant_folding = true;
            flags.dead_code_elimination = true;
            flags.peephole_optimization = true;
            flags.register_allocation = true;
            flags.tail_call_optimization = true;
            flags.inline_threshold = 20;
            flags.unroll_threshold = 4;
            flags.maintain_wcet_bounds = true;
            flags.preserve_security = true;
            break;
            
        case OPT_BALANCED:
            // Balance size and speed
            flags.remove_frame_pointer = true;
            flags.inline_small_functions = true;
            flags.constant_folding = true;
            flags.dead_code_elimination = true;
            flags.peephole_optimization = true;
            flags.inline_threshold = 10;
            flags.unroll_threshold = 2;
            flags.maintain_wcet_bounds = true;
            flags.preserve_security = true;
            break;
    }
    
    return flags;
}

// Analyze function characteristics
function_analysis_t optimizer_analyze_function(struct ast_node* func) {
    function_analysis_t analysis = {0};
    
    if (!func || func->type != NODE_FUNCTION) {
        return analysis;
    }
    
    // Walk the function body
    struct ast_node* body = func->right;
    
    analysis.is_leaf = true;  // Assume leaf until we find a call
    analysis.instruction_count = 0;
    analysis.stack_usage = 0;
    
    // Simple analysis - would be recursive in real implementation
    struct ast_node* stmt = body;
    while (stmt) {
        switch (stmt->type) {
            case NODE_CALL:
                analysis.is_leaf = false;
                analysis.instruction_count += 5;  // CALL overhead
                break;
                
            case NODE_LET:
                analysis.uses_local_vars = true;
                analysis.stack_usage += 4;  // 32-bit variable
                analysis.register_pressure++;
                analysis.instruction_count += 2;
                break;
                
            case NODE_WHILE:
                analysis.has_loops = true;
                analysis.instruction_count += 10;  // Loop overhead estimate
                break;
                
            case NODE_RETURN:
                // Check for tail recursion
                if (stmt->left && stmt->left->type == NODE_CALL) {
                    if (strcmp(stmt->left->value, func->value) == 0) {
                        analysis.is_tail_recursive = true;
                    }
                }
                analysis.instruction_count += 3;
                break;
                
            case NODE_BINARY_OP:
                analysis.instruction_count += 3;
                break;
                
            default:
                analysis.instruction_count += 1;
                break;
        }
        
        stmt = stmt->next;
    }
    
    return analysis;
}

// Check if frame pointer can be omitted
bool optimizer_can_omit_frame_pointer(const function_analysis_t* analysis) {
    // Can omit if:
    // 1. Leaf function (no calls)
    // 2. No local variables on stack
    // 3. No variable-length arrays (not supported yet)
    // 4. No exception handling (not supported yet)
    
    return analysis->is_leaf && 
           !analysis->uses_local_vars &&
           analysis->stack_usage == 0;
}

// Determine if function should be inlined
bool optimizer_should_inline(const function_analysis_t* caller,
                           const function_analysis_t* callee,
                           const optimization_flags_t* flags) {
    if (!flags->inline_small_functions) {
        return false;
    }
    
    // Don't inline if:
    // 1. Callee is too large
    if (callee->instruction_count > flags->inline_threshold) {
        return false;
    }
    
    // 2. Callee has loops (code bloat)
    if (callee->has_loops) {
        return false;
    }
    
    // 3. Would increase register pressure too much
    if (caller->register_pressure + callee->register_pressure > 4) {
        return false;
    }
    
    // 4. Recursive functions
    if (callee->is_tail_recursive) {
        return false;
    }
    
    return true;
}

// Constant folding optimization
void optimizer_constant_fold(struct ast_node* node) {
    if (!node) return;
    
    // Recursively fold children first
    optimizer_constant_fold(node->left);
    optimizer_constant_fold(node->right);
    
    // Check if this is a binary operation with constant operands
    if (node->type == NODE_BINARY_OP &&
        node->left && node->left->type == NODE_NUMBER &&
        node->right && node->right->type == NODE_NUMBER) {
        
        int left_val = atoi(node->left->value);
        int right_val = atoi(node->right->value);
        int result = 0;
        
        // Perform the operation
        if (strcmp(node->value, "+") == 0) {
            result = left_val + right_val;
        } else if (strcmp(node->value, "-") == 0) {
            result = left_val - right_val;
        } else if (strcmp(node->value, "*") == 0) {
            result = left_val * right_val;
        } else if (strcmp(node->value, "/") == 0 && right_val != 0) {
            result = left_val / right_val;
        } else {
            return;  // Can't fold
        }
        
        // Replace this node with a constant
        node->type = NODE_NUMBER;
        snprintf(node->value, sizeof(node->value), "%d", result);
        
        // Free children (in real implementation would properly free)
        node->left = NULL;
        node->right = NULL;
    }
}

// Dead code elimination
void optimizer_eliminate_dead_code(struct ast_node* node) {
    if (!node) return;
    
    // Simple dead code patterns:
    
    // 1. Unreachable code after return
    if (node->type == NODE_RETURN) {
        // Everything after return is dead
        node->next = NULL;
    }
    
    // 2. If with constant condition
    if (node->type == NODE_IF && 
        node->left && node->left->type == NODE_NUMBER) {
        
        int condition = atoi(node->left->value);
        if (condition) {
            // Always true - replace with then branch
            *node = *(node->right);
        } else {
            // Always false - remove entire if
            node->type = NODE_NUMBER;
            strcpy(node->value, "0");
            node->left = node->right = NULL;
        }
    }
    
    // 3. While with false condition
    if (node->type == NODE_WHILE &&
        node->left && node->left->type == NODE_NUMBER &&
        atoi(node->left->value) == 0) {
        // Loop never executes
        node->type = NODE_NUMBER;
        strcpy(node->value, "0");
        node->left = node->right = NULL;
    }
    
    // Recurse
    optimizer_eliminate_dead_code(node->left);
    optimizer_eliminate_dead_code(node->right);
    optimizer_eliminate_dead_code(node->next);
}

// Apply peephole optimizations
void optimizer_apply_peephole(char** instructions, uint32_t count) {
    if (!instructions || count < 2) return;
    
    // Scan for patterns
    for (uint32_t i = 0; i < count - 1; i++) {
        if (!instructions[i] || !instructions[i+1]) continue;
        
        // Pattern: push eax; pop eax -> (nothing)
        if (strstr(instructions[i], "push eax") &&
            strstr(instructions[i+1], "pop eax")) {
            instructions[i][0] = '\0';
            instructions[i+1][0] = '\0';
            continue;
        }
        
        // Pattern: mov eax, 0 -> xor eax, eax
        if (strstr(instructions[i], "mov eax, 0")) {
            strcpy(instructions[i], "    xor eax, eax");
            continue;
        }
        
        // Pattern: add eax, 1 -> inc eax
        if (strstr(instructions[i], "add eax, 1")) {
            strcpy(instructions[i], "    inc eax");
            continue;
        }
        
        // Pattern: sub eax, 1 -> dec eax
        if (strstr(instructions[i], "sub eax, 1")) {
            strcpy(instructions[i], "    dec eax");
            continue;
        }
        
        // Pattern: cmp eax, 0 -> test eax, eax
        if (strstr(instructions[i], "cmp eax, 0")) {
            strcpy(instructions[i], "    test eax, eax");
            continue;
        }
        
        // Pattern: mov edx, eax; add edx, ebx -> lea edx, [eax + ebx]
        if (i < count - 2 &&
            strstr(instructions[i], "mov edx, eax") &&
            strstr(instructions[i+1], "add edx, ebx")) {
            strcpy(instructions[i], "    lea edx, [eax + ebx]");
            instructions[i+1][0] = '\0';
            continue;
        }
    }
}

// Simple register allocator
void optimizer_allocate_registers(struct ast_node* func,
                                register_allocation_t* allocation,
                                uint32_t* num_allocations) {
    // Very simple allocator - just assigns first 4 variables to registers
    // Real implementation would do liveness analysis
    
    *num_allocations = 0;
    uint32_t instruction_counter = 0;
    
    // Find all variables in function
    struct ast_node* stmt = func->right;
    while (stmt && *num_allocations < 4) {
        if (stmt->type == NODE_LET) {
            allocation[*num_allocations].var_name = stmt->value;
            allocation[*num_allocations].live_start = instruction_counter;
            allocation[*num_allocations].live_end = instruction_counter + 100; // Guess
            allocation[*num_allocations].assigned_reg = *num_allocations; // EAX, EBX, ECX, EDX
            allocation[*num_allocations].spilled = false;
            (*num_allocations)++;
        }
        
        stmt = stmt->next;
        instruction_counter++;
    }
}

// Validate that optimization preserves WCET
wcet_optimization_result_t optimizer_validate_wcet(struct ast_node* original,
                                                 struct ast_node* optimized) {
    wcet_optimization_result_t result;
    
    result.original_wcet = original->wcet_cycles;
    result.optimized_wcet = optimized->wcet_cycles;
    
    // Optimization should not increase WCET
    result.wcet_preserved = (result.optimized_wcet <= result.original_wcet);
    
    return result;
}

// Emit optimized function prologue
void optimizer_emit_optimized_prologue(FILE* output, 
                                     const function_analysis_t* analysis) {
    if (optimizer_can_omit_frame_pointer(analysis)) {
        fprintf(output, "    ; Optimized: no frame pointer needed\n");
        // No prologue needed for true leaf functions
    } else {
        fprintf(output, "    push ebp\n");
        fprintf(output, "    mov ebp, esp\n");
        
        if (analysis->stack_usage > 0) {
            // Align stack allocation to 16 bytes for better performance
            uint32_t aligned_size = (analysis->stack_usage + 15) & ~15;
            fprintf(output, "    sub esp, %u\n", aligned_size);
        }
    }
}

// Emit optimized function epilogue
void optimizer_emit_optimized_epilogue(FILE* output,
                                     const function_analysis_t* analysis) {
    if (optimizer_can_omit_frame_pointer(analysis)) {
        fprintf(output, "    ret\n");
    } else {
        fprintf(output, "    mov esp, ebp\n");
        fprintf(output, "    pop ebp\n");
        fprintf(output, "    ret\n");
    }
}

// Print optimization statistics
void optimizer_print_stats(const optimization_stats_t* stats) {
    printf("Optimization Statistics:\n");
    printf("  Functions analyzed: %u\n", stats->functions_analyzed);
    printf("  Frame pointers removed: %u\n", stats->frame_pointers_removed);
    printf("  Functions inlined: %u\n", stats->functions_inlined);
    printf("  Constants folded: %u\n", stats->constants_folded);
    printf("  Dead code removed: %u\n", stats->dead_code_removed);
    printf("  Peephole optimizations: %u\n", stats->peephole_applied);
    printf("  Estimated cycles saved: %u\n", stats->cycles_saved);
    printf("  Code size saved: %u bytes\n", stats->bytes_saved);
}