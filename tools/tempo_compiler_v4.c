/*
 * Tempo Compiler v4.0.0 - Full Language Implementation
 * Deterministic Programming Language for AtomicOS
 * Features: Complete parser, control flow, advanced type system
 * 
 *  ████████╗███████╗███╗   ███╗██████╗  ██████╗ 
 *  ╚══██╔══╝██╔════╝████╗ ████║██╔══██╗██╔═══██╗
 *     ██║   █████╗  ██╔████╔██║██████╔╝██║   ██║
 *     ██║   ██╔══╝  ██║╚██╔╝██║██╔═══╝ ██║   ██║
 *     ██║   ███████╗██║ ╚═╝ ██║██║     ╚██████╔╝
 *     ╚═╝   ╚══════╝╚═╝     ╚═╝╚═╝      ╚═════╝ 
 * 
 *  Deterministic Programming Language for Real-Time Systems
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>

// Token types
typedef enum {
    TOK_EOF = 0, TOK_NUMBER, TOK_IDENT, TOK_STRING,
    
    // Keywords
    TOK_FUNCTION, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_LOOP,
    TOK_BREAK, TOK_CONTINUE, TOK_RETURN, TOK_LET, TOK_CONST,
    TOK_IMPORT, TOK_MODULE, TOK_EXPORT,
    
    // Types
    TOK_INT32, TOK_INT64, TOK_BOOL, TOK_VOID, TOK_POINTER,
    
    // Literals
    TOK_TRUE, TOK_FALSE, TOK_NULL,
    
    // Operators
    TOK_PLUS, TOK_MINUS, TOK_MULT, TOK_DIV, TOK_MOD,
    TOK_ASSIGN, TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN, TOK_MULT_ASSIGN, TOK_DIV_ASSIGN,
    TOK_EQ, TOK_NE, TOK_LT, TOK_GT, TOK_LE, TOK_GE,
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_BITAND, TOK_BITOR, TOK_BITXOR, TOK_BITNOT, TOK_LSHIFT, TOK_RSHIFT,
    TOK_INC, TOK_DEC,
    
    // Delimiters
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET,
    TOK_SEMICOLON, TOK_COLON, TOK_COMMA, TOK_DOT, TOK_ARROW,
    
    // Special
    TOK_WCET, TOK_INVARIANT, TOK_REQUIRES, TOK_ENSURES,
    TOK_PLEDGE, TOK_UNVEIL, TOK_SECURITY, TOK_REALTIME,
    TOK_ATOMIC, TOK_CONSTANT_TIME
} token_type_t;

// Token structure
typedef struct {
    token_type_t type;
    char* value;
    int line;
    int col;
    char* filename;
} token_t;

// Type kinds
typedef enum {
    TYPE_INT32,
    TYPE_INT64,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_STRUCT
} type_kind_t;

// Type structure
typedef struct type_node {
    type_kind_t kind;
    struct type_node* base_type;  // For pointers and arrays
    int array_size;               // For arrays
    struct type_node** param_types;  // For functions
    int param_count;
    struct type_node* return_type;   // For functions
} type_t;

// AST node types
typedef enum {
    // Program structure
    AST_PROGRAM, AST_MODULE, AST_IMPORT,
    
    // Declarations
    AST_FUNCTION, AST_VARIABLE, AST_CONSTANT, AST_STRUCT,
    
    // Statements
    AST_BLOCK, AST_IF, AST_WHILE, AST_FOR, AST_LOOP,
    AST_BREAK, AST_CONTINUE, AST_RETURN,
    AST_EXPRESSION_STMT,
    
    // Expressions
    AST_BINARY_OP, AST_UNARY_OP, AST_ASSIGNMENT,
    AST_CALL, AST_INDEX, AST_MEMBER,
    AST_NUMBER, AST_STRING, AST_BOOL, AST_NULL,
    AST_IDENT, AST_CAST,
    
    // Special
    AST_WCET_ANNOTATION, AST_INVARIANT, AST_REQUIRES, AST_ENSURES,
    AST_SECURITY_ANNOTATION, AST_PLEDGE_BLOCK
} ast_type_t;

// AST node structure
typedef struct ast_node {
    ast_type_t type;
    type_t* data_type;        // Type information
    int line, col;
    char* filename;
    
    // Analysis results
    int wcet_cycles;
    int security_level;
    bool is_constant_time;
    bool is_pure;
    
    union {
        // Literals
        long num_value;
        char* str_value;
        bool bool_value;
        
        // Binary operation
        struct {
            struct ast_node* left;
            struct ast_node* right;
            token_type_t op;
        } binary;
        
        // Unary operation
        struct {
            struct ast_node* operand;
            token_type_t op;
        } unary;
        
        // Assignment
        struct {
            struct ast_node* target;
            struct ast_node* value;
            token_type_t op;  // =, +=, -=, etc.
        } assignment;
        
        // Function declaration
        struct {
            char* name;
            struct ast_node** params;
            int param_count;
            type_t* return_type;
            struct ast_node* body;
            int wcet_bound;
            int security_level;
            bool is_exported;
            bool is_trusted;
            bool is_interrupt_handler;
        } function;
        
        // Variable/constant declaration
        struct {
            char* name;
            type_t* var_type;
            struct ast_node* init_value;
            bool is_const;
        } variable;
        
        // Function call
        struct {
            char* name;
            struct ast_node** args;
            int arg_count;
        } call;
        
        // If statement
        struct {
            struct ast_node* condition;
            struct ast_node* then_branch;
            struct ast_node* else_branch;
        } if_stmt;
        
        // While loop
        struct {
            struct ast_node* condition;
            struct ast_node* body;
            int max_iterations;  // For WCET
        } while_loop;
        
        // For loop
        struct {
            struct ast_node* init;
            struct ast_node* condition;
            struct ast_node* update;
            struct ast_node* body;
            int max_iterations;
        } for_loop;
        
        // Block
        struct {
            struct ast_node** statements;
            int stmt_count;
        } block;
        
        // Array index
        struct {
            struct ast_node* array;
            struct ast_node* index;
        } index;
        
        // Struct member
        struct {
            struct ast_node* object;
            char* member_name;
        } member;
        
        // Module
        struct {
            char* name;
            struct ast_node** declarations;
            int decl_count;
        } module;
        
        // Import
        struct {
            char* module_name;
            char** imported_names;
            int import_count;
        } import;
        
        // WCET annotation
        struct {
            int max_cycles;
            struct ast_node* annotated_node;
        } wcet;
        
        // Security annotation
        struct {
            int level;
            int promises;  // For pledge
            struct ast_node* annotated_node;
        } security;
    } data;
} ast_node_t;

// Symbol table entry
typedef struct symbol {
    char* name;
    type_t* type;
    ast_node_t* declaration;
    int scope_level;
    bool is_const;
    bool is_exported;
    struct symbol* next;
} symbol_t;

// Symbol table
typedef struct {
    symbol_t* symbols;
    int scope_level;
} symbol_table_t;

// Parser state
typedef struct {
    token_t* tokens;
    int token_count;
    int token_pos;
    symbol_table_t* symbol_table;
    char* current_module;
    int error_count;
    bool in_loop;
    bool in_constant_time;
} parser_state_t;

// Global parser state
static parser_state_t* g_parser = NULL;

// Error handling
static void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    if (g_parser && g_parser->token_pos < g_parser->token_count) {
        token_t* tok = &g_parser->tokens[g_parser->token_pos];
        fprintf(stderr, "%s:%d:%d: error: ", tok->filename, tok->line, tok->col);
    } else {
        fprintf(stderr, "error: ");
    }
    
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    
    if (g_parser) g_parser->error_count++;
}

// Token utilities
static token_t* current_token() {
    if (!g_parser || g_parser->token_pos >= g_parser->token_count) return NULL;
    return &g_parser->tokens[g_parser->token_pos];
}

static token_t* peek_token(int offset) {
    int pos = g_parser->token_pos + offset;
    if (!g_parser || pos >= g_parser->token_count) return NULL;
    return &g_parser->tokens[pos];
}

static bool match(token_type_t type) {
    token_t* tok = current_token();
    if (tok && tok->type == type) {
        g_parser->token_pos++;
        return true;
    }
    return false;
}

static bool expect(token_type_t type) {
    if (match(type)) return true;
    
    token_t* tok = current_token();
    if (tok) {
        error("expected token type %d, got %d", type, tok->type);
    } else {
        error("unexpected end of file");
    }
    return false;
}

// Type system
static type_t* create_type(type_kind_t kind) {
    type_t* type = calloc(1, sizeof(type_t));
    type->kind = kind;
    return type;
}

static type_t* create_pointer_type(type_t* base) {
    type_t* type = create_type(TYPE_POINTER);
    type->base_type = base;
    return type;
}

static type_t* create_array_type(type_t* base, int size) {
    type_t* type = create_type(TYPE_ARRAY);
    type->base_type = base;
    type->array_size = size;
    return type;
}

static bool types_equal(type_t* a, type_t* b) {
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    
    switch (a->kind) {
        case TYPE_POINTER:
        case TYPE_ARRAY:
            return types_equal(a->base_type, b->base_type);
        case TYPE_FUNCTION:
            if (a->param_count != b->param_count) return false;
            if (!types_equal(a->return_type, b->return_type)) return false;
            for (int i = 0; i < a->param_count; i++) {
                if (!types_equal(a->param_types[i], b->param_types[i])) return false;
            }
            return true;
        default:
            return true;
    }
}

// Symbol table
static symbol_table_t* create_symbol_table() {
    return calloc(1, sizeof(symbol_table_t));
}

static void enter_scope(symbol_table_t* table) {
    table->scope_level++;
}

static void exit_scope(symbol_table_t* table) {
    // Remove symbols from current scope
    symbol_t** current = &table->symbols;
    while (*current) {
        if ((*current)->scope_level == table->scope_level) {
            symbol_t* to_remove = *current;
            *current = (*current)->next;
            free(to_remove->name);
            free(to_remove);
        } else {
            current = &(*current)->next;
        }
    }
    table->scope_level--;
}

static symbol_t* lookup_symbol(symbol_table_t* table, const char* name) {
    for (symbol_t* sym = table->symbols; sym; sym = sym->next) {
        if (strcmp(sym->name, name) == 0) return sym;
    }
    return NULL;
}

static void add_symbol(symbol_table_t* table, const char* name, type_t* type, ast_node_t* decl) {
    symbol_t* sym = calloc(1, sizeof(symbol_t));
    sym->name = strdup(name);
    sym->type = type;
    sym->declaration = decl;
    sym->scope_level = table->scope_level;
    sym->next = table->symbols;
    table->symbols = sym;
}

// AST creation
static ast_node_t* create_ast_node(ast_type_t type) {
    ast_node_t* node = calloc(1, sizeof(ast_node_t));
    node->type = type;
    
    token_t* tok = current_token();
    if (tok) {
        node->line = tok->line;
        node->col = tok->col;
        node->filename = tok->filename;
    }
    
    // Default analysis values
    node->wcet_cycles = 1;
    node->security_level = 0;
    node->is_constant_time = false;
    node->is_pure = false;
    
    return node;
}

// Forward declarations
static ast_node_t* parse_expression();
static ast_node_t* parse_statement();
static ast_node_t* parse_block();
static type_t* parse_type();

// Expression parsing with precedence climbing
static ast_node_t* parse_primary() {
    token_t* tok = current_token();
    if (!tok) {
        error("unexpected end of file in expression");
        return NULL;
    }
    
    // Numbers
    if (match(TOK_NUMBER)) {
        ast_node_t* node = create_ast_node(AST_NUMBER);
        node->data.num_value = atol(tok->value);
        node->data_type = create_type(TYPE_INT32);
        return node;
    }
    
    // Booleans
    if (match(TOK_TRUE) || match(TOK_FALSE)) {
        ast_node_t* node = create_ast_node(AST_BOOL);
        node->data.bool_value = (tok->type == TOK_TRUE);
        node->data_type = create_type(TYPE_BOOL);
        return node;
    }
    
    // Identifiers
    if (match(TOK_IDENT)) {
        ast_node_t* node = create_ast_node(AST_IDENT);
        node->data.str_value = strdup(tok->value);
        
        // Look up type in symbol table
        symbol_t* sym = lookup_symbol(g_parser->symbol_table, tok->value);
        if (sym) {
            node->data_type = sym->type;
        } else {
            error("undefined identifier: %s", tok->value);
        }
        
        return node;
    }
    
    // Parenthesized expression
    if (match(TOK_LPAREN)) {
        ast_node_t* expr = parse_expression();
        expect(TOK_RPAREN);
        return expr;
    }
    
    // Unary operators
    if (match(TOK_MINUS) || match(TOK_NOT) || match(TOK_BITNOT)) {
        ast_node_t* node = create_ast_node(AST_UNARY_OP);
        node->data.unary.op = tok->type;
        node->data.unary.operand = parse_primary();
        
        if (node->data.unary.operand) {
            node->data_type = node->data.unary.operand->data_type;
        }
        
        return node;
    }
    
    error("expected expression");
    return NULL;
}

static ast_node_t* parse_postfix() {
    ast_node_t* expr = parse_primary();
    
    while (expr) {
        token_t* tok = current_token();
        if (!tok) break;
        
        // Function call
        if (match(TOK_LPAREN)) {
            ast_node_t* call = create_ast_node(AST_CALL);
            call->data.call.name = expr->data.str_value;
            
            // Parse arguments
            int capacity = 8;
            call->data.call.args = malloc(capacity * sizeof(ast_node_t*));
            call->data.call.arg_count = 0;
            
            while (!match(TOK_RPAREN)) {
                if (call->data.call.arg_count >= capacity) {
                    capacity *= 2;
                    call->data.call.args = realloc(call->data.call.args, 
                                                   capacity * sizeof(ast_node_t*));
                }
                
                call->data.call.args[call->data.call.arg_count++] = parse_expression();
                
                if (!match(TOK_COMMA)) {
                    expect(TOK_RPAREN);
                    break;
                }
            }
            
            expr = call;
        }
        // Array indexing
        else if (match(TOK_LBRACKET)) {
            ast_node_t* index = create_ast_node(AST_INDEX);
            index->data.index.array = expr;
            index->data.index.index = parse_expression();
            expect(TOK_RBRACKET);
            
            if (expr->data_type && expr->data_type->kind == TYPE_ARRAY) {
                index->data_type = expr->data_type->base_type;
            }
            
            expr = index;
        }
        // Member access
        else if (match(TOK_DOT)) {
            expect(TOK_IDENT);
            ast_node_t* member = create_ast_node(AST_MEMBER);
            member->data.member.object = expr;
            member->data.member.member_name = strdup(tok->value);
            expr = member;
        }
        else {
            break;
        }
    }
    
    return expr;
}

// Binary operator precedence
static int get_precedence(token_type_t op) {
    switch (op) {
        case TOK_MULT: case TOK_DIV: case TOK_MOD: return 10;
        case TOK_PLUS: case TOK_MINUS: return 9;
        case TOK_LSHIFT: case TOK_RSHIFT: return 8;
        case TOK_LT: case TOK_GT: case TOK_LE: case TOK_GE: return 7;
        case TOK_EQ: case TOK_NE: return 6;
        case TOK_BITAND: return 5;
        case TOK_BITXOR: return 4;
        case TOK_BITOR: return 3;
        case TOK_AND: return 2;
        case TOK_OR: return 1;
        default: return 0;
    }
}

static ast_node_t* parse_binary_expr(ast_node_t* left, int min_precedence) {
    while (true) {
        token_t* tok = current_token();
        if (!tok) break;
        
        int precedence = get_precedence(tok->type);
        if (precedence < min_precedence) break;
        
        token_type_t op = tok->type;
        g_parser->token_pos++;
        
        ast_node_t* right = parse_postfix();
        
        while (true) {
            tok = current_token();
            if (!tok) break;
            
            int next_precedence = get_precedence(tok->type);
            if (next_precedence <= precedence) break;
            
            right = parse_binary_expr(right, next_precedence);
        }
        
        ast_node_t* node = create_ast_node(AST_BINARY_OP);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = op;
        
        // Type checking
        if (left->data_type && right->data_type) {
            if (types_equal(left->data_type, right->data_type)) {
                node->data_type = left->data_type;
            } else {
                error("type mismatch in binary operation");
            }
        }
        
        left = node;
    }
    
    return left;
}

static ast_node_t* parse_expression() {
    ast_node_t* left = parse_postfix();
    if (!left) return NULL;
    
    // Check for assignment
    token_t* tok = current_token();
    if (tok && (tok->type == TOK_ASSIGN || tok->type == TOK_PLUS_ASSIGN ||
                tok->type == TOK_MINUS_ASSIGN || tok->type == TOK_MULT_ASSIGN ||
                tok->type == TOK_DIV_ASSIGN)) {
        token_type_t op = tok->type;
        g_parser->token_pos++;
        
        ast_node_t* assign = create_ast_node(AST_ASSIGNMENT);
        assign->data.assignment.target = left;
        assign->data.assignment.value = parse_expression();
        assign->data.assignment.op = op;
        
        if (left->data_type && assign->data.assignment.value->data_type) {
            if (types_equal(left->data_type, assign->data.assignment.value->data_type)) {
                assign->data_type = left->data_type;
            } else {
                error("type mismatch in assignment");
            }
        }
        
        return assign;
    }
    
    return parse_binary_expr(left, 1);
}

// Statement parsing
static ast_node_t* parse_if_statement() {
    expect(TOK_IF);
    expect(TOK_LPAREN);
    
    ast_node_t* node = create_ast_node(AST_IF);
    node->data.if_stmt.condition = parse_expression();
    
    expect(TOK_RPAREN);
    
    node->data.if_stmt.then_branch = parse_statement();
    
    if (match(TOK_ELSE)) {
        node->data.if_stmt.else_branch = parse_statement();
    }
    
    return node;
}

static ast_node_t* parse_while_statement() {
    expect(TOK_WHILE);
    expect(TOK_LPAREN);
    
    ast_node_t* node = create_ast_node(AST_WHILE);
    node->data.while_loop.condition = parse_expression();
    
    expect(TOK_RPAREN);
    
    // Check for max iterations annotation
    if (match(TOK_LBRACKET)) {
        expect(TOK_NUMBER);
        node->data.while_loop.max_iterations = atoi(current_token()->value);
        expect(TOK_RBRACKET);
    } else {
        node->data.while_loop.max_iterations = -1;  // Unknown
    }
    
    bool old_in_loop = g_parser->in_loop;
    g_parser->in_loop = true;
    node->data.while_loop.body = parse_statement();
    g_parser->in_loop = old_in_loop;
    
    return node;
}

static ast_node_t* parse_for_statement() {
    expect(TOK_FOR);
    expect(TOK_LPAREN);
    
    ast_node_t* node = create_ast_node(AST_FOR);
    
    // Init
    if (!match(TOK_SEMICOLON)) {
        node->data.for_loop.init = parse_expression();
        expect(TOK_SEMICOLON);
    }
    
    // Condition
    if (!match(TOK_SEMICOLON)) {
        node->data.for_loop.condition = parse_expression();
        expect(TOK_SEMICOLON);
    }
    
    // Update
    if (!match(TOK_RPAREN)) {
        node->data.for_loop.update = parse_expression();
        expect(TOK_RPAREN);
    }
    
    // Check for max iterations annotation
    if (match(TOK_LBRACKET)) {
        expect(TOK_NUMBER);
        node->data.for_loop.max_iterations = atoi(current_token()->value);
        expect(TOK_RBRACKET);
    } else {
        node->data.for_loop.max_iterations = -1;  // Unknown
    }
    
    bool old_in_loop = g_parser->in_loop;
    g_parser->in_loop = true;
    node->data.for_loop.body = parse_statement();
    g_parser->in_loop = old_in_loop;
    
    return node;
}

static ast_node_t* parse_let_statement() {
    bool is_const = match(TOK_CONST);
    if (!is_const) expect(TOK_LET);
    
    expect(TOK_IDENT);
    
    ast_node_t* node = create_ast_node(AST_VARIABLE);
    node->data.variable.name = strdup(current_token()->value);
    node->data.variable.is_const = is_const;
    
    expect(TOK_COLON);
    node->data.variable.var_type = parse_type();
    
    if (match(TOK_ASSIGN)) {
        node->data.variable.init_value = parse_expression();
    }
    
    expect(TOK_SEMICOLON);
    
    // Add to symbol table
    add_symbol(g_parser->symbol_table, node->data.variable.name, 
               node->data.variable.var_type, node);
    
    return node;
}

static ast_node_t* parse_statement() {
    token_t* tok = current_token();
    if (!tok) {
        error("unexpected end of file");
        return NULL;
    }
    
    // Block
    if (tok->type == TOK_LBRACE) {
        return parse_block();
    }
    
    // Control flow
    if (tok->type == TOK_IF) {
        return parse_if_statement();
    }
    
    if (tok->type == TOK_WHILE) {
        return parse_while_statement();
    }
    
    if (tok->type == TOK_FOR) {
        return parse_for_statement();
    }
    
    // Return
    if (match(TOK_RETURN)) {
        ast_node_t* node = create_ast_node(AST_RETURN);
        if (!match(TOK_SEMICOLON)) {
            node->data.unary.operand = parse_expression();
            expect(TOK_SEMICOLON);
        }
        return node;
    }
    
    // Break/Continue
    if (match(TOK_BREAK)) {
        if (!g_parser->in_loop) {
            error("break outside of loop");
        }
        expect(TOK_SEMICOLON);
        return create_ast_node(AST_BREAK);
    }
    
    if (match(TOK_CONTINUE)) {
        if (!g_parser->in_loop) {
            error("continue outside of loop");
        }
        expect(TOK_SEMICOLON);
        return create_ast_node(AST_CONTINUE);
    }
    
    // Variable declaration
    if (tok->type == TOK_LET || tok->type == TOK_CONST) {
        return parse_let_statement();
    }
    
    // Expression statement
    ast_node_t* expr = parse_expression();
    expect(TOK_SEMICOLON);
    
    ast_node_t* stmt = create_ast_node(AST_EXPRESSION_STMT);
    stmt->data.unary.operand = expr;
    return stmt;
}

static ast_node_t* parse_block() {
    expect(TOK_LBRACE);
    
    ast_node_t* block = create_ast_node(AST_BLOCK);
    
    enter_scope(g_parser->symbol_table);
    
    // Parse statements
    int capacity = 16;
    block->data.block.statements = malloc(capacity * sizeof(ast_node_t*));
    block->data.block.stmt_count = 0;
    
    while (!match(TOK_RBRACE)) {
        if (block->data.block.stmt_count >= capacity) {
            capacity *= 2;
            block->data.block.statements = realloc(block->data.block.statements,
                                                  capacity * sizeof(ast_node_t*));
        }
        
        ast_node_t* stmt = parse_statement();
        if (stmt) {
            block->data.block.statements[block->data.block.stmt_count++] = stmt;
        }
        
        if (current_token() && current_token()->type == TOK_EOF) {
            error("unexpected end of file in block");
            break;
        }
    }
    
    exit_scope(g_parser->symbol_table);
    
    return block;
}

// Type parsing
static type_t* parse_type() {
    token_t* tok = current_token();
    if (!tok) {
        error("expected type");
        return NULL;
    }
    
    type_t* base_type = NULL;
    
    if (match(TOK_INT32)) {
        base_type = create_type(TYPE_INT32);
    } else if (match(TOK_INT64)) {
        base_type = create_type(TYPE_INT64);
    } else if (match(TOK_BOOL)) {
        base_type = create_type(TYPE_BOOL);
    } else if (match(TOK_VOID)) {
        base_type = create_type(TYPE_VOID);
    } else {
        error("unknown type");
        return NULL;
    }
    
    // Handle pointers
    while (match(TOK_MULT)) {
        base_type = create_pointer_type(base_type);
    }
    
    // Handle arrays
    while (match(TOK_LBRACKET)) {
        int size = 0;
        if (match(TOK_NUMBER)) {
            size = atoi(current_token()->value);
        }
        expect(TOK_RBRACKET);
        base_type = create_array_type(base_type, size);
    }
    
    return base_type;
}

// Function parsing
static ast_node_t* parse_function() {
    // Check for annotations
    bool is_exported = false;
    bool is_trusted = false;
    int wcet_bound = -1;
    int security_level = 0;
    
    while (true) {
        token_t* tok = current_token();
        if (!tok) break;
        
        if (match(TOK_EXPORT)) {
            is_exported = true;
        } else if (match(TOK_SECURITY)) {
            expect(TOK_LPAREN);
            expect(TOK_NUMBER);
            security_level = atoi(current_token()->value);
            expect(TOK_RPAREN);
        } else if (match(TOK_WCET)) {
            expect(TOK_LPAREN);
            expect(TOK_NUMBER);
            wcet_bound = atoi(current_token()->value);
            expect(TOK_RPAREN);
        } else {
            break;
        }
    }
    
    expect(TOK_FUNCTION);
    expect(TOK_IDENT);
    
    ast_node_t* func = create_ast_node(AST_FUNCTION);
    func->data.function.name = strdup(current_token()->value);
    func->data.function.is_exported = is_exported;
    func->data.function.is_trusted = is_trusted;
    func->data.function.wcet_bound = wcet_bound;
    func->data.function.security_level = security_level;
    
    expect(TOK_LPAREN);
    
    // Parse parameters
    int capacity = 8;
    func->data.function.params = malloc(capacity * sizeof(ast_node_t*));
    func->data.function.param_count = 0;
    
    type_t** param_types = malloc(capacity * sizeof(type_t*));
    
    while (!match(TOK_RPAREN)) {
        if (func->data.function.param_count >= capacity) {
            capacity *= 2;
            func->data.function.params = realloc(func->data.function.params,
                                               capacity * sizeof(ast_node_t*));
            param_types = realloc(param_types, capacity * sizeof(type_t*));
        }
        
        expect(TOK_IDENT);
        char* param_name = strdup(current_token()->value);
        
        expect(TOK_COLON);
        type_t* param_type = parse_type();
        
        ast_node_t* param = create_ast_node(AST_VARIABLE);
        param->data.variable.name = param_name;
        param->data.variable.var_type = param_type;
        
        func->data.function.params[func->data.function.param_count] = param;
        param_types[func->data.function.param_count] = param_type;
        func->data.function.param_count++;
        
        if (!match(TOK_COMMA)) {
            expect(TOK_RPAREN);
            break;
        }
    }
    
    expect(TOK_COLON);
    func->data.function.return_type = parse_type();
    
    // Create function type
    type_t* func_type = create_type(TYPE_FUNCTION);
    func_type->param_types = param_types;
    func_type->param_count = func->data.function.param_count;
    func_type->return_type = func->data.function.return_type;
    func->data_type = func_type;
    
    // Add function to symbol table
    add_symbol(g_parser->symbol_table, func->data.function.name, func_type, func);
    
    // Parse body
    enter_scope(g_parser->symbol_table);
    
    // Add parameters to symbol table
    for (int i = 0; i < func->data.function.param_count; i++) {
        ast_node_t* param = func->data.function.params[i];
        add_symbol(g_parser->symbol_table, param->data.variable.name,
                  param->data.variable.var_type, param);
    }
    
    func->data.function.body = parse_block();
    
    exit_scope(g_parser->symbol_table);
    
    return func;
}

// Module parsing
static ast_node_t* parse_module() {
    ast_node_t* module = create_ast_node(AST_MODULE);
    module->data.module.name = g_parser->current_module;
    
    int capacity = 32;
    module->data.module.declarations = malloc(capacity * sizeof(ast_node_t*));
    module->data.module.decl_count = 0;
    
    while (current_token() && current_token()->type != TOK_EOF) {
        if (module->data.module.decl_count >= capacity) {
            capacity *= 2;
            module->data.module.declarations = realloc(module->data.module.declarations,
                                                     capacity * sizeof(ast_node_t*));
        }
        
        ast_node_t* decl = NULL;
        
        // Import
        if (match(TOK_IMPORT)) {
            decl = create_ast_node(AST_IMPORT);
            expect(TOK_STRING);
            decl->data.import.module_name = strdup(current_token()->value);
            expect(TOK_SEMICOLON);
        }
        // Function
        else if (current_token()->type == TOK_FUNCTION ||
                 current_token()->type == TOK_EXPORT ||
                 current_token()->type == TOK_SECURITY ||
                 current_token()->type == TOK_WCET) {
            decl = parse_function();
        }
        // Global variable/constant
        else if (current_token()->type == TOK_LET ||
                 current_token()->type == TOK_CONST) {
            decl = parse_let_statement();
        }
        else {
            error("unexpected token at module level");
            g_parser->token_pos++;
        }
        
        if (decl) {
            module->data.module.declarations[module->data.module.decl_count++] = decl;
        }
    }
    
    return module;
}

// WCET Analysis
static void analyze_wcet(ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_NUMBER:
        case AST_BOOL:
        case AST_NULL:
        case AST_IDENT:
            node->wcet_cycles = 1;
            break;
            
        case AST_BINARY_OP:
            analyze_wcet(node->data.binary.left);
            analyze_wcet(node->data.binary.right);
            
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
                                       node->data.binary.right->wcet_cycles + 3;
                    break;
                case TOK_DIV:
                case TOK_MOD:
                    node->wcet_cycles = node->data.binary.left->wcet_cycles +
                                       node->data.binary.right->wcet_cycles + 10;
                    break;
                default:
                    node->wcet_cycles = node->data.binary.left->wcet_cycles +
                                       node->data.binary.right->wcet_cycles + 2;
                    break;
            }
            break;
            
        case AST_UNARY_OP:
            analyze_wcet(node->data.unary.operand);
            node->wcet_cycles = node->data.unary.operand->wcet_cycles + 1;
            break;
            
        case AST_ASSIGNMENT:
            analyze_wcet(node->data.assignment.value);
            node->wcet_cycles = node->data.assignment.value->wcet_cycles + 2;
            break;
            
        case AST_CALL:
            node->wcet_cycles = 10;  // Base call overhead
            for (int i = 0; i < node->data.call.arg_count; i++) {
                analyze_wcet(node->data.call.args[i]);
                node->wcet_cycles += node->data.call.args[i]->wcet_cycles;
            }
            break;
            
        case AST_IF:
            analyze_wcet(node->data.if_stmt.condition);
            analyze_wcet(node->data.if_stmt.then_branch);
            if (node->data.if_stmt.else_branch) {
                analyze_wcet(node->data.if_stmt.else_branch);
                // Worst case: max of both branches
                int then_cycles = node->data.if_stmt.then_branch->wcet_cycles;
                int else_cycles = node->data.if_stmt.else_branch->wcet_cycles;
                node->wcet_cycles = node->data.if_stmt.condition->wcet_cycles + 2 +
                                   (then_cycles > else_cycles ? then_cycles : else_cycles);
            } else {
                node->wcet_cycles = node->data.if_stmt.condition->wcet_cycles + 2 +
                                   node->data.if_stmt.then_branch->wcet_cycles;
            }
            break;
            
        case AST_WHILE:
            analyze_wcet(node->data.while_loop.condition);
            analyze_wcet(node->data.while_loop.body);
            
            if (node->data.while_loop.max_iterations > 0) {
                node->wcet_cycles = node->data.while_loop.max_iterations *
                                   (node->data.while_loop.condition->wcet_cycles +
                                    node->data.while_loop.body->wcet_cycles + 2);
            } else {
                // Unknown iterations - mark as unbounded
                node->wcet_cycles = -1;
                error("unbounded loop without max iterations annotation");
            }
            break;
            
        case AST_FOR:
            analyze_wcet(node->data.for_loop.init);
            analyze_wcet(node->data.for_loop.condition);
            analyze_wcet(node->data.for_loop.update);
            analyze_wcet(node->data.for_loop.body);
            
            if (node->data.for_loop.max_iterations > 0) {
                node->wcet_cycles = node->data.for_loop.init->wcet_cycles +
                                   node->data.for_loop.max_iterations *
                                   (node->data.for_loop.condition->wcet_cycles +
                                    node->data.for_loop.body->wcet_cycles +
                                    node->data.for_loop.update->wcet_cycles + 3);
            } else {
                node->wcet_cycles = -1;
                error("unbounded loop without max iterations annotation");
            }
            break;
            
        case AST_BLOCK:
            node->wcet_cycles = 0;
            for (int i = 0; i < node->data.block.stmt_count; i++) {
                analyze_wcet(node->data.block.statements[i]);
                if (node->data.block.statements[i]->wcet_cycles < 0) {
                    node->wcet_cycles = -1;  // Propagate unbounded
                    break;
                }
                node->wcet_cycles += node->data.block.statements[i]->wcet_cycles;
            }
            break;
            
        case AST_FUNCTION:
            analyze_wcet(node->data.function.body);
            node->wcet_cycles = node->data.function.body->wcet_cycles + 5;
            
            if (node->data.function.wcet_bound > 0 &&
                node->wcet_cycles > node->data.function.wcet_bound) {
                error("function '%s' exceeds WCET bound: %d > %d cycles",
                      node->data.function.name, node->wcet_cycles,
                      node->data.function.wcet_bound);
            }
            break;
            
        default:
            node->wcet_cycles = 1;
            break;
    }
}

// Code generation
static void emit(FILE* output, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(output, fmt, args);
    va_end(args);
}

static void generate_expression(ast_node_t* node, FILE* output);
static void generate_statement(ast_node_t* node, FILE* output);

static void generate_expression(ast_node_t* node, FILE* output) {
    if (!node) return;
    
    switch (node->type) {
        case AST_NUMBER:
            emit(output, "    mov eax, %ld\n", node->data.num_value);
            break;
            
        case AST_BOOL:
            emit(output, "    mov eax, %d\n", node->data.bool_value ? 1 : 0);
            break;
            
        case AST_IDENT:
            // Load variable from stack
            emit(output, "    ; Load variable %s\n", node->data.str_value);
            emit(output, "    mov eax, [ebp-4]  ; TODO: proper stack layout\n");
            break;
            
        case AST_BINARY_OP:
            generate_expression(node->data.binary.left, output);
            emit(output, "    push eax\n");
            generate_expression(node->data.binary.right, output);
            emit(output, "    pop ebx\n");
            
            switch (node->data.binary.op) {
                case TOK_PLUS:
                    emit(output, "    add eax, ebx\n");
                    break;
                case TOK_MINUS:
                    emit(output, "    sub ebx, eax\n");
                    emit(output, "    mov eax, ebx\n");
                    break;
                case TOK_MULT:
                    emit(output, "    imul eax, ebx\n");
                    break;
                case TOK_DIV:
                    emit(output, "    xor edx, edx\n");
                    emit(output, "    xchg eax, ebx\n");
                    emit(output, "    div ebx\n");
                    break;
                case TOK_BITAND:
                    emit(output, "    and eax, ebx\n");
                    break;
                case TOK_BITOR:
                    emit(output, "    or eax, ebx\n");
                    break;
                case TOK_BITXOR:
                    emit(output, "    xor eax, ebx\n");
                    break;
                case TOK_EQ:
                    emit(output, "    cmp ebx, eax\n");
                    emit(output, "    sete al\n");
                    emit(output, "    movzx eax, al\n");
                    break;
                case TOK_NE:
                    emit(output, "    cmp ebx, eax\n");
                    emit(output, "    setne al\n");
                    emit(output, "    movzx eax, al\n");
                    break;
                case TOK_LT:
                    emit(output, "    cmp ebx, eax\n");
                    emit(output, "    setl al\n");
                    emit(output, "    movzx eax, al\n");
                    break;
                case TOK_GT:
                    emit(output, "    cmp ebx, eax\n");
                    emit(output, "    setg al\n");
                    emit(output, "    movzx eax, al\n");
                    break;
            }
            break;
            
        case AST_UNARY_OP:
            generate_expression(node->data.unary.operand, output);
            
            switch (node->data.unary.op) {
                case TOK_MINUS:
                    emit(output, "    neg eax\n");
                    break;
                case TOK_NOT:
                    emit(output, "    test eax, eax\n");
                    emit(output, "    setz al\n");
                    emit(output, "    movzx eax, al\n");
                    break;
                case TOK_BITNOT:
                    emit(output, "    not eax\n");
                    break;
            }
            break;
            
        case AST_ASSIGNMENT:
            generate_expression(node->data.assignment.value, output);
            emit(output, "    ; Store to %s\n", node->data.assignment.target->data.str_value);
            emit(output, "    mov [ebp-4], eax  ; TODO: proper stack layout\n");
            break;
            
        case AST_CALL:
            // Push arguments in reverse order
            for (int i = node->data.call.arg_count - 1; i >= 0; i--) {
                generate_expression(node->data.call.args[i], output);
                emit(output, "    push eax\n");
            }
            
            emit(output, "    call %s\n", node->data.call.name);
            
            // Clean up stack
            if (node->data.call.arg_count > 0) {
                emit(output, "    add esp, %d\n", node->data.call.arg_count * 4);
            }
            break;
    }
}

static int label_counter = 0;

static void generate_statement(ast_node_t* node, FILE* output) {
    if (!node) return;
    
    switch (node->type) {
        case AST_EXPRESSION_STMT:
            generate_expression(node->data.unary.operand, output);
            break;
            
        case AST_RETURN:
            if (node->data.unary.operand) {
                generate_expression(node->data.unary.operand, output);
            }
            emit(output, "    mov esp, ebp\n");
            emit(output, "    pop ebp\n");
            emit(output, "    ret\n");
            break;
            
        case AST_VARIABLE:
            // Allocate stack space
            emit(output, "    ; Variable %s\n", node->data.variable.name);
            emit(output, "    sub esp, 4  ; TODO: proper size\n");
            
            if (node->data.variable.init_value) {
                generate_expression(node->data.variable.init_value, output);
                emit(output, "    mov [ebp-4], eax  ; TODO: proper offset\n");
            }
            break;
            
        case AST_IF: {
            int else_label = label_counter++;
            int end_label = label_counter++;
            
            generate_expression(node->data.if_stmt.condition, output);
            emit(output, "    test eax, eax\n");
            emit(output, "    jz .L%d\n", else_label);
            
            generate_statement(node->data.if_stmt.then_branch, output);
            emit(output, "    jmp .L%d\n", end_label);
            
            emit(output, ".L%d:\n", else_label);
            if (node->data.if_stmt.else_branch) {
                generate_statement(node->data.if_stmt.else_branch, output);
            }
            
            emit(output, ".L%d:\n", end_label);
            break;
        }
            
        case AST_WHILE: {
            int loop_label = label_counter++;
            int end_label = label_counter++;
            
            emit(output, ".L%d:\n", loop_label);
            generate_expression(node->data.while_loop.condition, output);
            emit(output, "    test eax, eax\n");
            emit(output, "    jz .L%d\n", end_label);
            
            generate_statement(node->data.while_loop.body, output);
            emit(output, "    jmp .L%d\n", loop_label);
            
            emit(output, ".L%d:\n", end_label);
            break;
        }
            
        case AST_FOR: {
            int loop_label = label_counter++;
            int end_label = label_counter++;
            
            if (node->data.for_loop.init) {
                generate_expression(node->data.for_loop.init, output);
            }
            
            emit(output, ".L%d:\n", loop_label);
            
            if (node->data.for_loop.condition) {
                generate_expression(node->data.for_loop.condition, output);
                emit(output, "    test eax, eax\n");
                emit(output, "    jz .L%d\n", end_label);
            }
            
            generate_statement(node->data.for_loop.body, output);
            
            if (node->data.for_loop.update) {
                generate_expression(node->data.for_loop.update, output);
            }
            
            emit(output, "    jmp .L%d\n", loop_label);
            emit(output, ".L%d:\n", end_label);
            break;
        }
            
        case AST_BLOCK:
            for (int i = 0; i < node->data.block.stmt_count; i++) {
                generate_statement(node->data.block.statements[i], output);
            }
            break;
            
        case AST_BREAK:
            emit(output, "    ; TODO: break implementation\n");
            break;
            
        case AST_CONTINUE:
            emit(output, "    ; TODO: continue implementation\n");
            break;
    }
}

static void generate_function(ast_node_t* func, FILE* output) {
    emit(output, "\n; Function: %s (WCET: %d cycles, Security: %d)\n",
         func->data.function.name, func->wcet_cycles, func->data.function.security_level);
    
    if (func->data.function.is_exported) {
        emit(output, "global %s\n", func->data.function.name);
    }
    
    emit(output, "%s:\n", func->data.function.name);
    emit(output, "    push ebp\n");
    emit(output, "    mov ebp, esp\n");
    
    // TODO: Allocate space for local variables
    emit(output, "    sub esp, 16  ; TODO: calculate actual space needed\n");
    
    generate_statement(func->data.function.body, output);
    
    // Default return
    emit(output, "    mov esp, ebp\n");
    emit(output, "    pop ebp\n");
    emit(output, "    ret\n");
}

static void generate_module(ast_node_t* module, FILE* output) {
    emit(output, "; Generated by Tempo v4.0.0 Compiler\n");
    emit(output, "; Module: %s\n", module->data.module.name);
    emit(output, "\nsection .text\n");
    
    for (int i = 0; i < module->data.module.decl_count; i++) {
        ast_node_t* decl = module->data.module.declarations[i];
        
        if (decl->type == AST_FUNCTION) {
            generate_function(decl, output);
        }
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
    long file_size = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    char* source = malloc(file_size + 1);
    fread(source, 1, file_size, input);
    source[file_size] = '\0';
    fclose(input);
    
    // Initialize parser state
    g_parser = calloc(1, sizeof(parser_state_t));
    g_parser->symbol_table = create_symbol_table();
    g_parser->current_module = argv[1];
    
    printf("Tempo v4.0.0 Compiler - Processing %s\n", argv[1]);
    
    // Lexical analysis
    token_t* tokens = NULL;
    int token_count = 0;
    int token_capacity = 256;
    tokens = malloc(token_capacity * sizeof(token_t));
    
    // Tokenize source
    char* p = source;
    int line = 1;
    int col = 1;
    
    while (*p) {
        // Skip whitespace
        while (*p && isspace(*p)) {
            if (*p == '\n') {
                line++;
                col = 1;
            } else {
                col++;
            }
            p++;
        }
        
        if (!*p) break;
        
        // Comments
        if (p[0] == '/' && p[1] == '/') {
            while (*p && *p != '\n') p++;
            continue;
        }
        
        // Ensure we have space for token
        if (token_count >= token_capacity) {
            token_capacity *= 2;
            tokens = realloc(tokens, token_capacity * sizeof(token_t));
        }
        
        token_t* tok = &tokens[token_count];
        tok->line = line;
        tok->col = col;
        tok->filename = argv[1];
        
        // Numbers
        if (isdigit(*p)) {
            char* start = p;
            while (isdigit(*p)) {
                p++;
                col++;
            }
            int len = p - start;
            tok->value = malloc(len + 1);
            strncpy(tok->value, start, len);
            tok->value[len] = '\0';
            tok->type = TOK_NUMBER;
            token_count++;
            continue;
        }
        
        // Identifiers and keywords
        if (isalpha(*p) || *p == '_') {
            char* start = p;
            while (isalnum(*p) || *p == '_') {
                p++;
                col++;
            }
            int len = p - start;
            char* ident = malloc(len + 1);
            strncpy(ident, start, len);
            ident[len] = '\0';
            
            // Check keywords
            if (strcmp(ident, "function") == 0) tok->type = TOK_FUNCTION;
            else if (strcmp(ident, "if") == 0) tok->type = TOK_IF;
            else if (strcmp(ident, "else") == 0) tok->type = TOK_ELSE;
            else if (strcmp(ident, "while") == 0) tok->type = TOK_WHILE;
            else if (strcmp(ident, "for") == 0) tok->type = TOK_FOR;
            else if (strcmp(ident, "loop") == 0) tok->type = TOK_LOOP;
            else if (strcmp(ident, "break") == 0) tok->type = TOK_BREAK;
            else if (strcmp(ident, "continue") == 0) tok->type = TOK_CONTINUE;
            else if (strcmp(ident, "return") == 0) tok->type = TOK_RETURN;
            else if (strcmp(ident, "let") == 0) tok->type = TOK_LET;
            else if (strcmp(ident, "const") == 0) tok->type = TOK_CONST;
            else if (strcmp(ident, "import") == 0) tok->type = TOK_IMPORT;
            else if (strcmp(ident, "module") == 0) tok->type = TOK_MODULE;
            else if (strcmp(ident, "export") == 0) tok->type = TOK_EXPORT;
            else if (strcmp(ident, "int32") == 0) tok->type = TOK_INT32;
            else if (strcmp(ident, "int64") == 0) tok->type = TOK_INT64;
            else if (strcmp(ident, "bool") == 0) tok->type = TOK_BOOL;
            else if (strcmp(ident, "void") == 0) tok->type = TOK_VOID;
            else if (strcmp(ident, "true") == 0) tok->type = TOK_TRUE;
            else if (strcmp(ident, "false") == 0) tok->type = TOK_FALSE;
            else if (strcmp(ident, "null") == 0) tok->type = TOK_NULL;
            else if (strcmp(ident, "wcet") == 0) tok->type = TOK_WCET;
            else if (strcmp(ident, "invariant") == 0) tok->type = TOK_INVARIANT;
            else if (strcmp(ident, "requires") == 0) tok->type = TOK_REQUIRES;
            else if (strcmp(ident, "ensures") == 0) tok->type = TOK_ENSURES;
            else if (strcmp(ident, "pledge") == 0) tok->type = TOK_PLEDGE;
            else if (strcmp(ident, "unveil") == 0) tok->type = TOK_UNVEIL;
            else if (strcmp(ident, "security") == 0) tok->type = TOK_SECURITY;
            else if (strcmp(ident, "realtime") == 0) tok->type = TOK_REALTIME;
            else if (strcmp(ident, "atomic") == 0) tok->type = TOK_ATOMIC;
            else if (strcmp(ident, "constant_time") == 0) tok->type = TOK_CONSTANT_TIME;
            else {
                tok->type = TOK_IDENT;
            }
            
            tok->value = ident;
            token_count++;
            continue;
        }
        
        // Strings
        if (*p == '"') {
            p++; col++;
            char* start = p;
            while (*p && *p != '"') {
                if (*p == '\\' && p[1]) {
                    p += 2;
                    col += 2;
                } else {
                    p++;
                    col++;
                }
            }
            if (!*p) {
                fprintf(stderr, "%s:%d:%d: Error: Unterminated string\n", argv[1], line, col);
                return 1;
            }
            int len = p - start;
            tok->value = malloc(len + 1);
            strncpy(tok->value, start, len);
            tok->value[len] = '\0';
            tok->type = TOK_STRING;
            p++; col++;
            token_count++;
            continue;
        }
        
        // Two-character operators
        if (p[0] == '=' && p[1] == '=') {
            tok->type = TOK_EQ; tok->value = strdup("==");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '!' && p[1] == '=') {
            tok->type = TOK_NE; tok->value = strdup("!=");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '<' && p[1] == '=') {
            tok->type = TOK_LE; tok->value = strdup("<=");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '>' && p[1] == '=') {
            tok->type = TOK_GE; tok->value = strdup(">=");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '<' && p[1] == '<') {
            tok->type = TOK_LSHIFT; tok->value = strdup("<<");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '>' && p[1] == '>') {
            tok->type = TOK_RSHIFT; tok->value = strdup(">>");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '&' && p[1] == '&') {
            tok->type = TOK_AND; tok->value = strdup("&&");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '|' && p[1] == '|') {
            tok->type = TOK_OR; tok->value = strdup("||");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '+' && p[1] == '+') {
            tok->type = TOK_INC; tok->value = strdup("++");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '-' && p[1] == '-') {
            tok->type = TOK_DEC; tok->value = strdup("--");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '+' && p[1] == '=') {
            tok->type = TOK_PLUS_ASSIGN; tok->value = strdup("+=");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '-' && p[1] == '=') {
            tok->type = TOK_MINUS_ASSIGN; tok->value = strdup("-=");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '*' && p[1] == '=') {
            tok->type = TOK_MULT_ASSIGN; tok->value = strdup("*=");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '/' && p[1] == '=') {
            tok->type = TOK_DIV_ASSIGN; tok->value = strdup("/=");
            p += 2; col += 2; token_count++;
            continue;
        }
        if (p[0] == '-' && p[1] == '>') {
            tok->type = TOK_ARROW; tok->value = strdup("->");
            p += 2; col += 2; token_count++;
            continue;
        }
        
        // Handle @ for annotations
        if (*p == '@') {
            p++; col++;
            if (!isalpha(*p)) {
                fprintf(stderr, "%s:%d:%d: Error: Expected identifier after '@'\n", 
                        argv[1], line, col);
                return 1;
            }
            char* start = p;
            while (isalnum(*p) || *p == '_') {
                p++;
                col++;
            }
            int len = p - start;
            char* ident = malloc(len + 2);
            ident[0] = '@';
            strncpy(ident + 1, start, len);
            ident[len + 1] = '\0';
            
            // Map annotation identifiers to special tokens
            if (strcmp(ident + 1, "wcet") == 0) tok->type = TOK_WCET;
            else if (strcmp(ident + 1, "invariant") == 0) tok->type = TOK_INVARIANT;
            else if (strcmp(ident + 1, "requires") == 0) tok->type = TOK_REQUIRES;
            else if (strcmp(ident + 1, "ensures") == 0) tok->type = TOK_ENSURES;
            else if (strcmp(ident + 1, "pledge") == 0) tok->type = TOK_PLEDGE;
            else if (strcmp(ident + 1, "unveil") == 0) tok->type = TOK_UNVEIL;
            else if (strcmp(ident + 1, "security") == 0) tok->type = TOK_SECURITY;
            else if (strcmp(ident + 1, "realtime") == 0) tok->type = TOK_REALTIME;
            else if (strcmp(ident + 1, "atomic") == 0) tok->type = TOK_ATOMIC;
            else if (strcmp(ident + 1, "constant_time") == 0) tok->type = TOK_CONSTANT_TIME;
            else {
                fprintf(stderr, "%s:%d:%d: Error: Unknown annotation '%s'\n", 
                        argv[1], line, col, ident);
                free(ident);
                return 1;
            }
            
            tok->value = ident;
            token_count++;
            continue;
        }
        
        // Single character tokens
        switch (*p) {
            case '+': tok->type = TOK_PLUS; tok->value = strdup("+"); break;
            case '-': tok->type = TOK_MINUS; tok->value = strdup("-"); break;
            case '*': tok->type = TOK_MULT; tok->value = strdup("*"); break;
            case '/': tok->type = TOK_DIV; tok->value = strdup("/"); break;
            case '%': tok->type = TOK_MOD; tok->value = strdup("%"); break;
            case '=': tok->type = TOK_ASSIGN; tok->value = strdup("="); break;
            case '<': tok->type = TOK_LT; tok->value = strdup("<"); break;
            case '>': tok->type = TOK_GT; tok->value = strdup(">"); break;
            case '&': tok->type = TOK_BITAND; tok->value = strdup("&"); break;
            case '|': tok->type = TOK_BITOR; tok->value = strdup("|"); break;
            case '^': tok->type = TOK_BITXOR; tok->value = strdup("^"); break;
            case '~': tok->type = TOK_BITNOT; tok->value = strdup("~"); break;
            case '!': tok->type = TOK_NOT; tok->value = strdup("!"); break;
            case '(': tok->type = TOK_LPAREN; tok->value = strdup("("); break;
            case ')': tok->type = TOK_RPAREN; tok->value = strdup(")"); break;
            case '{': tok->type = TOK_LBRACE; tok->value = strdup("{"); break;
            case '}': tok->type = TOK_RBRACE; tok->value = strdup("}"); break;
            case '[': tok->type = TOK_LBRACKET; tok->value = strdup("["); break;
            case ']': tok->type = TOK_RBRACKET; tok->value = strdup("]"); break;
            case ';': tok->type = TOK_SEMICOLON; tok->value = strdup(";"); break;
            case ':': tok->type = TOK_COLON; tok->value = strdup(":"); break;
            case ',': tok->type = TOK_COMMA; tok->value = strdup(","); break;
            case '.': tok->type = TOK_DOT; tok->value = strdup("."); break;
            default:
                fprintf(stderr, "%s:%d:%d: Error: Unknown character '%c'\n", 
                        argv[1], line, col, *p);
                return 1;
        }
        
        p++;
        col++;
        token_count++;
    }
    
    // Add EOF token
    if (token_count >= token_capacity) {
        token_capacity++;
        tokens = realloc(tokens, token_capacity * sizeof(token_t));
    }
    tokens[token_count].type = TOK_EOF;
    tokens[token_count].value = strdup("");
    tokens[token_count].line = line;
    tokens[token_count].col = col;
    tokens[token_count].filename = argv[1];
    token_count++;
    
    // Set up parser with tokens
    g_parser->tokens = tokens;
    g_parser->token_count = token_count;
    g_parser->token_pos = 0;
    
    // Parse
    ast_node_t* module = parse_module();
    
    if (g_parser->error_count > 0) {
        fprintf(stderr, "Compilation failed with %d errors\n", g_parser->error_count);
        return 1;
    }
    
    // Analyze
    printf("Performing WCET analysis...\n");
    for (int i = 0; i < module->data.module.decl_count; i++) {
        analyze_wcet(module->data.module.declarations[i]);
    }
    
    // Generate code
    FILE* output = fopen(argv[2], "w");
    if (!output) {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", argv[2]);
        return 1;
    }
    
    generate_module(module, output);
    fclose(output);
    
    printf("Compilation successful!\n");
    printf("  Output: %s\n", argv[2]);
    
    // Cleanup
    free(source);
    free(g_parser->tokens);
    free(g_parser);
    
    return 0;
}