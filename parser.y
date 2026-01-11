
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>           // 添加这个头文件以支持 isspace、isalpha 和 isalnum
#include "parser.tab.h"
#include "java_ast.h"

#define AST_LOC_LINE(loc) ((loc).first_line)
#define AST_LOC_COL(loc) ((loc).first_column)

#define AST_BRANCH_AT(kind, loc, child_count, ...) \
    ast_branch((kind), AST_LOC_LINE(loc), AST_LOC_COL(loc), (child_count), ##__VA_ARGS__)

#define AST_EMPTY_NODE(kind, loc) \
    ast_branch((kind), AST_LOC_LINE(loc), AST_LOC_COL(loc), 0)


static void ast_prepend_child(AstNode *parent, AstNode *child) {
    if (!parent || !child) {
        return;
    }
    ast_reserve_children(parent, parent->child_count + 1);
    memmove(parent->children + 1, parent->children,
            parent->child_count * sizeof(*parent->children));
    parent->children[0] = child;
    parent->child_count++;
}

static AstNode *make_keyword_leaf(const char *text, YYLTYPE loc) {
    return ast_leaf(AST_IDENTIFIER, text, AST_LOC_LINE(loc), AST_LOC_COL(loc));
}

static AstNode *make_list_node(AstKind kind, YYLTYPE loc) {
    return ast_branch(kind, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
}

static AstNode *make_import_node(bool is_static, bool on_demand, AstNode *target, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_IMPORT_DECL, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (is_static) {
        ast_add_child(node, make_keyword_leaf("static", loc));
    }
    if (target) {
        ast_add_child(node, target);
    }
    if (on_demand) {
        ast_add_child(node, make_keyword_leaf("*", loc));
    }
    return node;
}

static AstNode *make_default_package_node(YYLTYPE loc) {
    AstNode *node = AST_EMPTY_NODE(AST_PACKAGE_DECL, loc);
    ast_set_text(node, "<default>");
    return node;
}

static AstNode *make_module_decl_node(AstNode *name, AstNode *directives,
                                      bool is_open, YYLTYPE module_loc,
                                      const YYLTYPE *open_loc) {
    AstNode *node = AST_BRANCH_AT(AST_MODULE_DECL, module_loc, 0);
    if (is_open && open_loc) {
        ast_add_child(node, make_keyword_leaf("open", *open_loc));
    }
    if (name) {
        ast_add_child(node, name);
    }
    if (directives) {
        ast_add_child(node, directives);
    } else {
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, module_loc));
    }
    return node;
}

static AstNode *make_module_directive_node(const char *keyword, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_UNKNOWN, loc, 0);
    ast_set_text(node, keyword);
    return node;
}

static AstNode *make_array_initializer_node(AstNode *elements, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_ARRAY_INIT, loc, 0);
    if (elements) {
        ast_add_child(node, elements);
    }
    return node;
}

static void attach_dims(AstNode *node, AstNode *dims) {
    if (node && dims) {
        ast_add_child(node, dims);
    }
}

/* Forward declarations for helpers used before their definitions */
static AstNode *make_parameter_node(AstNode *type_node, AstNode *name_node, YYLTYPE loc);

static AstNode *make_parameter_with_dims(AstNode *type_node, AstNode *name_node, AstNode *dims, YYLTYPE loc) {
    AstNode *param = make_parameter_node(type_node, name_node, loc);
    if (dims) {
        ast_add_child(param, dims);
    }
    return param;
}

static AstNode *make_block_node(YYLTYPE loc, AstNode *stmts) {
    AstNode *node = AST_BRANCH_AT(AST_BLOCK, loc, 0);
    if (stmts) {
        ast_add_child(node, stmts);
    }
    return node;
}

static AstNode *make_simple_stmt(AstKind kind, YYLTYPE loc) {
    return AST_BRANCH_AT(kind, loc, 0);
}

static AstNode *make_unary_stmt(AstKind kind, AstNode *child, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(kind, loc, 0);
    if (child) {
        ast_add_child(node, child);
    }
    return node;
}

static AstNode *make_binary_stmt(AstKind kind, AstNode *left, AstNode *right, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(kind, loc, 0);
    if (left) {
        ast_add_child(node, left);
    }
    if (right) {
        ast_add_child(node, right);
    }
    return node;
}

static AstNode *make_ternary_stmt(AstKind kind, AstNode *a, AstNode *b, AstNode *c, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(kind, loc, 0);
    if (a) {
        ast_add_child(node, a);
    }
    if (b) {
        ast_add_child(node, b);
    }
    if (c) {
        ast_add_child(node, c);
    }
    return node;
}

static AstNode *make_foreach_stmt(AstNode *param, AstNode *iterable, AstNode *body, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_FOR_EACH, loc, 0);
    if (param) {
        ast_add_child(node, param);
    }
    if (iterable) {
        ast_add_child(node, iterable);
    }
    if (body) {
        ast_add_child(node, body);
    }
    return node;
}

static AstNode *make_try_stmt(AstNode *block, AstNode *catches, AstNode *finally_node, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_TRY, loc, 0);
    if (block) {
        ast_add_child(node, block);
    }
    if (catches) {
        ast_add_child(node, catches);
    }
    if (finally_node) {
        ast_add_child(node, finally_node);
    }
    return node;
}

static AstNode *make_class_literal_node(AstNode *target, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_MEMBER_ACCESS, loc, 0);
    if (target) {
        ast_add_child(node, target);
    }
    ast_add_child(node, make_keyword_leaf("class", loc));
    return node;
}

static AstNode *make_field_access_node(AstNode *base, AstNode *member, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_MEMBER_ACCESS, loc, 0);
    if (base) {
        ast_add_child(node, base);
    }
    if (member) {
        ast_add_child(node, member);
    }
    return node;
}

static AstNode *make_array_access_node(AstNode *base, AstNode *index, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_ARRAY_ACCESS, loc, 0);
    if (base) {
        ast_add_child(node, base);
    }
    if (index) {
        ast_add_child(node, index);
    }
    return node;
}

static AstNode *make_method_invocation_node(AstNode *qualifier, AstNode *type_args, AstNode *name, AstNode *args, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_METHOD_INVOCATION, loc, 0);
    if (qualifier) {
        ast_add_child(node, qualifier);
    }
    if (type_args) {
        ast_add_child(node, type_args);
    }
    if (name) {
        ast_add_child(node, name);
    }
    if (args) {
        ast_add_child(node, args);
    } else {
        ast_add_child(node, make_list_node(AST_ARGUMENT_LIST, loc));
    }
    return node;
}

static AstNode *make_method_reference_node(AstNode *target, AstNode *type_args, AstNode *name, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_MEMBER_ACCESS, loc, 0);
    if (target) {
        ast_add_child(node, target);
    }
    if (type_args) {
        ast_add_child(node, type_args);
    }
    if (name) {
        ast_add_child(node, name);
    }
    return node;
}

static AstNode *make_new_class_core(AstNode *type, AstNode *args, AstNode *body, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_NEW_CLASS, loc, 0);
    if (type) {
        ast_add_child(node, type);
    }
    if (args) {
        ast_add_child(node, args);
    } else {
        ast_add_child(node, make_list_node(AST_ARGUMENT_LIST, loc));
    }
    if (body) {
        ast_add_child(node, body);
    }
    return node;
}

static AstNode *make_lambda_node(AstNode *params, AstNode *body, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_LAMBDA, loc, 0);
    if (params) {
        ast_add_child(node, params);
    }
    if (body) {
        ast_add_child(node, body);
    }
    return node;
}

static AstNode *make_assignment_node(AstNode *lhs, AstNode *op, AstNode *rhs, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_ASSIGN, loc, 0);
    if (lhs) {
        ast_add_child(node, lhs);
    }
    if (op) {
        ast_add_child(node, op);
    }
    if (rhs) {
        ast_add_child(node, rhs);
    }
    return node;
}

static AstNode *make_binary_expr(AstNode *lhs, const char *op, AstNode *rhs, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_BINARY_EXPR, loc, 0);
    if (lhs) {
        ast_add_child(node, lhs);
    }
    if (op) {
        ast_add_child(node, make_keyword_leaf(op, loc));
    }
    if (rhs) {
        ast_add_child(node, rhs);
    }
    return node;
}

static AstNode *make_unary_expr(const char *op, AstNode *expr, YYLTYPE loc, bool postfix) {
    AstNode *node = AST_BRANCH_AT(AST_UNARY_EXPR, loc, 0);
    if (!postfix && op) {
        ast_add_child(node, make_keyword_leaf(op, loc));
    }
    if (expr) {
        ast_add_child(node, expr);
    }
    if (postfix && op) {
        ast_add_child(node, make_keyword_leaf(op, loc));
    }
    return node;
}

static AstNode *make_conditional_expr(AstNode *cond, AstNode *if_true, AstNode *if_false, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_CONDITIONAL_EXPR, loc, 0);
    if (cond) {
        ast_add_child(node, cond);
    }
    if (if_true) {
        ast_add_child(node, if_true);
    }
    if (if_false) {
        ast_add_child(node, if_false);
    }
    return node;
}

static AstNode *make_array_creation_node(AstNode *type, AstNode *dim_exprs, AstNode *dims, AstNode *initializer, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_NEW_CLASS, loc, 0);
    if (type) {
        ast_add_child(node, type);
    }
    if (dim_exprs) {
        ast_add_child(node, dim_exprs);
    }
    if (dims) {
        ast_add_child(node, dims);
    }
    if (initializer) {
        ast_add_child(node, initializer);
    }
    return node;
}

static AstNode *make_cast_expr(AstNode *type, AstNode *expr, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_EXPRESSION, loc, 0);
    if (type) {
        ast_add_child(node, type);
    }
    if (expr) {
        ast_add_child(node, expr);
    }
    return node;
}

