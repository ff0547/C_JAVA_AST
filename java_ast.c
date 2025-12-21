#include "java_ast.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define AST_CHILD_GROW_STEP 4u

static void *xcalloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (!ptr) {
        fprintf(stderr, "java_ast: memory allocation failed\n");
        abort();
    }
    return ptr;
}

static char *xstrdup(const char *src) {
    if (!src) {
        return NULL;
    }
#if defined(_MSC_VER)
    char *dup = _strdup(src);
#else
    char *dup = strdup(src);
#endif
    if (!dup) {
        fprintf(stderr, "java_ast: string duplication failed\n");
        abort();
    }
    return dup;
}

static void ensure_capacity(AstNode *node, size_t min_capacity) {
    if (!node) {
        return;
    }
    if (node->child_capacity >= min_capacity) {
        return;
    }
    size_t new_capacity = node->child_capacity ? node->child_capacity : AST_CHILD_GROW_STEP;
    while (new_capacity < min_capacity) {
        new_capacity += AST_CHILD_GROW_STEP;
    }
    node->children = realloc(node->children, new_capacity * sizeof(*node->children));
    if (!node->children) {
        fprintf(stderr, "java_ast: child reallocation failed\n");
        abort();
    }
    memset(node->children + node->child_capacity, 0,
           (new_capacity - node->child_capacity) * sizeof(*node->children));
    node->child_capacity = new_capacity;
}

AstNode *ast_node_create(AstKind kind, const char *text, int line, int column) {
    AstNode *node = xcalloc(1, sizeof(*node));
    node->kind = kind;
    node->text = xstrdup(text);
    return node;
}


AstNode *ast_leaf(AstKind kind, const char *text, int line, int column) {
    return ast_node_create(kind, text, line, column);
}

AstNode *ast_branch(AstKind kind, int line, int column, size_t child_count, ...) {
    AstNode *node = ast_node_create(kind, NULL, line, column);
    if (child_count == 0) {
        return node;
    }

    va_list args;
    va_start(args, child_count);
    for (size_t i = 0; i < child_count; ++i) {
        AstNode *child = va_arg(args, AstNode *);
        ast_add_child(node, child);
    }
    va_end(args);
    return node;
}

void ast_set_text(AstNode *node, const char *text) {
    if (!node) {
        return;
    }
    free(node->text);
    node->text = xstrdup(text);
}

void ast_reserve_children(AstNode *parent, size_t capacity) {
    if (!parent) {
        return;
    }
    ensure_capacity(parent, capacity);
}

void ast_add_child(AstNode *parent, AstNode *child) {
    if (!parent || !child) {
        return;
    }
    ensure_capacity(parent, parent->child_count + 1u);
    parent->children[parent->child_count++] = child;
}

void ast_add_children(AstNode *parent, AstNode **children, size_t count) {
    if (!parent || !children || count == 0) {
        return;
    }
    ensure_capacity(parent, parent->child_count + count);
    for (size_t i = 0; i < count; ++i) {
        parent->children[parent->child_count++] = children[i];
    }
}

