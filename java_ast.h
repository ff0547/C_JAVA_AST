#ifndef JAVA_AST_H
#define JAVA_AST_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum AstKind {
    AST_COMPILATION_UNIT,
    AST_PACKAGE_DECL,
    AST_IMPORT_DECL,
    AST_MODULE_DECL,
    AST_CLASS_DECL,
    AST_INTERFACE_DECL,
    AST_ENUM_DECL,
    AST_RECORD_DECL,
    AST_FIELD_DECL,
    AST_LOCAL_VAR_DECL,
    AST_METHOD_DECL,
    AST_CONSTRUCTOR_DECL,
    AST_PARAMETER,
    AST_TYPE,
    AST_TYPE_PARAMETER,
    AST_TYPE_ARGUMENT,
    AST_MODIFIER_LIST,
    AST_ANNOTATION,
    AST_DIM,
    AST_DIM_LIST,
    AST_BLOCK,
    AST_ANNOTATION_LIST,
    AST_STATEMENT_LIST,
    AST_STATIC_INIT,
    AST_INSTANCE_INIT,
    AST_IF,
    AST_SWITCH,
    AST_SWITCH_LABEL,
    AST_FOR,
    AST_VAR_DECL_LIST,
    AST_FOR_EACH,
    AST_WHILE,
    AST_DO_WHILE,
    AST_TRY,
    AST_CATCH,
    AST_FINALLY,
    AST_SYNCHRONIZED,
    AST_ASSERT,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_THROW,
    AST_YIELD,
    AST_LABELED_STATEMENT,
    AST_EXPRESSION,
    AST_ASSIGN,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_CONDITIONAL_EXPR,
    AST_LITERAL,
    AST_ENUM_CONST,
    AST_IDENTIFIER,
    AST_RESOURCE_SPEC,
    AST_RESOURCE_LIST,
    AST_RESOURCE,
    AST_MEMBER_ACCESS,
    AST_ARRAY_ACCESS,
    AST_ARRAY_INIT,
    AST_METHOD_INVOCATION,
    AST_ARGUMENT_LIST,
    AST_EXCEPTION_TYPE_LIST,
    AST_NEW_CLASS,
    AST_LAMBDA,
    AST_VARIABLE_DECL,
    AST_EMPTY,
    AST_UNKNOWN,
    AST_IMPORT_LIST,
    AST_TYPE_DECL_LIST
} AstKind;

typedef struct AstNode {
    AstKind kind;
    char *text;
    struct AstNode **children;
    size_t child_count;
    size_t child_capacity;
} AstNode;

#ifdef __cplusplus
extern "C" {
#endif

AstNode *ast_node_create(AstKind kind, const char *text, int line, int column);
AstNode *ast_leaf(AstKind kind, const char *text, int line, int column);
AstNode *ast_branch(AstKind kind, int line, int column, size_t child_count, ...);

void ast_set_text(AstNode *node, const char *text);
void ast_add_child(AstNode *parent, AstNode *child);
void ast_add_children(AstNode *parent, AstNode **children, size_t count);
void ast_reserve_children(AstNode *parent, size_t capacity);

const char *ast_kind_name(AstKind kind);
void ast_print(const AstNode *node, FILE *out, int indent);
void ast_free(AstNode *node);

#ifdef __cplusplus
}
#endif

#endif /* JAVA_AST_H */
