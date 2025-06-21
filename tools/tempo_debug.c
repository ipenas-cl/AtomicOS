// Tempo Debug Information Implementation
// Enables source-level debugging of Tempo programs

#include "tempo_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TEMPO_DEBUG_MAGIC 0x54444200  // 'TDB\0'

// Internal structures
typedef struct {
    char* strings;           // String table
    uint32_t string_size;    // Current size
    uint32_t string_capacity; // Allocated size
} string_table_t;

struct tempo_debug_info {
    tempo_debug_header_t header;
    
    // Dynamic arrays
    debug_symbol_t* symbols;
    uint32_t symbol_count;
    uint32_t symbol_capacity;
    
    line_mapping_t* mappings;
    uint32_t mapping_count;
    uint32_t mapping_capacity;
    
    wcet_annotation_t* wcet_annotations;
    uint32_t wcet_count;
    uint32_t wcet_capacity;
    
    // String table for all strings
    string_table_t strings;
    
    // Metadata
    char* tempo_filename;
    char* asm_filename;
    char* compiler_version;
    char* compile_flags;
    uint64_t compile_timestamp;
    uint32_t checksum;
};

// String table management
static uint32_t add_string(string_table_t* table, const char* str) {
    if (!str) return 0;
    
    uint32_t len = strlen(str) + 1;
    
    // Grow if needed
    if (table->string_size + len > table->string_capacity) {
        table->string_capacity = (table->string_capacity + len) * 2;
        table->strings = realloc(table->strings, table->string_capacity);
    }
    
    // Add string
    uint32_t offset = table->string_size;
    memcpy(table->strings + offset, str, len);
    table->string_size += len;
    
    return offset;
}

static const char* get_string(const string_table_t* table, uint32_t offset) {
    if (offset >= table->string_size) return NULL;
    return table->strings + offset;
}

// CRC32 for checksum
static uint32_t crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    static const uint32_t table[256] = {
        // CRC32 lookup table (truncated for brevity)
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
        // ... full table would be here
    };
    
    for (size_t i = 0; i < length; i++) {
        crc = (crc >> 8) ^ table[(crc ^ data[i]) & 0xFF];
    }
    
    return crc ^ 0xFFFFFFFF;
}

// Create new debug info
tempo_debug_info_t* tempo_debug_create(const char* tempo_file, const char* asm_file) {
    tempo_debug_info_t* info = calloc(1, sizeof(tempo_debug_info_t));
    if (!info) return NULL;
    
    // Initialize header
    info->header.magic = TEMPO_DEBUG_MAGIC;
    info->header.version = TEMPO_DEBUG_VERSION;
    info->header.header_size = sizeof(tempo_debug_header_t);
    
    // Initialize dynamic arrays
    info->symbol_capacity = 16;
    info->symbols = calloc(info->symbol_capacity, sizeof(debug_symbol_t));
    
    info->mapping_capacity = 64;
    info->mappings = calloc(info->mapping_capacity, sizeof(line_mapping_t));
    
    info->wcet_capacity = 16;
    info->wcet_annotations = calloc(info->wcet_capacity, sizeof(wcet_annotation_t));
    
    // Initialize string table
    info->strings.string_capacity = 1024;
    info->strings.strings = calloc(1, info->strings.string_capacity);
    info->strings.string_size = 1; // Start at 1, 0 is null string
    
    // Store filenames
    info->tempo_filename = strdup(tempo_file);
    info->asm_filename = strdup(asm_file);
    
    // Set metadata
    info->compiler_version = strdup("Tempo v3 Enhanced");
    info->compile_timestamp = time(NULL);
    
    // Calculate source file checksum
    FILE* f = fopen(tempo_file, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        uint8_t* buffer = malloc(size);
        fread(buffer, 1, size, f);
        info->checksum = crc32(buffer, size);
        
        free(buffer);
        fclose(f);
    }
    
    return info;
}

// Add symbol
void tempo_debug_add_symbol(tempo_debug_info_t* info, const debug_symbol_t* symbol) {
    if (!info || !symbol) return;
    
    // Grow array if needed
    if (info->symbol_count >= info->symbol_capacity) {
        info->symbol_capacity *= 2;
        info->symbols = realloc(info->symbols, 
                               info->symbol_capacity * sizeof(debug_symbol_t));
    }
    
    // Copy symbol
    debug_symbol_t* dest = &info->symbols[info->symbol_count++];
    *dest = *symbol;
    
    // Intern strings
    dest->name = (const char*)add_string(&info->strings, symbol->name);
    if (symbol->location.filename) {
        dest->location.filename = (const char*)add_string(&info->strings, 
                                                         symbol->location.filename);
    }
}