static AstNode *make_parameter_node(AstNode *type_node, AstNode *name_node, YYLTYPE loc) {
    AstNode *param = ast_branch(AST_PARAMETER, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (type_node) {
        ast_add_child(param, type_node);
    }
    if (name_node) {
        ast_add_child(param, name_node);
    }
    return param;
}

static AstNode *make_params_list(AstNode *first, YYLTYPE loc) {
    AstNode *list = ast_branch(AST_ARGUMENT_LIST, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (first) {
        ast_add_child(list, first);
    }
    return list;
}

static AstNode *make_method_signature(AstNode *name_node, AstNode *params_node, YYLTYPE loc) {
    AstNode *method = ast_branch(AST_METHOD_DECL, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (name_node) {
        ast_add_child(method, name_node);
    }
    if (params_node) {
        ast_add_child(method, params_node);
    } else {
        ast_add_child(method, make_list_node(AST_ARGUMENT_LIST, loc));
    }
    return method;
}

static AstNode *make_annotation_element_decl(AstNode *modifiers, AstNode *type_node,
                                             AstNode *name_node, AstNode *dims_node,
                                             AstNode *default_value, YYLTYPE loc) {
    AstNode *method = make_method_signature(name_node, NULL, loc);
    if (type_node) {
        attach_dims(type_node, dims_node);
        ast_prepend_child(method, type_node);
    } else if (dims_node) {
        ast_add_child(method, dims_node);
    }
    if (modifiers) {
        ast_prepend_child(method, modifiers);
    }
    if (default_value) {
        ast_add_child(method, default_value);
    }
    return method;
}

static AstNode *make_constructor_node(AstNode *name_node, AstNode *params_node, YYLTYPE loc) {
    AstNode *ctor = ast_branch(AST_CONSTRUCTOR_DECL, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (name_node) {
        ast_add_child(ctor, name_node);
    }
    if (params_node) {
        ast_add_child(ctor, params_node);
    } else {
        ast_add_child(ctor, make_list_node(AST_ARGUMENT_LIST, loc));
    }
    return ctor;
}

static AstNode *make_explicit_ctor_invocation(AstNode *qualifier, AstNode *type_args,
                                              const char *name, AstNode *args, YYLTYPE loc) {
    AstNode *name_node = make_keyword_leaf(name, loc);
    return make_method_invocation_node(qualifier, type_args, name_node,
                                       args ? args : make_list_node(AST_ARGUMENT_LIST, loc),
                                       loc);
}

static AstNode *make_throws_node(AstNode *types, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_ARGUMENT_LIST, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (types) {
        ast_add_child(node, types);
    }
    return node;
}

static AstNode *make_enum_constant(AstNode *annotations, AstNode *name,
                                   AstNode *args, AstNode *class_body, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_ENUM_CONST, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (annotations) {
        ast_add_child(node, annotations);
    }
    if (name) {
        ast_add_child(node, name);
    }
    if (args) {
        ast_add_child(node, args);
    } else {
        ast_add_child(node, make_list_node(AST_ARGUMENT_LIST, loc));
    }
    if (class_body) {
        ast_add_child(node, class_body);
    }
    return node;
}

static AstNode *make_field_node(AstNode *mods, AstNode *type_node, AstNode *vars_node, YYLTYPE loc) {
    AstNode *field = ast_branch(AST_FIELD_DECL, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);

    if (mods) {
        ast_add_child(field, mods);
    }
    if (type_node) {
        ast_add_child(field, type_node);
    }
    if (vars_node) {
        ast_add_child(field, vars_node);
    } else {
        ast_add_child(field, make_list_node(AST_STATEMENT_LIST, loc));
    } 
    return field;
}



static AstNode *make_local_variable_node(AstNode *type_node, AstNode *vars_node, YYLTYPE loc) {
    AstNode *stmt = ast_branch(AST_LOCAL_VAR_DECL, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (type_node) {
        ast_add_child(stmt, type_node);
    }
    if (vars_node) {
        ast_add_child(stmt, vars_node);
    } else {
        ast_add_child(stmt, make_list_node(AST_STATEMENT_LIST, loc));
    } 
    return stmt;
}

/* annotation / type / type-parameter / type-argument / dims */
static AstNode *make_annotation_node      (AstNode *name, YYLTYPE loc);
static AstNode *make_type_node            (AstNode *core, YYLTYPE loc);
static AstNode *make_type_parameter_node  (AstNode *name, AstNode *bounds, YYLTYPE loc);
static AstNode *make_type_argument_node   (AstNode *value, YYLTYPE loc);
static AstNode *make_dim_node             (YYLTYPE loc);

/* simple stmt / unary stmt / binary stmt / foreach / try */
static AstNode *make_simple_stmt(AstKind kind, YYLTYPE loc);
static AstNode *make_unary_stmt(AstKind kind, AstNode *child, YYLTYPE loc);
static AstNode *make_binary_stmt(AstKind kind, AstNode *left, AstNode *right, YYLTYPE loc);
static AstNode *make_foreach_stmt         (AstNode *param, AstNode *expr, AstNode *body, YYLTYPE loc);
static AstNode *make_try_stmt             (AstNode *block, AstNode *catches, AstNode *finally_block, YYLTYPE loc);

/* 参数、变量、本地变量 */
static AstNode *make_parameter_with_dims  (AstNode *type_node, AstNode *name_node,
                                           AstNode *dims, YYLTYPE loc);
static AstNode *make_parameter_node       (AstNode *type_node, AstNode *name_node, YYLTYPE loc);
static AstNode *make_local_variable_node  (AstNode *type_node, AstNode *vars_node, YYLTYPE loc);
static AstNode *make_class_basic(int line, int column, 
                               AstNode *modifiers, 
                               AstNode *name, 
                               AstNode *type_params,
                               AstNode *super_class,
                               AstNode *super_interfaces,
                               AstNode *permits,
                               AstNode *body);
static AstNode *make_interface_basic(int line, int column, 
                                   AstNode *modifiers,
                                   AstNode *name, 
                                   AstNode *type_params,
                                   AstNode *extends_interfaces,
                                   AstNode *permits,
                                   AstNode *body);
static AstNode *make_annotation_element_decl(AstNode *modifiers, AstNode *type_node,
                                             AstNode *name_node, AstNode *dims_node,
                                             AstNode *default_value, YYLTYPE loc);

/* 访问相关：field / class-literal / dims / new-class / array */
static AstNode *make_field_access_node    (AstNode *qualifier, AstNode *name, YYLTYPE loc);
static AstNode *make_class_literal_node   (AstNode *type_or_kw, YYLTYPE loc);
static AstNode *make_new_class_core       (AstNode *type, AstNode *args, AstNode *body, YYLTYPE loc);
static AstNode *make_array_access_node    (AstNode *array_expr, AstNode *index_expr, YYLTYPE loc);
static AstNode *make_array_creation_node  (AstNode *type, AstNode *dim_exprs,
                                           AstNode *dims, AstNode *initializer, YYLTYPE loc);

/* 调用 / 方法引用 / lambda / 赋值 / 条件表达式 / 一元二元表达式 / 强制类型转换 */
static AstNode *make_method_invocation_node (AstNode *qualifier, AstNode *type_args,
                                             AstNode *name, AstNode *args, YYLTYPE loc);
static AstNode *make_method_reference_node  (AstNode *qualifier, AstNode *type_args,
                                             AstNode *name, YYLTYPE loc);
static AstNode *make_lambda_node            (AstNode *params, AstNode *body, YYLTYPE loc);
static AstNode *make_assignment_node        (AstNode *lhs, AstNode *op, AstNode *rhs, YYLTYPE loc);
static AstNode *make_conditional_expr       (AstNode *cond, AstNode *then_expr,
                                             AstNode *else_expr, YYLTYPE loc);
static AstNode *make_binary_expr            (AstNode *lhs, const char *op,
                                             AstNode *rhs, YYLTYPE loc);
static AstNode *make_unary_expr             (const char *op, AstNode *expr,
                                             YYLTYPE loc, bool is_postfix);
static AstNode *make_cast_expr              (AstNode *type, AstNode *expr, YYLTYPE loc);

/* 通用构造：列表、关键字叶子 */
static AstNode *make_list_node(AstKind kind, YYLTYPE loc);
static AstNode *make_keyword_leaf           (const char *kw, YYLTYPE loc);
static AstNode *wrap_labeled_block(const char *label, AstNode *block, YYLTYPE loc);
%union {
    char* str;
    int val;
    AstNode* node;
}

%locations

// LPAREN_LambdaParameters RPAREN_LambdaParameters UPLUS UMINUS 待研究

// %expect 0

%token AMPERSAND RPAREN_CastExpression

// %token IDENTIFIER
%token <node> TYPE_IDENTIFIER_IdentifierforLambdaParameterList               // 向后检查两个是 '(' ARROW 或者向后检查一个是 ARROW 或者向后检查一个是 ','  就在向后检查两个
%token <node> VAR_IdentifierComplement_IdentifierforLambdaParameterList
%token <node> YIELD_IdentifierComplement_IdentifierforLambdaParameterList
%token <node> TYPE_IDENTIFIER_MethodDeclarator
%token <node> VAR_MethodDeclarator
%token <node> YIELD_MethodDeclarator
%token <node> TYPE_IDENTIFIER_AnnotationIdentifiers
%token <node> VAR_AnnotationIdentifiers
%token <node> YIELD_AnnotationIdentifiers
%token <node> TYPE_IDENTIFIER
%token <node> TYPE_IDENTIFIER_EnumDeclaration

%token LANGLE                             // TypeArguments 、 TypeParameters 中尖括号的左半边
%token AT_Dims                            // Dims 中的原 '@' 符号
%token AT_Modifier                        // Modifier 中的原 '@' 符号
%token AT_AnnotationTypeDeclaration       // AnnotationTypeDeclaration 中的原 '@' 符号
%token LBRACK                             // DimsNoAnnotations 中的原 '[' 符号
%token LBRACK_ArrayAccess                 // 向后判断一个token不为']'的'['
// %token LPAREN_LambdaParameters            // LambdaParameters 中的原 '(' 符号 （之前有冲突后来调试发现冲突意外没了，不清楚什么情况，故而建议不删以待后续研究）
// %token RPAREN_LambdaParameters            // LambdaParameters 中的原 ')' 符号

%token DEFAULT_SwitchLabel                // SwitchLabel 中的原 Default 关键字（判断 ARROW 和 ':' ）
%token DOT
%token DOT_CommonName           // CommonName 中的原 '.' 符号 （向前检测一个为 TYPE_IDENTIFIER 或者 IdentifierComplement ，向后也是）
%token <node> VAR_IdentifierComplement
%token <node> YIELD_IdentifierComplement
%token CLASS
%token BYTE INT SHORT LONG CHAR
%token FLOAT DOUBLE
%token EXTENDS SUPER
%token OPEN OPENS
%token REQUIRES EXPORTS USES PROVIDES
%token PACKAGE
%token IMPORT
%token PUBLIC PROTECTED PRIVATE ABSTRACT STATIC FINAL STRICTFP TRANSITIVE
%token IMPLEMENTS
%token TRANSIENT VOLATILE
%token SYNCHRONIZED NATIVE
%token BOOLEAN
%token VOID
%token THROW THROWS
%token THIS
%token ENUM
%token INTERFACE
%token DEFAULT
%token VAR
%token IF ELSE ASSERT SWITCH
%token CASE
%token DO WHILE FOR
%token BREAK YIELD CONTINUE RETURN
%token TRY CATCH
%token FINALLY
%token NEW
%token MODULE
%token <node> UnqualifiedMethodIdentifier
%token <node> NUMBER
%token <node> TRUE FALSE
%token <node> INTEGERLITERAL
%token <node> FLOATINGPOINTLITERAL
%token <node> CHARACTERLITERAL
%token <node> STRINGLITERAL
%token <node> MY_NULL

// 定义多字符运算符的 Token
%token LE GE EQ NE AND OR     // <= >= == != && ||
%token TO WITH ARROW DIAMOND  // to with -> <>
%token DOUBLE_COLON // "::"
%token MUL_ASSIGN   // "*="
%token DIV_ASSIGN   // "/="
%token MOD_ASSIGN   // "%="   
%token ADD_ASSIGN   // "+="    
%token SUB_ASSIGN   // "-="    
%token SHL_ASSIGN   // "<<="   
%token SHR_ASSIGN   // ">>="   
%token USHR_ASSIGN  // ">>>="  
%token AND_ASSIGN   // "&="    
%token XOR_ASSIGN   // "^="    
%token OR_ASSIGN    // "|="    
%token INSTANCEOF   // "instanceof"
%token SHL_OP       // "<<"   
%token SHR_OP       // ">>"    
%token USHR_OP      // ">>>"   
%token INC_OP       // "++"    
%token DEC_OP       // "--"
%token PREFIX_INC PREFIX_DEC
%token ELLIPSIS     // "..."
%token TRAILING_COMMA  // ","

%token <node> TextBlock    // 文本框模块
%token SEALED NON_SEALED
%token <node> SEALED_IdentifierComplement
%token <node> NON_SEALED_IdentifierComplement
%token PERMITS
%token EMPTY_STMT

// 待确定以下顺序的唯一正确性
%nonassoc PREC_ConditionalExpression_1
%nonassoc PREC_ConditionalExpression
%nonassoc PREC_ConditionalOrExpression_1
%nonassoc PREC_ConditionalAndExpression
%nonassoc PREC_ConditionalAndExpression_1
%nonassoc PREC_InclusiveOrExpression
%nonassoc PREC_InclusiveOrExpression_1
%nonassoc PREC_ExclusiveOrExpression
%nonassoc PREC_ExclusiveOrExpression_1
%nonassoc PREC_AndExpression
%nonassoc PREC_AndExpression_1
%nonassoc PREC_EqualityExpression
%nonassoc PREC_RelationalExpression
%nonassoc PREC_RelationalExpression_1
%nonassoc PREC_SimpleRelationalExpression
%nonassoc PREC_ShiftExpression
%nonassoc PREC_ShiftExpression_1
%nonassoc PREC_AdditiveExpression
%nonassoc PREC_AdditiveExpression_1
%nonassoc PREC_MultiplicativeExpression

%left ','                        // ,（逗号运算符） 
%right '=' ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN OR_ASSIGN XOR_ASSIGN AND_ASSIGN SHL_ASSIGN SHR_ASSIGN USHR_ASSIGN   // = += -= *= /= %= |= ^= &= <<= >>= >>>=
%right ARROW DIAMOND             // -> <>
%right '?' ':'                   // ? :
// %right ':'                   // ? :
%left OR                         // ||（逻辑或）
%left AND                        // &&（逻辑与）
%left '|'                        // |（按位或）
%left '^'                        // ^（按位异或）
%left '&'                        // &（按位与）
%left EQ NE                      // == !=（相等比较）
%left TO WITH                    // to with
%nonassoc INSTANCEOF             // instanceof（类型检查）
%left '<' '>' LE GE LT_RelationalExpression             // < > <= >=
%left SHL_OP SHR_OP USHR_OP      // << >> >>>（位移运算）
%left '+' '-'                    // + -（加减号）
%left '*' '/' '%'                // * / %
%left '.' DOT DOUBLE_COLON       // . ::  （ DOUBLE_COLON_MethodReference_COI DOUBLE_COLON_MethodReference_AT ）
%right '!' '~'                   // !(逻辑非) ~（按位取反）
%nonassoc PREFIX_INC PREFIX_DEC  // ++ --(前缀形式) （UPLUS UMINUS（+-正负号））
%left INC_OP DEC_OP              // ++ -- (后缀形式) 
%left '[' ']' LBRACK RBRACK '(' ')'    // [] () （ LPAREN_LambdaParameters RPAREN_LambdaParameters ）

%left '@'  AT_Dims AT_Modifier AT_AnnotationTypeDeclaration

%start CompilationUnit

%type <node> TypeParameters
%type <node> TypeParameterList
%type <node> ClassOrInterfacePermits


%type <node> Superclass
%type <node> CompilationUnit OrdinaryCompilationUnit ModularCompilationUnit
%type <node> PackageDeclaration ImportDeclarations ImportDeclaration
%type <node> SingleTypeImportDeclaration TypeImportOnDemandDeclaration
%type <node> SingleStaticImportDeclaration StaticImportOnDemandDeclaration
%type <node> TypeDeclarations TypeDeclaration Literal Modifiers Modifier
%type <node> ClassDeclaration NormalClassDeclaration EnumDeclaration
%type <node> InterfaceDeclaration NormalInterfaceDeclaration
%type <node> ClassBody ClassBodyDeclarationList ClassBodyDeclaration ClassMemberDeclaration
%type <node> FieldDeclaration MethodDeclaration ConstructorDeclaration
%type <node> MethodHeader MethodBody MethodDeclarator
%type <node> FormalParameterList FormalParameter VariableArityParameter
%type <node> VariableDeclaratorList VariableDeclarator VariableDeclaratorId ArrayInitializer VariableInitializerList VariableInitializer
%type <node> UnannType UnannPrimitiveType UnannReferenceType
%type <node> UnannClassOrInterfaceType UnannArrayType NumericType IntegralType FloatingPointType
%type <node> PrimitiveType ReferenceType ClassOrInterfaceType ArrayType Dims DimsNoAnnotations Annotation_Dims Annotations_Dims NormalAnnotation_Dims MarkerAnnotation_Dims SingleElementAnnotation_Dims
%type <node> TypeParameter TypeBound AdditionalBounds TypeArguments TypeArguments_UnannClassOrInterfaceType TypeArgumentList TypeArgument Wildcard WildcardBounds
%type <node> Result ConstructorDeclarator ConstructorBody SimpleTypeName
%type <node> Throws ExceptionTypeList ExceptionType ExplicitConstructorInvocation Superinterfaces EnumBody EnumConstantList EnumConstant EnumBodyDeclarations
%type <node> InstanceInitializer StaticInitializer
%type <node> InterfaceBody InterfaceMemberDeclaration InterfaceMemberDeclarationList ConstantDeclaration InterfaceMethodDeclaration InterfaceTypeList
%type <node> TypeName TypeNames CommonName IdentifierComplement IDENTIFIER_MethodDeclarator IDENTIFIER_AnnotationIdentifiers
%type <node> ModuleName ModuleNames ModuleDeclaration ModuleDirectives ModuleDirective ExtendsInterfaces AnnotationTypeDeclaration AnnotationTypeBody AnnotationTypeMemberDeclarationList AnnotationTypeMemberDeclaration AnnotationTypeElementDeclaration DefaultValue Annotation Annotations NormalAnnotation ElementValuePairList ElementValuePair ElementValueArrayInitializer ElementValueList MarkerAnnotation SingleElementAnnotation ElementValue ConditionalExpression
%type <node> Expression LambdaExpression LambdaParameters LambdaParameterList LambdaParameterforLambdaParameterList
%type <node> IdentifierforLambdaParameterList IdentifierComplement_IdentifierforLambdaParameterList LambdaParameter LambdaParameterType LambdaBody
%type <node> AssignmentExpression Assignment AssignmentOperator LeftHandSide
%type <node> ConditionalOrExpression ConditionalAndExpression InclusiveOrExpression ExclusiveOrExpression AndExpression
%type <node> EqualityExpression SimpleRelationalExpression RelationalExpression ShiftExpression AdditiveExpression MultiplicativeExpression
%type <node> UnaryExpression PreIncrementExpression PreDecrementExpression UnaryExpressionNotPlusMinus
%type <node> PostfixExpression PostIncrementExpression PostDecrementExpression CastExpression
%type <node> Primary PrimaryNoNewArray ClassLiteral ClassInstanceCreationExpression UnqualifiedClassInstanceCreationExpression
%type <node> ClassOrInterfaceTypeToInstantiate AnnotationIdentifiers TypeArgumentsOrDiamond FieldAccess ArrayAccess
%type <node> MethodInvocation MethodReference ArgumentList ArrayCreationExpression DimExprs DimExpr
%type <node> Pattern TypePattern LocalVariableDeclaration_TypePattern VariableAccess
%type <node> Resource ResourceList ResourceSpecification TryWithResourcesStatement SwitchExpression
%type <node> Annotation_Modifier NormalAnnotation_Modifier MarkerAnnotation_Modifier SingleElementAnnotation_Modifier TypeName_ModifierOrDims
%type <node> OptTypeParameters OptSuperclass OptSuperinterfaces OptClassOrInterfacePermits OptExtendsInterfaces
%type <node> Block BlockStatements BlockStatement LocalVariableDeclarationStatement LocalVariableDeclaration LocalVariableType LocalClassOrInterfaceDeclaration Statement StatementNoShortIf StatementWithoutTrailingSubstatement EmptyStatement LabeledStatement LabeledStatementNoShortIf ExpressionStatement StatementExpression IfThenStatement IfThenElseStatement IfThenElseStatementNoShortIf AssertStatement SwitchStatement SwitchBlock SwitchRule SwitchRules SwitchBlockStatementGroup SwitchBlockStatementGroups SwitchLabel SwitchLabels CaseConstant CaseConstants WhileStatement WhileStatementNoShortIf DoStatement ForStatement ForStatementNoShortIf BasicForStatement BasicForStatementNoShortIf ForInit ForUpdate StatementExpressionList EnhancedForStatement EnhancedForStatementNoShortIf BreakStatement YieldStatement ContinueStatement ReturnStatement ThrowStatement SynchronizedStatement TryStatement Catches CatchClause CatchFormalParameter CatchType Finally
%type <node> VariableModifiers

%%

IdentifierComplement:
    VAR_IdentifierComplement        { $$ = $1; }
    | SEALED_IdentifierComplement   { $$ = $1; }
    | NON_SEALED_IdentifierComplement { $$ = $1; }
    | YIELD_IdentifierComplement    { $$ = $1; }
;

IDENTIFIER_MethodDeclarator:
    TYPE_IDENTIFIER_MethodDeclarator { $$ = $1; }
    | VAR_MethodDeclarator { $$ = $1; }
    | YIELD_MethodDeclarator { $$ = $1; }
;

IDENTIFIER_AnnotationIdentifiers:
    TYPE_IDENTIFIER_AnnotationIdentifiers { $$ = $1; }
    | VAR_AnnotationIdentifiers { $$ = $1; }
    | YIELD_AnnotationIdentifiers { $$ = $1; }
;

// 词法定义
Literal:
    INTEGERLITERAL         { $$ = $1; }
    | FLOATINGPOINTLITERAL { $$ = $1; }
    | TRUE                 { $$ = $1; }
    | FALSE                { $$ = $1; }
    | CHARACTERLITERAL     { $$ = $1; }
    | STRINGLITERAL        { $$ = $1; }
    | TextBlock            { $$ = $1; }
    | MY_NULL              { $$ = $1; }
;

Modifier:
    Annotation_Modifier { $$ = $1; }
//    Annotation
    | PUBLIC { $$ = make_keyword_leaf("public", @1); }
    | PROTECTED { $$ = make_keyword_leaf("protected", @1); }
    | PRIVATE { $$ = make_keyword_leaf("private", @1); }
    | ABSTRACT { $$ = make_keyword_leaf("abstract", @1); }
    | STATIC { $$ = make_keyword_leaf("static", @1); }
    | FINAL { $$ = make_keyword_leaf("final", @1); }
    | STRICTFP { $$ = make_keyword_leaf("strictfp", @1); }
    | TRANSIENT { $$ = make_keyword_leaf("transient", @1); }
    | VOLATILE { $$ = make_keyword_leaf("volatile", @1); }
    | SYNCHRONIZED { $$ = make_keyword_leaf("synchronized", @1); }
    | NATIVE { $$ = make_keyword_leaf("native", @1); }
    | DEFAULT { $$ = make_keyword_leaf("default", @1); }
    | SEALED { $$ = make_keyword_leaf("sealed", @1); }
    | NON_SEALED { $$ = make_keyword_leaf("non-sealed", @1); }
;
Modifiers:
    Modifier {
        $$ = make_list_node(AST_MODIFIER_LIST, @$);
        ast_add_child($$, $1);
    }
    | Modifiers Modifier {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

// 以下4条语法是专门为 Modifier 服务的 Annotation
Annotation_Modifier:
    NormalAnnotation_Modifier { $$ = $1; }
    | MarkerAnnotation_Modifier { $$ = $1; }
    | SingleElementAnnotation_Modifier { $$ = $1; }
;

NormalAnnotation_Modifier:
    AT_Modifier TypeName_ModifierOrDims '(' ElementValuePairList ')' {
        AstNode *node = make_annotation_node($2, @1);
        ast_add_child(node, $4);
        $$ = node;
    }
    | AT_Modifier TypeName_ModifierOrDims '(' ')' {
        $$ = make_annotation_node($2, @1);
    }
;

MarkerAnnotation_Modifier:
    AT_Modifier TypeName {
        $$ = make_annotation_node($2, @1);
    }
;

SingleElementAnnotation_Modifier:
    AT_Modifier TypeName_ModifierOrDims '(' ElementValue ')' {
        AstNode *node = make_annotation_node($2, @1);
        AstNode *args = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child(args, $4);
        ast_add_child(node, args);
        $$ = node;
    }
;

TypeName_ModifierOrDims:
    TYPE_IDENTIFIER_MethodDeclarator { $$ = $1; }
  | CommonName DOT_CommonName TYPE_IDENTIFIER_MethodDeclarator {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
;


//-------------------Types, Values, and Variables-----------------------


/*
Type:
    PrimitiveType
  | ReferenceType
;
*/

// 这里把 PrimitiveType 缩减定义为必须被一个或多个 Annotation 修饰的 NumericType 或 BOOLEAN ，然后在以下所有用到的 PrimitiveType 的地方都加入了替换为 UnannPrimitiveType 的新定义以保证语法正确
PrimitiveType:
//    Annotations NumericType
//    | Annotations BOOLEAN
    Annotations UnannPrimitiveType {
        $$ = make_type_node($2, @$);
    }
//    | NumericType
//    | BOOLEAN
;

NumericType:
    IntegralType { $$ = $1; }
  | FloatingPointType { $$ = $1; }
;

IntegralType:
    BYTE { $$ = make_keyword_leaf("byte", @1); }
  | SHORT { $$ = make_keyword_leaf("short", @1); }
  | INT { $$ = make_keyword_leaf("int", @1); }
  | LONG { $$ = make_keyword_leaf("long", @1); }
  | CHAR { $$ = make_keyword_leaf("char", @1); }
;

FloatingPointType:
    FLOAT { $$ = make_keyword_leaf("float", @1); }
  | DOUBLE { $$ = make_keyword_leaf("double", @1); }
;

// 因 ClassOrInterfaceType 中实际包括 TypeVariable 的具体语法，故而此处将原定义中的 TypeVariable 去掉
ReferenceType:
    ClassOrInterfaceType { $$ = $1; }
//  | TypeVariable
    | ArrayType { $$ = $1; }
//    | UnannArrayType
;

// ClassOrInterfaceType ClassType InterfaceType 合并， Annotations TYPE_IDENTIFIER 替换为 TypeVariable
/* 
这样定义实际上在扩宽语义哈
因为 ClassOrInterfaceType: Annotations PackageName . {Annotation} TypeIdentifier [TypeArguments] 不应该规约成 ClassOrInterfaceType
*/
ClassOrInterfaceType:
    UnannClassOrInterfaceType {
        $$ = make_type_node($1, @$);
    }
    | Annotations UnannClassOrInterfaceType {
        $$ = make_type_node($2, @$);
    }
;

// 因为在 ClassOrInterfaceType 中实际包括一个语法上的 TypeVariable ，则实际在语法上 TypeVariable Dims 与 ClassOrInterfaceType Dims 有一定重叠，故删去
ArrayType:
    Annotations UnannPrimitiveType Dims {
        AstNode *type = make_type_node($2, @$);
        if ($3) {
            ast_add_child(type, $3);
        }
        $$ = type;
    }
    | Annotations UnannClassOrInterfaceType Dims {
        AstNode *type = make_type_node($2, @$);
        if ($3) {
            ast_add_child(type, $3);
        }
        $$ = type;
    }
    | UnannArrayType {
        $$ = make_type_node($1, @$);
    }
;

Dims:
    Annotations_Dims '[' ']' {
        AstNode *list = make_list_node(AST_DIM_LIST, @$);
        AstNode *dim = make_dim_node(@2);
        if ($1) {
            ast_add_child(dim, $1);
        }
        ast_add_child(list, dim);
        $$ = list;
    }
    | Annotations_Dims '[' ']' Dims {
        AstNode *dim = make_dim_node(@2);
        if ($1) {
            ast_add_child(dim, $1);
        }
        ast_prepend_child($4, dim);
        $$ = $4;
    }
    | '[' ']' {
        AstNode *list = make_list_node(AST_DIM_LIST, @$);
        ast_add_child(list, make_dim_node(@1));
        $$ = list;
    }
    | '[' ']' Dims {
        AstNode *dim = make_dim_node(@1);
        ast_prepend_child($3, dim);
        $$ = $3;
    }
;

// 以下五条语法是专门为 Dims 服务的 Annotations
Annotation_Dims:
    NormalAnnotation_Dims { $$ = $1; }
    | MarkerAnnotation_Dims { $$ = $1; }
    | SingleElementAnnotation_Dims { $$ = $1; }
;

//允许使用一个或者多个Annotation，如遇到需要零个的情况，请自行在语句中定义
Annotations_Dims:
    Annotations_Dims Annotation_Dims {
        ast_add_child($1, $2);
        $$ = $1;
    }
    | Annotation_Dims {
        $$ = make_list_node(AST_MODIFIER_LIST, @$);
        ast_add_child($$, $1);
    }
;

NormalAnnotation_Dims:
    AT_Dims TypeName_ModifierOrDims '(' ElementValuePairList ')' {
        AstNode *node = make_annotation_node($2, @1);
        ast_add_child(node, $4);
        $$ = node;
    }
    | AT_Dims TypeName_ModifierOrDims '(' ')' {
        $$ = make_annotation_node($2, @1);
    }
;

MarkerAnnotation_Dims:
    AT_Dims TypeName {
        $$ = make_annotation_node($2, @1);
    }
;

SingleElementAnnotation_Dims:
    AT_Dims TypeName_ModifierOrDims '(' ElementValue ')' {
        AstNode *node = make_annotation_node($2, @1);
        AstNode *args = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child(args, $4);
        ast_add_child(node, args);
        $$ = node;
    }
;

// TypeParameterModifier 纯粹由 Annotation 构成，故而这里把 TypeParameterModifier 直接替换为了 Annotation
TypeParameter:
    Annotations TYPE_IDENTIFIER {
        $$ = make_type_parameter_node($2, NULL, @$);
    }
    | Annotations TYPE_IDENTIFIER TypeBound {
        $$ = make_type_parameter_node($2, $3, @$);
    }
    | TYPE_IDENTIFIER {
        $$ = make_type_parameter_node($1, NULL, @$);
    }
    | TYPE_IDENTIFIER TypeBound {
        $$ = make_type_parameter_node($1, $2, @$);
    }
;

TypeBound:
    EXTENDS ClassOrInterfaceType AdditionalBounds {
        if ($3) {
            ast_prepend_child($3, $2);
            $$ = $3;
        } else {
            $$ = make_list_node(AST_ARGUMENT_LIST, @$);
            ast_add_child($$, $2);
        }
    }
;

// 这里允许 AdditionalBounds 为空，这是我代码中极少数允许为空的
AdditionalBounds:
//    AMPERSAND ClassOrInterfaceType
    AdditionalBounds AMPERSAND ClassOrInterfaceType {
        AstNode *list = $1;
        if (!list) {
            list = make_list_node(AST_ARGUMENT_LIST, @$);
        }
        ast_add_child(list, $3);
        $$ = list;
    }
    |  { $$ = NULL; }
;

TypeArguments:
    '<' TypeArgumentList '>' { $$ = $2; }
    | LANGLE TypeArgumentList '>' { $$ = $2; }
;

TypeArguments_UnannClassOrInterfaceType:
//    '<' TypeArgumentList '>'
    LANGLE TypeArgumentList '>' { $$ = $2; }
;

TypeArgumentList:
    TypeArgument {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | TypeArgumentList ',' TypeArgument {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

TypeArgument:
    ReferenceType {
        $$ = make_type_argument_node($1, @$);
    }
  | Wildcard {
        $$ = make_type_argument_node($1, @$);
    }
;

Wildcard:
    Annotations '?' WildcardBounds {
        AstNode *node = make_type_node(make_keyword_leaf("?", @2), @$);
        if ($3) {
            ast_add_child(node, $3);
        }
        $$ = node;
    }
  | Annotations '?' {
        $$ = make_type_node(make_keyword_leaf("?", @2), @$);
    }
  | '?' WildcardBounds {
        AstNode *node = make_type_node(make_keyword_leaf("?", @1), @$);
        if ($2) {
            ast_add_child(node, $2);
        }
        $$ = node;
    }
  | '?' {
        $$ = make_type_node(make_keyword_leaf("?", @1), @$);
    }
;

WildcardBounds:
    EXTENDS ReferenceType AdditionalBounds {
        AstNode *types;
        if ($3) {
            ast_prepend_child($3, $2);
            types = $3;
        } else {
            types = make_list_node(AST_ARGUMENT_LIST, @$);
            ast_add_child(types, $2);
        }
        AstNode *node = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child(node, make_keyword_leaf("extends", @1));
        ast_add_child(node, types);
        $$ = node;
    }
  | SUPER ReferenceType {
        AstNode *node = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child(node, make_keyword_leaf("super", @1));
        ast_add_child(node, $2);
        $$ = node;
    }
;


//-------------------------------Names------------------------------


// 模块名称
ModuleName:
    TYPE_IDENTIFIER {
        $$ = $1;
    }
    | IdentifierComplement {
        $$ = $1;
    }
    | ModuleName DOT_CommonName TYPE_IDENTIFIER {
        AstNode *children[] = { $1, $3 };
        $$ = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_children($$, children, 2);
    }
    | ModuleName DOT_CommonName IdentifierComplement {
        AstNode *children[] = { $1, $3 };
        $$ = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_children($$, children, 2);
    }
;

// 允许使用一个或者多个ModuleName，不允许零个ModuleName
ModuleNames:
    ModuleName {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
  | ModuleNames ',' ModuleName {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

TypeName:
    TYPE_IDENTIFIER { $$ = $1; }
  | CommonName DOT_CommonName TYPE_IDENTIFIER {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
;
TypeNames:
    TypeName {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
  | TypeNames ',' TypeName {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

// MethodName:
//     UnqualifiedMethodIdentifier
// ;

/*
   ExpressionName PackageOrTypeName AmbiguousName PackageName 
   如上四条合并为 CommonName
   有点地方好像还掺杂一点 ModuleName
*/
/*
CommonName:
    IDENTIFIER
    | CommonName '.' IDENTIFIER
;
*/
CommonName:
    TYPE_IDENTIFIER {
        $$ = $1;
    }
    | IdentifierComplement {
        $$ = $1;
    }
    | CommonName DOT_CommonName TYPE_IDENTIFIER {
        AstNode *children[] = { $1, $3 };
        $$ = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_children($$, children, 2);
    }
    | CommonName DOT_CommonName IdentifierComplement {
        AstNode *children[] = { $1, $3 };
        $$ = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_children($$, children, 2);
    }
;


//----------------------------Packages and Modules-----------------------------



// 根规则：编译单元
CompilationUnit:
    OrdinaryCompilationUnit { $$ = $1; root_ast = $$; }
  | ModularCompilationUnit {
        $$ = $1;
        root_ast = $$;
    }
;

// 普通编译单元
// 普通编译单元：固定 3 子节点 (package?, imports, types)
OrdinaryCompilationUnit:
    PackageDeclaration ImportDeclarations TypeDeclarations {
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @$, 3, $1, $2, $3);
    }
  | ImportDeclarations TypeDeclarations {
        AstNode *empty_pkg = make_default_package_node(@$);
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @$, 3, empty_pkg, $1, $2);
    }
  | PackageDeclaration TypeDeclarations {
        AstNode *empty_imports = AST_EMPTY_NODE(AST_IMPORT_LIST, @$);
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @$, 3, $1, empty_imports, $2);
    }
  | TypeDeclarations {
        AstNode *empty_pkg = make_default_package_node(@$);
        AstNode *empty_imports = AST_EMPTY_NODE(AST_IMPORT_LIST, @$);
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @$, 3, empty_pkg, empty_imports, $1);
    }
;


// 模块编译单元
ModularCompilationUnit:
    ImportDeclarations ModuleDeclaration {
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @1, 2, $1, $2);
    }
    | ModuleDeclaration {
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @1, 1, $1);
    }
;

// 包声明：包修改符 PackageModifier 完全由 Annotation 构成，故而这里把 PackageModifier 直接替换为了 Annotation
PackageDeclaration:
    Annotations PACKAGE CommonName ';' {
        $$ = AST_BRANCH_AT(AST_PACKAGE_DECL, @2, 2, $1, $3);
    }
  | PACKAGE CommonName ';' {
        AstNode *empty_ann = make_list_node(AST_ANNOTATION, @$);   // 与你 Annotations 的 list kind 保持一致
        $$ = AST_BRANCH_AT(AST_PACKAGE_DECL, @1, 2, empty_ann, $2);
    }
;


// 导入声明
ImportDeclarations:
    ImportDeclaration {
        $$ = AST_BRANCH_AT(AST_IMPORT_LIST, @$, 1, $1);
    }
  | ImportDeclarations ImportDeclaration {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

ImportDeclaration:
    SingleTypeImportDeclaration          { $$ = $1; }
  | TypeImportOnDemandDeclaration        { $$ = $1; }
  | SingleStaticImportDeclaration        { $$ = $1; }
  | StaticImportOnDemandDeclaration      { $$ = $1; }
;

// 单一类型导入声明
SingleTypeImportDeclaration:
    IMPORT TypeName ';' {
        $$ = make_import_node(false, false, $2, @1);
    }
;

// 类型导入请求
TypeImportOnDemandDeclaration:
    IMPORT CommonName '.' '*' ';' {
        $$ = make_import_node(false, true, $2, @1);
    }
;

// 单一静态导入声明
// 这里为了避免冲突把 TypeName 换成了 CommonName ，实际上扩大了语义，后续可升级
SingleStaticImportDeclaration:
//   IMPORT STATIC TypeName '.' TYPE_IDENTIFIER ';'
//    | IMPORT STATIC TypeName '.' IdentifierComplement ';'
    IMPORT STATIC CommonName ';' {
        $$ = make_import_node(true, false, $3, @1);
    }
;

// 静态导入请求
// 这里为了避免冲突把 TypeName 换成了 CommonName ，实际上扩大了语义，后续可升级
StaticImportOnDemandDeclaration:
//    IMPORT STATIC TypeName '.' '*' ';'
    IMPORT STATIC CommonName '.' '*' ';' {
        $$ = make_import_node(true, true, $3, @1);
    }
;

// 类型声明
TypeDeclarations:
    TypeDeclaration {
        $$ = AST_BRANCH_AT(AST_TYPE_DECL_LIST, @$, 1, $1);
    }
  | TypeDeclarations TypeDeclaration {
        ast_add_child($1, $2);
        $$ = $1;
    }
;


TypeDeclaration:
    ClassDeclaration { $$ = $1; }
    | InterfaceDeclaration { $$ = $1; }
    | EMPTY_STMT { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
    | ';' { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
;

// 模块声明
ModuleDeclaration:
    Annotations OPEN MODULE CommonName '{' ModuleDirectives '}' {
        YYLTYPE open_loc = @2;
        $$ = make_module_decl_node($4, $6, true, @3, &open_loc);
    }
    | Annotations MODULE CommonName '{' ModuleDirectives '}' {
        $$ = make_module_decl_node($3, $5, false, @2, NULL);
    }
    | OPEN MODULE CommonName '{' ModuleDirectives '}' {
        YYLTYPE open_loc = @1;
        $$ = make_module_decl_node($3, $5, true, @2, &open_loc);
    }
    | MODULE CommonName '{' ModuleDirectives '}' {
        $$ = make_module_decl_node($2, $4, false, @1, NULL);
    }
    | Annotations OPEN MODULE CommonName '{' '}' {
        YYLTYPE open_loc = @2;
        $$ = make_module_decl_node($4, NULL, true, @3, &open_loc);
    }
    | Annotations MODULE CommonName '{' '}' {
        $$ = make_module_decl_node($3, NULL, false, @2, NULL);
    }
    | OPEN MODULE CommonName '{' '}' {
        YYLTYPE open_loc = @1;
        $$ = make_module_decl_node($3, NULL, true, @2, &open_loc);
    }
    | MODULE CommonName '{' '}' {
        $$ = make_module_decl_node($2, NULL, false, @1, NULL);
    }
;

// 模块指令
ModuleDirectives:
    ModuleDirective {
        $$ = AST_BRANCH_AT(AST_STATEMENT_LIST, @1, 1, $1);
    }
    | ModuleDirectives ModuleDirective {
        ast_add_child($1, $2);
        $$ = $1;
    }
//  | /*空*/
;

// 这里把原 ModuleDirective 中的 RequiresModifier 替换为了由 TRANSITIVE 和 STATIC 及其排列组合
ModuleDirective:
    REQUIRES TRANSITIVE ModuleName ';' {
        AstNode *node = make_module_directive_node("requires", @1);
        ast_add_child(node, make_keyword_leaf("transitive", @2));
        ast_add_child(node, $3);
        $$ = node;
    }
    | REQUIRES ModuleName ';' {
        AstNode *node = make_module_directive_node("requires", @1);
        ast_add_child(node, $2);
        $$ = node;
    }
    | REQUIRES STATIC ModuleName ';' {
        AstNode *node = make_module_directive_node("requires", @1);
        ast_add_child(node, make_keyword_leaf("static", @2));
        ast_add_child(node, $3);
        $$ = node;
    }
    | EXPORTS CommonName ';' {
        AstNode *node = make_module_directive_node("exports", @1);
        ast_add_child(node, $2);
        $$ = node;
    }
    | EXPORTS CommonName TO ModuleNames ';' {
        AstNode *node = make_module_directive_node("exports", @1);
        ast_add_child(node, $2);
        ast_add_child(node, make_keyword_leaf("to", @3));
        ast_add_child(node, $4);
        $$ = node;
    }
    | OPENS CommonName ';' {
        AstNode *node = make_module_directive_node("opens", @1);
        ast_add_child(node, $2);
        $$ = node;
    }
    | OPENS CommonName TO ModuleNames ';' {
        AstNode *node = make_module_directive_node("opens", @1);
        ast_add_child(node, $2);
        ast_add_child(node, make_keyword_leaf("to", @3));
        ast_add_child(node, $4);
        $$ = node;
    }
    | USES TypeName ';' {
        AstNode *node = make_module_directive_node("uses", @1);
        ast_add_child(node, $2);
        $$ = node;
    }
    | PROVIDES TypeName WITH TypeNames ';' {
        AstNode *node = make_module_directive_node("provides", @1);
        ast_add_child(node, $2);
        ast_add_child(node, make_keyword_leaf("with", @3));
        ast_add_child(node, $4);
        $$ = node;
    }
;


//------------------------Arrays------------------------------------


//数组初始化器
ArrayInitializer:
    '{' VariableInitializerList '}' {
        $$ = make_array_initializer_node($2, @1);
    }
    | '{' VariableInitializerList ',' '}' {
        $$ = make_array_initializer_node($2, @1);
    }
//    | '{' VariableInitializerList TRAILING_COMMA '}' // 初始化器列表末尾可以有逗号
    | '{' '}' {
        $$ = make_array_initializer_node(NULL, @1);
    }
;

//变量初始化器的列表
VariableInitializerList:
    VariableInitializer {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | VariableInitializerList ',' VariableInitializer {
        ast_add_child($1, $3);
        $$ = $1;
    }
    // 列表中的元素用逗号分隔
;


//-------------------------------Classes--------------------------------


ClassDeclaration:
    NormalClassDeclaration { $$ = $1; }
    | EnumDeclaration { $$ = $1; }
;

NormalClassDeclaration:
    Modifiers CLASS TYPE_IDENTIFIER OptTypeParameters OptSuperclass OptSuperinterfaces OptClassOrInterfacePermits ClassBody {
        /* 参数顺序对应修改后的 make_class_basic */
        $$ = make_class_basic(@2.first_line, @2.first_column, 
                              $1,  /* Modifiers */
                              $3,  /* Name */
                              $4,  /* TypeParams */
                              $5,  /* SuperClass */
                              $6,  /* Interfaces */
                              $7,  /* Permits */
                              $8); /* Body */
    }
    | CLASS TYPE_IDENTIFIER OptTypeParameters OptSuperclass OptSuperinterfaces OptClassOrInterfacePermits ClassBody {
        $$ = make_class_basic(@1.first_line, @1.first_column, 
                              NULL, /* No Modifiers */
                              $2, 
                              $3, 
                              $4, 
                              $5, 
                              $6, 
                              $7);
    }
;
OptTypeParameters:
    TypeParameters { $$ = $1; }
    | /* empty */ { $$ = NULL; }
;

OptSuperclass:
    Superclass { $$ = $1; }
    | /* empty */ { $$ = NULL; }
;

OptSuperinterfaces:
    Superinterfaces { $$ = $1; }
    | /* empty */ { $$ = NULL; }
;

OptClassOrInterfacePermits:
    ClassOrInterfacePermits { $$ = $1; }
    | /* empty */ { $$ = NULL; }
;

TypeParameters:
    '<' TypeParameterList '>'  { $$ = $2; }
  | LANGLE TypeParameterList '>' { $$ = $2; }
;

TypeParameterList:
    TypeParameter {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
  | TypeParameterList ',' TypeParameter {
        ast_add_child($1, $3);
        $$ = $1;
    }
;


Superclass:
    EXTENDS ClassOrInterfaceType {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, make_keyword_leaf("extends", @1));
        ast_add_child($$, $2);
    }
;

Superinterfaces:
    IMPLEMENTS InterfaceTypeList {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, make_keyword_leaf("implements", @1));
        ast_add_child($$, $2);
    }
;

InterfaceTypeList:
    ClassOrInterfaceType {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | InterfaceTypeList ',' ClassOrInterfaceType {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

ClassOrInterfacePermits:
    PERMITS TypeNames
;

ClassBody:
    '{' ClassBodyDeclarationList '}' { $$ = $2; }
    | '{' '}' { $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 0); }
;

// 引入 ClassBodyDeclaration
ClassBodyDeclarationList:
    ClassBodyDeclaration {
        $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 1, $1);
    }
    | ClassBodyDeclarationList ClassBodyDeclaration {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

ClassBodyDeclaration:
    ClassMemberDeclaration { $$ = $1; }
    | InstanceInitializer { $$ = $1; }
    | StaticInitializer { $$ = $1; }
    | ConstructorDeclaration { $$ = $1; }
;

ClassMemberDeclaration:
    FieldDeclaration { $$ = $1; }
    | MethodDeclaration { $$ = $1; }
    | ClassDeclaration { $$ = $1; }
    | InterfaceDeclaration { $$ = $1; }
    | ';' { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
    | EMPTY_STMT { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
;
FieldDeclaration:
    Modifiers UnannType VariableDeclaratorList ';' {
        $$ = make_field_node($1, $2, $3, @2);
    }
  | UnannType VariableDeclaratorList ';' {
        $$ = make_field_node(NULL, $1, $2, @1);
    }
;





VariableDeclaratorList:
    VariableDeclarator {
        $$ = make_list_node(AST_STATEMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | VariableDeclaratorList ',' VariableDeclarator {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

VariableDeclarator:
    VariableDeclaratorId '=' VariableInitializer {
        AstNode *node = ast_branch(AST_VARIABLE_DECL, @1.first_line, @1.first_column, 0);
        ast_add_child(node, $1);
        if ($3) {
            ast_add_child(node, $3);
        }
        $$ = node;
    }
    | VariableDeclaratorId {
        AstNode *node = ast_branch(AST_VARIABLE_DECL, @1.first_line, @1.first_column, 0);
        ast_add_child(node, $1);
        $$ = node;
    }
;

VariableDeclaratorId:
    TYPE_IDENTIFIER Dims {
        if ($2) {
            ast_add_child($1, $2);
        }
        $$ = $1;
    }
    | IdentifierComplement Dims {
        if ($2) {
            ast_add_child($1, $2);
        }
        $$ = $1;
    }
    | TYPE_IDENTIFIER { $$ = $1; }
    | IdentifierComplement { $$ = $1; }
;

VariableInitializer:
    Expression {
        $$ = $<node>1;
    }
    | ArrayInitializer {
        $$ = $1;
    }
;

UnannType:
    UnannReferenceType { $$ = $1; }
    | UnannPrimitiveType { $$ = $1; }
;

// 将 FloatingPointType IntegralType NumericType UnannPrimitiveType 合并
UnannPrimitiveType:
//    BYTE
//    | SHORT
//    | INT
//    | LONG
//    | CHAR
//    | FLOAT
//    | DOUBLE
//    | BOOLEAN
    NumericType { $$ = $1; }
    | BOOLEAN { $$ = make_keyword_leaf("boolean", @1); }
;

// UnannClassOrInterfaceType 中实际包括 UnannTypeVariable ，这里予以去除
UnannReferenceType:
    UnannArrayType { $$ = $1; }    //结尾是']'
//    | UnannTypeVariable
    | UnannClassOrInterfaceType { $$ = $1; }
;

// UnannClassOrInterfaceType、UnannClassType、UnannInterfaceType 合并在一起
/*
   请注意，这里用 CommonName 替代 TypeName 替代原 UnannClassOrInterfaceType: TYPE_IDENTIFIER 和原 UnannClassOrInterfaceType: PackageName . TypeIdentifier 的行为
   实际上扩宽了语义
*/
UnannClassOrInterfaceType:
    TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType {
        $$ = $1;
    }
    | CommonName { $$ = $1; }
//    | TYPE_IDENTIFIER
//    | TYPE_IDENTIFIER '.' TYPE_IDENTIFIER
//    | IdentifierComplement '.' TYPE_IDENTIFIER
    | CommonName '.' Annotations TYPE_IDENTIFIER {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_child(node, $1);
        ast_add_child(node, $4);
        $$ = node;
    }
    | CommonName '.' Annotations TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_child(node, $1);
        ast_add_child(node, $4);
        $$ = node;
    }
    | CommonName DOT_CommonName TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
//    | UnannClassOrInterfaceType '.' TYPE_IDENTIFIER
    | UnannClassOrInterfaceType DOT TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
    | UnannClassOrInterfaceType DOT Annotations TYPE_IDENTIFIER {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_child(node, $1);
        ast_add_child(node, $4);
        $$ = node;
    }
    | UnannClassOrInterfaceType DOT Annotations TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        ast_add_child(node, $1);
        ast_add_child(node, $4);
        $$ = node;
    }
;

// UnannClassOrInterfaceType 中实际包括 UnannTypeVariable ，这里予以去除
UnannArrayType:
    UnannPrimitiveType Dims { $$ = $1; }
    | UnannClassOrInterfaceType Dims { $$ = $1; }
//    | UnannTypeVariable Dims
;

MethodDeclaration:
    Modifiers MethodHeader MethodBody {
        if ($3) {
            ast_add_child($2, $3);
        }
        $$ = $2;
    }
  | MethodHeader MethodBody {
        if ($2) {
            ast_add_child($1, $2);
        }
        $$ = $1;
    }
;

MethodHeader:
    Result MethodDeclarator Throws {
        if ($1) {
            ast_prepend_child($2, $1);
        }
        if ($3) {
            ast_add_child($2, $3);
        }
        $$ = $2;
    }
    | Result MethodDeclarator {
        if ($1) {
            ast_prepend_child($2, $1);
        }
        $$ = $2;
    }
    | TypeParameters Annotations Result MethodDeclarator Throws {
        if ($3) {
            ast_prepend_child($4, $3);
        }
        if ($5) {
            ast_add_child($4, $5);
        }
        $$ = $4;
    }
    | TypeParameters Annotations Result MethodDeclarator {
        if ($3) {
            ast_prepend_child($4, $3);
        }
        $$ = $4;
    }
    | TypeParameters Result MethodDeclarator Throws {
        if ($2) {
            ast_prepend_child($3, $2);
        }
        if ($4) {
            ast_add_child($3, $4);
        }
        $$ = $3;
    }
    | TypeParameters Result MethodDeclarator {
        if ($2) {
            ast_prepend_child($3, $2);
        }
        $$ = $3;
    }
;

Result:
    UnannType { $$ = $1; }
    | VOID { $$ = make_keyword_leaf("void", @1); }
;

MethodDeclarator:
    IDENTIFIER_MethodDeclarator '(' ReceiverParameter ',' FormalParameterList ')' Dims {
        $$ = make_method_signature($1, $5, @$);
    }
    | IDENTIFIER_MethodDeclarator '(' ReceiverParameter ')' Dims {
        $$ = make_method_signature($1, NULL, @$);
    }
    | IDENTIFIER_MethodDeclarator '(' FormalParameterList ')' Dims {
        $$ = make_method_signature($1, $3, @$);
    }
    | IDENTIFIER_MethodDeclarator '(' ')' Dims {
       AstNode *sig = make_method_signature($1, NULL, @$);
       if ($4) ast_add_child(sig, $4);
       $$ = sig;
    }

    | IDENTIFIER_MethodDeclarator '(' ReceiverParameter ',' FormalParameterList ')' {
        $$ = make_method_signature($1, $5, @$);
    }
    | IDENTIFIER_MethodDeclarator '(' ReceiverParameter ')' {
        $$ = make_method_signature($1, NULL, @$);
    }
    | IDENTIFIER_MethodDeclarator '(' FormalParameterList ')' {
        $$ = make_method_signature($1, $3, @$);
    }
    | IDENTIFIER_MethodDeclarator '(' ')' {
        $$ = make_method_signature($1, NULL, @$);
    }
;

ReceiverParameter:
    Annotations UnannType TYPE_IDENTIFIER '.' THIS
    | Annotations UnannType IdentifierComplement '.' THIS
    | Annotations UnannType THIS
    | UnannType TYPE_IDENTIFIER '.' THIS
    | UnannType IdentifierComplement '.' THIS
    | UnannType THIS
;

FormalParameterList:
    FormalParameter {
        $$ = make_params_list($1, @$);
    }
    | FormalParameter ',' FormalParameterList {
        ast_prepend_child($3, $1);
        $$ = $3;
    }
;

// 这里为了避免冲突把 VariableDeclaratorId 直接诠释为了 IDENTIFIER Dims / IDENTIFIER ;
FormalParameter:
//    VariableModifiers UnannType VariableDeclaratorId
//    | Annotations UnannType VariableDeclaratorId
//    | UnannType VariableDeclaratorId
    VariableModifiers UnannType TYPE_IDENTIFIER Dims {
        $$ = make_parameter_node($2, $3, @$);
}
| Annotations UnannType TYPE_IDENTIFIER Dims {
        $$ = make_parameter_node($2, $3, @$);
}
| UnannType TYPE_IDENTIFIER Dims {
    AstNode *p = make_parameter_node($1, $2, @$);
    if ($3) ast_add_child(p, $3);
    $$ = p;
}
    | VariableModifiers UnannType TYPE_IDENTIFIER {
        $$ = make_parameter_node($2, $3, @$);
    }
    | Annotations UnannType TYPE_IDENTIFIER {
        $$ = make_parameter_node($2, $3, @$);
    }
    | UnannType TYPE_IDENTIFIER {
        $$ = make_parameter_node($1, $2, @$);
    }
    | VariableModifiers UnannType IdentifierComplement Dims {
        $$ = make_parameter_node($2, $3, @$);
}
| Annotations UnannType IdentifierComplement Dims {
        $$ = make_parameter_node($2, $3, @$);
}
| UnannType IdentifierComplement Dims {
        $$ = make_parameter_node($1, $2, @$);
}
    | VariableModifiers UnannType IdentifierComplement {
        $$ = make_parameter_node($2, $3, @$);
    }
    | Annotations UnannType IdentifierComplement {
        $$ = make_parameter_node($2, $3, @$);
    }
    | UnannType IdentifierComplement {
        $$ = make_parameter_node($1, $2, @$);
    }
    | VariableArityParameter { $$ = $1; }
    | UnannType TYPE_IDENTIFIER DOT_CommonName TYPE_IDENTIFIER {
        yyerror("Invalid parameter syntax: unexpected '.' in parameter (did you mean String[] args?)");
        YYERROR;
    }
    | UnannType TYPE_IDENTIFIER '.' TYPE_IDENTIFIER {
        yyerror("Invalid parameter syntax: unexpected '.' in parameter");
        YYERROR;
    }
;

// 这里需要前探去看 Annotations 后面是 '[ ]' 还是 '...'(ELLIPSIS)
VariableArityParameter:
    VariableModifiers UnannType Annotations ELLIPSIS TYPE_IDENTIFIER {
        AstNode *param = make_parameter_node($2, $5, @$);
        ast_add_child(param, make_keyword_leaf("...", @4));
        $$ = param;
    }
    | VariableModifiers UnannType ELLIPSIS TYPE_IDENTIFIER {
        AstNode *param = make_parameter_node($2, $4, @$);
        ast_add_child(param, make_keyword_leaf("...", @3));
        $$ = param;
    }
    | Annotations UnannType Annotations ELLIPSIS TYPE_IDENTIFIER {
        AstNode *param = make_parameter_node($2, $5, @$);
        ast_add_child(param, make_keyword_leaf("...", @4));
        $$ = param;
    }
    | Annotations UnannType ELLIPSIS TYPE_IDENTIFIER {
        AstNode *param = make_parameter_node($2, $4, @$);
        ast_add_child(param, make_keyword_leaf("...", @3));
        $$ = param;
    }
    | UnannType Annotations ELLIPSIS TYPE_IDENTIFIER {
        AstNode *param = make_parameter_node($1, $4, @$);
        ast_add_child(param, make_keyword_leaf("...", @3));
        $$ = param;
    }
    | UnannType ELLIPSIS TYPE_IDENTIFIER {
        AstNode *param = make_parameter_node($1, $3, @$);
        ast_add_child(param, make_keyword_leaf("...", @2));
        $$ = param;
    }
    | VariableModifiers UnannType Annotations ELLIPSIS IdentifierComplement {
        AstNode *param = make_parameter_node($2, $5, @$);
        ast_add_child(param, make_keyword_leaf("...", @4));
        $$ = param;
    }
    | VariableModifiers UnannType ELLIPSIS IdentifierComplement {
        AstNode *param = make_parameter_node($2, $4, @$);
        ast_add_child(param, make_keyword_leaf("...", @3));
        $$ = param;
    }
    | Annotations UnannType Annotations ELLIPSIS IdentifierComplement {
        AstNode *param = make_parameter_node($2, $5, @$);
        ast_add_child(param, make_keyword_leaf("...", @4));
        $$ = param;
    }
    | Annotations UnannType ELLIPSIS IdentifierComplement {
        AstNode *param = make_parameter_node($2, $4, @$);
        ast_add_child(param, make_keyword_leaf("...", @3));
        $$ = param;
    }
    | UnannType Annotations ELLIPSIS IdentifierComplement {
        AstNode *param = make_parameter_node($1, $4, @$);
        ast_add_child(param, make_keyword_leaf("...", @3));
        $$ = param;
    }
    | UnannType ELLIPSIS IdentifierComplement {
        AstNode *param = make_parameter_node($1, $3, @$);
        ast_add_child(param, make_keyword_leaf("...", @2));
        $$ = param;
    }
;

VariableModifiers:
    FINAL {
        $$ = make_list_node(AST_MODIFIER_LIST, @$);
        ast_add_child($$, make_keyword_leaf("final", @1));
    }
    | Annotations FINAL {
        $$ = make_list_node(AST_MODIFIER_LIST, @$);
        ast_add_child($$, $1);
        ast_add_child($$, make_keyword_leaf("final", @2));
    }
    | FINAL Annotations {
        $$ = make_list_node(AST_MODIFIER_LIST, @$);
        ast_add_child($$, make_keyword_leaf("final", @1));
        ast_add_child($$, $2);
    }
    | Annotations FINAL Annotations {
        $$ = make_list_node(AST_MODIFIER_LIST, @$);
        ast_add_child($$, $1);
        ast_add_child($$, make_keyword_leaf("final", @2));
        ast_add_child($$, $3);
    }
;

Throws:
    THROWS ExceptionTypeList { $$ = make_throws_node($2, @$); }
;

ExceptionTypeList:
    ExceptionType {
        $$ = make_list_node(AST_EXCEPTION_TYPE_LIST, @$);
        ast_add_child($$, $1);
    }
    | ExceptionTypeList ',' ExceptionType {
        ast_add_child($1, $3);
        $$ = $1;
    }
;


ExceptionType:
    ClassOrInterfaceType
//    | TypeVariable      // 因为 ClassOrInterfaceType 实际包括 TypeVariable 的内容（ ClassOrInterfaceType: TypeVariable ），故而这里将这一行语句去掉
;

MethodBody:
    Block { $$ = $1; }
    | ';' { $$ = AST_EMPTY_NODE(AST_BLOCK, @1); }
;

InstanceInitializer:
    Block { $$ = wrap_labeled_block("instance-init", $1, @1); }
;

StaticInitializer:
    STATIC Block { $$ = wrap_labeled_block("static-init", $2, @1); }
;


ConstructorDeclaration:
    Modifiers ConstructorDeclarator Throws ConstructorBody {
        if ($3) {
            ast_add_child($2, $3);
        }
        if ($4) {
            ast_add_child($2, $4);
        }
        $$ = $2;
    }
    | Modifiers ConstructorDeclarator ConstructorBody {
        if ($3) {
            ast_add_child($2, $3);
        }
        $$ = $2;
    }
    | ConstructorDeclarator Throws ConstructorBody {
        if ($2) {
            ast_add_child($1, $2);
        }
        if ($3) {
            ast_add_child($1, $3);
        }
        $$ = $1;
    }
    | ConstructorDeclarator ConstructorBody {
        if ($2) {
            ast_add_child($1, $2);
        }
        $$ = $1;
    }
;

ConstructorDeclarator:
    TypeParameters SimpleTypeName '(' ReceiverParameter ',' FormalParameterList ')' {
        $$ = make_constructor_node($2, $6, @$);
    }
    | TypeParameters SimpleTypeName '(' FormalParameterList ')' {
        $$ = make_constructor_node($2, $4, @$);
    }
    | TypeParameters SimpleTypeName '(' ')' {
        $$ = make_constructor_node($2, NULL, @$);
    }
    | SimpleTypeName '(' ReceiverParameter ',' FormalParameterList ')' {
        $$ = make_constructor_node($1, $5, @$);
    }
    | SimpleTypeName '(' FormalParameterList ')' {
        $$ = make_constructor_node($1, $3, @$);
    }
    | SimpleTypeName '(' ')' {
        $$ = make_constructor_node($1, NULL, @$);
    }
;

SimpleTypeName:
    TYPE_IDENTIFIER_MethodDeclarator { $$ = $1; }
;

ConstructorBody:
    '{' ExplicitConstructorInvocation BlockStatements '}' {
        $$ = AST_BRANCH_AT(AST_BLOCK, @1, 0);
    }
    | '{' ExplicitConstructorInvocation '}' {
        $$ = AST_BRANCH_AT(AST_BLOCK, @1, 0);
    }
    | '{' BlockStatements '}' {
        $$ = AST_BRANCH_AT(AST_BLOCK, @1, 0);
    }
    | '{' '}' {
        $$ = AST_BRANCH_AT(AST_BLOCK, @1, 0);
    }
;

ExplicitConstructorInvocation:
    TypeArguments THIS '(' ArgumentList ')' ';' {
        $$ = make_explicit_ctor_invocation(NULL, $1, "this", $4, @2);
    }
    | TypeArguments THIS '(' ')' ';' {
        $$ = make_explicit_ctor_invocation(NULL, $1, "this", NULL, @2);
    }
    | THIS '(' ArgumentList ')' ';' {
        $$ = make_explicit_ctor_invocation(NULL, NULL, "this", $3, @1);
    }
    | THIS '(' ')' ';' {
        $$ = make_explicit_ctor_invocation(NULL, NULL, "this", NULL, @1);
    }
    | TypeArguments SUPER '(' ArgumentList ')' ';' {
        $$ = make_explicit_ctor_invocation(NULL, $1, "super", $4, @2);
    }
    | TypeArguments SUPER '(' ')' ';' {
        $$ = make_explicit_ctor_invocation(NULL, $1, "super", NULL, @2);
    }
    | SUPER '(' ArgumentList ')' ';' {
        $$ = make_explicit_ctor_invocation(NULL, NULL, "super", $3, @1);
    }
    | SUPER '(' ')' ';' {
        $$ = make_explicit_ctor_invocation(NULL, NULL, "super", NULL, @1);
    }
    | CommonName '.' TypeArguments SUPER '(' ArgumentList ')' ';' {
        $$ = make_explicit_ctor_invocation($1, $3, "super", $6, @4);
    }
    | CommonName '.' TypeArguments SUPER '(' ')' ';' {
        $$ = make_explicit_ctor_invocation($1, $3, "super", NULL, @4);
    }
    | CommonName '.' SUPER '(' ArgumentList ')' ';' {
        $$ = make_explicit_ctor_invocation($1, NULL, "super", $5, @3);
    }
    | CommonName '.' SUPER '(' ')' ';' {
        $$ = make_explicit_ctor_invocation($1, NULL, "super", NULL, @3);
    }
    | Primary '.' TypeArguments SUPER '(' ArgumentList ')' ';' {
        $$ = make_explicit_ctor_invocation($1, $3, "super", $6, @4);
    }
    | Primary '.' TypeArguments SUPER '(' ')' ';' {
        $$ = make_explicit_ctor_invocation($1, $3, "super", NULL, @4);
    }
    | Primary '.' SUPER '(' ArgumentList ')' ';' {
        $$ = make_explicit_ctor_invocation($1, NULL, "super", $5, @3);
    }
    | Primary '.' SUPER '(' ')' ';' {
        $$ = make_explicit_ctor_invocation($1, NULL, "super", NULL, @3);
    }
;

EnumDeclaration:
    Modifiers ENUM TYPE_IDENTIFIER_EnumDeclaration Superinterfaces EnumBody {
        /* 4 个孩子: 修饰符, 名字, superinterfaces, body */
        $$ = ast_branch(AST_ENUM_DECL,
                        @2.first_line, @2.first_column,
                        4,
                        $1,     /* modifiers */
                        $3,     /* name */
                        $4,     /* superinterfaces */
                        $5);    /* body (EnumBody) */
    }
  | Modifiers ENUM TYPE_IDENTIFIER_EnumDeclaration EnumBody {
        $$ = ast_branch(AST_ENUM_DECL,
                        @2.first_line, @2.first_column,
                        4,
                        $1,     /* modifiers */
                        $3,     /* name */
                        NULL,   /* no superinterfaces */
                        $4);    /* body */
    }
  | ENUM TYPE_IDENTIFIER_EnumDeclaration Superinterfaces EnumBody {
        $$ = ast_branch(AST_ENUM_DECL,
                        @1.first_line, @1.first_column,
                        4,
                        NULL,   /* no modifiers */
                        $2,     /* name */
                        $3,     /* superinterfaces */
                        $4);    /* body */
    }
  | ENUM TYPE_IDENTIFIER_EnumDeclaration EnumBody {
        $$ = ast_branch(AST_ENUM_DECL,
                        @1.first_line, @1.first_column,
                        4,
                        NULL,   /* no modifiers */
                        $2,     /* name */
                        NULL,   /* no superinterfaces */
                        $3);    /* body */
    }
;


EnumBody:
    '{' EnumConstantList ',' EnumBodyDeclarations '}' {
        $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 0);
        ast_add_child($$, $2);
        ast_add_child($$, $4);
    }
    | '{' EnumConstantList EnumBodyDeclarations '}' {
        $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 0);
        ast_add_child($$, $2);
        ast_add_child($$, $3);
    }
    | '{' EnumConstantList '}' {
        $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 1, $2);
    }
    | '{' EnumConstantList ',' '}' {
        $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 1, $2);
    }
//    | '{' EnumConstantList TRAILING_COMMA '}'
    | '{' EnumBodyDeclarations '}' {
        $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 1, $2);
    }
    | '{' '}' {
        $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 0);
    }
;

EnumConstantList:
    EnumConstant {
        $$ = ast_branch(AST_STATEMENT_LIST, @1.first_line, @1.first_column, 1, $1);
    }
    | EnumConstantList ',' EnumConstant {
        ast_add_child($1, $3);
        $$ = $1;
    }
;


// 在新一次的迭代中，我们认为可以直接把 EnumConstantModifiers 替换为 Annotations
// 这里为了避免冲突把 TYPE_IDENTIFIER 直接换成了 SimpleTypeName
EnumConstant:
    Annotations SimpleTypeName '(' ArgumentList ')' ClassBody {
        $$ = make_enum_constant($1, $2, $4, $6, @$);
    }
    | Annotations SimpleTypeName '(' ArgumentList ')' {
        $$ = make_enum_constant($1, $2, $4, NULL, @$);
    }
    | Annotations TYPE_IDENTIFIER ClassBody {
        $$ = make_enum_constant($1, $2, NULL, $3, @$);
    }
    | Annotations TYPE_IDENTIFIER {
        $$ = make_enum_constant($1, $2, NULL, NULL, @$);
    }
    | SimpleTypeName '(' ArgumentList ')' ClassBody {
        $$ = make_enum_constant(NULL, $1, $3, $5, @$);
    }
    | SimpleTypeName '(' ArgumentList ')' {
        $$ = make_enum_constant(NULL, $1, $3, NULL, @$);
    }
    | TYPE_IDENTIFIER ClassBody {
        $$ = make_enum_constant(NULL, $1, NULL, $2, @$);
    }
    | TYPE_IDENTIFIER {
        $$ = make_enum_constant(NULL, $1, NULL, NULL, @$);
    }
    | Annotations IdentifierComplement '(' ArgumentList ')' ClassBody {
        $$ = make_enum_constant($1, $2, $4, $6, @$);
    }
    | Annotations IdentifierComplement '(' ArgumentList ')' {
        $$ = make_enum_constant($1, $2, $4, NULL, @$);
    }
    | Annotations IdentifierComplement ClassBody {
        $$ = make_enum_constant($1, $2, NULL, $3, @$);
    }
    | Annotations IdentifierComplement {
        $$ = make_enum_constant($1, $2, NULL, NULL, @$);
    }
    | IdentifierComplement '(' ArgumentList ')' ClassBody {
        $$ = make_enum_constant(NULL, $1, $3, $5, @$);
    }
    | IdentifierComplement '(' ArgumentList ')' {
        $$ = make_enum_constant(NULL, $1, $3, NULL, @$);
    }
    | IdentifierComplement ClassBody {
        $$ = make_enum_constant(NULL, $1, NULL, $2, @$);
    }
    | IdentifierComplement {
        $$ = make_enum_constant(NULL, $1, NULL, NULL, @$);
    }
;

EnumBodyDeclarations:
    ';' ClassBodyDeclarationList {
        $$ = $2;
    }
;



//-----------------------------------Interfaces-------------------------------------



InterfaceDeclaration:
    NormalInterfaceDeclaration { $$ = $1; }
    | AnnotationTypeDeclaration {
        $$ = $1;
    }
;

NormalInterfaceDeclaration:
    Modifiers INTERFACE TYPE_IDENTIFIER OptTypeParameters OptExtendsInterfaces OptClassOrInterfacePermits InterfaceBody {
        $$ = make_interface_basic(@2.first_line, @2.first_column, 
                                  $1, /* Modifiers */
                                  $3, /* Name */
                                  $4, /* TypeParams */
                                  $5, /* Extends */
                                  $6, /* Permits */
                                  $7);/* Body */
    }
  | INTERFACE TYPE_IDENTIFIER OptTypeParameters OptExtendsInterfaces OptClassOrInterfacePermits InterfaceBody {
        $$ = make_interface_basic(@1.first_line, @1.first_column, 
                                  NULL, 
                                  $2, 
                                  $3, 
                                  $4, 
                                  $5, 
                                  $6);
    }
;


ExtendsInterfaces:
    EXTENDS InterfaceTypeList {
        $$ = $2;
    }
;

OptExtendsInterfaces:
    ExtendsInterfaces { $$ = $1; }
    | /* empty */ { $$ = NULL; }
;

InterfaceBody:
    '{' InterfaceMemberDeclarationList '}' { $$ = $2; }
    | '{' '}' { $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 0); }
;

// 允许使用一个或者多个InterfaceMemberDeclaration
InterfaceMemberDeclarationList:
    InterfaceMemberDeclaration {
        $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 1, $1);
    }
    | InterfaceMemberDeclarationList InterfaceMemberDeclaration {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

InterfaceMemberDeclaration:
    ConstantDeclaration { $$ = $1; }
    | InterfaceMethodDeclaration { $$ = $1; }
    | ClassDeclaration { $$ = $1; }
    | InterfaceDeclaration { $$ = $1; }
    | ';' { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
    | EMPTY_STMT { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
;

ConstantDeclaration:
    Modifiers UnannType VariableDeclaratorList ';' {
        $$ = make_field_node($1, $2, $3, @2);
    }
  | UnannType VariableDeclaratorList ';' {
        $$ = make_field_node(NULL, $1, $2, @1);
    }
;


InterfaceMethodDeclaration:
    Modifiers MethodHeader MethodBody {
        if ($3) {
            ast_add_child($2, $3);
        }
        $$ = $2;
    }
    | MethodHeader MethodBody {
        if ($2) {
            ast_add_child($1, $2);
        }
        $$ = $1;
    }
;

// 在词法里区分 AnnotationTypeDeclaration 的 '@' 和 Annotation 的 '@'
AnnotationTypeDeclaration:
    /* 原有规则保持不变 */
    Modifiers AT_AnnotationTypeDeclaration INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
        $$ = make_interface_basic(@3.first_line, @3.first_column, $1, $4, NULL, NULL, NULL, $5 ? $5 : ast_branch(AST_BLOCK, @3.first_line, @3.first_column, 0));
    }
    | AT_AnnotationTypeDeclaration INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
        $$ = make_interface_basic(@2.first_line, @2.first_column, NULL, $3, NULL, NULL, NULL, $4 ? $4 : ast_branch(AST_BLOCK, @2.first_line, @2.first_column, 0));
    }
    /* === 新增以下规则以兼容 Lexer 返回的普通 AT Token === */
    | Modifiers AT_Modifier INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
        $$ = make_interface_basic(@3.first_line, @3.first_column, $1, $4, NULL, NULL, NULL, $5 ? $5 : ast_branch(AST_BLOCK, @3.first_line, @3.first_column, 0));
    }
    | AT_Modifier INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
        $$ = make_interface_basic(@2.first_line, @2.first_column, NULL, $3, NULL, NULL, NULL, $4 ? $4 : ast_branch(AST_BLOCK, @2.first_line, @2.first_column, 0));
    }
    /* 如果你的 Lexer 返回的是字符 '@' 而不是宏定义，可以把下面这行也加上 */
    | Modifiers '@' INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
         $$ = make_interface_basic(@3.first_line, @3.first_column, $1, $4, NULL, NULL, NULL, $5 ? $5 : ast_branch(AST_BLOCK, @3.first_line, @3.first_column, 0));
    }
    | '@' INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
         $$ = make_interface_basic(@2.first_line, @2.first_column, NULL, $3, NULL, NULL, NULL, $4 ? $4 : ast_branch(AST_BLOCK, @2.first_line, @2.first_column, 0));
    }
;
AnnotationTypeBody:
    '{' AnnotationTypeMemberDeclarationList '}' {
        $$ = $2;
    }
    | '{' '}' {
        $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 0);
    }
;

// 允许使用一个或者多个AnnotationTypeMemberDeclaration
AnnotationTypeMemberDeclarationList:
    AnnotationTypeMemberDeclaration {
        $$ = ast_branch(AST_BLOCK, @1.first_line, @1.first_column, 1, $1);
    }
    | AnnotationTypeMemberDeclarationList AnnotationTypeMemberDeclaration {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

AnnotationTypeMemberDeclaration:
    AnnotationTypeElementDeclaration { $$ = $1; }
    | ConstantDeclaration { $$ = $1; }
    | ClassDeclaration { $$ = $1; }
    | InterfaceDeclaration { $$ = $1; }
    | ';' { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
    | EMPTY_STMT { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
;

AnnotationTypeElementDeclaration:
//    AnnotationTypeElementModifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' Dims DefaultValue ';'
//    | AnnotationTypeElementModifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' Dims ';'
//    | AnnotationTypeElementModifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' DefaultValue ';'
//    | AnnotationTypeElementModifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' ';'
    Modifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' Dims DefaultValue ';' {
        $$ = make_annotation_element_decl($1, $2, $3, $6, $7, @$);
    }
    | Modifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' Dims ';' {
        $$ = make_annotation_element_decl($1, $2, $3, $6, NULL, @$);
    }
    | Modifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' DefaultValue ';' {
        $$ = make_annotation_element_decl($1, $2, $3, NULL, $6, @$);
    }
    | Modifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' ';' {
        $$ = make_annotation_element_decl($1, $2, $3, NULL, NULL, @$);
    }
    | UnannType IDENTIFIER_MethodDeclarator '(' ')' Dims DefaultValue ';' {
        $$ = make_annotation_element_decl(NULL, $1, $2, $5, $6, @$);
    }
    | UnannType IDENTIFIER_MethodDeclarator '(' ')' Dims ';' {
        $$ = make_annotation_element_decl(NULL, $1, $2, $5, NULL, @$);
    }
    | UnannType IDENTIFIER_MethodDeclarator '(' ')' DefaultValue ';' {
        $$ = make_annotation_element_decl(NULL, $1, $2, NULL, $5, @$);
    }
    | UnannType IDENTIFIER_MethodDeclarator '(' ')' ';' {
        $$ = make_annotation_element_decl(NULL, $1, $2, NULL, NULL, @$);
    }
;

DefaultValue:
    DEFAULT ElementValue {
        $$ = AST_BRANCH_AT(AST_ASSIGN, @1, 1, $2);
    }
;

Annotation:
    NormalAnnotation { $$ = $1; }
    | MarkerAnnotation { $$ = $1; }
    | SingleElementAnnotation { $$ = $1; }
;

//允许使用一个或者多个Annotation，如遇到需要零个的情况，请自行在语句中定义
Annotations:
    Annotations Annotation {
        ast_add_child($1, $2);
        $$ = $1;
    }
    | Annotation {
        $$ = make_list_node(AST_ANNOTATION, @$);
        ast_add_child($$, $1);
    }
;

NormalAnnotation:
    '@' TypeName_ModifierOrDims '(' ElementValuePairList ')' {
        AstNode *node = make_annotation_node($2, @1);
        ast_add_child(node, $4);
        $$ = node;
    }
    | '@' TypeName_ModifierOrDims '(' ')' {
        $$ = make_annotation_node($2, @1);
    }
;

ElementValuePairList:
    ElementValuePair {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | ElementValuePairList ',' ElementValuePair {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

ElementValuePair:
    TYPE_IDENTIFIER '=' ElementValue {
        AstNode *node = AST_BRANCH_AT(AST_ASSIGN, @2, 0);
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
    | IdentifierComplement '=' ElementValue {
        AstNode *node = AST_BRANCH_AT(AST_ASSIGN, @2, 0);
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
;

ElementValue:
    ConditionalExpression
//    | ConditionalOrExpression
    | ElementValueArrayInitializer { $$ = $1; }
    | Annotation { $$ = $1; }
;

ElementValueArrayInitializer:
    '{' ElementValueList ',' '}' {
        $$ = make_array_initializer_node($2, @1);
    }
//    '{' ElementValueList TRAILING_COMMA '}'
    | '{' ElementValueList '}' {
        $$ = make_array_initializer_node($2, @1);
    }
    | '{' '}' {
        $$ = make_array_initializer_node(NULL, @1);
    }
;

ElementValueList:
    ElementValue {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | ElementValueList ',' ElementValue {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

MarkerAnnotation:
    '@' TypeName {
        $$ = make_annotation_node($2, @1);
    }
;

SingleElementAnnotation:
    '@' TypeName_ModifierOrDims '(' ElementValue ')' {
        AstNode *node = make_annotation_node($2, @1);
        AstNode *args = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child(args, $4);
        ast_add_child(node, args);
        $$ = node;
    }
;



//--------------------------------Blocks and Statements---------------------------



Block:
    '{' BlockStatements '}' {
        $$ = make_block_node(@1, $2);
    }
    | '{' '}' {
        $$ = make_block_node(@1, NULL);
    }
;

BlockStatements:
    BlockStatement {
        $$ = make_list_node(AST_STATEMENT_LIST, @$);
        if ($1) {
            ast_add_child($$, $1);
        }
    }
    | BlockStatements BlockStatement {
        if ($2) {
            ast_add_child($1, $2);
        }
        $$ = $1;
    }
;

BlockStatement:
    LocalVariableDeclarationStatement { $$ = $1; }
    | LocalClassOrInterfaceDeclaration { $$ = $1; }
    | Statement { $$ = $1; }
;

LocalVariableDeclarationStatement:
    LocalVariableDeclaration ';' { $$ = $1; }
;

LocalVariableDeclaration:
    VariableModifiers LocalVariableType VariableDeclaratorList {
        $$ = make_local_variable_node($2, $3, @2);
    }
    | Annotations LocalVariableType VariableDeclaratorList {
        $$ = make_local_variable_node($2, $3, @2);
    }
    | LocalVariableType VariableDeclaratorList {
        $$ = make_local_variable_node($1, $2, @1);
    }
;

LocalVariableType:
    UnannType { $$ = $1; }
    | VAR { $$ = make_keyword_leaf("var", @1); }
;

LocalClassOrInterfaceDeclaration:
    ClassDeclaration { $$ = $1; }
    | NormalInterfaceDeclaration { $$ = $1; }
;

Statement:
    StatementWithoutTrailingSubstatement { $$ = $1; }
    | LabeledStatement { $$ = $1; }
    | IfThenStatement { $$ = $1; }
    | IfThenElseStatement { $$ = $1; }
    | WhileStatement { $$ = $1; }
    | ForStatement { $$ = $1; }
;

StatementNoShortIf:
    StatementWithoutTrailingSubstatement { $$ = $1; }
    | LabeledStatementNoShortIf { $$ = $1; }
    | IfThenElseStatementNoShortIf { $$ = $1; }
    | WhileStatementNoShortIf { $$ = $1; }
    | ForStatementNoShortIf { $$ = $1; }
;

StatementWithoutTrailingSubstatement:
    Block { $$ = $1; }
    | EmptyStatement { $$ = $1; }
    | ExpressionStatement { $$ = $1; }
    | AssertStatement { $$ = $1; }
    | SwitchStatement { $$ = $1; }
    | DoStatement { $$ = $1; }
    | BreakStatement { $$ = $1; }
    | ContinueStatement { $$ = $1; }
    | ReturnStatement { $$ = $1; }
    | SynchronizedStatement { $$ = $1; }
    | ThrowStatement { $$ = $1; }
    | TryStatement { $$ = $1; }
    | YieldStatement { $$ = $1; }
;

EmptyStatement:
    EMPTY_STMT {
        $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column);
    }
    | ';' {
        $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column);
    }
;

LabeledStatement:
    TYPE_IDENTIFIER ':' Statement {
        AstNode *node = make_unary_stmt(AST_LABELED_STATEMENT, $3, @1);
        ast_prepend_child(node, $1);
        $$ = node;
    }
    | IdentifierComplement ':' Statement {
        AstNode *node = make_unary_stmt(AST_LABELED_STATEMENT, $3, @1);
        ast_prepend_child(node, $1);
        $$ = node;
    }
;

LabeledStatementNoShortIf:
    TYPE_IDENTIFIER ':' StatementNoShortIf {
        AstNode *node = make_unary_stmt(AST_LABELED_STATEMENT, $3, @1);
        ast_prepend_child(node, $1);
        $$ = node;
    }
    | IdentifierComplement ':' StatementNoShortIf {
        AstNode *node = make_unary_stmt(AST_LABELED_STATEMENT, $3, @1);
        ast_prepend_child(node, $1);
        $$ = node;
    }
;

ExpressionStatement:
    StatementExpression ';' {
        $$ = $1;
    }
;

StatementExpression:
    Assignment { $$ = $1; }
    | PreIncrementExpression { $$ = $1; }
    | PreDecrementExpression { $$ = $1; }
    | PostIncrementExpression { $$ = $1; }
    | PostDecrementExpression { $$ = $1; }
    | MethodInvocation { $$ = $1; }
    | ClassInstanceCreationExpression { $$ = $1; }
;

IfThenStatement:
    IF '(' Expression ')' Statement {
        $$ = make_ternary_stmt(AST_IF, $3, $5, NULL, @1);
    }
;

IfThenElseStatement:
    IF '(' Expression ')' StatementNoShortIf ELSE Statement {
        $$ = make_ternary_stmt(AST_IF, $3, $5, $7, @1);
    }
;

IfThenElseStatementNoShortIf:
    IF '(' Expression ')' StatementNoShortIf ELSE StatementNoShortIf {
        $$ = make_ternary_stmt(AST_IF, $3, $5, $7, @1);
    }
;

AssertStatement:
    ASSERT Expression ';' {
        $$ = make_unary_stmt(AST_ASSERT, $2, @1);
    }
    | ASSERT Expression ':' Expression ';' {
        AstNode *node = make_binary_stmt(AST_ASSERT, $2, $4, @1);
        $$ = node;
    }
;

SwitchStatement:
    SWITCH '(' Expression ')' SwitchBlock {
        AstNode *node = make_unary_stmt(AST_SWITCH, $3, @1);
        if ($5) {
            ast_add_child(node, $5);
        }
        $$ = node;
    }
;

SwitchBlock:
    '{' SwitchRules '}' {
        $$ = $2;
    }
//    | '{' SwitchBlockStatementGroups SwitchLabels '}'
    | '{' SwitchBlockStatementGroups '}' {
        $$ = $2;
    }
    | '{''}' {
        $$ = make_list_node(AST_STATEMENT_LIST, @$);
    }
;

SwitchRule:
    SwitchLabel ARROW Expression ';' {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_LABEL, @1, 0);
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
    | SwitchLabel ARROW Block {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_LABEL, @1, 0);
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
    | SwitchLabel ARROW ThrowStatement {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_LABEL, @1, 0);
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
;

// 允许使用一个或者多个SwitchRule
SwitchRules:
    SwitchRule {
        $$ = make_list_node(AST_STATEMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | SwitchRules SwitchRule {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

SwitchBlockStatementGroup:
    SwitchLabels BlockStatements {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_LABEL, @1, 0);
        ast_add_child(node, $1);
        if ($2) {
            ast_add_child(node, $2);
        }
        $$ = node;
    }
;

// 允许使用一个或者多个SwitchBlockStatementGroup
SwitchBlockStatementGroups:
    SwitchBlockStatementGroup {
        $$ = make_list_node(AST_STATEMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | SwitchBlockStatementGroups SwitchBlockStatementGroup {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

SwitchLabel:
    CASE CaseConstants {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_LABEL, @1, 0);
        ast_add_child(node, $2);
        $$ = node;
    }
    | DEFAULT_SwitchLabel {
        $$ = make_keyword_leaf("default", @1);
    }
;

// 允许使用一个或者多个SwitchLabel
SwitchLabels:
    SwitchLabel ':' {
        AstNode *node = make_list_node(AST_STATEMENT_LIST, @$);
        ast_add_child(node, $1);
        $$ = node;
    }
    | SwitchLabels SwitchLabel ':' {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

CaseConstant:
    ConditionalExpression { $$ = $1; }
;

CaseConstants:
    CaseConstant {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | CaseConstants ',' CaseConstant {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

WhileStatement:
    WHILE '(' Expression ')' Statement {
        $$ = make_binary_stmt(AST_WHILE, $3, $5, @1);
    }
;

WhileStatementNoShortIf:
    WHILE '(' Expression ')' StatementNoShortIf {
        $$ = make_binary_stmt(AST_WHILE, $3, $5, @1);
    }
;

DoStatement:
    DO Statement WHILE '(' Expression ')' ';' {
        AstNode *node = make_binary_stmt(AST_DO_WHILE, $2, $5, @1);
        $$ = node;
    }
;

ForStatement:
    BasicForStatement { $$ = $1; }
    | EnhancedForStatement { $$ = $1; }
;

ForStatementNoShortIf:
    BasicForStatementNoShortIf { $$ = $1; }
    | EnhancedForStatementNoShortIf { $$ = $1; }
;

BasicForStatement:
    FOR '(' ForInit ';' Expression ';' ForUpdate ')' Statement {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        if ($3) ast_add_child(node, $3);
        if ($5) ast_add_child(node, $5);
        if ($7) ast_add_child(node, $7);
        if ($9) ast_add_child(node, $9);
        $$ = node;
    }
    | FOR '(' ForInit ';' Expression ';' ')' Statement {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        if ($3) ast_add_child(node, $3);
        if ($5) ast_add_child(node, $5);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($8) ast_add_child(node, $8);
        $$ = node;
    }
    | FOR '(' ForInit ';' ';' ForUpdate ')' Statement {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        if ($3) ast_add_child(node, $3);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($6) ast_add_child(node, $6);
        if ($8) ast_add_child(node, $8);
        $$ = node;
    }
    | FOR '(' ForInit ';' ';' ')' Statement {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        if ($3) ast_add_child(node, $3);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($7) ast_add_child(node, $7);
        $$ = node;
    }
    | FOR '(' ';' Expression ';' ForUpdate ')' Statement {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($4) ast_add_child(node, $4);
        if ($6) ast_add_child(node, $6);
        if ($8) ast_add_child(node, $8);
        $$ = node;
    }
    | FOR '(' ';' Expression ';' ')' Statement {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($4) ast_add_child(node, $4);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($7) ast_add_child(node, $7);
        $$ = node;
    }
    | FOR '(' ';' ';' ForUpdate ')' Statement {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($5) ast_add_child(node, $5);
        if ($7) ast_add_child(node, $7);
        $$ = node;
    }
    | FOR '(' ';' ';' ')' Statement {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($6) ast_add_child(node, $6);
        $$ = node;
    }
;

BasicForStatementNoShortIf:
    FOR '(' ForInit ';' Expression ';' ForUpdate ')' StatementNoShortIf {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        if ($3) ast_add_child(node, $3);
        if ($5) ast_add_child(node, $5);
        if ($7) ast_add_child(node, $7);
        if ($9) ast_add_child(node, $9);
        $$ = node;
    }
    | FOR '(' ForInit ';' Expression ';' ')' StatementNoShortIf {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        if ($3) ast_add_child(node, $3);
        if ($5) ast_add_child(node, $5);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($8) ast_add_child(node, $8);
        $$ = node;
    }
    | FOR '(' ForInit ';' ';' ForUpdate ')' StatementNoShortIf {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        if ($3) ast_add_child(node, $3);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($6) ast_add_child(node, $6);
        if ($8) ast_add_child(node, $8);
        $$ = node;
    }
    | FOR '(' ForInit ';' ';' ')' StatementNoShortIf {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        if ($3) ast_add_child(node, $3);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($7) ast_add_child(node, $7);
        $$ = node;
    }
    | FOR '(' ';' Expression ';' ForUpdate ')' StatementNoShortIf {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($4) ast_add_child(node, $4);
        if ($6) ast_add_child(node, $6);
        if ($8) ast_add_child(node, $8);
        $$ = node;
    }
    | FOR '(' ';' Expression ';' ')' StatementNoShortIf {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($4) ast_add_child(node, $4);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($7) ast_add_child(node, $7);
        $$ = node;
    }
    | FOR '(' ';' ';' ForUpdate ')' StatementNoShortIf {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($5) ast_add_child(node, $5);
        if ($7) ast_add_child(node, $7);
        $$ = node;
    }
    | FOR '(' ';' ';' ')' StatementNoShortIf {
        AstNode *node = AST_BRANCH_AT(AST_FOR, @1, 0);
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        ast_add_child(node, make_list_node(AST_STATEMENT_LIST, @$));
        if ($6) ast_add_child(node, $6);
        $$ = node;
    }
;

ForInit:
    StatementExpressionList { $$ = $1; }
    | LocalVariableDeclaration { $$ = $1; }
;

ForUpdate:
    StatementExpressionList { $$ = $1; }
;

StatementExpressionList:
    StatementExpression {
        $$ = make_list_node(AST_STATEMENT_LIST, @$);
        if ($1) ast_add_child($$, $1);
    }
    | StatementExpressionList ',' StatementExpression {
        if ($3) {
            ast_add_child($1, $3);
        }
        $$ = $1;
    }
;

// 这里为了避免冲突把 VariableDeclaratorId 直接诠释为了 IDENTIFIER Dims / IDENTIFIER ;
EnhancedForStatement:
//    FOR '(' VariableModifiers LocalVariableType VariableDeclaratorId ':' Expression ')' Statement
//    | FOR '(' Annotations LocalVariableType VariableDeclaratorId ':' Expression ')' Statement
//    | FOR '(' LocalVariableType VariableDeclaratorId ':' Expression ')' Statement
    FOR '(' VariableModifiers LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($4, $5, $6, @$);
        $$ = make_foreach_stmt(param, $8, $10, @1);
    }
    | FOR '(' Annotations LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($4, $5, $6, @$);
        $$ = make_foreach_stmt(param, $8, $10, @1);
    }
    | FOR '(' LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($3, $4, $5, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' VariableModifiers LocalVariableType TYPE_IDENTIFIER ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($4, $5, NULL, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' Annotations LocalVariableType TYPE_IDENTIFIER ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($4, $5, NULL, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' LocalVariableType TYPE_IDENTIFIER ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($3, $4, NULL, @$);
        $$ = make_foreach_stmt(param, $6, $8, @1);
    }
    | FOR '(' VariableModifiers LocalVariableType IdentifierComplement Dims ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($4, $5, $6, @$);
        $$ = make_foreach_stmt(param, $8, $10, @1);
    }
    | FOR '(' Annotations LocalVariableType IdentifierComplement Dims ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($4, $5, $6, @$);
        $$ = make_foreach_stmt(param, $8, $10, @1);
    }
    | FOR '(' LocalVariableType IdentifierComplement Dims ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($3, $4, $5, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' VariableModifiers LocalVariableType IdentifierComplement ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($4, $5, NULL, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' Annotations LocalVariableType IdentifierComplement ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($4, $5, NULL, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' LocalVariableType IdentifierComplement ':' Expression ')' Statement {
        AstNode *param = make_parameter_with_dims($3, $4, NULL, @$);
        $$ = make_foreach_stmt(param, $6, $8, @1);
    }
;

// 这里为了避免冲突把 VariableDeclaratorId 直接诠释为了 IDENTIFIER Dims / IDENTIFIER ;
EnhancedForStatementNoShortIf:
//    FOR '(' VariableModifiers LocalVariableType VariableDeclaratorId ':' Expression ')' StatementNoShortIf
//    | FOR '(' Annotations LocalVariableType VariableDeclaratorId ':' Expression ')' StatementNoShortIf
//    | FOR '(' LocalVariableType VariableDeclaratorId ':' Expression ')' StatementNoShortIf
    FOR '(' VariableModifiers LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($4, $5, $6, @$);
        $$ = make_foreach_stmt(param, $8, $10, @1);
    }
    | FOR '(' Annotations LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($4, $5, $6, @$);
        $$ = make_foreach_stmt(param, $8, $10, @1);
    }
    | FOR '(' LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($3, $4, $5, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' VariableModifiers LocalVariableType TYPE_IDENTIFIER ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($4, $5, NULL, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' Annotations LocalVariableType TYPE_IDENTIFIER ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($4, $5, NULL, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' LocalVariableType TYPE_IDENTIFIER ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($3, $4, NULL, @$);
        $$ = make_foreach_stmt(param, $6, $8, @1);
    }
    | FOR '(' VariableModifiers LocalVariableType IdentifierComplement Dims ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($4, $5, $6, @$);
        $$ = make_foreach_stmt(param, $8, $10, @1);
    }
    | FOR '(' Annotations LocalVariableType IdentifierComplement Dims ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($4, $5, $6, @$);
        $$ = make_foreach_stmt(param, $8, $10, @1);
    }
    | FOR '(' LocalVariableType IdentifierComplement Dims ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($3, $4, $5, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' VariableModifiers LocalVariableType IdentifierComplement ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($4, $5, NULL, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' Annotations LocalVariableType IdentifierComplement ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($4, $5, NULL, @$);
        $$ = make_foreach_stmt(param, $7, $9, @1);
    }
    | FOR '(' LocalVariableType IdentifierComplement ':' Expression ')' StatementNoShortIf {
        AstNode *param = make_parameter_with_dims($3, $4, NULL, @$);
        $$ = make_foreach_stmt(param, $6, $8, @1);
    }
;

BreakStatement:
    BREAK TYPE_IDENTIFIER ';' {
        AstNode *node = make_unary_stmt(AST_BREAK, $2, @1);
        $$ = node;
    }
    | BREAK IdentifierComplement ';' {
        AstNode *node = make_unary_stmt(AST_BREAK, $2, @1);
        $$ = node;
    }
    | BREAK ';' {
        $$ = make_simple_stmt(AST_BREAK, @1);
    }
;

YieldStatement:
    YIELD Expression ';' {
        $$ = make_unary_stmt(AST_YIELD, $2, @1);
    }
;

ContinueStatement:
    CONTINUE ';' {
        $$ = make_simple_stmt(AST_CONTINUE, @1);
    }
    | CONTINUE TYPE_IDENTIFIER ';' {
        AstNode *node = make_unary_stmt(AST_CONTINUE, $2, @1);
        $$ = node;
    }
    | CONTINUE IdentifierComplement ';' {
        AstNode *node = make_unary_stmt(AST_CONTINUE, $2, @1);
        $$ = node;
    }
;

ReturnStatement:
    RETURN Expression ';' {
        $$ = make_unary_stmt(AST_RETURN, $2, @1);
    }
    | RETURN ';' {
        $$ = make_simple_stmt(AST_RETURN, @1);
    }
;

ThrowStatement:
    THROW Expression ';' {
        $$ = make_unary_stmt(AST_THROW, $2, @1);
    }
;

SynchronizedStatement:
    SYNCHRONIZED '(' Expression ')' Block {
        $$ = make_binary_stmt(AST_SYNCHRONIZED, $3, $5, @1);
    }
;

TryStatement:
    TRY Block Catches {
        $$ = make_try_stmt($2, $3, NULL, @1);
    }
    | TRY Block Catches Finally {
        $$ = make_try_stmt($2, $3, $4, @1);
    }
    | TRY Block Finally {
        $$ = make_try_stmt($2, NULL, $3, @1);
    }
    | TryWithResourcesStatement {
        $$ = $1;
    }
;

Catches:
    CatchClause {
        $$ = make_list_node(AST_STATEMENT_LIST, @$);
        if ($1) ast_add_child($$, $1);
    }
    | Catches CatchClause {
        if ($2) ast_add_child($1, $2);
        $$ = $1;
    }
;

CatchClause:
    CATCH '(' CatchFormalParameter ')' Block {
        AstNode *node = AST_BRANCH_AT(AST_CATCH, @1, 0);
        if ($3) ast_add_child(node, $3);
        if ($5) ast_add_child(node, $5);
        $$ = node;
    }
;

// 这里为了避免冲突把 VariableDeclaratorId 直接诠释为了 IDENTIFIER Dims / IDENTIFIER ;
CatchFormalParameter:
//    VariableModifiers CatchType VariableDeclaratorId
//    | Annotations CatchType VariableDeclaratorId
//    | CatchType VariableDeclaratorId
    VariableModifiers CatchType TYPE_IDENTIFIER Dims {
        $$ = make_parameter_with_dims($2, $3, $4, @$);
    }
    | Annotations CatchType TYPE_IDENTIFIER Dims {
        $$ = make_parameter_with_dims($2, $3, $4, @$);
    }
    | CatchType TYPE_IDENTIFIER Dims {
        $$ = make_parameter_with_dims($1, $2, $3, @$);
    }
    | VariableModifiers CatchType TYPE_IDENTIFIER {
        $$ = make_parameter_with_dims($2, $3, NULL, @$);
    }
    | Annotations CatchType TYPE_IDENTIFIER {
        $$ = make_parameter_with_dims($2, $3, NULL, @$);
    }
    | CatchType TYPE_IDENTIFIER {
        $$ = make_parameter_with_dims($1, $2, NULL, @$);
    }
    | VariableModifiers CatchType IdentifierComplement Dims {
        $$ = make_parameter_with_dims($2, $3, $4, @$);
    }
    | Annotations CatchType IdentifierComplement Dims {
        $$ = make_parameter_with_dims($2, $3, $4, @$);
    }
    | CatchType IdentifierComplement Dims {
        $$ = make_parameter_with_dims($1, $2, $3, @$);
    }
    | VariableModifiers CatchType IdentifierComplement {
        $$ = make_parameter_with_dims($2, $3, NULL, @$);
    }
    | Annotations CatchType IdentifierComplement {
        $$ = make_parameter_with_dims($2, $3, NULL, @$);
    }
    | CatchType IdentifierComplement {
        $$ = make_parameter_with_dims($1, $2, NULL, @$);
    }
;

CatchType:
    UnannClassOrInterfaceType { $$ = $1; }
    | CatchType '|' ClassOrInterfaceType {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

Finally:
    FINALLY Block {
        AstNode *node = AST_BRANCH_AT(AST_FINALLY, @1, 0);
        if ($2) ast_add_child(node, $2);
        $$ = node;
    }
;

TryWithResourcesStatement:
    TRY ResourceSpecification Block Catches Finally {
        AstNode *node = make_try_stmt($3, $4, $5, @1);
        if ($2) {
            ast_prepend_child(node, $2);
        }
        $$ = node;
    }
    | TRY ResourceSpecification Block Catches {
        AstNode *node = make_try_stmt($3, $4, NULL, @1);
        if ($2) {
            ast_prepend_child(node, $2);
        }
        $$ = node;
    }
    | TRY ResourceSpecification Block Finally {
        AstNode *node = make_try_stmt($3, NULL, $4, @1);
        if ($2) {
            ast_prepend_child(node, $2);
        }
        $$ = node;
    }
    | TRY ResourceSpecification Block {
        AstNode *node = make_try_stmt($3, NULL, NULL, @1);
        if ($2) {
            ast_prepend_child(node, $2);
        }
        $$ = node;
    }
;

ResourceSpecification:
    '(' ResourceList ')' {
        $$ = AST_BRANCH_AT(AST_RESOURCE_SPEC, @$, 1, $2);
    }
  | '(' ResourceList ';' ')' {
        $$ = AST_BRANCH_AT(AST_RESOURCE_SPEC, @$, 1, $2);
    }
;


ResourceList:
    Resource {
        $$ = make_list_node(AST_RESOURCE_LIST, @$);
        ast_add_child($$, $1);
    }
  | ResourceList ';' Resource {
        ast_add_child($1, $3);
        $$ = $1;
    }
;


Resource:
    VariableModifiers LocalVariableType TYPE_IDENTIFIER '=' Expression {
        AstNode *var = make_parameter_node($2, $3, @$);
        AstNode *assign = AST_BRANCH_AT(AST_ASSIGN, @4, 0);
        ast_add_child(assign, var);
        ast_add_child(assign, $5);

        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, assign);
    }
  | Annotations LocalVariableType TYPE_IDENTIFIER '=' Expression {
        AstNode *var = make_parameter_node($2, $3, @$);
        AstNode *assign = AST_BRANCH_AT(AST_ASSIGN, @4, 0);
        ast_add_child(assign, var);
        ast_add_child(assign, $5);

        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, assign);
    }
  | LocalVariableType TYPE_IDENTIFIER '=' Expression {
        AstNode *var = make_parameter_node($1, $2, @$);
        AstNode *assign = AST_BRANCH_AT(AST_ASSIGN, @3, 0);
        ast_add_child(assign, var);
        ast_add_child(assign, $4);

        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, assign);
    }
  | VariableModifiers LocalVariableType IdentifierComplement '=' Expression {
        AstNode *var = make_parameter_node($2, $3, @$);
        AstNode *assign = AST_BRANCH_AT(AST_ASSIGN, @4, 0);
        ast_add_child(assign, var);
        ast_add_child(assign, $5);

        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, assign);
    }
  | Annotations LocalVariableType IdentifierComplement '=' Expression {
        AstNode *var = make_parameter_node($2, $3, @$);
        AstNode *assign = AST_BRANCH_AT(AST_ASSIGN, @4, 0);
        ast_add_child(assign, var);
        ast_add_child(assign, $5);

        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, assign);
    }
  | LocalVariableType IdentifierComplement '=' Expression {
        AstNode *var = make_parameter_node($1, $2, @$);
        AstNode *assign = AST_BRANCH_AT(AST_ASSIGN, @3, 0);
        ast_add_child(assign, var);
        ast_add_child(assign, $4);

        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, assign);
    }
  | VariableAccess {
        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, $1);
    }
;


VariableAccess:
    CommonName { $$ = $1; }
    | FieldAccess { $$ = $1; }
;

Pattern:
    TypePattern { $$ = $1; }
//    | RecordPattern
;

/*
`TypePattern`是一个局部变量声明，但它不允许包含多个变量声明（即不允许使用逗号分隔的变量声明列表）
*/
TypePattern:
    LocalVariableDeclaration_TypePattern { $$ = $1; }
;

LocalVariableDeclaration_TypePattern:
    VariableModifiers LocalVariableType VariableDeclarator {
        $$ = make_local_variable_node($2, $3, @2);
    }
    | Annotations LocalVariableType VariableDeclarator {
        $$ = make_local_variable_node($2, $3, @2);
    }
    | LocalVariableType VariableDeclarator {
        $$ = make_local_variable_node($1, $2, @1);
    }
;



//------------------------------------Expressions-----------------------------------



Primary:
    PrimaryNoNewArray { $$ = $1; }
    | ArrayCreationExpression { $$ = $1; }
;

PrimaryNoNewArray:
    Literal { $$ = $1; }
    | ClassLiteral { $$ = $1; }
    | THIS {
        $$ = make_keyword_leaf("this", @1);
    }
    | CommonName '.' THIS {
        AstNode *this_node = make_keyword_leaf("this", @3);
        $$ = make_field_access_node($1, this_node, @2);
    }
//    | TypeName '.' THIS
    | '(' Expression ')' { $$ = $2; }
    | ClassInstanceCreationExpression { $$ = $1; }
    | FieldAccess { $$ = $1; }
    | ArrayAccess { $$ = $1; }
    | MethodInvocation { $$ = $1; }
    | MethodReference { $$ = $1; }
;

ClassLiteral:
//    TypeName DimsNoAnnotations '.' CLASS
    CommonName DimsNoAnnotations '.' CLASS {
        attach_dims($1, $2);
        $$ = make_class_literal_node($1, @4);
    }
    | NumericType DimsNoAnnotations '.' CLASS {
        attach_dims($1, $2);
        $$ = make_class_literal_node($1, @4);
    }
    | BOOLEAN DimsNoAnnotations '.' CLASS {
        AstNode *bool_node = make_keyword_leaf("boolean", @1);
        attach_dims(bool_node, $2);
        $$ = make_class_literal_node(bool_node, @4);
    }
//    | UnannPrimitiveType DimsNoAnnotations '.' CLASS
//    | TypeName '.' CLASS
    | CommonName '.' CLASS {
        $$ = make_class_literal_node($1, @3);
    }
    | NumericType '.' CLASS {
        $$ = make_class_literal_node($1, @3);
    }
    | BOOLEAN '.' CLASS {
        AstNode *bool_node = make_keyword_leaf("boolean", @1);
        $$ = make_class_literal_node(bool_node, @3);
    }
//    | UnannPrimitiveType '.' CLASS
    | VOID '.' CLASS {
        AstNode *void_node = make_keyword_leaf("void", @1);
        $$ = make_class_literal_node(void_node, @3);
    }
;

// 专门为了给 ClassLiteral 引进一个或者多个 '[]'
DimsNoAnnotations:
    LBRACK ']' {
        AstNode *list = make_list_node(AST_DIM_LIST, @$);
        ast_add_child(list, make_dim_node(@1));
        $$ = list;
    }
    | DimsNoAnnotations LBRACK ']' {
        ast_add_child($1, make_dim_node(@2));
        $$ = $1;
    }
;

ClassInstanceCreationExpression:
    UnqualifiedClassInstanceCreationExpression { $$ = $1; }
    | CommonName '.' UnqualifiedClassInstanceCreationExpression {
        ast_prepend_child($3, $1);
        $$ = $3;
    }
    | Primary '.' UnqualifiedClassInstanceCreationExpression {
        ast_prepend_child($3, $1);
        $$ = $3;
    }
;

UnqualifiedClassInstanceCreationExpression:
    NEW TypeArguments ClassOrInterfaceTypeToInstantiate '(' ArgumentList ')' ClassBody {
        AstNode *type = $3;
        if ($2) {
            ast_add_child(type, $2);
        }
        $$ = make_new_class_core(type, $5, $7, @1);
    }
    | NEW TypeArguments ClassOrInterfaceTypeToInstantiate '(' ArgumentList ')' {
        AstNode *type = $3;
        if ($2) {
            ast_add_child(type, $2);
        }
        $$ = make_new_class_core(type, $5, NULL, @1);
    }
    | NEW TypeArguments ClassOrInterfaceTypeToInstantiate '(' ')' ClassBody {
        AstNode *type = $3;
        if ($2) {
            ast_add_child(type, $2);
        }
        $$ = make_new_class_core(type, make_list_node(AST_ARGUMENT_LIST, @$), $6, @1);
    }
    | NEW TypeArguments ClassOrInterfaceTypeToInstantiate '(' ')' {
        AstNode *type = $3;
        if ($2) {
            ast_add_child(type, $2);
        }
        $$ = make_new_class_core(type, make_list_node(AST_ARGUMENT_LIST, @$), NULL, @1);
    }
    | NEW ClassOrInterfaceTypeToInstantiate '(' ArgumentList ')' ClassBody {
        $$ = make_new_class_core($2, $4, $6, @1);
    }
    | NEW ClassOrInterfaceTypeToInstantiate '(' ArgumentList ')' {
        $$ = make_new_class_core($2, $4, NULL, @1);
    }
    | NEW ClassOrInterfaceTypeToInstantiate '(' ')' ClassBody {
        $$ = make_new_class_core($2, make_list_node(AST_ARGUMENT_LIST, @$), $5, @1);
    }
    | NEW ClassOrInterfaceTypeToInstantiate '(' ')' {
        $$ = make_new_class_core($2, make_list_node(AST_ARGUMENT_LIST, @$), NULL, @1);
    }
;

ClassOrInterfaceTypeToInstantiate:
    AnnotationIdentifiers TypeArgumentsOrDiamond {
        if ($2) {
            ast_add_child($1, $2);
        }
        $$ = $1;
    }
    | AnnotationIdentifiers {
        $$ = $1;
    }
;

// 专门为了在 ClassOrInterfaceTypeToInstantiate 中引入一个或者多个 Annotations Identifier ，用'.'隔开
AnnotationIdentifiers:
    Annotations IDENTIFIER_AnnotationIdentifiers {
        ast_prepend_child($2, $1);
        $$ = $2;
    }
    | IDENTIFIER_AnnotationIdentifiers { $$ = $1; }
    | AnnotationIdentifiers '.' Annotations IDENTIFIER_AnnotationIdentifiers {
        ast_prepend_child($4, $3);
        $$ = make_field_access_node($1, $4, @2);
    }
    | AnnotationIdentifiers '.' IDENTIFIER_AnnotationIdentifiers {
        $$ = make_field_access_node($1, $3, @2);
    }
//    | AnnotationIdentifiers DOT_CommonName IDENTIFIER_AnnotationIdentifiers
;

TypeArgumentsOrDiamond:
    TypeArguments {
        $$ = $1;
    }  //显式指定类型参数
    | DIAMOND {
        $$ = make_keyword_leaf("<>", @1);
    }      //钻石操作符
;

FieldAccess:
    Primary '.' TYPE_IDENTIFIER {
        $$ = make_field_access_node($1, $3, @2);
    }
    | SUPER '.' TYPE_IDENTIFIER {
        $$ = make_field_access_node(make_keyword_leaf("super", @1), $3, @2);
    }
//    | TypeName '.' SUPER '.' TYPE_IDENTIFIER
    | CommonName '.' SUPER '.' TYPE_IDENTIFIER {
        AstNode *super_node = make_field_access_node($1, make_keyword_leaf("super", @3), @2);
        $$ = make_field_access_node(super_node, $5, @4);
    }
    | Primary '.' IdentifierComplement {
        $$ = make_field_access_node($1, $3, @2);
    }
    | SUPER '.' IdentifierComplement {
        $$ = make_field_access_node(make_keyword_leaf("super", @1), $3, @2);
    }
//    | TypeName '.' SUPER '.' IdentifierComplement
    | CommonName '.' SUPER '.' IdentifierComplement {
        AstNode *super_node = make_field_access_node($1, make_keyword_leaf("super", @3), @2);
        $$ = make_field_access_node(super_node, $5, @4);
    }
;

ArrayAccess:
    CommonName LBRACK_ArrayAccess Expression ']' {
        $$ = make_array_access_node($1, $3, @2);
    }
//    CommonName '[' Expression ']'
    | PrimaryNoNewArray LBRACK_ArrayAccess Expression ']' {
        $$ = make_array_access_node($1, $3, @2);
    }
    | NEW PrimitiveType Dims ArrayInitializer LBRACK_ArrayAccess Expression ']' {
        AstNode *type = $2;
        attach_dims(type, $3);
        AstNode *base = make_array_creation_node(type, NULL, NULL, $4, @1);
        $$ = make_array_access_node(base, $6, @5);
    }
    | NEW UnannPrimitiveType Dims ArrayInitializer LBRACK_ArrayAccess Expression ']' {
        AstNode *type = $2;
        attach_dims(type, $3);
        AstNode *base = make_array_creation_node(type, NULL, NULL, $4, @1);
        $$ = make_array_access_node(base, $6, @5);
    }
    | NEW ClassOrInterfaceType Dims ArrayInitializer LBRACK_ArrayAccess Expression ']' {
        AstNode *type = $2;
        attach_dims(type, $3);
        AstNode *base = make_array_creation_node(type, NULL, NULL, $4, @1);
        $$ = make_array_access_node(base, $6, @5);
    }
;

/*
MethodInvocation:
    MethodName '(' ArgumentList ')'
    | MethodName '(' ')'
    | TypeName '.' TypeArguments IDENTIFIER '(' ArgumentList ')'
    | TypeName '.' TypeArguments IDENTIFIER '(' ')'
    | TypeName '.' IDENTIFIER '(' ArgumentList ')'
    | TypeName '.' IDENTIFIER '(' ')'
    | CommonName '.' TypeArguments IDENTIFIER '(' ArgumentList ')'
    | CommonName '.' TypeArguments IDENTIFIER '(' ')'
    | CommonName '.' IDENTIFIER_MethodInvocation '(' ArgumentList ')'
    | CommonName '.' IDENTIFIER_MethodInvocation '(' ')'
    | Primary '.' TypeArguments IDENTIFIER '(' ArgumentList ')'
    | Primary '.' TypeArguments IDENTIFIER '(' ')'
    | Primary '.' IDENTIFIER_MethodInvocation '(' ArgumentList ')'
    | Primary '.' IDENTIFIER_MethodInvocation '(' ')'
    | SUPER '.' TypeArguments IDENTIFIER '(' ArgumentList ')'
    | SUPER '.' TypeArguments IDENTIFIER '(' ')'
    | SUPER '.' IDENTIFIER_MethodInvocation '(' ArgumentList ')'
    | SUPER '.' IDENTIFIER_MethodInvocation '(' ')'
    | TypeName '.' SUPER '.' TypeArguments IDENTIFIER '(' ArgumentList ')'
    | TypeName '.' SUPER '.' TypeArguments IDENTIFIER '(' ')'
    | TypeName '.' SUPER '.' IDENTIFIER_MethodInvocation '(' ArgumentList ')'
    | TypeName '.' SUPER '.' IDENTIFIER_MethodInvocation '(' ')'
;
*/
// 这里用 CommonName 替换 TypeName 实际上扩宽了语义
MethodInvocation:
    // MethodName '(' ArgumentList ')'
    // | MethodName '(' ')'
    IDENTIFIER_MethodDeclarator '(' ArgumentList ')' {
        $$ = make_method_invocation_node(NULL, NULL, $1, $3, @1);
    }
    | IDENTIFIER_MethodDeclarator '(' ')' {
        $$ = make_method_invocation_node(NULL, NULL, $1, NULL, @1);
    }
//    | TypeName '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ArgumentList ')'
//    | TypeName '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ')'
//    | TypeName '.' IDENTIFIER_MethodDeclarator '(' ArgumentList ')'
//    | TypeName '.' IDENTIFIER_MethodDeclarator '(' ')'
    | CommonName '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ArgumentList ')' {
        $$ = make_method_invocation_node($1, $3, $4, $6, @2);
    }
    | CommonName '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ')' {
        $$ = make_method_invocation_node($1, $3, $4, NULL, @2);
    }
    | CommonName '.' IDENTIFIER_MethodDeclarator '(' ArgumentList ')' {
        $$ = make_method_invocation_node($1, NULL, $3, $5, @2);
    }
    | CommonName '.' IDENTIFIER_MethodDeclarator '(' ')' {
        $$ = make_method_invocation_node($1, NULL, $3, NULL, @2);
    }
    | Primary '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ArgumentList ')' {
        $$ = make_method_invocation_node($1, $3, $4, $6, @2);
    }
    | Primary '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ')' {
        $$ = make_method_invocation_node($1, $3, $4, NULL, @2);
    }
    | Primary '.' IDENTIFIER_MethodDeclarator '(' ArgumentList ')' {
        $$ = make_method_invocation_node($1, NULL, $3, $5, @2);
    }
    | Primary '.' IDENTIFIER_MethodDeclarator '(' ')' {
        $$ = make_method_invocation_node($1, NULL, $3, NULL, @2);
    }
    | SUPER '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ArgumentList ')' {
        $$ = make_method_invocation_node(make_keyword_leaf("super", @1), $3, $4, $6, @2);
    }
    | SUPER '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ')' {
        $$ = make_method_invocation_node(make_keyword_leaf("super", @1), $3, $4, NULL, @2);
    }
    | SUPER '.' IDENTIFIER_MethodDeclarator '(' ArgumentList ')' {
        $$ = make_method_invocation_node(make_keyword_leaf("super", @1), NULL, $3, $5, @2);
    }
    | SUPER '.' IDENTIFIER_MethodDeclarator '(' ')' {
        $$ = make_method_invocation_node(make_keyword_leaf("super", @1), NULL, $3, NULL, @2);
    }
