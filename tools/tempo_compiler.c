/*
 * Copyright (c) 2024 Ignacio Peña
 * Part of AtomicOS Project - https://github.com/ipenas-cl/AtomicOS
 * Licensed under MIT License - see LICENSE file for details
 *
 * Tempo Compiler v1.2.2 - Complete Systems Programming Language
 * Deterministic Programming Language for AtomicOS
 * Features: Structs, Pointers, Inline Assembly, WCET analysis, Full type system
 * 
 *  ████████╗███████╗███╗   ███╗██████╗  ██████╗ 
 *  ╚══██╔══╝██╔════╝████╗ ████║██╔══██╗██╔═══██╗
 *     ██║   █████╗  ██╔████╔██║██████╔╝██║   ██║
 *     ██║   ██╔══╝  ██║╚██╔╝██║██╔═══╝ ██║   ██║
 *     ██║   ███████╗██║ ╚═╝ ██║██║     ╚██████╔╝
 *     ╚═╝   ╚══════╝╚═╝     ╚═╝╚═╝      ╚═════╝ 
 * 
 *  No more C needed - Everything in Tempo!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

// Token types for Tempo v1.0 - Complete language
typedef enum {
    TOK_EOF = 0, TOK_NUMBER, TOK_IDENT, TOK_STRING, TOK_CHAR,
    
    // Keywords
    TOK_FUNCTION, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_MATCH,
    TOK_LOOP, TOK_FROM, TOK_TO, TOK_WCET, TOK_DEADLINE, TOK_LET, TOK_RETURN,
    TOK_BREAK, TOK_CONTINUE, TOK_CONST, TOK_STATIC, TOK_PUB,
    
    // Types
    TOK_INT8, TOK_INT16, TOK_INT32, TOK_INT64, TOK_BOOL, TOK_VOID,
    TOK_TRUE, TOK_FALSE, TOK_NULL,
    
    // Struct/Union/Enum
    TOK_STRUCT, TOK_UNION, TOK_ENUM, TOK_TYPE,
    
    // Pointers
    TOK_PTR, TOK_RAW_PTR, TOK_REF, TOK_MUT,
    
    // Operators
    TOK_PLUS, TOK_MINUS, TOK_MULT, TOK_DIV, TOK_MOD, TOK_ASSIGN,
    TOK_EQ, TOK_NE, TOK_LT, TOK_GT, TOK_LE, TOK_GE, TOK_AND, TOK_OR, TOK_NOT,
    TOK_BITAND, TOK_BITOR, TOK_BITXOR, TOK_BITNOT, TOK_LSHIFT, TOK_RSHIFT,
    TOK_DOT, TOK_ARROW, TOK_DOUBLE_COLON,
    
    // Delimiters
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET,
    TOK_SEMICOLON, TOK_COLON, TOK_COMMA, TOK_AT, TOK_QUESTION,
    
    // Module system
    TOK_MODULE, TOK_IMPORT, TOK_EXPORT, TOK_USE,
    
    // Inline assembly
    TOK_ASM, TOK_VOLATILE,
    
    // Security & Real-time
    TOK_PLEDGE, TOK_UNVEIL, TOK_SECURITY, TOK_REALTIME, TOK_ATOMIC,
    TOK_CONSTANT_TIME, TOK_TRUSTED, TOK_INTERRUPT, TOK_PRIORITY,
    
    // Annotations
    TOK_PACKED, TOK_REPR, TOK_ALIGN, TOK_INLINE, TOK_NO_INLINE,
    TOK_MUST_USE, TOK_DEPRECATED
} token_type_t;

typedef struct {
    token_type_t type;
    char* value;
    int line, col;
} token_t;

// AST node types for Tempo v1.0
typedef enum {
    // Basic nodes
    AST_PROGRAM, AST_MODULE, AST_FUNCTION, AST_BLOCK, AST_IF, AST_WHILE, AST_FOR,
    AST_LET, AST_RETURN, AST_BREAK, AST_CONTINUE, AST_MATCH, AST_MATCH_ARM,
    
    // Expressions
    AST_BINARY_OP, AST_UNARY_OP, AST_NUMBER, AST_STRING, AST_CHAR, AST_BOOL,
    AST_IDENT, AST_CALL, AST_INDEX, AST_FIELD_ACCESS, AST_CAST, AST_SIZEOF,
    
    // Types
    AST_TYPE_NAME, AST_POINTER_TYPE, AST_ARRAY_TYPE, AST_STRUCT_TYPE,
    AST_UNION_TYPE, AST_ENUM_TYPE, AST_FUNCTION_TYPE,
    
    // Declarations
    AST_STRUCT_DECL, AST_UNION_DECL, AST_ENUM_DECL, AST_TYPE_ALIAS,
    AST_FIELD, AST_VARIANT, AST_PARAM,
    
    // Inline assembly
    AST_ASM_BLOCK, AST_ASM_OPERAND,
    
    // Literals
    AST_STRUCT_LITERAL, AST_ARRAY_LITERAL,
    
    // Security & Real-time
    AST_PLEDGE_BLOCK, AST_SECURITY_ANNOTATION, AST_REALTIME_TASK,
    AST_CONSTANT_TIME_BLOCK, AST_TRUSTED_FUNCTION, AST_INTERRUPT_HANDLER,
    AST_WCET_BOUND, AST_ATOMIC_SECTION,
    
    // Module system
    AST_IMPORT, AST_EXPORT, AST_USE
} ast_type_t;

// Type representation
typedef struct type_info {
    enum {
        TYPE_INT8, TYPE_INT16, TYPE_INT32, TYPE_INT64,
        TYPE_BOOL, TYPE_VOID, TYPE_POINTER, TYPE_ARRAY,
        TYPE_STRUCT, TYPE_UNION, TYPE_ENUM, TYPE_FUNCTION
    } kind;
    
    union {
        struct { struct type_info* base_type; int is_raw; } pointer;
        struct { struct type_info* elem_type; int size; } array;
        struct { char* name; struct field_info* fields; int field_count; } struct_type;
        struct { struct type_info* return_type; struct type_info** param_types; int param_count; } func_type;
    };
} type_info_t;

typedef struct field_info {
    char* name;
    type_info_t* type;
    int offset;
} field_info_t;

// Complete AST node structure for Tempo v1.0
typedef struct ast_node {
    ast_type_t type;
    type_info_t* data_type;   // Type information
    int wcet_cycles;          // WCET analysis result
    int security_level;       // Security classification
    int is_constant_time;     // Constant-time guarantee flag
    int line, col;            // Source location
    
    union {
        // Literals
        long num_value;
        char* str_value;
        char char_value;
        int bool_value;
        
        // Expressions
        struct { struct ast_node* left; struct ast_node* right; token_type_t op; } binary;
        struct { struct ast_node* operand; token_type_t op; } unary;
        struct { struct ast_node* array; struct ast_node* index; } index;
        struct { struct ast_node* object; char* field; } field_access;
        struct { struct ast_node* expr; type_info_t* target_type; } cast;
        struct { type_info_t* type; } sizeof_expr;
        
        // Declarations
        struct { 
            char* name; 
            struct ast_node** params; 
            int param_count;
            type_info_t* return_type;
            struct ast_node* body; 
            int wcet_bound;
            int security_level;
            int is_public;
            int is_inline;
            int is_trusted;
            int is_constant_time;
        } function;
        
        struct {
            char* name;
            struct field_info* fields;
            int field_count;
            int is_packed;
            int alignment;
        } struct_decl;
        
        struct {
            char* name;
            type_info_t* type;
            struct ast_node* init;
            int is_const;
            int is_static;
        } var_decl;
        
        // Statements
        struct { char* name; struct ast_node** args; int arg_count; } call;
        struct { struct ast_node* condition; struct ast_node* then_branch; struct ast_node* else_branch; } if_stmt;
        struct { struct ast_node* condition; struct ast_node* body; int max_iterations; } while_stmt;
        struct { struct ast_node* init; struct ast_node* cond; struct ast_node* update; struct ast_node* body; int max_iter; } for_stmt;
        struct { struct ast_node* value; } return_stmt;
        struct { struct ast_node** statements; int stmt_count; } block;
        struct { struct ast_node* expr; struct ast_node** arms; int arm_count; } match_expr;
        
        // Inline assembly
        struct {
            char* code;
            struct ast_node** outputs;
            struct ast_node** inputs;
            char** clobbers;
            int output_count;
            int input_count;
            int clobber_count;
            int is_volatile;
        } asm_block;
        
        // Module system
        struct { char* name; struct ast_node** items; int item_count; } module;
        struct { char* module_path; char** symbols; int symbol_count; } import;
        
        // Pointers
        struct { struct ast_node* expr; int64_t bounds; } ptr_expr;
        
        // Literals
        struct {
            type_info_t* type;
            struct { char* field; struct ast_node* value; }* fields;
            int field_count;
        } struct_literal;
        
        struct {
            struct ast_node** elements;
            int element_count;
        } array_literal;
        
        // Security & Real-time
        struct { 
            int promises;         // Pledge promise flags
            struct ast_node* body;
        } pledge;
        
        struct {
            int level;            // Security level (0-3)
            char* classification; // Security classification string
            struct ast_node* body;
        } security;
        
        struct {
            int priority;         // RT priority (1-255)
            int period_us;        // Period in microseconds
            int deadline_us;      // Deadline in microseconds
            struct ast_node* body;
        } realtime;
        
        struct {
            int max_cycles;       // Maximum execution cycles
            struct ast_node* body;
        } constant_time;
    } data;
} ast_node_t;

// Compiler state
static const char* source;
static int source_len, pos, line, col, token_count, token_pos;
static token_t* tokens;

// WCET analysis data
static int total_cycles = 0;
static int max_function_cycles = 0;

// Security analysis data
static int current_security_level = 0;
static int trusted_function_count = 0;

// Symbol table for variable tracking
typedef struct symbol {
    char* name;
    int offset;
    type_info_t* type;
    int scope_level;
    struct symbol* next;
} symbol_t;

static symbol_t* symbol_table = NULL;
static int current_scope_level = 0;
static int current_stack_offset = 0;

// Symbol table functions
static void push_symbol(const char* name, int offset, type_info_t* type) {
    symbol_t* sym = malloc(sizeof(symbol_t));
    sym->name = strdup(name);
    sym->offset = offset;
    sym->type = type;
    sym->scope_level = current_scope_level;
    sym->next = symbol_table;
    symbol_table = sym;
}

static symbol_t* find_symbol(const char* name) {
    symbol_t* sym = symbol_table;
    while (sym) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

static void enter_scope() {
    current_scope_level++;
}

static void exit_scope() {
    // Remove symbols from current scope
    while (symbol_table && symbol_table->scope_level == current_scope_level) {
        symbol_t* temp = symbol_table;
        symbol_table = symbol_table->next;
        free(temp->name);
        free(temp);
    }
    current_scope_level--;
}

// Error handling
static void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Tempo Compiler Error [%d:%d]: ", line, col);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    fprintf(stderr, "Debug: token_pos=%d, token_count=%d\n", token_pos, token_count);
    if (token_pos < token_count) {
        fprintf(stderr, "Current token: type=%d, value='%s'\n", 
                tokens[token_pos].type, tokens[token_pos].value ? tokens[token_pos].value : "(null)");
    }
    va_end(args);
    exit(1);
}

// Lexer functions
static void advance() {
    if (pos < source_len) {
        if (source[pos] == '\n') { line++; col = 1; } else { col++; }
        pos++;
    }
}

static char peek() { return pos < source_len ? source[pos] : '\0'; }

static void skip_whitespace() {
    while (pos < source_len && isspace(source[pos])) advance();
}

static token_t make_token(token_type_t type, const char* value) {
    token_t token = {type, value ? strdup(value) : NULL, line, col};
    return token;
}

// Extended tokenizer for v3.0
static void tokenize() {
    tokens = malloc(2000 * sizeof(token_t));  // Increased capacity
    token_count = 0;
    
    while (pos < source_len) {
        skip_whitespace();
        if (pos >= source_len) break;
        
        char c = peek();
        
        // Handle comments
        if (c == '/' && pos + 1 < source_len && source[pos + 1] == '/') {
            while (pos < source_len && source[pos] != '\n') advance();
            continue;
        }
        
        if (isdigit(c)) {
            char buffer[32] = {0}; 
            int i = 0;
            buffer[i++] = c;  // Add the first digit
            advance(); // Move past the first digit
            
            // Check for hex numbers (0x prefix)
            if (c == '0' && (peek() == 'x' || peek() == 'X')) {
                buffer[i++] = peek(); advance(); // 'x' or 'X'
                
                // Parse hex digits
                while ((isdigit(peek()) || (peek() >= 'a' && peek() <= 'f') || 
                        (peek() >= 'A' && peek() <= 'F')) && i < 31) {
                    buffer[i++] = peek(); 
                    advance();
                }
            } else {
                // Regular decimal number
                while (isdigit(peek()) && i < 31) { 
                    buffer[i++] = peek(); 
                    advance(); 
                }
            }
            buffer[i] = '\0';
            tokens[token_count++] = make_token(TOK_NUMBER, buffer);
        } else if (isalpha(c) || c == '_') {
            char buffer[64]; int i = 0;  // Increased buffer size
            while ((isalnum(peek()) || peek() == '_') && i < 63) {
                buffer[i++] = peek(); advance();
            }
            buffer[i] = '\0';
            
            // Complete keyword recognition for Tempo v1.0
            token_type_t type = TOK_IDENT;
            // Control flow
            if (strcmp(buffer, "function") == 0) type = TOK_FUNCTION;
            else if (strcmp(buffer, "if") == 0) type = TOK_IF;
            else if (strcmp(buffer, "else") == 0) type = TOK_ELSE;
            else if (strcmp(buffer, "while") == 0) type = TOK_WHILE;
            else if (strcmp(buffer, "for") == 0) type = TOK_FOR;
            else if (strcmp(buffer, "match") == 0) type = TOK_MATCH;
            else if (strcmp(buffer, "break") == 0) type = TOK_BREAK;
            else if (strcmp(buffer, "continue") == 0) type = TOK_CONTINUE;
            else if (strcmp(buffer, "return") == 0) type = TOK_RETURN;
            
            // Declarations
            else if (strcmp(buffer, "let") == 0) type = TOK_LET;
            else if (strcmp(buffer, "const") == 0) type = TOK_CONST;
            else if (strcmp(buffer, "static") == 0) type = TOK_STATIC;
            else if (strcmp(buffer, "pub") == 0) type = TOK_PUB;
            
            // Types
            else if (strcmp(buffer, "int8") == 0) type = TOK_INT8;
            else if (strcmp(buffer, "int16") == 0) type = TOK_INT16;
            else if (strcmp(buffer, "int32") == 0) type = TOK_INT32;
            else if (strcmp(buffer, "int64") == 0) type = TOK_INT64;
            else if (strcmp(buffer, "bool") == 0) type = TOK_BOOL;
            else if (strcmp(buffer, "void") == 0) type = TOK_VOID;
            else if (strcmp(buffer, "true") == 0) type = TOK_TRUE;
            else if (strcmp(buffer, "false") == 0) type = TOK_FALSE;
            else if (strcmp(buffer, "null") == 0) type = TOK_NULL;
            
            // Struct/Union/Enum
            else if (strcmp(buffer, "struct") == 0) type = TOK_STRUCT;
            else if (strcmp(buffer, "union") == 0) type = TOK_UNION;
            else if (strcmp(buffer, "enum") == 0) type = TOK_ENUM;
            else if (strcmp(buffer, "type") == 0) type = TOK_TYPE;
            
            // Pointers
            else if (strcmp(buffer, "ptr") == 0) type = TOK_PTR;
            else if (strcmp(buffer, "raw_ptr") == 0) type = TOK_RAW_PTR;
            else if (strcmp(buffer, "ref") == 0) type = TOK_REF;
            else if (strcmp(buffer, "mut") == 0) type = TOK_MUT;
            
            // Module system
            else if (strcmp(buffer, "module") == 0) type = TOK_MODULE;
            else if (strcmp(buffer, "import") == 0) type = TOK_IMPORT;
            else if (strcmp(buffer, "export") == 0) type = TOK_EXPORT;
            else if (strcmp(buffer, "use") == 0) type = TOK_USE;
            
            // Inline assembly
            else if (strcmp(buffer, "asm") == 0) type = TOK_ASM;
            else if (strcmp(buffer, "volatile") == 0) type = TOK_VOLATILE;
            
            // Security & Real-time
            else if (strcmp(buffer, "pledge") == 0) type = TOK_PLEDGE;
            else if (strcmp(buffer, "unveil") == 0) type = TOK_UNVEIL;
            else if (strcmp(buffer, "security") == 0) type = TOK_SECURITY;
            else if (strcmp(buffer, "realtime") == 0) type = TOK_REALTIME;
            else if (strcmp(buffer, "atomic") == 0) type = TOK_ATOMIC;
            else if (strcmp(buffer, "constant_time") == 0) type = TOK_CONSTANT_TIME;
            else if (strcmp(buffer, "trusted") == 0) type = TOK_TRUSTED;
            else if (strcmp(buffer, "interrupt") == 0) type = TOK_INTERRUPT;
            else if (strcmp(buffer, "priority") == 0) type = TOK_PRIORITY;
            else if (strcmp(buffer, "wcet") == 0) type = TOK_WCET;
            
            tokens[token_count++] = make_token(type, buffer);
        } else if (c == '"') {
            // String literals
            advance(); // skip opening quote
            char buffer[256]; int i = 0;
            while (peek() != '"' && peek() != '\0' && i < 255) {
                if (peek() == '\\') {
                    advance();
                    switch(peek()) {
                        case 'n': buffer[i++] = '\n'; break;
                        case 't': buffer[i++] = '\t'; break;
                        case 'r': buffer[i++] = '\r'; break;
                        case '\\': buffer[i++] = '\\'; break;
                        case '"': buffer[i++] = '"'; break;
                        default: buffer[i++] = peek();
                    }
                    advance();
                } else {
                    buffer[i++] = peek();
                    advance();
                }
            }
            buffer[i] = '\0';
            if (peek() == '"') advance();
            tokens[token_count++] = make_token(TOK_STRING, buffer);
        } else if (c == '\'') {
            // Character literals
            advance(); // skip opening quote
            char ch;
            if (peek() == '\\') {
                advance();
                switch(peek()) {
                    case 'n': ch = '\n'; break;
                    case 't': ch = '\t'; break;
                    case 'r': ch = '\r'; break;
                    case '\\': ch = '\\'; break;
                    case '\'': ch = '\''; break;
                    default: ch = peek();
                }
                advance();
            } else {
                ch = peek();
                advance();
            }
            if (peek() == '\'') advance();
            char buffer[2] = {ch, '\0'};
            tokens[token_count++] = make_token(TOK_CHAR, buffer);
        } else {
            switch (c) {
                case '+': advance(); tokens[token_count++] = make_token(TOK_PLUS, "+"); break;
                case '-': 
                    advance();
                    if (peek() == '>') {
                        advance();
                        tokens[token_count++] = make_token(TOK_ARROW, "->");
                    } else {
                        tokens[token_count++] = make_token(TOK_MINUS, "-");
                    }
                    break;
                case '*': advance(); tokens[token_count++] = make_token(TOK_MULT, "*"); break;
                case '/': advance(); tokens[token_count++] = make_token(TOK_DIV, "/"); break;
                case '%': advance(); tokens[token_count++] = make_token(TOK_MOD, "%"); break;
                case '(': advance(); tokens[token_count++] = make_token(TOK_LPAREN, "("); break;
                case ')': advance(); tokens[token_count++] = make_token(TOK_RPAREN, ")"); break;
                case '{': advance(); tokens[token_count++] = make_token(TOK_LBRACE, "{"); break;
                case '}': advance(); tokens[token_count++] = make_token(TOK_RBRACE, "}"); break;
                case '[': advance(); tokens[token_count++] = make_token(TOK_LBRACKET, "["); break;
                case ']': advance(); tokens[token_count++] = make_token(TOK_RBRACKET, "]"); break;
                case ':': 
                    advance();
                    if (peek() == ':') {
                        advance();
                        tokens[token_count++] = make_token(TOK_DOUBLE_COLON, "::");
                    } else {
                        tokens[token_count++] = make_token(TOK_COLON, ":");
                    }
                    break;
                case ';': advance(); tokens[token_count++] = make_token(TOK_SEMICOLON, ";"); break;
                case ',': advance(); tokens[token_count++] = make_token(TOK_COMMA, ","); break;
                case '.': advance(); tokens[token_count++] = make_token(TOK_DOT, "."); break;
                case '?': advance(); tokens[token_count++] = make_token(TOK_QUESTION, "?"); break;
                case '@': advance(); tokens[token_count++] = make_token(TOK_AT, "@"); break;
                case '=': 
                    advance();
                    if (peek() == '=') {
                        advance();
                        tokens[token_count++] = make_token(TOK_EQ, "==");
                    } else {
                        tokens[token_count++] = make_token(TOK_ASSIGN, "=");
                    }
                    break;
                case '!': 
                    advance();
                    if (peek() == '=') {
                        advance();
                        tokens[token_count++] = make_token(TOK_NE, "!=");
                    } else {
                        tokens[token_count++] = make_token(TOK_NOT, "!");
                    }
                    break;
                case '&': 
                    advance();
                    if (peek() == '&') {
                        advance();
                        tokens[token_count++] = make_token(TOK_AND, "&&");
                    } else {
                        tokens[token_count++] = make_token(TOK_BITAND, "&");
                    }
                    break;
                case '|': 
                    advance();
                    if (peek() == '|') {
                        advance();
                        tokens[token_count++] = make_token(TOK_OR, "||");
                    } else {
                        tokens[token_count++] = make_token(TOK_BITOR, "|");
                    }
                    break;
                case '^': advance(); tokens[token_count++] = make_token(TOK_BITXOR, "^"); break;
                case '~': advance(); tokens[token_count++] = make_token(TOK_BITNOT, "~"); break;
                case '<': 
                    advance();
                    if (peek() == '<') {
                        advance();
                        tokens[token_count++] = make_token(TOK_LSHIFT, "<<");
                    } else if (peek() == '=') {
                        advance();
                        tokens[token_count++] = make_token(TOK_LE, "<=");
                    } else {
                        tokens[token_count++] = make_token(TOK_LT, "<");
                    }
                    break;
                case '>':
                    advance();
                    if (peek() == '>') {
                        advance();
                        tokens[token_count++] = make_token(TOK_RSHIFT, ">>");
                    } else if (peek() == '=') {
                        advance();
                        tokens[token_count++] = make_token(TOK_GE, ">=");
                    } else {
                        tokens[token_count++] = make_token(TOK_GT, ">");
                    }
                    break;
                default: advance(); break;
            }
        }
    }
    tokens[token_count++] = make_token(TOK_EOF, NULL);
}

// Parser utilities
static token_t* peek_token() { return token_pos < token_count ? &tokens[token_pos] : NULL; }
static token_t* advance_token() { return token_pos < token_count ? &tokens[token_pos++] : NULL; }

static int match(token_type_t type) {
    if (peek_token() && peek_token()->type == type) {
        advance_token(); return 1;
    }
    return 0;
}

static ast_node_t* create_node(ast_type_t type) {
    ast_node_t* node = calloc(1, sizeof(ast_node_t));
    node->type = type;
    node->wcet_cycles = 1;  // Default: 1 cycle
    node->security_level = 0;  // Default: lowest security
    node->is_constant_time = 0;  // Default: not constant-time
    return node;
}

// Forward declarations for parser
static ast_node_t* parse_expression();
static ast_node_t* parse_statement();
static ast_node_t* parse_block();
static ast_node_t* parse_type();
static ast_node_t* parse_function();
static ast_node_t* parse_struct();
static ast_node_t* parse_module();

// Type parsing for complete type system
static type_info_t* parse_type_spec() {
    type_info_t* type = calloc(1, sizeof(type_info_t));
    
    // Handle pointer types
    if (match(TOK_PTR)) {
        type->kind = TYPE_POINTER;
        if (!match(TOK_LT)) error("Expected '<' after 'ptr'");
        type->pointer.base_type = parse_type_spec();
        type->pointer.is_raw = 0;
        if (match(TOK_COMMA)) {
            // Bounded pointer with size
            if (!match(TOK_NUMBER)) error("Expected size for bounded pointer");
            // Store bounds info
        }
        if (!match(TOK_GT)) error("Expected '>' to close pointer type");
    }
    else if (match(TOK_RAW_PTR)) {
        type->kind = TYPE_POINTER;
        if (!match(TOK_LT)) error("Expected '<' after 'raw_ptr'");
        type->pointer.base_type = parse_type_spec();
        type->pointer.is_raw = 1;
        if (!match(TOK_GT)) error("Expected '>' to close raw pointer type");
    }
    // Array types
    else if (match(TOK_LBRACKET)) {
        type->kind = TYPE_ARRAY;
        if (!match(TOK_NUMBER)) error("Expected array size");
        type->array.size = atoi(tokens[token_pos-1].value);
        if (!match(TOK_RBRACKET)) error("Expected ']'");
        type->array.elem_type = parse_type_spec();
    }
    // Basic types
    else if (match(TOK_INT8)) type->kind = TYPE_INT8;
    else if (match(TOK_INT16)) type->kind = TYPE_INT16;
    else if (match(TOK_INT32)) type->kind = TYPE_INT32;
    else if (match(TOK_INT64)) type->kind = TYPE_INT64;
    else if (match(TOK_BOOL)) type->kind = TYPE_BOOL;
    else if (match(TOK_VOID)) type->kind = TYPE_VOID;
    else if (match(TOK_IDENT)) {
        // User-defined type (struct/union/enum)
        type->kind = TYPE_STRUCT;
        type->struct_type.name = strdup(tokens[token_pos-1].value);
    }
    else {
        error("Expected type specifier");
    }
    
    return type;
}

// Parse struct declaration
static ast_node_t* parse_struct() {
    ast_node_t* node = create_node(AST_STRUCT_DECL);
    
    if (!match(TOK_IDENT)) error("Expected struct name");
    node->data.struct_decl.name = strdup(tokens[token_pos-1].value);
    
    // Handle attributes
    while (match(TOK_AT)) {
        if (match(TOK_PACKED)) {
            node->data.struct_decl.is_packed = 1;
        } else if (match(TOK_ALIGN)) {
            if (!match(TOK_LPAREN)) error("Expected '(' after align");
            if (!match(TOK_NUMBER)) error("Expected alignment value");
            node->data.struct_decl.alignment = atoi(tokens[token_pos-1].value);
            if (!match(TOK_RPAREN)) error("Expected ')'");
        }
    }
    
    if (!match(TOK_LBRACE)) error("Expected '{' after struct name");
    
    // Parse fields
    node->data.struct_decl.fields = malloc(100 * sizeof(field_info_t));
    node->data.struct_decl.field_count = 0;
    
    while (!match(TOK_RBRACE)) {
        field_info_t* field = &node->data.struct_decl.fields[node->data.struct_decl.field_count++];
        
        if (!match(TOK_IDENT)) error("Expected field name");
        field->name = strdup(tokens[token_pos-1].value);
        
        if (!match(TOK_COLON)) error("Expected ':' after field name");
        field->type = parse_type_spec();
        
        if (!match(TOK_COMMA) && peek_token()->type != TOK_RBRACE) {
            error("Expected ',' or '}' after field");
        }
    }
    
    return node;
}

// Parse inline assembly
static ast_node_t* parse_asm() {
    ast_node_t* node = create_node(AST_ASM_BLOCK);
    
    if (match(TOK_VOLATILE)) {
        node->data.asm_block.is_volatile = 1;
    }
    
    if (!match(TOK_LBRACE)) error("Expected '{' after asm");
    
    // Concatenate all strings until we hit '}'
    char asm_code[1024] = "";
    int code_len = 0;
    
    while (!match(TOK_RBRACE)) {
        if (match(TOK_STRING)) {
            char* str = tokens[token_pos-1].value;
            int len = strlen(str);
            if (code_len + len + 2 < 1024) {
                if (code_len > 0) {
                    strcat(asm_code, "\n");
                    code_len++;
                }
                strcat(asm_code, str);
                code_len += len;
            }
        } else {
            error("Expected string or '}' in asm block");
        }
    }
    
    node->data.asm_block.code = strdup(asm_code);
    
    return node;
}

// Parse primary expression
static ast_node_t* parse_primary() {
    ast_node_t* node = NULL;
    
    if (match(TOK_NUMBER)) {
        node = create_node(AST_NUMBER);
        char* num_str = tokens[token_pos-1].value;
        
        // Check if it's a hex number
        if (num_str[0] == '0' && (num_str[1] == 'x' || num_str[1] == 'X')) {
            // Parse hexadecimal
            node->data.num_value = strtol(num_str, NULL, 16);
        } else {
            // Parse decimal
            node->data.num_value = atol(num_str);
        }
    }
    else if (match(TOK_STRING)) {
        node = create_node(AST_STRING);
        node->data.str_value = strdup(tokens[token_pos-1].value);
    }
    else if (match(TOK_CHAR)) {
        node = create_node(AST_CHAR);
        node->data.char_value = tokens[token_pos-1].value[0];
    }
    else if (match(TOK_TRUE) || match(TOK_FALSE)) {
        node = create_node(AST_BOOL);
        node->data.bool_value = (tokens[token_pos-1].type == TOK_TRUE);
    }
    else if (match(TOK_NULL)) {
        node = create_node(AST_NUMBER);
        node->data.num_value = 0;
    }
    else if (match(TOK_IDENT)) {
        char* name = strdup(tokens[token_pos-1].value);
        
        // Check for function call
        if (peek_token() && peek_token()->type == TOK_LPAREN) {
            match(TOK_LPAREN);
            node = create_node(AST_CALL);
            node->data.call.name = name;
            node->data.call.args = malloc(32 * sizeof(ast_node_t*));
            node->data.call.arg_count = 0;
            
            while (!match(TOK_RPAREN)) {
                node->data.call.args[node->data.call.arg_count++] = parse_expression();
                if (!match(TOK_COMMA) && peek_token()->type != TOK_RPAREN) {
                    error("Expected ',' or ')' in argument list");
                }
            }
        }
        // Struct literal
        else if (match(TOK_LBRACE)) {
            node = create_node(AST_STRUCT_LITERAL);
            node->data.struct_literal.type = calloc(1, sizeof(type_info_t));
            node->data.struct_literal.type->kind = TYPE_STRUCT;
            node->data.struct_literal.type->struct_type.name = name;
            
            // Parse field initializers
            while (!match(TOK_RBRACE)) {
                if (!match(TOK_IDENT)) error("Expected field name");
                char* field_name = strdup(tokens[token_pos-1].value);
                if (!match(TOK_COLON)) error("Expected ':' after field name");
                ast_node_t* value = parse_expression();
                
                // Store field initializer
                // TODO: Add to struct literal
                
                if (!match(TOK_COMMA) && peek_token()->type != TOK_RBRACE) {
                    error("Expected ',' or '}' in struct literal");
                }
            }
        }
        else {
            node = create_node(AST_IDENT);
            node->data.str_value = name;
        }
    }
    else if (match(TOK_LPAREN)) {
        node = parse_expression();
        if (!match(TOK_RPAREN)) error("Expected ')'");
    }
    else if (match(TOK_LBRACKET)) {
        // Array literal
        node = create_node(AST_ARRAY_LITERAL);
        node->data.array_literal.elements = malloc(100 * sizeof(ast_node_t*));
        node->data.array_literal.element_count = 0;
        
        while (!match(TOK_RBRACKET)) {
            node->data.array_literal.elements[node->data.array_literal.element_count++] = parse_expression();
            if (!match(TOK_COMMA) && peek_token()->type != TOK_RBRACKET) {
                error("Expected ',' or ']' in array literal");
            }
        }
    }
    else if (match(TOK_ASM)) {
        node = parse_asm();
    }
    else {
        error("Expected expression");
    }
    
    // Handle postfix operators
    while (1) {
        if (match(TOK_DOT)) {
            if (!match(TOK_IDENT)) error("Expected field name after '.'");
            ast_node_t* field = create_node(AST_FIELD_ACCESS);
            field->data.field_access.object = node;
            field->data.field_access.field = strdup(tokens[token_pos-1].value);
            node = field;
        }
        else if (match(TOK_ARROW)) {
            if (!match(TOK_IDENT)) error("Expected field name after '->'");
            ast_node_t* deref = create_node(AST_UNARY_OP);
            deref->data.unary.op = TOK_MULT; // Dereference
            deref->data.unary.operand = node;
            
            ast_node_t* field = create_node(AST_FIELD_ACCESS);
            field->data.field_access.object = deref;
            field->data.field_access.field = strdup(tokens[token_pos-1].value);
            node = field;
        }
        else if (match(TOK_LBRACKET)) {
            ast_node_t* index = create_node(AST_INDEX);
            index->data.index.array = node;
            index->data.index.index = parse_expression();
            if (!match(TOK_RBRACKET)) error("Expected ']'");
            node = index;
        }
        else {
            break;
        }
    }
    
    return node;
}

// Parse unary expression
static ast_node_t* parse_unary() {
    if (match(TOK_NOT) || match(TOK_BITNOT) || match(TOK_MINUS) || match(TOK_BITAND) || match(TOK_MULT)) {
        ast_node_t* node = create_node(AST_UNARY_OP);
        node->data.unary.op = tokens[token_pos-1].type;
        node->data.unary.operand = parse_unary();
        return node;
    }
    
    return parse_primary();
}

// Parse binary expression with precedence
static ast_node_t* parse_binary(int min_prec) {
    ast_node_t* left = parse_unary();
    
    while (1) {
        token_t* op = peek_token();
        if (!op) break;
        
        int prec = 0;
        switch (op->type) {
            case TOK_MULT: case TOK_DIV: case TOK_MOD: prec = 10; break;
            case TOK_PLUS: case TOK_MINUS: prec = 9; break;
            case TOK_LSHIFT: case TOK_RSHIFT: prec = 8; break;
            case TOK_LT: case TOK_GT: case TOK_LE: case TOK_GE: prec = 7; break;
            case TOK_EQ: case TOK_NE: prec = 6; break;
            case TOK_BITAND: prec = 5; break;
            case TOK_BITXOR: prec = 4; break;
            case TOK_BITOR: prec = 3; break;
            case TOK_AND: prec = 2; break;
            case TOK_OR: prec = 1; break;
            default: return left;
        }
        
        if (prec < min_prec) return left;
        
        advance_token();
        ast_node_t* right = parse_binary(prec + 1);
        
        ast_node_t* node = create_node(AST_BINARY_OP);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = op->type;
        left = node;
    }
    
    return left;
}

// Parse expression
static ast_node_t* parse_expression() {
    return parse_binary(0);
}

// Parse let statement
static ast_node_t* parse_let() {
    ast_node_t* node = create_node(AST_LET);
    
    if (match(TOK_CONST)) node->data.var_decl.is_const = 1;
    if (match(TOK_STATIC)) node->data.var_decl.is_static = 1;
    
    if (!match(TOK_IDENT)) error("Expected variable name");
    node->data.var_decl.name = strdup(tokens[token_pos-1].value);
    
    if (match(TOK_COLON)) {
        node->data.var_decl.type = parse_type_spec();
    }
    
    if (match(TOK_ASSIGN)) {
        node->data.var_decl.init = parse_expression();
    }
    
    return node;
}

// Parse statement
static ast_node_t* parse_statement() {
    ast_node_t* node = NULL;
    
    if (match(TOK_LET)) {
        node = parse_let();
        if (!match(TOK_SEMICOLON)) error("Expected ';' after let statement");
    }
    else if (match(TOK_IF)) {
        node = create_node(AST_IF);
        node->data.if_stmt.condition = parse_expression();
        node->data.if_stmt.then_branch = parse_block();
        if (match(TOK_ELSE)) {
            node->data.if_stmt.else_branch = parse_block();
        }
    }
    else if (match(TOK_WHILE)) {
        node = create_node(AST_WHILE);
        
        // Check for @wcet annotation
        if (match(TOK_AT) && match(TOK_WCET)) {
            if (!match(TOK_LPAREN)) error("Expected '(' after @wcet");
            if (!match(TOK_NUMBER)) error("Expected max iterations");
            node->data.while_stmt.max_iterations = atoi(tokens[token_pos-1].value);
            if (!match(TOK_RPAREN)) error("Expected ')'");
        }
        
        node->data.while_stmt.condition = parse_expression();
        node->data.while_stmt.body = parse_block();
    }
    else if (match(TOK_FOR)) {
        node = create_node(AST_FOR);
        if (!match(TOK_LPAREN)) error("Expected '(' after for");
        
        // Init
        if (!match(TOK_SEMICOLON)) {
            node->data.for_stmt.init = parse_statement();
        }
        
        // Condition
        if (!match(TOK_SEMICOLON)) {
            node->data.for_stmt.cond = parse_expression();
            if (!match(TOK_SEMICOLON)) error("Expected ';' after for condition");
        }
        
        // Update
        if (!match(TOK_RPAREN)) {
            node->data.for_stmt.update = parse_expression();
            if (!match(TOK_RPAREN)) error("Expected ')' after for");
        }
        
        node->data.for_stmt.body = parse_block();
    }
    else if (match(TOK_RETURN)) {
        node = create_node(AST_RETURN);
        if (peek_token() && peek_token()->type != TOK_SEMICOLON) {
            node->data.return_stmt.value = parse_expression();
        }
        if (!match(TOK_SEMICOLON)) error("Expected ';' after return");
    }
    else if (match(TOK_BREAK)) {
        node = create_node(AST_BREAK);
        if (!match(TOK_SEMICOLON)) error("Expected ';' after break");
    }
    else if (match(TOK_CONTINUE)) {
        node = create_node(AST_CONTINUE);
        if (!match(TOK_SEMICOLON)) error("Expected ';' after continue");
    }
    else if (match(TOK_LBRACE)) {
        token_pos--; // Put back the brace
        node = parse_block();
    }
    else {
        // Check if we have a valid expression to parse
        if (peek_token() && peek_token()->type != TOK_RBRACE && peek_token()->type != TOK_EOF) {
            // Expression statement
            node = parse_expression();
            if (!match(TOK_SEMICOLON)) error("Expected ';' after expression");
        }
    }
    
    return node;
}

// Parse block
static ast_node_t* parse_block() {
    if (!match(TOK_LBRACE)) error("Expected '{'");
    
    ast_node_t* node = create_node(AST_BLOCK);
    node->data.block.statements = malloc(100 * sizeof(ast_node_t*));
    node->data.block.stmt_count = 0;
    
    while (!match(TOK_RBRACE)) {
        node->data.block.statements[node->data.block.stmt_count++] = parse_statement();
    }
    
    return node;
}

// Parse function
static ast_node_t* parse_function() {
    ast_node_t* node = create_node(AST_FUNCTION);
    
    if (match(TOK_PUB)) node->data.function.is_public = 1;
    if (match(TOK_INLINE)) node->data.function.is_inline = 1;
    
    if (!match(TOK_IDENT)) error("Expected function name");
    node->data.function.name = strdup(tokens[token_pos-1].value);
    
    // Parameters
    if (!match(TOK_LPAREN)) error("Expected '(' after function name");
    node->data.function.params = malloc(32 * sizeof(ast_node_t*));
    node->data.function.param_count = 0;
    
    while (!match(TOK_RPAREN)) {
        ast_node_t* param = create_node(AST_PARAM);
        if (!match(TOK_IDENT)) error("Expected parameter name");
        param->data.var_decl.name = strdup(tokens[token_pos-1].value);
        if (!match(TOK_COLON)) error("Expected ':' after parameter name");
        param->data.var_decl.type = parse_type_spec();
        
        node->data.function.params[node->data.function.param_count++] = param;
        
        if (!match(TOK_COMMA) && peek_token()->type != TOK_RPAREN) {
            error("Expected ',' or ')' in parameter list");
        }
    }
    
    // Return type
    if (match(TOK_ARROW)) {
        node->data.function.return_type = parse_type_spec();
    } else {
        node->data.function.return_type = calloc(1, sizeof(type_info_t));
        node->data.function.return_type->kind = TYPE_VOID;
    }
    
    // Attributes
    while (match(TOK_AT)) {
        if (match(TOK_WCET)) {
            if (!match(TOK_LPAREN)) error("Expected '(' after @wcet");
            if (!match(TOK_NUMBER)) error("Expected WCET bound");
            char* num_str = tokens[token_pos-1].value;
            if (num_str[0] == '0' && (num_str[1] == 'x' || num_str[1] == 'X')) {
                node->data.function.wcet_bound = strtol(num_str, NULL, 16);
            } else {
                node->data.function.wcet_bound = atoi(num_str);
            }
            if (!match(TOK_RPAREN)) error("Expected ')'");
        }
        else if (match(TOK_SECURITY)) {
            if (!match(TOK_LPAREN)) error("Expected '(' after @security");
            if (!match(TOK_NUMBER)) error("Expected security level");
            char* sec_str = tokens[token_pos-1].value;
            if (sec_str[0] == '0' && (sec_str[1] == 'x' || sec_str[1] == 'X')) {
                node->data.function.security_level = strtol(sec_str, NULL, 16);
            } else {
                node->data.function.security_level = atoi(sec_str);
            }
            if (!match(TOK_RPAREN)) error("Expected ')'");
        }
        else if (match(TOK_CONSTANT_TIME)) {
            node->data.function.is_constant_time = 1;
            // @constant_time doesn't require parameters
        }
        else if (match(TOK_TRUSTED)) {
            node->data.function.is_trusted = 1;
        }
        else if (match(TOK_INLINE)) {
            node->data.function.is_inline = 1;
        }
    }
    
    // Body
    node->data.function.body = parse_block();
    
    return node;
}

// Parse module
static ast_node_t* parse_module() {
    ast_node_t* root = create_node(AST_MODULE);
    root->data.module.name = "main";
    root->data.module.items = malloc(100 * sizeof(ast_node_t*));
    root->data.module.item_count = 0;
    
    while (peek_token() && peek_token()->type != TOK_EOF) {
        ast_node_t* item = NULL;
        
        if (peek_token()->type == TOK_FUNCTION) {
            match(TOK_FUNCTION);
            item = parse_function();
        }
        else if (peek_token()->type == TOK_STRUCT) {
            match(TOK_STRUCT);
            item = parse_struct();
        }
        else if (peek_token()->type == TOK_TYPE) {
            match(TOK_TYPE);
            // TODO: Parse type alias
            advance_token(); // Skip for now
        }
        else if (peek_token()->type == TOK_CONST || peek_token()->type == TOK_STATIC) {
            // Global variable
            item = parse_let();
            if (!match(TOK_SEMICOLON)) error("Expected ';' after global declaration");
        }
        else {
            // If we hit an unexpected token, it's an error
            error("Expected top-level declaration (function, struct, const, etc.)");
        }
        
        if (item) {
            root->data.module.items[root->data.module.item_count++] = item;
        }
    }
    
    return root;
}

// WCET Analysis Functions
static void analyze_wcet(ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_NUMBER:
        case AST_IDENT:
            node->wcet_cycles = 1;  // Load operations
            break;
            
        case AST_BINARY_OP:
            analyze_wcet(node->data.binary.left);
            analyze_wcet(node->data.binary.right);
            
            // Different operations have different costs
            switch (node->data.binary.op) {
                case TOK_PLUS:
                case TOK_MINUS:
                case TOK_BITAND:
                case TOK_BITOR:
                case TOK_BITXOR:
                    node->wcet_cycles = node->data.binary.left->wcet_cycles + 
                                       node->data.binary.right->wcet_cycles + 1;
                    break;
                case TOK_MULT:
                    node->wcet_cycles = node->data.binary.left->wcet_cycles + 
                                       node->data.binary.right->wcet_cycles + 3;  // Multiplication is slower
                    break;
                case TOK_DIV:
                case TOK_MOD:
                    node->wcet_cycles = node->data.binary.left->wcet_cycles + 
                                       node->data.binary.right->wcet_cycles + 10;  // Division is much slower
                    break;
                case TOK_LSHIFT:
                case TOK_RSHIFT:
                    node->wcet_cycles = node->data.binary.left->wcet_cycles + 
                                       node->data.binary.right->wcet_cycles + 1;
                    break;
                default:
                    node->wcet_cycles = node->data.binary.left->wcet_cycles + 
                                       node->data.binary.right->wcet_cycles + 2;
                    break;
            }
            break;
            
        case AST_IF:
            analyze_wcet(node->data.if_stmt.condition);
            analyze_wcet(node->data.if_stmt.then_branch);
            
            // Worst case: condition + branch + overhead
            node->wcet_cycles = node->data.if_stmt.condition->wcet_cycles + 
                               node->data.if_stmt.then_branch->wcet_cycles + 2;
            break;
            
        case AST_BLOCK:
            node->wcet_cycles = 0;
            for (int i = 0; i < node->data.block.stmt_count; i++) {
                analyze_wcet(node->data.block.statements[i]);
                node->wcet_cycles += node->data.block.statements[i]->wcet_cycles;
            }
            break;
            
        case AST_FUNCTION:
            analyze_wcet(node->data.function.body);
            node->wcet_cycles = node->data.function.body->wcet_cycles + 5;  // Function call overhead
            
            // Check against declared WCET bound
            if (node->data.function.wcet_bound > 0 && 
                node->wcet_cycles > node->data.function.wcet_bound) {
                error("Function '%s' exceeds WCET bound: %d > %d cycles", 
                      node->data.function.name, node->wcet_cycles, node->data.function.wcet_bound);
            }
            break;
            
        case AST_CALL:
            // Check for intrinsic functions with known WCET
            if (strncmp(node->data.call.name, "memory_read", 11) == 0 ||
                strncmp(node->data.call.name, "memory_write", 12) == 0) {
                node->wcet_cycles = 4;  // Memory access intrinsics
            }
            else if (strncmp(node->data.call.name, "io_in", 5) == 0 ||
                     strncmp(node->data.call.name, "io_out", 6) == 0) {
                node->wcet_cycles = 20;  // I/O operations are slower
            }
            else if (strcmp(node->data.call.name, "cpu_cli") == 0 ||
                     strcmp(node->data.call.name, "cpu_sti") == 0) {
                node->wcet_cycles = 2;  // Fast CPU control instructions
            }
            else if (strcmp(node->data.call.name, "cpu_hlt") == 0) {
                node->wcet_cycles = 1;  // Halt is immediate
            }
            else {
                node->wcet_cycles = 10;  // Regular function call overhead
            }
            break;
            
        case AST_CONSTANT_TIME_BLOCK:
            analyze_wcet(node->data.constant_time.body);
            node->wcet_cycles = node->data.constant_time.max_cycles;
            node->is_constant_time = 1;
            
            // Verify that body doesn't exceed declared cycles
            if (node->data.constant_time.body->wcet_cycles > node->data.constant_time.max_cycles) {
                error("Constant-time block exceeds declared cycles: %d > %d", 
                      node->data.constant_time.body->wcet_cycles, node->data.constant_time.max_cycles);
            }
            break;
            
        default:
            node->wcet_cycles = 1;
            break;
    }
    
    total_cycles += node->wcet_cycles;
    if (node->wcet_cycles > max_function_cycles) {
        max_function_cycles = node->wcet_cycles;
    }
}

// Security Analysis Functions
static void analyze_security(ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_FUNCTION:
            if (node->data.function.is_trusted) {
                node->security_level = 3;  // Kernel level
                trusted_function_count++;
            } else {
                node->security_level = node->data.function.security_level;
            }
            
            analyze_security(node->data.function.body);
            break;
            
        case AST_SECURITY_ANNOTATION:
            current_security_level = node->data.security.level;
            node->security_level = node->data.security.level;
            analyze_security(node->data.security.body);
            break;
            
        case AST_PLEDGE_BLOCK:
            // Pledge blocks inherit current security level but are sandboxed
            node->security_level = current_security_level;
            analyze_security(node->data.pledge.body);
            break;
            
        case AST_BLOCK:
            for (int i = 0; i < node->data.block.stmt_count; i++) {
                analyze_security(node->data.block.statements[i]);
            }
            break;
            
        default:
            node->security_level = current_security_level;
            break;
    }
}

// Code generation for x86 assembly
static void generate_assembly(ast_node_t* node, FILE* output) {
    if (!node) return;
    
    switch (node->type) {
        case AST_MODULE:
            fprintf(output, "section .data\n");
            fprintf(output, "    ; Global data section\n\n");
            
            fprintf(output, "section .text\n");
            fprintf(output, "global _start\n\n");
            
            // Generate all items in module
            for (int i = 0; i < node->data.module.item_count; i++) {
                generate_assembly(node->data.module.items[i], output);
            }
            break;
            
        case AST_FUNCTION:
            fprintf(output, "\n; Function: %s (WCET: %d cycles, Security: %d)\n", 
                    node->data.function.name, node->wcet_cycles, node->security_level);
            
            if (node->data.function.is_public) {
                fprintf(output, "global %s\n", node->data.function.name);
            }
            
            fprintf(output, "%s:\n", node->data.function.name);
            fprintf(output, "    push ebp\n");
            fprintf(output, "    mov ebp, esp\n");
            
            // Reset stack offset for local variables
            current_stack_offset = 0;
            enter_scope();
            
            // Add parameters to symbol table (parameters are at positive offsets)
            for (int i = 0; i < node->data.function.param_count; i++) {
                ast_node_t* param = node->data.function.params[i];
                int param_offset = 8 + (i * 4); // Skip old ebp and return address
                push_symbol(param->data.var_decl.name, param_offset, param->data.var_decl.type);
            }
            
            // Allocate stack frame if needed
            fprintf(output, "    sub esp, 64    ; Local variable space\n");
            
            if (node->data.function.is_inline) {
                fprintf(output, "    ; INLINE FUNCTION\n");
            }
            
            generate_assembly(node->data.function.body, output);
            
            // Clean up scope
            exit_scope();
            
            // Add implicit return if function doesn't end with explicit return
            // This handles void functions or missing returns
            fprintf(output, "    ; Implicit return\n");
            fprintf(output, "    mov esp, ebp\n");
            fprintf(output, "    pop ebp\n");
            fprintf(output, "    ret\n");
            break;
            
        case AST_STRUCT_DECL:
            fprintf(output, "\n; Struct: %s\n", node->data.struct_decl.name);
            fprintf(output, "; Size: %d bytes\n", 
                    node->data.struct_decl.field_count * 4); // Simplified
            if (node->data.struct_decl.is_packed) {
                fprintf(output, "; @packed\n");
            }
            break;
            
        case AST_NUMBER:
            fprintf(output, "    mov eax, %ld\n", node->data.num_value);
            break;
            
        case AST_STRING:
            // Generate string in data section
            fprintf(output, "    ; String literal: \"%s\"\n", node->data.str_value);
            fprintf(output, "    mov eax, str_%p\n", (void*)node);
            break;
            
        case AST_CHAR:
            fprintf(output, "    mov al, %d\n", node->data.char_value);
            break;
            
        case AST_BOOL:
            fprintf(output, "    mov eax, %d\n", node->data.bool_value ? 1 : 0);
            break;
            
        case AST_IDENT:
            fprintf(output, "    ; Load variable %s\n", node->data.str_value);
            symbol_t* sym = find_symbol(node->data.str_value);
            if (sym) {
                fprintf(output, "    mov eax, [ebp%+d]\n", sym->offset);
            } else {
                error("Undefined variable: %s", node->data.str_value);
            }
            break;
            
        case AST_BINARY_OP:
            generate_assembly(node->data.binary.left, output);
            fprintf(output, "    push eax\n");
            generate_assembly(node->data.binary.right, output);
            fprintf(output, "    pop ebx\n");
            
            switch (node->data.binary.op) {
                case TOK_PLUS:
                    fprintf(output, "    add eax, ebx\n");
                    break;
                case TOK_MINUS:
                    fprintf(output, "    sub ebx, eax\n");
                    fprintf(output, "    mov eax, ebx\n");
                    break;
                case TOK_MULT:
                    fprintf(output, "    imul eax, ebx\n");
                    break;
                case TOK_DIV:
                    fprintf(output, "    xor edx, edx\n");
                    fprintf(output, "    xchg eax, ebx\n");
                    fprintf(output, "    div ebx\n");
                    break;
                case TOK_MOD:
                    fprintf(output, "    xor edx, edx\n");
                    fprintf(output, "    xchg eax, ebx\n");
                    fprintf(output, "    div ebx\n");
                    fprintf(output, "    mov eax, edx\n");
                    break;
                case TOK_BITAND:
                    fprintf(output, "    and eax, ebx\n");
                    break;
                case TOK_BITOR:
                    fprintf(output, "    or eax, ebx\n");
                    break;
                case TOK_BITXOR:
                    fprintf(output, "    xor eax, ebx\n");
                    break;
                case TOK_LSHIFT:
                    fprintf(output, "    mov ecx, eax\n");
                    fprintf(output, "    mov eax, ebx\n");
                    fprintf(output, "    shl eax, cl\n");
                    break;
                case TOK_RSHIFT:
                    fprintf(output, "    mov ecx, eax\n");
                    fprintf(output, "    mov eax, ebx\n");
                    fprintf(output, "    shr eax, cl\n");
                    break;
                case TOK_EQ:
                    fprintf(output, "    cmp ebx, eax\n");
                    fprintf(output, "    sete al\n");
                    fprintf(output, "    movzx eax, al\n");
                    break;
                case TOK_NE:
                    fprintf(output, "    cmp ebx, eax\n");
                    fprintf(output, "    setne al\n");
                    fprintf(output, "    movzx eax, al\n");
                    break;
                case TOK_LT:
                    fprintf(output, "    cmp ebx, eax\n");
                    fprintf(output, "    setl al\n");
                    fprintf(output, "    movzx eax, al\n");
                    break;
                case TOK_GT:
                    fprintf(output, "    cmp ebx, eax\n");
                    fprintf(output, "    setg al\n");
                    fprintf(output, "    movzx eax, al\n");
                    break;
                case TOK_LE:
                    fprintf(output, "    cmp ebx, eax\n");
                    fprintf(output, "    setle al\n");
                    fprintf(output, "    movzx eax, al\n");
                    break;
                case TOK_GE:
                    fprintf(output, "    cmp ebx, eax\n");
                    fprintf(output, "    setge al\n");
                    fprintf(output, "    movzx eax, al\n");
                    break;
            }
            break;
            
        case AST_UNARY_OP:
            generate_assembly(node->data.unary.operand, output);
            
            switch (node->data.unary.op) {
                case TOK_MINUS:
                    fprintf(output, "    neg eax\n");
                    break;
                case TOK_BITNOT:
                    fprintf(output, "    not eax\n");
                    break;
                case TOK_NOT:
                    fprintf(output, "    test eax, eax\n");
                    fprintf(output, "    setz al\n");
                    fprintf(output, "    movzx eax, al\n");
                    break;
                case TOK_BITAND:  // Address-of
                    fprintf(output, "    lea eax, [ebp-8]    ; TODO: proper address\n");
                    break;
                case TOK_MULT:    // Dereference
                    fprintf(output, "    mov eax, [eax]\n");
                    break;
            }
            break;
            
        case AST_CALL:
            // Check for intrinsic functions
            if (strcmp(node->data.call.name, "memory_read8") == 0) {
                // memory_read8(address) -> byte
                if (node->data.call.arg_count != 1) error("memory_read8 expects 1 argument");
                generate_assembly(node->data.call.args[0], output);
                fprintf(output, "    movzx eax, byte [eax]\n");
            }
            else if (strcmp(node->data.call.name, "memory_read16") == 0) {
                // memory_read16(address) -> word
                if (node->data.call.arg_count != 1) error("memory_read16 expects 1 argument");
                generate_assembly(node->data.call.args[0], output);
                fprintf(output, "    movzx eax, word [eax]\n");
            }
            else if (strcmp(node->data.call.name, "memory_read32") == 0) {
                // memory_read32(address) -> dword
                if (node->data.call.arg_count != 1) error("memory_read32 expects 1 argument");
                generate_assembly(node->data.call.args[0], output);
                fprintf(output, "    mov eax, [eax]\n");
            }
            else if (strcmp(node->data.call.name, "memory_write8") == 0) {
                // memory_write8(address, value)
                if (node->data.call.arg_count != 2) error("memory_write8 expects 2 arguments");
                generate_assembly(node->data.call.args[1], output);  // value
                fprintf(output, "    push eax\n");
                generate_assembly(node->data.call.args[0], output);  // address
                fprintf(output, "    pop ebx\n");
                fprintf(output, "    mov [eax], bl\n");
            }
            else if (strcmp(node->data.call.name, "memory_write16") == 0) {
                // memory_write16(address, value)
                if (node->data.call.arg_count != 2) error("memory_write16 expects 2 arguments");
                generate_assembly(node->data.call.args[1], output);  // value
                fprintf(output, "    push eax\n");
                generate_assembly(node->data.call.args[0], output);  // address
                fprintf(output, "    pop ebx\n");
                fprintf(output, "    mov [eax], bx\n");
            }
            else if (strcmp(node->data.call.name, "memory_write32") == 0) {
                // memory_write32(address, value)
                if (node->data.call.arg_count != 2) error("memory_write32 expects 2 arguments");
                generate_assembly(node->data.call.args[1], output);  // value
                fprintf(output, "    push eax\n");
                generate_assembly(node->data.call.args[0], output);  // address
                fprintf(output, "    pop ebx\n");
                fprintf(output, "    mov [eax], ebx\n");
            }
            else if (strcmp(node->data.call.name, "io_in8") == 0) {
                // io_in8(port) -> byte
                if (node->data.call.arg_count != 1) error("io_in8 expects 1 argument");
                generate_assembly(node->data.call.args[0], output);
                fprintf(output, "    mov dx, ax\n");
                fprintf(output, "    in al, dx\n");
                fprintf(output, "    movzx eax, al\n");
            }
            else if (strcmp(node->data.call.name, "io_in16") == 0) {
                // io_in16(port) -> word
                if (node->data.call.arg_count != 1) error("io_in16 expects 1 argument");
                generate_assembly(node->data.call.args[0], output);
                fprintf(output, "    mov dx, ax\n");
                fprintf(output, "    in ax, dx\n");
                fprintf(output, "    movzx eax, ax\n");
            }
            else if (strcmp(node->data.call.name, "io_in32") == 0) {
                // io_in32(port) -> dword
                if (node->data.call.arg_count != 1) error("io_in32 expects 1 argument");
                generate_assembly(node->data.call.args[0], output);
                fprintf(output, "    mov dx, ax\n");
                fprintf(output, "    in eax, dx\n");
            }
            else if (strcmp(node->data.call.name, "io_out8") == 0) {
                // io_out8(port, value)
                if (node->data.call.arg_count != 2) error("io_out8 expects 2 arguments");
                generate_assembly(node->data.call.args[1], output);  // value
                fprintf(output, "    push eax\n");
                generate_assembly(node->data.call.args[0], output);  // port
                fprintf(output, "    mov dx, ax\n");
                fprintf(output, "    pop eax\n");
                fprintf(output, "    out dx, al\n");
            }
            else if (strcmp(node->data.call.name, "io_out16") == 0) {
                // io_out16(port, value)
                if (node->data.call.arg_count != 2) error("io_out16 expects 2 arguments");
                generate_assembly(node->data.call.args[1], output);  // value
                fprintf(output, "    push eax\n");
                generate_assembly(node->data.call.args[0], output);  // port
                fprintf(output, "    mov dx, ax\n");
                fprintf(output, "    pop eax\n");
                fprintf(output, "    out dx, ax\n");
            }
            else if (strcmp(node->data.call.name, "io_out32") == 0) {
                // io_out32(port, value)
                if (node->data.call.arg_count != 2) error("io_out32 expects 2 arguments");
                generate_assembly(node->data.call.args[1], output);  // value
                fprintf(output, "    push eax\n");
                generate_assembly(node->data.call.args[0], output);  // port
                fprintf(output, "    mov dx, ax\n");
                fprintf(output, "    pop eax\n");
                fprintf(output, "    out dx, eax\n");
            }
            else if (strcmp(node->data.call.name, "cpu_cli") == 0) {
                // cpu_cli() - disable interrupts
                if (node->data.call.arg_count != 0) error("cpu_cli expects no arguments");
                fprintf(output, "    cli\n");
            }
            else if (strcmp(node->data.call.name, "cpu_sti") == 0) {
                // cpu_sti() - enable interrupts
                if (node->data.call.arg_count != 0) error("cpu_sti expects no arguments");
                fprintf(output, "    sti\n");
            }
            else if (strcmp(node->data.call.name, "cpu_hlt") == 0) {
                // cpu_hlt() - halt CPU
                if (node->data.call.arg_count != 0) error("cpu_hlt expects no arguments");
                fprintf(output, "    hlt\n");
            }
            else {
                // Regular function call
                // Push arguments in reverse order
                for (int i = node->data.call.arg_count - 1; i >= 0; i--) {
                    generate_assembly(node->data.call.args[i], output);
                    fprintf(output, "    push eax\n");
                }
                
                fprintf(output, "    call %s\n", node->data.call.name);
                
                // Clean up stack
                if (node->data.call.arg_count > 0) {
                    fprintf(output, "    add esp, %d\n", node->data.call.arg_count * 4);
                }
            }
            break;
            
        case AST_IF:
            generate_assembly(node->data.if_stmt.condition, output);
            fprintf(output, "    test eax, eax\n");
            fprintf(output, "    jz .L%p_else\n", (void*)node);
            
            generate_assembly(node->data.if_stmt.then_branch, output);
            fprintf(output, "    jmp .L%p_end\n", (void*)node);
            
            fprintf(output, ".L%p_else:\n", (void*)node);
            if (node->data.if_stmt.else_branch) {
                generate_assembly(node->data.if_stmt.else_branch, output);
            }
            
            fprintf(output, ".L%p_end:\n", (void*)node);
            break;
            
        case AST_WHILE:
            fprintf(output, ".L%p_start:\n", (void*)node);
            generate_assembly(node->data.while_stmt.condition, output);
            fprintf(output, "    test eax, eax\n");
            fprintf(output, "    jz .L%p_end\n", (void*)node);
            
            generate_assembly(node->data.while_stmt.body, output);
            fprintf(output, "    jmp .L%p_start\n", (void*)node);
            
            fprintf(output, ".L%p_end:\n", (void*)node);
            break;
            
        case AST_FOR:
            if (node->data.for_stmt.init) {
                generate_assembly(node->data.for_stmt.init, output);
            }
            
            fprintf(output, ".L%p_start:\n", (void*)node);
            if (node->data.for_stmt.cond) {
                generate_assembly(node->data.for_stmt.cond, output);
                fprintf(output, "    test eax, eax\n");
                fprintf(output, "    jz .L%p_end\n", (void*)node);
            }
            
            generate_assembly(node->data.for_stmt.body, output);
            
            if (node->data.for_stmt.update) {
                generate_assembly(node->data.for_stmt.update, output);
            }
            fprintf(output, "    jmp .L%p_start\n", (void*)node);
            
            fprintf(output, ".L%p_end:\n", (void*)node);
            break;
            
        case AST_LET:
            fprintf(output, "    ; Variable: %s\n", node->data.var_decl.name);
            // Allocate space on stack
            current_stack_offset -= 4; // Assume 32-bit variables for now
            push_symbol(node->data.var_decl.name, current_stack_offset, node->data.var_decl.type);
            
            if (node->data.var_decl.init) {
                generate_assembly(node->data.var_decl.init, output);
                fprintf(output, "    mov [ebp%+d], eax\n", current_stack_offset);
            }
            break;
            
        case AST_BLOCK:
            enter_scope();
            for (int i = 0; i < node->data.block.stmt_count; i++) {
                generate_assembly(node->data.block.statements[i], output);
            }
            exit_scope();
            break;
            
        case AST_RETURN:
            if (node->data.return_stmt.value) {
                generate_assembly(node->data.return_stmt.value, output);
            }
            fprintf(output, "    mov esp, ebp\n");
            fprintf(output, "    pop ebp\n");
            fprintf(output, "    ret\n");
            break;
            
        case AST_BREAK:
            fprintf(output, "    jmp .L_loop_end    ; TODO: track loop context\n");
            break;
            
        case AST_CONTINUE:
            fprintf(output, "    jmp .L_loop_continue    ; TODO: track loop context\n");
            break;
            
        case AST_FIELD_ACCESS:
            generate_assembly(node->data.field_access.object, output);
            fprintf(output, "    ; Access field %s\n", node->data.field_access.field);
            fprintf(output, "    add eax, 0    ; TODO: field offset\n");
            fprintf(output, "    mov eax, [eax]\n");
            break;
            
        case AST_INDEX:
            generate_assembly(node->data.index.array, output);
            fprintf(output, "    push eax\n");
            generate_assembly(node->data.index.index, output);
            fprintf(output, "    pop ebx\n");
            fprintf(output, "    lea eax, [ebx + eax*4]    ; Assume 4-byte elements\n");
            fprintf(output, "    mov eax, [eax]\n");
            break;
            
        case AST_ASM_BLOCK:
            fprintf(output, "    ; Inline assembly\n");
            if (node->data.asm_block.is_volatile) {
                fprintf(output, "    ; volatile\n");
            }
            fprintf(output, "%s\n", node->data.asm_block.code);
            break;
            
        case AST_CONSTANT_TIME_BLOCK:
            fprintf(output, "    ; CONSTANT TIME BLOCK - Max %d cycles\n", 
                    node->data.constant_time.max_cycles);
            generate_assembly(node->data.constant_time.body, output);
            fprintf(output, "    ; END CONSTANT TIME BLOCK\n");
            break;
            
        case AST_PLEDGE_BLOCK:
            fprintf(output, "    ; PLEDGE BLOCK - Promises: 0x%02X\n", 
                    node->data.pledge.promises);
            generate_assembly(node->data.pledge.body, output);
            fprintf(output, "    ; END PLEDGE BLOCK\n");
            break;
            
        default:
            fprintf(output, "    ; TODO: Generate code for AST node type %d\n", node->type);
            break;
    }
}

// Main compiler function
int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.tempo> <output.s>\n", argv[0]);
        return 1;
    }
    
    // Read input file
    FILE* input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", argv[1]);
        return 1;
    }
    
    fseek(input, 0, SEEK_END);
    source_len = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    char* source_buffer = malloc(source_len + 1);
    fread(source_buffer, 1, source_len, input);
    source_buffer[source_len] = '\0';
    fclose(input);
    
    source = source_buffer;
    pos = 0; line = 1; col = 1; token_pos = 0;
    
    // Show Tempo logo
    printf("\n");
    printf("  ████████╗███████╗███╗   ███╗██████╗  ██████╗ \n");
    printf("  ╚══██╔══╝██╔════╝████╗ ████║██╔══██╗██╔═══██╗\n");
    printf("     ██║   █████╗  ██╔████╔██║██████╔╝██║   ██║\n");
    printf("     ██║   ██╔══╝  ██║╚██╔╝██║██╔═══╝ ██║   ██║\n");
    printf("     ██║   ███████╗██║ ╚═╝ ██║██║     ╚██████╔╝\n");
    printf("     ╚═╝   ╚══════╝╚═╝     ╚═╝╚═╝      ╚═════╝ \n");
    printf("\n");
    printf("  Tempo v1.2.2 - Complete Systems Programming Language\n");
    printf("  Processing: %s\n", argv[1]);
    printf("  ================================================\n\n");
    
    // Tokenize
    tokenize();
    printf("Tokenized %d tokens\n", token_count);
    
    
    // Parse the module
    ast_node_t* ast = parse_module();
    
    if (ast) {
        // Perform analysis
        printf("Performing WCET analysis...\n");
        analyze_wcet(ast);
        
        printf("Performing security analysis...\n");
        analyze_security(ast);
        
        // Generate assembly
        FILE* output = fopen(argv[2], "w");
        if (!output) {
            fprintf(stderr, "Error: Cannot create output file '%s'\n", argv[2]);
            return 1;
        }
        
        fprintf(output, "; Generated by Tempo v1.2.2 Compiler\n");
        fprintf(output, "; Source: %s\n", argv[1]);
        fprintf(output, "; Total WCET: %d cycles\n", total_cycles);
        fprintf(output, "; Max function WCET: %d cycles\n", max_function_cycles);
        fprintf(output, "; Trusted functions: %d\n", trusted_function_count);
        fprintf(output, "\nsection .text\n");
        
        generate_assembly(ast, output);
        fclose(output);
        
        printf("Compilation successful!\n");
        printf("  Output: %s\n", argv[2]);
        printf("  Total WCET: %d cycles\n", total_cycles);
        printf("  Max function WCET: %d cycles\n", max_function_cycles);
        printf("  Security level: %d\n", ast->security_level);
        printf("  Trusted functions: %d\n", trusted_function_count);
    } else {
        fprintf(stderr, "Error: No valid function found in source\n");
        return 1;
    }
    
    free(source_buffer);
    return 0;
}