// Add line mapping
void tempo_debug_add_mapping(tempo_debug_info_t* info, uint32_t tempo_line, 
                           uint32_t asm_line, const char* tempo_code) {
    if (!info) return;
    
    // Grow array if needed
    if (info->mapping_count >= info->mapping_capacity) {
        info->mapping_capacity *= 2;
        info->mappings = realloc(info->mappings,
                               info->mapping_capacity * sizeof(line_mapping_t));
    }
    
    // Add mapping
    line_mapping_t* mapping = &info->mappings[info->mapping_count++];
    mapping->tempo_line = tempo_line;
    mapping->asm_line = asm_line;
    mapping->asm_offset = asm_line * 16; // Estimate
    mapping->tempo_code = (const char*)add_string(&info->strings, tempo_code);
    mapping->asm_code = NULL; // Set later
}

// Add WCET annotation
void tempo_debug_add_wcet(tempo_debug_info_t* info, source_location_t loc,
                         uint32_t min_cycles, uint32_t max_cycles) {
    if (!info) return;
    
    // Grow array if needed
    if (info->wcet_count >= info->wcet_capacity) {
        info->wcet_capacity *= 2;
        info->wcet_annotations = realloc(info->wcet_annotations,
                                       info->wcet_capacity * sizeof(wcet_annotation_t));
    }
    
    // Add annotation
    wcet_annotation_t* wcet = &info->wcet_annotations[info->wcet_count++];
    wcet->location = loc;
    wcet->min_cycles = min_cycles;
    wcet->max_cycles = max_cycles;
    wcet->measured_cycles = 0;
    wcet->validated = false;
    
    if (loc.filename) {
        wcet->location.filename = (const char*)add_string(&info->strings, loc.filename);
    }
}

// Write debug info to file
bool tempo_debug_write(const tempo_debug_info_t* info, const char* filename) {
    if (!info || !filename) return false;
    
    FILE* f = fopen(filename, "wb");
    if (!f) return false;
    
    // Calculate offsets
    tempo_debug_header_t header = info->header;
    header.symbols_offset = sizeof(tempo_debug_header_t);
    header.mappings_offset = header.symbols_offset + 
                           (info->symbol_count * sizeof(debug_symbol_t));
    header.wcet_offset = header.mappings_offset + 
                        (info->mapping_count * sizeof(line_mapping_t));
    header.strings_offset = header.wcet_offset + 
                          (info->wcet_count * sizeof(wcet_annotation_t));
    header.total_size = header.strings_offset + info->strings.string_size;
    
    // Write header
    fwrite(&header, sizeof(header), 1, f);
    
    // Write symbols
    fwrite(info->symbols, sizeof(debug_symbol_t), info->symbol_count, f);
    
    // Write mappings
    fwrite(info->mappings, sizeof(line_mapping_t), info->mapping_count, f);
    
    // Write WCET annotations
    fwrite(info->wcet_annotations, sizeof(wcet_annotation_t), info->wcet_count, f);
    
    // Write string table
    fwrite(info->strings.strings, 1, info->strings.string_size, f);
    
    fclose(f);
    return true;
}

// Read debug info from file
tempo_debug_info_t* tempo_debug_read(const char* filename) {
    if (!filename) return NULL;
    
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    
    tempo_debug_info_t* info = calloc(1, sizeof(tempo_debug_info_t));
    
    // Read header
    fread(&info->header, sizeof(tempo_debug_header_t), 1, f);
    
    if (info->header.magic != TEMPO_DEBUG_MAGIC) {
        free(info);
        fclose(f);
        return NULL;
    }
    
    // Calculate counts
    info->symbol_count = (info->header.mappings_offset - info->header.symbols_offset) / 
                        sizeof(debug_symbol_t);
    info->mapping_count = (info->header.wcet_offset - info->header.mappings_offset) / 
                         sizeof(line_mapping_t);
    info->wcet_count = (info->header.strings_offset - info->header.wcet_offset) / 
                      sizeof(wcet_annotation_t);
    info->strings.string_size = info->header.total_size - info->header.strings_offset;
    
    // Allocate arrays
    info->symbols = calloc(info->symbol_count, sizeof(debug_symbol_t));
    info->mappings = calloc(info->mapping_count, sizeof(line_mapping_t));
    info->wcet_annotations = calloc(info->wcet_count, sizeof(wcet_annotation_t));
    info->strings.strings = calloc(1, info->strings.string_size);
    
    // Read data
    fseek(f, info->header.symbols_offset, SEEK_SET);
    fread(info->symbols, sizeof(debug_symbol_t), info->symbol_count, f);
    
    fseek(f, info->header.mappings_offset, SEEK_SET);
    fread(info->mappings, sizeof(line_mapping_t), info->mapping_count, f);
    
    fseek(f, info->header.wcet_offset, SEEK_SET);
    fread(info->wcet_annotations, sizeof(wcet_annotation_t), info->wcet_count, f);
    
    fseek(f, info->header.strings_offset, SEEK_SET);
    fread(info->strings.strings, 1, info->strings.string_size, f);
    
    fclose(f);
    return info;
}