//    | TypeName '.' SUPER '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ArgumentList ')'
//    | TypeName '.' SUPER '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ')'
//    | TypeName '.' SUPER '.' IDENTIFIER_MethodDeclarator '(' ArgumentList ')'
//    | TypeName '.' SUPER '.' IDENTIFIER_MethodDeclarator '(' ')'
    | CommonName '.' SUPER '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ArgumentList ')' {
        AstNode *super_node = make_field_access_node($1, make_keyword_leaf("super", @3), @2);
        $$ = make_method_invocation_node(super_node, $5, $6, $8, @4);
    }
    | CommonName '.' SUPER '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ')' {
        AstNode *super_node = make_field_access_node($1, make_keyword_leaf("super", @3), @2);
        $$ = make_method_invocation_node(super_node, $5, $6, NULL, @4);
    }
    | CommonName '.' SUPER '.' IDENTIFIER_MethodDeclarator '(' ArgumentList ')' {
        AstNode *super_node = make_field_access_node($1, make_keyword_leaf("super", @3), @2);
        $$ = make_method_invocation_node(super_node, NULL, $5, $7, @4);
    }
    | CommonName '.' SUPER '.' IDENTIFIER_MethodDeclarator '(' ')' {
        AstNode *super_node = make_field_access_node($1, make_keyword_leaf("super", @3), @2);
        $$ = make_method_invocation_node(super_node, NULL, $5, NULL, @4);
    }
