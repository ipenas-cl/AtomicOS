// Tempo Debug Information Support
// Maps Tempo source code to generated assembly

#ifndef TEMPO_DEBUG_H
#define TEMPO_DEBUG_H

#include <stdint.h>
#include <stdbool.h>

// Debug information version
#define TEMPO_DEBUG_VERSION 1

// Source location information
typedef struct {
    const char* filename;
    uint32_t line;
    uint32_t column;
} source_location_t;

// Debug symbol types
typedef enum {
    SYMBOL_FUNCTION,
    SYMBOL_VARIABLE,
    SYMBOL_PARAMETER,
    SYMBOL_CONSTANT
} symbol_type_t;

// Debug symbol entry
typedef struct {
    const char* name;
    symbol_type_t type;
    source_location_t location;
    uint32_t asm_offset;      // Offset in generated assembly
    uint32_t size;            // Size in bytes
    uint32_t wcet_cycles;     // WCET for functions
} debug_symbol_t;

// Line mapping entry
typedef struct {
    uint32_t tempo_line;      // Line in Tempo source
    uint32_t asm_line;        // Line in generated assembly
    uint32_t asm_offset;      // Byte offset in assembly
    const char* tempo_code;   // Original Tempo code
    const char* asm_code;     // Generated assembly code
} line_mapping_t;

// WCET annotation
typedef struct {
    source_location_t location;
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint32_t measured_cycles; // From actual execution
    bool validated;
} wcet_annotation_t;

// Complete debug information for a Tempo file
typedef struct {
    uint32_t version;
    const char* tempo_filename;
    const char* asm_filename;
    uint32_t checksum;        // CRC32 of source file
    
    // Symbol table
    debug_symbol_t* symbols;
    uint32_t symbol_count;
    
    // Line mappings
    line_mapping_t* mappings;
    uint32_t mapping_count;
    
    // WCET annotations
    wcet_annotation_t* wcet_annotations;
    uint32_t wcet_count;
    
    // Compilation info
    const char* compiler_version;
    const char* compile_flags;
    uint64_t compile_timestamp;
} tempo_debug_info_t;

// Debug info file format (.tdb - Tempo Debug)
typedef struct {
    uint32_t magic;           // 'TDB\0'
    uint32_t version;
    uint32_t header_size;
    uint32_t total_size;
    
    // Offsets to sections
    uint32_t symbols_offset;
    uint32_t mappings_offset;
    uint32_t wcet_offset;
    uint32_t strings_offset;  // String table
} tempo_debug_header_t;

// API for debug information
tempo_debug_info_t* tempo_debug_create(const char* tempo_file, const char* asm_file);
void tempo_debug_add_symbol(tempo_debug_info_t* info, const debug_symbol_t* symbol);
void tempo_debug_add_mapping(tempo_debug_info_t* info, uint32_t tempo_line, 
                           uint32_t asm_line, const char* tempo_code);
void tempo_debug_add_wcet(tempo_debug_info_t* info, source_location_t loc,
                         uint32_t min_cycles, uint32_t max_cycles);

// Serialization
bool tempo_debug_write(const tempo_debug_info_t* info, const char* filename);
tempo_debug_info_t* tempo_debug_read(const char* filename);

// Query functions
const debug_symbol_t* tempo_debug_find_symbol(const tempo_debug_info_t* info, 
                                            const char* name);
uint32_t tempo_debug_get_tempo_line(const tempo_debug_info_t* info, 
                                   uint32_t asm_offset);
const char* tempo_debug_get_source_code(const tempo_debug_info_t* info, 
                                       uint32_t tempo_line);

// Cleanup
void tempo_debug_free(tempo_debug_info_t* info);

// Integration with GDB
void tempo_debug_generate_gdb_script(const tempo_debug_info_t* info, 
                                   const char* output_file);

#endif // TEMPO_DEBUG_H