static const char *kind_name_table[] = {
    [AST_COMPILATION_UNIT] = "CompilationUnit",
    [AST_PACKAGE_DECL] = "PackageDeclaration",
    [AST_IMPORT_DECL] = "ImportDeclaration",
    [AST_MODULE_DECL] = "ModuleDeclaration",
    [AST_CLASS_DECL] = "ClassDeclaration",
    [AST_INTERFACE_DECL] = "InterfaceDeclaration",
    [AST_ENUM_DECL] = "EnumDeclaration",
    [AST_RECORD_DECL] = "RecordDeclaration",
    [AST_FIELD_DECL] = "FieldDeclaration",
    [AST_LOCAL_VAR_DECL] = "LocalVariableDeclaration",
    [AST_METHOD_DECL] = "MethodDeclaration",
    [AST_CONSTRUCTOR_DECL] = "ConstructorDeclaration",
    [AST_PARAMETER] = "Parameter",
    [AST_TYPE] = "Type",
    [AST_ANNOTATION_LIST] = "AnnotationList",
    [AST_VAR_DECL_LIST] = "VarDeclList",
    [AST_RESOURCE_SPEC] = "ResourceSpec",
    [AST_RESOURCE_LIST] = "ResourceList",
    [AST_EXCEPTION_TYPE_LIST] = "ExceptionTypeList",
    [AST_TYPE_PARAMETER] = "TypeParameter",
    [AST_TYPE_ARGUMENT] = "TypeArgument",
    [AST_MODIFIER_LIST] = "ModifierList",
    [AST_ANNOTATION] = "Annotation",
    [AST_DIM] = "Dim",
    [AST_DIM_LIST] = "DimList",
    [AST_BLOCK] = "Block",
    [AST_RESOURCE]="Resource",
    [AST_STATEMENT_LIST] = "StatementList",
    [AST_IF] = "IfStatement",
    [AST_SWITCH] = "SwitchStatement",
    [AST_SWITCH_LABEL] = "SwitchLabel",
    [AST_FOR] = "ForStatement",
    [AST_FOR_EACH] = "ForEachStatement",
    [AST_WHILE] = "WhileStatement",
    [AST_DO_WHILE] = "DoWhileStatement",
    [AST_TRY] = "TryStatement",
    [AST_CATCH] = "CatchClause",
    [AST_FINALLY] = "FinallyClause",
    [AST_SYNCHRONIZED] = "SynchronizedStatement",
    [AST_ASSERT] = "AssertStatement",
    [AST_RETURN] = "ReturnStatement",
    [AST_BREAK] = "BreakStatement",
    [AST_CONTINUE] = "ContinueStatement",
    [AST_THROW] = "ThrowStatement",
    [AST_YIELD] = "YieldStatement",
    [AST_LABELED_STATEMENT] = "LabeledStatement",
    [AST_EXPRESSION] = "Expression",
    [AST_ASSIGN] = "Assignment",
    [AST_BINARY_EXPR] = "BinaryExpression",
    [AST_UNARY_EXPR] = "UnaryExpression",
    [AST_CONDITIONAL_EXPR] = "ConditionalExpression",
    [AST_LITERAL] = "Literal",
    [AST_IDENTIFIER] = "Identifier",
    [AST_MEMBER_ACCESS] = "MemberAccess",
    [AST_ARRAY_ACCESS] = "ArrayAccess",
    [AST_ARRAY_INIT] = "ArrayInitializer",
    [AST_METHOD_INVOCATION] = "MethodInvocation",
    [AST_ARGUMENT_LIST] = "ArgumentList",
    [AST_NEW_CLASS] = "ObjectCreation",
    [AST_LAMBDA] = "LambdaExpression",
    [AST_VARIABLE_DECL] = "VariableDeclaration",
    [AST_EMPTY] = "Empty",
    [AST_IMPORT_LIST] = "ImportList",
    [AST_TYPE_DECL_LIST] = "TypeDeclList",
    [AST_UNKNOWN] = "Unknown"
};

const char *ast_kind_name(AstKind kind) {
    if (kind < 0 || kind >= (int)(sizeof(kind_name_table) / sizeof(kind_name_table[0]))) {
        return "InvalidKind";
    }
    const char *name = kind_name_table[kind];
    return name ? name : "UnnamedKind";
}

static void print_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; ++i) {
        fputc(' ', out);
    }
}

void ast_print(const AstNode *node, FILE *out, int indent) {
    if (!node || !out) {
        return;
    }
    print_indent(out, indent);
    fprintf(out, "%s", ast_kind_name(node->kind));
    if (node->text) {
        fprintf(out, "('%s')", node->text);
    }
    fputc('\n', out);
    for (size_t i = 0; i < node->child_count; ++i) {
        ast_print(node->children[i], out, indent + 2);
    }
}

void ast_free(AstNode *node) {
    if (!node) {
        return;
    }
    for (size_t i = 0; i < node->child_count; ++i) {
        ast_free(node->children[i]);
    }
    free(node->children);
    free(node->text);
    free(node);
}