;

ArgumentList:
    Expression  {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        if ($1) {
            ast_add_child($$, $1);
        }
    }
    | ArgumentList ',' Expression {
        if ($3) {
            ast_add_child($1, $3);
        }
        $$ = $1;
    }
;

MethodReference:
//    CommonName DOUBLE_COLON TypeArguments TYPE_IDENTIFIER
//    | CommonName DOUBLE_COLON TYPE_IDENTIFIER
    Primary DOUBLE_COLON TypeArguments TYPE_IDENTIFIER {
        $$ = make_method_reference_node($1, $3, $4, @2);
    }
    | Primary DOUBLE_COLON TYPE_IDENTIFIER {
        $$ = make_method_reference_node($1, NULL, $3, @2);
    }
    | ReferenceType DOUBLE_COLON TypeArguments TYPE_IDENTIFIER {
        $$ = make_method_reference_node($1, $3, $4, @2);
    }
    | ReferenceType DOUBLE_COLON TYPE_IDENTIFIER {
        $$ = make_method_reference_node($1, NULL, $3, @2);
    }
    | SUPER DOUBLE_COLON TypeArguments TYPE_IDENTIFIER {
        $$ = make_method_reference_node(make_keyword_leaf("super", @1), $3, $4, @2);
    }
    | SUPER DOUBLE_COLON TYPE_IDENTIFIER {
        $$ = make_method_reference_node(make_keyword_leaf("super", @1), NULL, $3, @2);
    }
