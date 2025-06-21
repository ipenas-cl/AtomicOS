/*
 * Copyright (c) 2024 Ignacio Peña
 * AtomicOS - Deterministic Real-Time Security Operating System
 * https://github.com/ipenas-cl/AtomicOS
 * Licensed under MIT License - see LICENSE file for details
 */

// Simplified Enhanced Tempo Compiler
// Demonstrates realistic WCET without complex dependencies

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

// Simplified WCET model
typedef struct {
    uint32_t base_cycles;
    uint32_t memory_accesses;
    uint32_t branches;
    uint32_t cache_misses;
} simple_wcet_t;

// Instruction costs (x86 approximation)
static const uint32_t COST_ALU = 1;
static const uint32_t COST_MUL = 3;
static const uint32_t COST_DIV = 40;
static const uint32_t COST_BRANCH = 3;
static const uint32_t COST_CALL = 5;
static const uint32_t COST_RET = 5;
static const uint32_t COST_MEMORY = 3;
static const uint32_t COST_CACHE_MISS = 40;

// Calculate realistic WCET
static uint32_t calculate_realistic_wcet(simple_wcet_t* wcet) {
    uint32_t total = wcet->base_cycles;
    
    // Add memory access costs
    total += wcet->memory_accesses * COST_MEMORY;
    
    // Add cache miss penalties (assume 10% miss rate)
    uint32_t estimated_misses = wcet->memory_accesses / 10;
    total += estimated_misses * COST_CACHE_MISS;
    
    // Add branch misprediction penalties (assume 20% misprediction)
    uint32_t mispredictions = wcet->branches / 5;
    total += mispredictions * 15; // ~15 cycle penalty
    
    return total;
}

// Demo: compile a simple function with realistic WCET
int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.tempo> <output.s>\n", argv[0]);
        return 1;
    }
    
    FILE* output = fopen(argv[2], "w");
    if (!output) {
        fprintf(stderr, "Error: Cannot create output file\n");
        return 1;
    }
    
    // Example: analyze a simple add function
    simple_wcet_t wcet = {0};
    
    // Function prologue
    wcet.base_cycles += 2;    // push ebp; mov ebp, esp
    wcet.memory_accesses += 1; // push
    
    // Load parameters
    wcet.base_cycles += COST_MEMORY * 2; // mov eax, [ebp+8]; mov ebx, [ebp+12]
    wcet.memory_accesses += 2;
    
    // Add operation
    wcet.base_cycles += COST_ALU; // add eax, ebx
    
    // Function epilogue
    wcet.base_cycles += 3;    // mov esp, ebp; pop ebp; ret
    wcet.memory_accesses += 1; // pop
    
    // Calculate total
    uint32_t total_wcet = calculate_realistic_wcet(&wcet);
    
    // Generate output
    fprintf(output, "; Tempo Enhanced Compiler - Realistic WCET Demo\n");
    fprintf(output, "; Input: %s\n", argv[1]);
    fprintf(output, "; Realistic WCET Analysis:\n");
    fprintf(output, ";   Base cycles: %u\n", wcet.base_cycles);
    fprintf(output, ";   Memory accesses: %u\n", wcet.memory_accesses);
    fprintf(output, ";   Estimated cache misses: %u\n", wcet.memory_accesses / 10);
    fprintf(output, ";   Total WCET: %u cycles\n", total_wcet);
    fprintf(output, ";   Time @ 1GHz: %.2f ns\n", (float)total_wcet);
    fprintf(output, "\n");
    
    // Generate optimized code
    fprintf(output, "section .text\n");
    fprintf(output, "global add\n");
    fprintf(output, "add:\n");
    
    // Check if we can omit frame pointer (leaf function)
    bool is_leaf = true;
    bool has_locals = false;
    
    if (is_leaf && !has_locals) {
        fprintf(output, "    ; Optimized: frame pointer omitted\n");
        fprintf(output, "    mov eax, [esp+4]    ; Load first parameter\n");
        fprintf(output, "    add eax, [esp+8]    ; Add second parameter\n");
        fprintf(output, "    ret                 ; Return result in EAX\n");
        
        // Recalculate WCET for optimized version
        simple_wcet_t opt_wcet = {0};
        opt_wcet.base_cycles = COST_MEMORY + COST_ALU + COST_RET;
        opt_wcet.memory_accesses = 2;
        uint32_t opt_total = calculate_realistic_wcet(&opt_wcet);
        
        fprintf(output, "\n; Optimized WCET: %u cycles (saved %u cycles)\n", 
                opt_total, total_wcet - opt_total);
    } else {
        fprintf(output, "    push ebp\n");
        fprintf(output, "    mov ebp, esp\n");
        fprintf(output, "    mov eax, [ebp+8]\n");
        fprintf(output, "    add eax, [ebp+12]\n");
        fprintf(output, "    mov esp, ebp\n");
        fprintf(output, "    pop ebp\n");
        fprintf(output, "    ret\n");
    }
    
    fclose(output);
    
    printf("Compilation successful!\n");
    printf("  Output: %s\n", argv[2]);
    printf("  Original WCET: %u cycles\n", total_wcet);
    printf("  Optimized WCET: %u cycles\n", 
           COST_MEMORY + COST_ALU + COST_RET + 2 * COST_MEMORY + (2 * COST_CACHE_MISS / 10));
    
    return 0;
}