#include "java_ast.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// 子节点容量扩容步长。
#define AST_CHILD_GROW_STEP 4u

// 安全分配内存（失败直接退出）。
static void *xcalloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (!ptr) {
        fprintf(stderr, "java_ast: memory allocation failed\n");
        abort();
    }
    return ptr;
}

// 复制字符串（失败直接退出）。
static char *xstrdup(const char *src) {
    if (!src) {
        return NULL;
    }
    size_t len = strlen(src) + 1u;
    char *dup = malloc(len);
    if (dup) {
        memcpy(dup, src, len);
    }
    if (!dup) {
        fprintf(stderr, "java_ast: string duplication failed\n");
        abort();
    }
    return dup;
}

// 确保子节点数组容量满足需求。
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

// 创建 AST 节点。
AstNode *ast_node_create(AstKind kind, const char *text, int line, int column) {
    AstNode *node = xcalloc(1, sizeof(*node));
    node->kind = kind;
    node->text = xstrdup(text);
    node->scope = NULL;
    return node;
}

// 创建叶子节点。
AstNode *ast_leaf(AstKind kind, const char *text, int line, int column) {
    return ast_node_create(kind, text, line, column);
}

// 创建分支节点并追加子节点。
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

// 设置节点文本（深拷贝）。
void ast_set_text(AstNode *node, const char *text) {
    if (!node) {
        return;
    }
    free(node->text);
    node->text = xstrdup(text);
}

// 预留子节点容量。
void ast_reserve_children(AstNode *parent, size_t capacity) {
    if (!parent) {
        return;
    }
    ensure_capacity(parent, capacity);
}

// 追加子节点。
void ast_add_child(AstNode *parent, AstNode *child) {
    if (!parent || !child) {
        return;
    }
    ensure_capacity(parent, parent->child_count + 1u);
    parent->children[parent->child_count++] = child;
}

// 追加多个子节点。
void ast_add_children(AstNode *parent, AstNode **children, size_t count) {
    if (!parent || !children || count == 0) {
        return;
    }
    ensure_capacity(parent, parent->child_count + count);
    for (size_t i = 0; i < count; ++i) {
        parent->children[parent->child_count++] = children[i];
    }
}

// 节点类型名称表。
static const char *kind_name_table[] = {
    [AST_COMPILATION_UNIT] = "CompilationUnit",
    [AST_PACKAGE_DECL] = "PackageDeclaration",
    [AST_IMPORT_DECL] = "ImportDeclaration",
    [AST_MODULE_DECL] = "ModuleDeclaration",
    [AST_REQUIRES_DIRECTIVE] = "RequiresDirective",
    [AST_EXPORTS_DIRECTIVE] = "ExportsDirective",
    [AST_OPENS_DIRECTIVE] = "OpensDirective",
    [AST_USES_DIRECTIVE] = "UsesDirective",
    [AST_PROVIDES_DIRECTIVE] = "ProvidesDirective",
    [AST_CLASS_DECL] = "ClassDeclaration",
    [AST_INTERFACE_DECL] = "InterfaceDeclaration",
    [AST_ANNOTATION_DECL] = "AnnotationTypeDeclaration",
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
    [AST_RESOURCE_DECL] = "ResourceDeclaration",
    [AST_EXCEPTION_TYPE_LIST] = "ExceptionTypeList",
    [AST_TYPE_PARAMETER] = "TypeParameter",
    [AST_TYPE_PARAMETER_LIST] = "TypeParameterList",
    [AST_TYPE_ARGUMENT] = "TypeArgument",
    [AST_TYPE_ARGUMENT_TYPE] = "TypeArgumentType",
    [AST_TYPE_ARGUMENT_WILDCARD] = "TypeArgumentWildcard",
    [AST_TYPE_ARGUMENT_LIST] = "TypeArgumentList",
    [AST_TYPE_BOUND_LIST] = "TypeBoundList",
    [AST_TYPE_BOUND] = "TypeBound",
    [AST_ADDITIONAL_BOUNDS] = "AdditionalBounds",
    [AST_WILDCARD] = "Wildcard",
    [AST_WILDCARD_BOUND] = "WildcardBound",
    [AST_TYPE_PATTERN] = "TypePattern",
    [AST_EXTENDS] = "ExtendsClause",
    [AST_IMPLEMENTS] = "ImplementsClause",
    [AST_PERMITS] = "PermitsClause",
    [AST_MODIFIER_LIST] = "ModifierList",
    [AST_ANNOTATION] = "Annotation",
    [AST_DIM] = "Dim",
    [AST_DIM_LIST] = "DimList",
    [AST_BLOCK] = "Block",
    [AST_RESOURCE] = "Resource",
    [AST_STATEMENT_LIST] = "StatementList",
    [AST_STATIC_INIT] = "StaticInitializer",
    [AST_INSTANCE_INIT] = "InstanceInitializer",
    [AST_IF] = "IfStatement",
    [AST_ELSE_CLAUSE] = "ElseClause",
    [AST_SWITCH] = "SwitchStatement",
    [AST_SWITCH_EXPR] = "SwitchExpression",
    [AST_SWITCH_LABEL] = "SwitchLabel",
    [AST_SWITCH_GROUP] = "SwitchGroup",
    [AST_SWITCH_RULE] = "SwitchRule",
    [AST_SWITCH_RULE_LIST] = "SwitchRuleList",
    [AST_SWITCH_LABEL_LIST] = "SwitchLabelList",
    [AST_FOR] = "ForStatement",
    [AST_FOR_INIT] = "ForInit",
    [AST_FOR_COND] = "ForCondition",
    [AST_FOR_UPDATE] = "ForUpdate",
    [AST_FOR_INIT_LIST] = "ForInitList",
    [AST_FOR_UPDATE_LIST] = "ForUpdateList",
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
    [AST_DEFAULT_VALUE] = "DefaultValue",
    [AST_BINARY_EXPR] = "BinaryExpression",
    [AST_INSTANCEOF] = "InstanceofExpression",
    [AST_UNARY_EXPR] = "UnaryExpression",
    [AST_CAST] = "CastExpression",
    [AST_CONDITIONAL_EXPR] = "ConditionalExpression",
    [AST_LITERAL] = "Literal",
    [AST_INT_LITERAL] = "IntLiteral",
    [AST_FLOAT_LITERAL] = "FloatLiteral",
    [AST_STRING_LITERAL] = "StringLiteral",
    [AST_TEXT_BLOCK] = "TextBlock",
    [AST_CHAR_LITERAL] = "CharLiteral",
    [AST_BOOL_LITERAL] = "BoolLiteral",
    [AST_NULL_LITERAL] = "NullLiteral",
    [AST_CLASS_LITERAL] = "ClassLiteral",
    [AST_THIS_EXPR] = "ThisExpression",
    [AST_ENUM_CONST] = "EnumConstant",
    [AST_IDENTIFIER] = "Identifier",
    [AST_MEMBER_ACCESS] = "MemberAccess",
    [AST_METHOD_REFERENCE] = "MethodReference",
    [AST_ARRAY_ACCESS] = "ArrayAccess",
    [AST_ARRAY_INIT] = "ArrayInitializer",
    [AST_METHOD_INVOCATION] = "MethodInvocation",
    [AST_EXPLICIT_CTOR_INVOCATION] = "ExplicitConstructorInvocation",
    [AST_ARGUMENT_LIST] = "ArgumentList",
    [AST_EXPRESSION_LIST] = "ExpressionList",
    [AST_DIM_EXPR_LIST] = "DimExprList",
    [AST_NEW_CLASS] = "ObjectCreation",
    [AST_ARRAY_CREATION] = "ArrayCreation",
    [AST_LAMBDA] = "LambdaExpression",
    [AST_VARIABLE_DECL] = "VariableDeclaration",
    [AST_EMPTY] = "Empty",
    [AST_IMPORT_LIST] = "ImportList",
    [AST_TYPE_DECL_LIST] = "TypeDeclList"
};