//    | TypeName '.' SUPER DOUBLE_COLON TypeArguments TYPE_IDENTIFIER
//    | TypeName '.' SUPER DOUBLE_COLON TYPE_IDENTIFIER
    | CommonName '.' SUPER DOUBLE_COLON TypeArguments TYPE_IDENTIFIER {
        AstNode *super_node = make_field_access_node($1, make_keyword_leaf("super", @3), @2);
        $$ = make_method_reference_node(super_node, $5, $6, @4);
    }
    | CommonName '.' SUPER DOUBLE_COLON TYPE_IDENTIFIER {
        AstNode *super_node = make_field_access_node($1, make_keyword_leaf("super", @3), @2);
        $$ = make_method_reference_node(super_node, NULL, $5, @4);
    }
//    | CommonName DOUBLE_COLON TypeArguments IdentifierComplement
//    | CommonName DOUBLE_COLON IdentifierComplement
    | Primary DOUBLE_COLON TypeArguments IdentifierComplement {
        $$ = make_method_reference_node($1, $3, $4, @2);
    }
    | Primary DOUBLE_COLON IdentifierComplement {
        $$ = make_method_reference_node($1, NULL, $3, @2);
    }
    | ReferenceType DOUBLE_COLON TypeArguments IdentifierComplement {
        $$ = make_method_reference_node($1, $3, $4, @2);
    }
    | ReferenceType DOUBLE_COLON IdentifierComplement {
        $$ = make_method_reference_node($1, NULL, $3, @2);
    }
    | SUPER DOUBLE_COLON TypeArguments IdentifierComplement {
        $$ = make_method_reference_node(make_keyword_leaf("super", @1), $3, $4, @2);
    }
    | SUPER DOUBLE_COLON IdentifierComplement {
        $$ = make_method_reference_node(make_keyword_leaf("super", @1), NULL, $3, @2);
    }