// Find symbol by name
const debug_symbol_t* tempo_debug_find_symbol(const tempo_debug_info_t* info, 
                                            const char* name) {
    if (!info || !name) return NULL;
    
    for (uint32_t i = 0; i < info->symbol_count; i++) {
        const char* sym_name = get_string(&info->strings, (uint32_t)(uintptr_t)info->symbols[i].name);
        if (sym_name && strcmp(sym_name, name) == 0) {
            return &info->symbols[i];
        }
    }
    
    return NULL;
}

// Get Tempo line from assembly offset
uint32_t tempo_debug_get_tempo_line(const tempo_debug_info_t* info, 
                                   uint32_t asm_offset) {
    if (!info) return 0;
    
    // Find closest mapping
    uint32_t best_line = 0;
    uint32_t best_distance = UINT32_MAX;
    
    for (uint32_t i = 0; i < info->mapping_count; i++) {
        if (info->mappings[i].asm_offset <= asm_offset) {
            uint32_t distance = asm_offset - info->mappings[i].asm_offset;
            if (distance < best_distance) {
                best_distance = distance;
                best_line = info->mappings[i].tempo_line;
            }
        }
    }
    
    return best_line;
}

// Get source code for line
const char* tempo_debug_get_source_code(const tempo_debug_info_t* info, 
                                       uint32_t tempo_line) {
    if (!info) return NULL;
    
    for (uint32_t i = 0; i < info->mapping_count; i++) {
        if (info->mappings[i].tempo_line == tempo_line) {
            return get_string(&info->strings, (uint32_t)(uintptr_t)info->mappings[i].tempo_code);
        }
    }
    
    return NULL;
}

// Generate GDB helper script
void tempo_debug_generate_gdb_script(const tempo_debug_info_t* info, 
                                   const char* output_file) {
    if (!info || !output_file) return;
    
    FILE* f = fopen(output_file, "w");
    if (!f) return;
    
    fprintf(f, "# GDB script for debugging Tempo program\n");
    fprintf(f, "# Generated by Tempo Debug System\n\n");
    
    fprintf(f, "# Load symbol mappings\n");
    for (uint32_t i = 0; i < info->symbol_count; i++) {
        const char* name = get_string(&info->strings, (uint32_t)(uintptr_t)info->symbols[i].name);
        if (name && info->symbols[i].type == SYMBOL_FUNCTION) {
            fprintf(f, "# Function: %s (WCET: %u cycles)\n", 
                    name, info->symbols[i].wcet_cycles);
        }
    }
    
    fprintf(f, "\n# Breakpoint commands\n");
    fprintf(f, "define tempo-break\n");
    fprintf(f, "  if $argc != 1\n");
    fprintf(f, "    printf \"Usage: tempo-break <line>\\n\"\n");
    fprintf(f, "  else\n");
    fprintf(f, "    # Set breakpoint at Tempo line $arg0\n");
    
    // Generate line-to-address mappings
    for (uint32_t i = 0; i < info->mapping_count; i++) {
        fprintf(f, "    if $arg0 == %u\n", info->mappings[i].tempo_line);
        fprintf(f, "      break *0x%x\n", info->mappings[i].asm_offset);
        fprintf(f, "    end\n");
    }
    
    fprintf(f, "  end\n");
    fprintf(f, "end\n\n");
    
    fprintf(f, "# Show Tempo source\n");
    fprintf(f, "define tempo-list\n");
    fprintf(f, "  python\n");
    fprintf(f, "import gdb\n");
    fprintf(f, "pc = gdb.selected_frame().pc()\n");
    fprintf(f, "# Map PC to Tempo line and display source\n");
    fprintf(f, "  end\n");
    fprintf(f, "end\n\n");
    
    fprintf(f, "# WCET information\n");
    fprintf(f, "define tempo-wcet\n");
    fprintf(f, "  printf \"WCET Analysis:\\n\"\n");
    for (uint32_t i = 0; i < info->wcet_count; i++) {
        fprintf(f, "  printf \"  Line %u: %u-%u cycles\\n\"\n",
                info->wcet_annotations[i].location.line,
                info->wcet_annotations[i].min_cycles,
                info->wcet_annotations[i].max_cycles);
    }
    fprintf(f, "end\n");
    
    fclose(f);
}

// Free debug info
void tempo_debug_free(tempo_debug_info_t* info) {
    if (!info) return;
    
    free(info->symbols);
    free(info->mappings);
    free(info->wcet_annotations);
    free(info->strings.strings);
    free(info->tempo_filename);
    free(info->asm_filename);
    free(info->compiler_version);
    free(info->compile_flags);
    free(info);
}