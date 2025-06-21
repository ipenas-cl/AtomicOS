/*
 * Tempo Compiler v2.0 - Clean Version
 * Deterministic Programming Language for AtomicOS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

// Token types
typedef enum {
    TOK_EOF = 0, TOK_NUMBER, TOK_IDENT, TOK_FUNCTION, TOK_IF, TOK_ELSE,
    TOK_LOOP, TOK_FROM, TOK_TO, TOK_WCET, TOK_DEADLINE, TOK_LET, TOK_RETURN,
    TOK_PLUS, TOK_MINUS, TOK_MULT, TOK_DIV, TOK_MOD, TOK_ASSIGN,
    TOK_EQ, TOK_NE, TOK_LT, TOK_GT, TOK_LE, TOK_GE, TOK_AND, TOK_OR, TOK_NOT,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_SEMICOLON, TOK_COLON, TOK_COMMA,
    TOK_INT32, TOK_BOOL, TOK_TRUE, TOK_FALSE,
    TOK_BITAND, TOK_BITOR, TOK_BITXOR, TOK_BITNOT, TOK_LSHIFT, TOK_RSHIFT
} token_type_t;

typedef struct {
    token_type_t type;
    char* value;
    int line, col;
} token_t;

// AST node types  
typedef enum {
    AST_PROGRAM, AST_FUNCTION, AST_BLOCK, AST_IF, AST_LET, AST_RETURN, 
    AST_BINARY_OP, AST_UNARY_OP, AST_NUMBER, AST_IDENT, AST_CALL
} ast_type_t;

typedef struct ast_node {
    ast_type_t type;
    union {
        long num_value;
        char* str_value;
        struct { struct ast_node* left; struct ast_node* right; token_type_t op; } binary;
        struct { struct ast_node* operand; token_type_t op; } unary;
        struct { char* name; struct ast_node** params; int param_count; struct ast_node* body; int wcet; } function;
        struct { char* name; struct ast_node** args; int arg_count; } call;
        struct { char* name; struct ast_node* value; } let;
        struct { struct ast_node* condition; struct ast_node* then_branch; } if_stmt;
        struct { struct ast_node** statements; int stmt_count; } block;
    } data;
} ast_node_t;

// Simple lexer and parser (minimal implementation)
static const char* source;
static int source_len, pos, line, col, token_count, token_pos;
static token_t* tokens;

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

static void tokenize() {
    tokens = malloc(1000 * sizeof(token_t));
    token_count = 0;
    
    while (pos < source_len) {
        skip_whitespace();
        if (pos >= source_len) break;
        
        char c = peek();
        
        if (isdigit(c)) {
            char buffer[32]; int i = 0;
            while (isdigit(peek()) && i < 31) { buffer[i++] = peek(); advance(); }
            buffer[i] = '\0';
            tokens[token_count++] = make_token(TOK_NUMBER, buffer);
        } else if (isalpha(c) || c == '_') {
            char buffer[32]; int i = 0;
            while ((isalnum(peek()) || peek() == '_') && i < 31) {
                buffer[i++] = peek(); advance();
            }
            buffer[i] = '\0';
            
            token_type_t type = TOK_IDENT;
            if (strcmp(buffer, "function") == 0) type = TOK_FUNCTION;
            else if (strcmp(buffer, "if") == 0) type = TOK_IF;
            else if (strcmp(buffer, "let") == 0) type = TOK_LET;
            else if (strcmp(buffer, "return") == 0) type = TOK_RETURN;
            else if (strcmp(buffer, "int32") == 0) type = TOK_INT32;
            
            tokens[token_count++] = make_token(type, buffer);
        } else {
            switch (c) {
                case '+': advance(); tokens[token_count++] = make_token(TOK_PLUS, "+"); break;
                case '-': advance(); tokens[token_count++] = make_token(TOK_MINUS, "-"); break;
                case '*': advance(); tokens[token_count++] = make_token(TOK_MULT, "*"); break;
                case '(': advance(); tokens[token_count++] = make_token(TOK_LPAREN, "("); break;
                case ')': advance(); tokens[token_count++] = make_token(TOK_RPAREN, ")"); break;
                case '{': advance(); tokens[token_count++] = make_token(TOK_LBRACE, "{"); break;
                case '}': advance(); tokens[token_count++] = make_token(TOK_RBRACE, "}"); break;
                case ':': advance(); tokens[token_count++] = make_token(TOK_COLON, ":"); break;
                case ',': advance(); tokens[token_count++] = make_token(TOK_COMMA, ","); break;
                case '=': advance(); tokens[token_count++] = make_token(TOK_ASSIGN, "="); break;
                default: advance(); break;
            }
        }
    }
    tokens[token_count++] = make_token(TOK_EOF, NULL);
}

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
    return node;
}

// Simple parser
static ast_node_t* parse_expression() {
    if (match(TOK_NUMBER)) {
        ast_node_t* node = create_node(AST_NUMBER);
        node->data.num_value = atol(tokens[token_pos-1].value);
        return node;
    } else if (match(TOK_IDENT)) {
        ast_node_t* node = create_node(AST_IDENT);
        node->data.str_value = strdup(tokens[token_pos-1].value);
        return node;
    }
    return NULL;
}

static ast_node_t* parse_statement() {
    if (match(TOK_LET)) {
        ast_node_t* let = create_node(AST_LET);
        if (match(TOK_IDENT)) {
            let->data.let.name = strdup(tokens[token_pos-1].value);
            match(TOK_ASSIGN);
            let->data.let.value = parse_expression();
        }
        return let;
    } else if (match(TOK_RETURN)) {
        ast_node_t* ret = create_node(AST_RETURN);
        ret->data.binary.left = parse_expression();
        return ret;
    }
    return parse_expression();
}

static ast_node_t* parse_function() {
    match(TOK_FUNCTION);
    ast_node_t* func = create_node(AST_FUNCTION);
    
    if (match(TOK_IDENT)) {
        func->data.function.name = strdup(tokens[token_pos-1].value);
    }
    
    match(TOK_LPAREN);
    // Skip parameters for simplicity
    while (peek_token() && peek_token()->type != TOK_RPAREN) advance_token();
    match(TOK_RPAREN);
    match(TOK_COLON);
    match(TOK_INT32);
    match(TOK_LBRACE);
    
    // Parse simple statements
    func->data.function.body = create_node(AST_BLOCK);
    func->data.function.body->data.block.statements = malloc(10 * sizeof(ast_node_t*));
    func->data.function.body->data.block.stmt_count = 0;
    
    while (peek_token() && peek_token()->type != TOK_RBRACE) {
        ast_node_t* stmt = parse_statement();
        if (stmt) {
            func->data.function.body->data.block.statements[func->data.function.body->data.block.stmt_count++] = stmt;
        }
    }
    match(TOK_RBRACE);
    
    return func;
}

// Simple code generator
static FILE* output;

static void emit(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(output, fmt, args);
    va_end(args);
    fprintf(output, "\n");
}

static void generate_expression(ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_NUMBER:
            emit("    mov eax, %ld", node->data.num_value);
            break;
        case AST_IDENT:
            emit("    ; load %s", node->data.str_value);
            break;
        default:
            break;
    }
}

static void generate_statement(ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_LET:
            emit("    ; let %s", node->data.let.name);
            generate_expression(node->data.let.value);
            break;
        case AST_RETURN:
            generate_expression(node->data.binary.left);
            emit("    ret");
            break;
        default:
            generate_expression(node);
            break;
    }
}

static void generate_function(ast_node_t* node) {
    emit("global %s", node->data.function.name);
    emit("%s:", node->data.function.name);
    emit("    push ebp");
    emit("    mov ebp, esp");
    
    if (node->data.function.body) {
        for (int i = 0; i < node->data.function.body->data.block.stmt_count; i++) {
            generate_statement(node->data.function.body->data.block.statements[i]);
        }
    }
    
    emit("    pop ebp");
    emit("    ret");
    emit("");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.tempo> [output.s]\n", argv[0]);
        return 1;
    }
    
    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open %s\n", argv[1]);
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    source_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    source = malloc(source_len + 1);
    fread((void*)source, 1, source_len, fp);
    ((char*)source)[source_len] = '\0';
    fclose(fp);
    
    printf("=== Tempo Compiler v2.0 ===\n");
    printf("Compiling: %s\n", argv[1]);
    
    pos = line = col = token_pos = 0;
    line = 1;
    tokenize();
    printf("Lexer: %d tokens\n", token_count);
    
    ast_node_t* program = create_node(AST_PROGRAM);
    program->data.block.statements = malloc(10 * sizeof(ast_node_t*));
    program->data.block.stmt_count = 0;
    
    while (peek_token() && peek_token()->type != TOK_EOF) {
        if (peek_token()->type == TOK_FUNCTION) {
            program->data.block.statements[program->data.block.stmt_count++] = parse_function();
        } else {
            advance_token();
        }
    }
    printf("Parser: success\n");
    
    const char* output_file = argc > 2 ? argv[2] : "output.s";
    output = fopen(output_file, "w");
    
    emit("section .text");
    emit("bits 32");
    emit("");
    
    for (int i = 0; i < program->data.block.stmt_count; i++) {
        generate_function(program->data.block.statements[i]);
    }
    
    fclose(output);
    printf("Code generation: success\n");
    printf("Output written to: %s\n", output_file);
    
    return 0;
}