//    | TypeName '.' SUPER DOUBLE_COLON TypeArguments IdentifierComplement
//    | TypeName '.' SUPER DOUBLE_COLON IdentifierComplement
    | CommonName '.' SUPER DOUBLE_COLON TypeArguments IdentifierComplement {
        AstNode *super_node = make_field_access_node($1, make_keyword_leaf("super", @3), @2);
        $$ = make_method_reference_node(super_node, $5, $6, @4);
    }
    | CommonName '.' SUPER DOUBLE_COLON IdentifierComplement {
        AstNode *super_node = make_field_access_node($1, make_keyword_leaf("super", @3), @2);
        $$ = make_method_reference_node(super_node, NULL, $5, @4);
    }
//    | ClassOrInterfaceType DOUBLE_COLON_MethodReference_COI TypeArguments NEW
//    | ClassOrInterfaceType DOUBLE_COLON_MethodReference_COI NEW
//    | ArrayType DOUBLE_COLON_MethodReference_COI NEW
// 以下注释掉的部分是一个扩宽语义的部分，如果上面的审查方法不好使可以考虑注释掉从本行起上数三行（不含本行），并采用如下的扩宽语义法（经检测效果一致，应该不会导致新的冲突）(已采用此备案)
    | ReferenceType DOUBLE_COLON TypeArguments NEW {
        $$ = make_method_reference_node($1, $3, make_keyword_leaf("new", @4), @2);
    }
    | ReferenceType DOUBLE_COLON NEW {
        $$ = make_method_reference_node($1, NULL, make_keyword_leaf("new", @3), @2);
    }