// 获取节点类型名称。
const char *ast_kind_name(AstKind kind) {
    if (kind < 0 || kind >= (int)(sizeof(kind_name_table) / sizeof(kind_name_table[0]))) {
        return "InvalidKind";
    }
    const char *name = kind_name_table[kind];
    return name ? name : "UnnamedKind";
}

// 输出缩进。
static void print_indent(FILE *out, int indent);

#ifdef DEBUG_SCOPE
static bool scope_kind_needs_print(AstKind kind) {
    switch (kind) {
    case AST_METHOD_INVOCATION:
    case AST_MEMBER_ACCESS:
    case AST_NEW_CLASS:
    case AST_METHOD_REFERENCE:
        return true;
    default:
        return false;
    }
}

static const char *method_invocation_name(const AstNode *node) {
    if (!node) {
        return NULL;
    }
    if (node->child_count >= 2 && node->children[1] &&
        node->children[1]->kind == AST_IDENTIFIER && node->children[1]->text) {
        return node->children[1]->text;
    }
    if (node->child_count >= 1 && node->children[0] &&
        node->children[0]->kind == AST_IDENTIFIER && node->children[0]->text) {
        return node->children[0]->text;
    }
    return NULL;
}

static const char *member_access_name(const AstNode *node) {
    if (!node) {
        return NULL;
    }
    if (node->child_count >= 2 && node->children[1] &&
        node->children[1]->kind == AST_IDENTIFIER && node->children[1]->text) {
        return node->children[1]->text;
    }
    return NULL;
}

static const char *type_name_hint(const AstNode *node) {
    if (!node) {
        return NULL;
    }
    if (node->child_count >= 1 && node->children[0] &&
        node->children[0]->kind == AST_IDENTIFIER && node->children[0]->text) {
        return node->children[0]->text;
    }
    return NULL;
}

static void print_scope_info(const AstNode *node, FILE *out, int indent) {
    if (!node || !scope_kind_needs_print(node->kind)) {
        return;
    }
    print_indent(out, indent + 2);
    if (!node->scope) {
        fprintf(out, "scope: NULL\n");
        return;
    }

    const AstNode *scope = node->scope;
    const char *kind_name = ast_kind_name(scope->kind);
    const char *detail = NULL;
    bool quote_detail = false;

    if (scope->kind == AST_TYPE) {
        const char *inner = type_name_hint(scope);
        if (inner) {
            kind_name = "Identifier";
            detail = inner;
            quote_detail = true;
        }
    }

    if (!detail && scope->text && scope->text[0] != '\0') {
        detail = scope->text;
        quote_detail = true;
    } else if (!detail && scope->kind == AST_METHOD_INVOCATION) {
        detail = method_invocation_name(scope);
    } else if (!detail && scope->kind == AST_MEMBER_ACCESS) {
        detail = member_access_name(scope);
    }

    if (detail) {
        if (quote_detail) {
            fprintf(out, "scope: %s('%s')\n", kind_name, detail);
        } else {
            fprintf(out, "scope: %s(%s)\n", kind_name, detail);
        }
    } else {
        fprintf(out, "scope: %s\n", kind_name);
    }
}
#endif

// 输出缩进。
static void print_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; ++i) {
        fputc(' ', out);
    }
}

// 打印 AST。
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
#ifdef DEBUG_SCOPE
    print_scope_info(node, out, indent);
#endif
    for (size_t i = 0; i < node->child_count; ++i) {
        ast_print(node->children[i], out, indent + 2);
    }
}

// 释放 AST。
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

