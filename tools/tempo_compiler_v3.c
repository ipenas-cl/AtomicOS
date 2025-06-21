/*
 * Tempo Compiler v3.0 - Extended Version for Ideal OS
 * Deterministic Programming Language for AtomicOS
 * Features: WCET analysis, security annotations, real-time constructs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

// Extended token types for v3.0
typedef enum {
    TOK_EOF = 0, TOK_NUMBER, TOK_IDENT, TOK_FUNCTION, TOK_IF, TOK_ELSE,
    TOK_LOOP, TOK_FROM, TOK_TO, TOK_WCET, TOK_DEADLINE, TOK_LET, TOK_RETURN,
    TOK_PLUS, TOK_MINUS, TOK_MULT, TOK_DIV, TOK_MOD, TOK_ASSIGN,
    TOK_EQ, TOK_NE, TOK_LT, TOK_GT, TOK_LE, TOK_GE, TOK_AND, TOK_OR, TOK_NOT,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_SEMICOLON, TOK_COLON, TOK_COMMA,
    TOK_INT32, TOK_BOOL, TOK_TRUE, TOK_FALSE,
    TOK_BITAND, TOK_BITOR, TOK_BITXOR, TOK_BITNOT, TOK_LSHIFT, TOK_RSHIFT,
    
    // New v3.0 tokens for ideal OS features
    TOK_PLEDGE, TOK_UNVEIL, TOK_SECURITY, TOK_REALTIME, TOK_ATOMIC,
    TOK_CONSTANT_TIME, TOK_TRUSTED, TOK_ENCLAVE, TOK_TSN,
    TOK_INTERRUPT, TOK_PRIORITY, TOK_WATCHDOG, TOK_GUARD,
    TOK_LSM_HOOK, TOK_ZFS_BLOCK, TOK_TPM_PCR
} token_type_t;

typedef struct {
    token_type_t type;
    char* value;
    int line, col;
} token_t;

// Extended AST node types for v3.0
typedef enum {
    AST_PROGRAM, AST_FUNCTION, AST_BLOCK, AST_IF, AST_LET, AST_RETURN, 
    AST_BINARY_OP, AST_UNARY_OP, AST_NUMBER, AST_IDENT, AST_CALL,
    
    // New v3.0 AST nodes
    AST_PLEDGE_BLOCK, AST_SECURITY_ANNOTATION, AST_REALTIME_TASK,
    AST_CONSTANT_TIME_BLOCK, AST_TRUSTED_FUNCTION, AST_INTERRUPT_HANDLER,
    AST_WCET_BOUND, AST_ATOMIC_SECTION
} ast_type_t;

// Extended AST node structure
typedef struct ast_node {
    ast_type_t type;
    int wcet_cycles;          // WCET analysis result
    int security_level;       // Security classification
    int is_constant_time;     // Constant-time guarantee flag
    
    union {
        long num_value;
        char* str_value;
        
        struct { struct ast_node* left; struct ast_node* right; token_type_t op; } binary;
        struct { struct ast_node* operand; token_type_t op; } unary;
        
        struct { 
            char* name; 
            struct ast_node** params; 
            int param_count; 
            struct ast_node* body; 
            int wcet_bound;
            int security_level;
            int is_trusted;
            int is_interrupt_handler;
        } function;
        
        struct { char* name; struct ast_node** args; int arg_count; } call;
        struct { char* name; struct ast_node* value; } let;
        struct { struct ast_node* condition; struct ast_node* then_branch; } if_stmt;
        struct { struct ast_node** statements; int stmt_count; } block;
        
        // New v3.0 node data
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

// Error handling
static void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Tempo Compiler Error [%d:%d]: ", line, col);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
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
            char buffer[32]; int i = 0;
            while (isdigit(peek()) && i < 31) { buffer[i++] = peek(); advance(); }
            buffer[i] = '\0';
            tokens[token_count++] = make_token(TOK_NUMBER, buffer);
        } else if (isalpha(c) || c == '_') {
            char buffer[64]; int i = 0;  // Increased buffer size
            while ((isalnum(peek()) || peek() == '_') && i < 63) {
                buffer[i++] = peek(); advance();
            }
            buffer[i] = '\0';
            
            // Extended keyword recognition
            token_type_t type = TOK_IDENT;
            if (strcmp(buffer, "function") == 0) type = TOK_FUNCTION;
            else if (strcmp(buffer, "if") == 0) type = TOK_IF;
            else if (strcmp(buffer, "let") == 0) type = TOK_LET;
            else if (strcmp(buffer, "return") == 0) type = TOK_RETURN;
            else if (strcmp(buffer, "int32") == 0) type = TOK_INT32;
            else if (strcmp(buffer, "bool") == 0) type = TOK_BOOL;
            else if (strcmp(buffer, "true") == 0) type = TOK_TRUE;
            else if (strcmp(buffer, "false") == 0) type = TOK_FALSE;
            
            // New v3.0 keywords
            else if (strcmp(buffer, "pledge") == 0) type = TOK_PLEDGE;
            else if (strcmp(buffer, "unveil") == 0) type = TOK_UNVEIL;
            else if (strcmp(buffer, "security") == 0) type = TOK_SECURITY;
            else if (strcmp(buffer, "realtime") == 0) type = TOK_REALTIME;
            else if (strcmp(buffer, "atomic") == 0) type = TOK_ATOMIC;
            else if (strcmp(buffer, "constant_time") == 0) type = TOK_CONSTANT_TIME;
            else if (strcmp(buffer, "trusted") == 0) type = TOK_TRUSTED;
            else if (strcmp(buffer, "enclave") == 0) type = TOK_ENCLAVE;
            else if (strcmp(buffer, "interrupt") == 0) type = TOK_INTERRUPT;
            else if (strcmp(buffer, "priority") == 0) type = TOK_PRIORITY;
            else if (strcmp(buffer, "wcet") == 0) type = TOK_WCET;
            else if (strcmp(buffer, "watchdog") == 0) type = TOK_WATCHDOG;
            
            tokens[token_count++] = make_token(type, buffer);
        } else {
            switch (c) {
                case '+': advance(); tokens[token_count++] = make_token(TOK_PLUS, "+"); break;
                case '-': advance(); tokens[token_count++] = make_token(TOK_MINUS, "-"); break;
                case '*': advance(); tokens[token_count++] = make_token(TOK_MULT, "*"); break;
                case '/': advance(); tokens[token_count++] = make_token(TOK_DIV, "/"); break;
                case '%': advance(); tokens[token_count++] = make_token(TOK_MOD, "%"); break;
                case '(': advance(); tokens[token_count++] = make_token(TOK_LPAREN, "("); break;
                case ')': advance(); tokens[token_count++] = make_token(TOK_RPAREN, ")"); break;
                case '{': advance(); tokens[token_count++] = make_token(TOK_LBRACE, "{"); break;
                case '}': advance(); tokens[token_count++] = make_token(TOK_RBRACE, "}"); break;
                case ':': advance(); tokens[token_count++] = make_token(TOK_COLON, ":"); break;
                case ';': advance(); tokens[token_count++] = make_token(TOK_SEMICOLON, ";"); break;
                case ',': advance(); tokens[token_count++] = make_token(TOK_COMMA, ","); break;
                case '=': advance(); tokens[token_count++] = make_token(TOK_ASSIGN, "="); break;
                case '&': advance(); tokens[token_count++] = make_token(TOK_BITAND, "&"); break;
                case '|': advance(); tokens[token_count++] = make_token(TOK_BITOR, "|"); break;
                case '^': advance(); tokens[token_count++] = make_token(TOK_BITXOR, "^"); break;
                case '~': advance(); tokens[token_count++] = make_token(TOK_BITNOT, "~"); break;
                case '<': 
                    advance();
                    if (peek() == '<') {
                        advance();
                        tokens[token_count++] = make_token(TOK_LSHIFT, "<<");
                    } else {
                        tokens[token_count++] = make_token(TOK_LT, "<");
                    }
                    break;
                case '>':
                    advance();
                    if (peek() == '>') {
                        advance();
                        tokens[token_count++] = make_token(TOK_RSHIFT, ">>");
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
            node->wcet_cycles = 10;  // Assume 10 cycles for function call
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

// Code generation (simplified x86 assembly output)
static void generate_assembly(ast_node_t* node, FILE* output) {
    if (!node) return;
    
    switch (node->type) {
        case AST_FUNCTION:
            fprintf(output, "\n; Function: %s (WCET: %d cycles, Security: %d)\n", 
                    node->data.function.name, node->wcet_cycles, node->security_level);
            fprintf(output, "%s:\n", node->data.function.name);
            fprintf(output, "    push ebp\n");
            fprintf(output, "    mov ebp, esp\n");
            
            if (node->data.function.is_trusted) {
                fprintf(output, "    ; TRUSTED FUNCTION - Verified signature required\n");
            }
            
            generate_assembly(node->data.function.body, output);
            
            fprintf(output, "    pop ebp\n");
            fprintf(output, "    ret\n");
            break;
            
        case AST_NUMBER:
            fprintf(output, "    mov eax, %ld\n", node->data.num_value);
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
                    fprintf(output, "    sub ebx, eax\n    mov eax, ebx\n");
                    break;
                case TOK_MULT:
                    fprintf(output, "    imul eax, ebx\n");
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
            }
            break;
            
        case AST_BLOCK:
            for (int i = 0; i < node->data.block.stmt_count; i++) {
                generate_assembly(node->data.block.statements[i], output);
            }
            break;
            
        case AST_RETURN:
            if (node->data.binary.left) {
                generate_assembly(node->data.binary.left, output);
            }
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
    
    printf("Tempo v0.7 Compiler - Processing %s\n", argv[1]);
    
    // Tokenize
    tokenize();
    printf("Tokenized %d tokens\n", token_count);
    
    // Simple parsing with error handling
    ast_node_t* ast = NULL;
    if (peek_token() && peek_token()->type == TOK_FUNCTION) {
        if (!match(TOK_FUNCTION)) {
            fprintf(stderr, "Error: Expected 'function' keyword\n");
            return 1;
        }
        ast = create_node(AST_FUNCTION);
        
        if (match(TOK_IDENT)) {
            ast->data.function.name = strdup(tokens[token_pos-1].value);
        }
        
        // Parse parameters with error checking
        if (!match(TOK_LPAREN)) {
            fprintf(stderr, "Error: Expected '(' after function name\n");
            return 1;
        }
        while (peek_token() && peek_token()->type != TOK_RPAREN && peek_token()->type != TOK_EOF) {
            advance_token();
        }
        if (!match(TOK_RPAREN)) {
            fprintf(stderr, "Error: Expected ')' to close parameter list\n");
            return 1;
        }
        if (!match(TOK_COLON)) {
            fprintf(stderr, "Error: Expected ':' after parameter list\n");
            return 1;
        }
        if (!match(TOK_INT32)) {
            fprintf(stderr, "Error: Expected return type 'int32'\n");
            return 1;
        }
        if (!match(TOK_LBRACE)) {
            fprintf(stderr, "Error: Expected '{' to start function body\n");
            return 1;
        }
        
        // Simple body parsing
        ast->data.function.body = create_node(AST_BLOCK);
        ast->data.function.body->data.block.statements = malloc(100 * sizeof(ast_node_t*));
        ast->data.function.body->data.block.stmt_count = 0;
        
        while (peek_token() && peek_token()->type != TOK_RBRACE && peek_token()->type != TOK_EOF) {
            if (match(TOK_RETURN)) {
                ast_node_t* ret = create_node(AST_RETURN);
                if (match(TOK_NUMBER)) {
                    ret->data.binary.left = create_node(AST_NUMBER);
                    ret->data.binary.left->data.num_value = atol(tokens[token_pos-1].value);
                }
                ast->data.function.body->data.block.statements[ast->data.function.body->data.block.stmt_count++] = ret;
            } else {
                advance_token();  // Skip unknown tokens
            }
        }
        if (!match(TOK_RBRACE)) {
            fprintf(stderr, "Error: Expected '}' to close function body\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Error: Expected function declaration\n");
        return 1;
    }
    
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
        
        fprintf(output, "; Generated by Tempo v0.7 Compiler\n");
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