;

ArrayCreationExpression:
    NEW PrimitiveType DimExprs Dims {
        attach_dims($2, $4);
        $$ = make_array_creation_node($2, $3, NULL, NULL, @1);
    }
    | NEW PrimitiveType DimExprs {
        $$ = make_array_creation_node($2, $3, NULL, NULL, @1);
    }
    | NEW UnannPrimitiveType DimExprs Dims {
        attach_dims($2, $4);
        $$ = make_array_creation_node($2, $3, NULL, NULL, @1);
    }
    | NEW UnannPrimitiveType DimExprs {
        $$ = make_array_creation_node($2, $3, NULL, NULL, @1);
    }
    | NEW ClassOrInterfaceType DimExprs Dims {
        attach_dims($2, $4);
        $$ = make_array_creation_node($2, $3, NULL, NULL, @1);
    }
    | NEW ClassOrInterfaceType DimExprs {
        $$ = make_array_creation_node($2, $3, NULL, NULL, @1);
    }
    | NEW PrimitiveType Dims ArrayInitializer {
        attach_dims($2, $3);
        $$ = make_array_creation_node($2, NULL, NULL, $4, @1);
    }
    | NEW UnannPrimitiveType Dims ArrayInitializer {
        attach_dims($2, $3);
        $$ = make_array_creation_node($2, NULL, NULL, $4, @1);
    }
    | NEW ClassOrInterfaceType Dims ArrayInitializer {
        attach_dims($2, $3);
        $$ = make_array_creation_node($2, NULL, NULL, $4, @1);
    }
;

//允许使用一个或者多个DimExpr
DimExprs:
    DimExpr  {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | DimExprs DimExpr {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

DimExpr:
    Annotations_Dims LBRACK_ArrayAccess Expression ']' {
        AstNode *dim = make_dim_node(@2);
        if ($1) {
            ast_add_child(dim, $1);
        }
        ast_add_child(dim, $3);
        $$ = dim;
    }
    | LBRACK_ArrayAccess Expression ']' {
        AstNode *dim = make_dim_node(@1);
        ast_add_child(dim, $2);
        $$ = dim;
    }
;

Expression:
    LambdaExpression { $$ = $1; }
    | AssignmentExpression { $$ = $1; }
//    ExpressionWithoutTernary
//    | ConditionalExpression
;

LambdaExpression:
    LambdaParameters ARROW LambdaBody {
        $$ = make_lambda_node($1, $3, @2);
    }
;

// 这里有点小问题哈，就是我之前这里有冲突我才把'('')'替换为 LPAREN_LambdaParameters 和 RPAREN_LambdaParameters 的，但现在调试的时候替换回'('')'突然就没冲突了，我也很奇怪哈，跟那个正负号一样，待研究
LambdaParameters:
    '(' LambdaParameterList ')' {
        $$ = $2;
    }   // RPAREN_LambdaParameters
    | '(' ')' {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
    }                     // RPAREN_LambdaParameters
    | TYPE_IDENTIFIER_IdentifierforLambdaParameterList { $$ = $1; }
    | IdentifierComplement_IdentifierforLambdaParameterList { $$ = $1; }
//    LPAREN_LambdaParameters LambdaParameterList ')' // RPAREN_LambdaParameters
//    | LPAREN_LambdaParameters ')' // RPAREN_LambdaParameters
//    | IDENTIFIER_LambdaParameters
;

LambdaParameterList:
    LambdaParameterforLambdaParameterList { $$ = $1; }
    | IdentifierforLambdaParameterList { $$ = $1; }
;

IdentifierforLambdaParameterList:
    TYPE_IDENTIFIER_IdentifierforLambdaParameterList {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | IdentifierComplement_IdentifierforLambdaParameterList {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | IdentifierforLambdaParameterList ',' TYPE_IDENTIFIER_IdentifierforLambdaParameterList {
        ast_add_child($1, $3);
        $$ = $1;
    }
    | IdentifierforLambdaParameterList ',' IdentifierComplement_IdentifierforLambdaParameterList {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

IdentifierComplement_IdentifierforLambdaParameterList:
    VAR_IdentifierComplement_IdentifierforLambdaParameterList { $$ = $1; }
    | YIELD_IdentifierComplement_IdentifierforLambdaParameterList { $$ = $1; }
;

LambdaParameterforLambdaParameterList:
    LambdaParameter {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | LambdaParameterforLambdaParameterList ',' LambdaParameter {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

LambdaParameter:
    VariableModifiers LambdaParameterType TYPE_IDENTIFIER Dims {
        $$ = make_parameter_with_dims($2, $3, $4, @$);
    }
    | Annotations LambdaParameterType TYPE_IDENTIFIER Dims {
        $$ = make_parameter_with_dims($2, $3, $4, @$);
    }
    | LambdaParameterType TYPE_IDENTIFIER Dims {
        $$ = make_parameter_with_dims($1, $2, $3, @$);
    }
    | VariableModifiers LambdaParameterType TYPE_IDENTIFIER {
        $$ = make_parameter_with_dims($2, $3, NULL, @$);
    }
    | Annotations LambdaParameterType TYPE_IDENTIFIER {
        $$ = make_parameter_with_dims($2, $3, NULL, @$);
    }
    | LambdaParameterType TYPE_IDENTIFIER {
        $$ = make_parameter_with_dims($1, $2, NULL, @$);
    }
    | VariableModifiers LambdaParameterType IdentifierComplement Dims {
        $$ = make_parameter_with_dims($2, $3, $4, @$);
    }
    | Annotations LambdaParameterType IdentifierComplement Dims {
        $$ = make_parameter_with_dims($2, $3, $4, @$);
    }
    | LambdaParameterType IdentifierComplement Dims {
        $$ = make_parameter_with_dims($1, $2, $3, @$);
    }
    | VariableModifiers LambdaParameterType IdentifierComplement {
        $$ = make_parameter_with_dims($2, $3, NULL, @$);
    }
    | Annotations LambdaParameterType IdentifierComplement {
        $$ = make_parameter_with_dims($2, $3, NULL, @$);
    }
    | LambdaParameterType IdentifierComplement {
        $$ = make_parameter_with_dims($1, $2, NULL, @$);
    }
    | VariableArityParameter { $$ = $1; }
;

LambdaParameterType:
    UnannType { $$ = $1; }
    | VAR { $$ = make_keyword_leaf("var", @1); }
;

LambdaBody:
    Expression { $$ = $1; }
    | Block { $$ = $1; }
;

AssignmentExpression:
    ConditionalExpression { $$ = $1; }
    | Assignment { $$ = $1; }
;

Assignment:
    LeftHandSide AssignmentOperator Expression {
        $$ = make_assignment_node($1, $2, $3, @2);
    }
;

LeftHandSide:
    CommonName { $$ = $1; }
    | FieldAccess { $$ = $1; }
    | ArrayAccess { $$ = $1; }
;

AssignmentOperator:
    '='  { $$ = make_keyword_leaf("=", @1); }
    | MUL_ASSIGN  { $$ = make_keyword_leaf("*=", @1); }
    | DIV_ASSIGN  { $$ = make_keyword_leaf("/=", @1); }
    | MOD_ASSIGN { $$ = make_keyword_leaf("%=", @1); }
    | ADD_ASSIGN  { $$ = make_keyword_leaf("+=", @1); }
    | SUB_ASSIGN  { $$ = make_keyword_leaf("-=", @1); }
    | SHL_ASSIGN { $$ = make_keyword_leaf("<<=", @1); }
    | SHR_ASSIGN  { $$ = make_keyword_leaf(">>=", @1); }
    | USHR_ASSIGN  { $$ = make_keyword_leaf(">>>=", @1); }
    | AND_ASSIGN  { $$ = make_keyword_leaf("&=", @1); }
    | XOR_ASSIGN  { $$ = make_keyword_leaf("^=", @1); }
    | OR_ASSIGN  { $$ = make_keyword_leaf("|=", @1); }
;

ConditionalExpression:
    ConditionalOrExpression  %prec PREC_ConditionalExpression_1 {
        $$ = $1;
    }
    | ConditionalOrExpression '?' Expression ':' ConditionalExpression %prec PREC_ConditionalExpression {
        $$ = make_conditional_expr($1, $3, $5, @2);
    }
    | ConditionalOrExpression '?' Expression ':' LambdaExpression %prec PREC_ConditionalExpression {
        $$ = make_conditional_expr($1, $3, $5, @2);
    }
;

ConditionalOrExpression:
    ConditionalAndExpression %prec PREC_ConditionalOrExpression_1 {
        $$ = $1;
    }
    | ConditionalOrExpression OR ConditionalAndExpression   // %prec PREC_ConditionalOrExpression
    {
        $$ = make_binary_expr($1, "||", $3, @2);
    }
;

ConditionalAndExpression:
    InclusiveOrExpression %prec PREC_ConditionalAndExpression_1 {
        $$ = $1;
    }
    | ConditionalAndExpression AND InclusiveOrExpression %prec PREC_ConditionalAndExpression {
        $$ = make_binary_expr($1, "&&", $3, @2);
    }
;

InclusiveOrExpression:
    ExclusiveOrExpression %prec PREC_InclusiveOrExpression_1 {
        $$ = $1;
    }
    | InclusiveOrExpression '|' ExclusiveOrExpression %prec PREC_InclusiveOrExpression {
        $$ = make_binary_expr($1, "|", $3, @2);
    }
;

ExclusiveOrExpression:
    AndExpression %prec PREC_ExclusiveOrExpression_1 {
        $$ = $1;
    }
    | ExclusiveOrExpression '^' AndExpression %prec PREC_ExclusiveOrExpression {
        $$ = make_binary_expr($1, "^", $3, @2);
    }
;

AndExpression:
    EqualityExpression %prec PREC_AndExpression_1 {
        $$ = $1;
    }
    | AndExpression '&' EqualityExpression %prec PREC_AndExpression {
        $$ = make_binary_expr($1, "&", $3, @2);
    }
;

EqualityExpression:
    RelationalExpression { $$ = $1; }
    | EqualityExpression EQ RelationalExpression %prec PREC_EqualityExpression {
        $$ = make_binary_expr($1, "==", $3, @2);
    }
    | EqualityExpression NE RelationalExpression %prec PREC_EqualityExpression {
        $$ = make_binary_expr($1, "!=", $3, @2);
    }
;

// 基础部分（不可递归扩展）
SimpleRelationalExpression:
    ShiftExpression %prec PREC_SimpleRelationalExpression {
        $$ = $1;
    }
;

// 允许 `instanceof`，但不允许递归扩展
RelationalExpression:
    SimpleRelationalExpression %prec PREC_RelationalExpression_1 {
        $$ = $1;
    }
    | SimpleRelationalExpression INSTANCEOF ReferenceType %prec PREC_RelationalExpression {
        $$ = make_binary_expr($1, "instanceof", $3, @2);
    } // 仅允许单层 `instanceof`，下同
    | SimpleRelationalExpression INSTANCEOF Pattern %prec PREC_RelationalExpression {
        $$ = make_binary_expr($1, "instanceof", $3, @2);
    } // 仅允许单层 `instanceof`，下同
    | SimpleRelationalExpression '<' ShiftExpression %prec PREC_RelationalExpression {
        $$ = make_binary_expr($1, "<", $3, @2);
    }
    | SimpleRelationalExpression '>' ShiftExpression %prec PREC_RelationalExpression {
        $$ = make_binary_expr($1, ">", $3, @2);
    }
    | SimpleRelationalExpression LE ShiftExpression %prec PREC_RelationalExpression {
        $$ = make_binary_expr($1, "<=", $3, @2);
    }
    | SimpleRelationalExpression GE ShiftExpression %prec PREC_RelationalExpression {
        $$ = make_binary_expr($1, ">=", $3, @2);
    }
;

ShiftExpression:
    AdditiveExpression %prec PREC_ShiftExpression_1 {
        $$ = $1;
    }
    | ShiftExpression SHL_OP AdditiveExpression  %prec PREC_ShiftExpression {
        $$ = make_binary_expr($1, "<<", $3, @2);
    }
    | ShiftExpression SHR_OP AdditiveExpression  %prec PREC_ShiftExpression {
        $$ = make_binary_expr($1, ">>", $3, @2);
    }
    | ShiftExpression USHR_OP AdditiveExpression %prec PREC_ShiftExpression {
        $$ = make_binary_expr($1, ">>>", $3, @2);
    }
;

AdditiveExpression:
    MultiplicativeExpression  %prec PREC_AdditiveExpression_1 {
        $$ = $1;
    }
    | AdditiveExpression '+' MultiplicativeExpression  %prec PREC_AdditiveExpression {
        $$ = make_binary_expr($1, "+", $3, @2);
    }
    | AdditiveExpression '-' MultiplicativeExpression  %prec PREC_AdditiveExpression {
        $$ = make_binary_expr($1, "-", $3, @2);
    }
;

MultiplicativeExpression:
    UnaryExpression { $$ = $1; }
    | MultiplicativeExpression '*' UnaryExpression  %prec PREC_MultiplicativeExpression {
        $$ = make_binary_expr($1, "*", $3, @2);
    }
    | MultiplicativeExpression '/' UnaryExpression  %prec PREC_MultiplicativeExpression {
        $$ = make_binary_expr($1, "/", $3, @2);
    }
    | MultiplicativeExpression '%' UnaryExpression  %prec PREC_MultiplicativeExpression {
        $$ = make_binary_expr($1, "%", $3, @2);
    }
;

UnaryExpression:
    PreIncrementExpression { $$ = $1; }
    | PreDecrementExpression { $$ = $1; }
    | '+' UnaryExpression {
        $$ = make_unary_expr("+", $2, @1, false);
    } //这里正负号之前我记得有冲突，我就给它换成UPLUS和UMINUS了，后来为了测试调整回来，发现没冲突了，不知道为什么？
    | '-' UnaryExpression {
        $$ = make_unary_expr("-", $2, @1, false);
    }
//    | UPLUS UnaryExpression
//    | UMINUS UnaryExpression
    | UnaryExpressionNotPlusMinus { $$ = $1; }
;

PreIncrementExpression:
    PREFIX_INC UnaryExpression %prec PREFIX_INC {
        $$ = make_unary_expr("++", $2, @1, false);
    }
;

PreDecrementExpression:
    PREFIX_DEC UnaryExpression %prec PREFIX_DEC {
        $$ = make_unary_expr("--", $2, @1, false);
    }
;

UnaryExpressionNotPlusMinus:
    PostfixExpression { $$ = $1; }
    | '~' UnaryExpression {
        $$ = make_unary_expr("~", $2, @1, false);
    }
    | '!' UnaryExpression {
        $$ = make_unary_expr("!", $2, @1, false);
    }
    | CastExpression { $$ = $1; }
    | SwitchExpression { $$ = $1; }
;

PostfixExpression:
    Primary { $$ = $1; }
    | CommonName      // 原 ExpressionName
    {
        $$ = $1;
    }
    | PostIncrementExpression { $$ = $1; }
    | PostDecrementExpression { $$ = $1; }
;

PostIncrementExpression:
    PostfixExpression INC_OP %prec INC_OP {
        $$ = make_unary_expr("++", $1, @2, true);
    }
;

PostDecrementExpression:
    PostfixExpression DEC_OP %prec DEC_OP {
        $$ = make_unary_expr("--", $1, @2, true);
    }
;

CastExpression:
    '(' PrimitiveType ')' UnaryExpression {
        $$ = make_cast_expr($2, $4, @1);
    }
    | '(' UnannPrimitiveType ')' UnaryExpression {
        $$ = make_cast_expr($2, $4, @1);
    }
//    | '(' ReferenceType AdditionalBounds ')' UnaryExpressionNotPlusMinus
//    | '(' ReferenceType AdditionalBounds ')' LambdaExpression
    | '(' ReferenceType AdditionalBounds RPAREN_CastExpression UnaryExpressionNotPlusMinus {
        $$ = make_cast_expr($2, $5, @1);
    }
    | '(' ReferenceType AdditionalBounds RPAREN_CastExpression LambdaExpression {
        $$ = make_cast_expr($2, $5, @1);
    }
;

SwitchExpression:
    SWITCH '(' Expression ')' SwitchBlock {
        AstNode *node = make_unary_stmt(AST_SWITCH, $3, @1);
        if ($5) {
            ast_add_child(node, $5);
        }
        $$ = node;
    }
;


%%


// 修改 make_class_basic 以接收所有组件
static AstNode *make_class_basic(int line, int column, 
                               AstNode *modifiers, 
                               AstNode *name, 
                               AstNode *type_params,
                               AstNode *super_class,
                               AstNode *super_interfaces,
                               AstNode *permits,
                               AstNode *body) {
    AstNode *node = ast_branch(AST_CLASS_DECL, line, column, 0);
    
    // 1. 添加修饰符 (public, static...)
    if (modifiers) {
        ast_add_child(node, modifiers);
    }
    // 2. 添加类名
    if (name) {
        ast_add_child(node, name);
    }
    // 3. 添加泛型参数 (<T>)
    if (type_params) {
        ast_add_child(node, type_params);
    }
    // 4. 添加父类 (extends Base)
    if (super_class) {
        // 可以选择在这里加一个 keyword leaf "extends" 或者直接加节点
        ast_add_child(node, super_class);
    }
    // 5. 添加接口 (implements A, B)
    if (super_interfaces) {
        ast_add_child(node, super_interfaces);
    }
    // 6. 添加 permits (sealed class)
    if (permits) {
        ast_add_child(node, permits);
    }
    // 7. 添加类体 ({...})
    if (body) {
        ast_add_child(node, body);
    }
    return node;
}

static AstNode *wrap_labeled_block(const char *label, AstNode *block, YYLTYPE loc) {
    AstKind kind = AST_UNKNOWN;
    if (label) {
        if (strcmp(label, "static-init") == 0) {
            kind = AST_STATIC_INIT;
        } else if (strcmp(label, "instance-init") == 0) {
            kind = AST_INSTANCE_INIT;
        }
    }
    AstNode *n = AST_BRANCH_AT(kind, loc, 0);
    if (kind == AST_UNKNOWN && label) {
        ast_set_text(n, label);
    }
    if (block) ast_add_child(n, block);
    return n;
}



static AstNode *make_interface_basic(int line, int column, 
                                   AstNode *modifiers,
                                   AstNode *name, 
                                   AstNode *type_params,
                                   AstNode *extends_interfaces,
                                   AstNode *permits,
                                   AstNode *body) {
    AstNode *node = ast_branch(AST_INTERFACE_DECL, line, column, 0);
    
    if (modifiers) ast_add_child(node, modifiers);
    if (name) ast_add_child(node, name);
    if (type_params) ast_add_child(node, type_params);
    if (extends_interfaces) ast_add_child(node, extends_interfaces);
    if (permits) ast_add_child(node, permits);
    if (body) ast_add_child(node, body);
    
    return node;
}

static AstNode *make_annotation_node(AstNode *name, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_ANNOTATION, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (name) {
        ast_add_child(node, name);
    }
    return node;
}

static AstNode *make_type_node(AstNode *core, YYLTYPE loc) {
    if (core && core->kind == AST_TYPE) {
        return core;
    }
    AstNode *node = ast_branch(AST_TYPE, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (core) {
        ast_add_child(node, core);
    }
    return node;
}

static AstNode *make_type_parameter_node(AstNode *name, AstNode *bounds, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_TYPE_PARAMETER, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (name) {
        ast_add_child(node, name);
    }
    if (bounds) {
        ast_add_child(node, bounds);
    }
    return node;
}

static AstNode *make_type_argument_node(AstNode *value, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_TYPE_ARGUMENT, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (value) {
        ast_add_child(node, value);
    }
    return node;
}

static AstNode *make_dim_node(YYLTYPE loc) {
    return ast_branch(AST_DIM, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
}
