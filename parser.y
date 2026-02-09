
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>           // 添加这个头文件以支持 isspace、isalpha、isalnum
#include "java_ast.h"
#include "parser.tab.h"

int yylex(void);
void yyerror(const char *s);

AstNode *root_ast = NULL;
#define AST_LOC_LINE(loc) ((loc).first_line)
#define AST_LOC_COL(loc) ((loc).first_column)

#define AST_BRANCH_AT(kind, loc, child_count, ...) \
    ast_branch((kind), AST_LOC_LINE(loc), AST_LOC_COL(loc), (child_count), ##__VA_ARGS__)

#define AST_EMPTY_NODE(kind, loc) \
    ast_branch((kind), AST_LOC_LINE(loc), AST_LOC_COL(loc), 0)


/* 语法动作使用的 AST 构建辅助函数。 */
// 将子节点插入父节点子列表的最前面。
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

// 用关键字文本生成标识符叶子节点。
static AstNode *make_keyword_leaf(const char *text, YYLTYPE loc) {
    return ast_leaf(AST_IDENTIFIER, text, AST_LOC_LINE(loc), AST_LOC_COL(loc));
}

// 生成 this 表达式节点（可带限定名）。
static AstNode *make_this_expr_node(AstNode *qualifier, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_THIS_EXPR, loc, 0);
    if (qualifier) {
        ast_add_child(node, qualifier);
    }
    return node;
}

// 生成指定种类的空列表节点。
static AstNode *make_list_node(AstKind kind, YYLTYPE loc) {
    return ast_branch(kind, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
}

// 重新标记列表节点的类型。
static AstNode *relabel_list(AstNode *node, AstKind kind) {
    if (node) {
        node->kind = kind;
    }
    return node;
}

// 将注解列表包装为修饰符列表节点。
static AstNode *make_modifiers_from_annotations(AstNode *annotations, YYLTYPE loc) {
    if (!annotations) {
        return NULL;
    }
    AstNode *mods = make_list_node(AST_MODIFIER_LIST, loc);
    ast_add_child(mods, annotations);
    return mods;
}

// 构造 import 声明节点（static/按需）。
static AstNode *make_import_node(bool is_static, bool on_demand, AstNode *target, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_IMPORT_DECL, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (is_static) {
        AstNode *mods = make_list_node(AST_MODIFIER_LIST, loc);
        ast_add_child(mods, make_keyword_leaf("static", loc));
        ast_add_child(node, mods);
    }
    if (target) {
        ast_add_child(node, target);
    }
    if (on_demand) {
        ast_add_child(node, make_keyword_leaf("*", loc));
    }
    return node;
}

// 构造默认包声明节点。
// 构造 module 声明节点（可选 open 与指令）。
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

// 构造 module 指令节点。
static AstNode *make_module_directive_node(AstKind kind, YYLTYPE loc) {
    return AST_BRANCH_AT(kind, loc, 0);
}

// 构造数组初始化器节点。
static AstNode *make_array_initializer_node(AstNode *elements, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_ARRAY_INIT, loc, 0);
    if (elements) {
        ast_add_child(node, elements);
    }
    return node;
}

// 将维度节点挂到类型或变量节点上。
static void attach_dims(AstNode *node, AstNode *dims) {
    if (node && dims) {
        ast_add_child(node, dims);
    }
}

/* 前置声明：供前面的语法动作提前引用。 */
// 参数节点构造函数的前置声明。
static AstNode *make_parameter_node(AstNode *type_node, AstNode *name_node, YYLTYPE loc);
// 成员访问构造函数前置声明（供 receiver parameter 使用）。
static AstNode *make_field_access_node(AstNode *base, AstNode *member, YYLTYPE loc);
// 子句节点构造函数前置声明（供 for/init/cond/update 使用）。
static AstNode *make_clause_node(AstKind kind, AstNode *payload, int line, int column);

// 构造参数并附加维度。
static AstNode *make_parameter_with_dims(AstNode *type_node, AstNode *name_node, AstNode *dims, YYLTYPE loc) {
    AstNode *param = make_parameter_node(type_node, name_node, loc);
    if (dims) {
        ast_add_child(param, dims);
    }
    return param;
}

// 构造参数并前置修饰符。
static AstNode *make_parameter_with_mods(AstNode *mods, AstNode *type_node,
                                         AstNode *name_node, AstNode *dims, YYLTYPE loc) {
    AstNode *param = make_parameter_with_dims(type_node, name_node, dims, loc);
    if (mods) {
        ast_prepend_child(param, mods);
    }
    return param;
}

// 标记可变参数。
static AstNode *mark_varargs_parameter(AstNode *param) {
    if (param) {
        ast_set_text(param, "varargs");
    }
    return param;
}

// 构造 receiver parameter。
static AstNode *make_receiver_parameter_node(AstNode *mods, AstNode *type_node,
                                             AstNode *qualifier, YYLTYPE this_loc,
                                             YYLTYPE loc) {
    AstNode *name_node = make_keyword_leaf("this", this_loc);
    if (qualifier) {
        name_node = make_field_access_node(qualifier, name_node, this_loc);
    }
    AstNode *param = make_parameter_with_mods(mods, type_node, name_node, NULL, loc);
    ast_set_text(param, "receiver");
    return param;
}

// 合并修饰符与注解列表。
static AstNode *merge_param_modifiers(AstNode *mods, AstNode *annotations, YYLTYPE loc) {
    if (!annotations) {
        return mods;
    }
    if (!mods) {
        return make_modifiers_from_annotations(annotations, loc);
    }
    ast_add_child(mods, annotations);
    return mods;
}

// 构造语句块节点并附带语句列表。
static AstNode *make_block_node(YYLTYPE loc, AstNode *stmts) {
    AstNode *node = AST_BRANCH_AT(AST_BLOCK, loc, 0);
    if (stmts) {
        ast_add_child(node, stmts);
    }
    return node;
}

// 构造无子节点的语句节点。
static AstNode *make_simple_stmt(AstKind kind, YYLTYPE loc) {
    return AST_BRANCH_AT(kind, loc, 0);
}

// 构造一元语句节点。
static AstNode *make_unary_stmt(AstKind kind, AstNode *child, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(kind, loc, 0);
    if (child) {
        ast_add_child(node, child);
    }
    return node;
}

// 构造二元语句节点。
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

// 构造三元语句节点。
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

// 构造通配符节点。
static AstNode *make_wildcard_node(AstNode *annotations, AstNode *bound, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_WILDCARD, loc, 0);
    if (annotations) {
        ast_add_child(node, annotations);
    }
    if (bound) {
        ast_add_child(node, bound);
    }
    return node;
}

// 构造通配符边界节点（extends/super）。
static AstNode *make_wildcard_bound_node(const char *kind, AstNode *types, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_WILDCARD_BOUND, loc, 0);
    if (kind) {
        ast_set_text(node, kind);
    }
    if (types) {
        ast_add_child(node, types);
    }
    return node;
}

// 构造 else 子句节点。
static AstNode *make_else_clause_node(AstNode *child, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_ELSE_CLAUSE, loc, 0);
    if (child) {
        ast_add_child(node, child);
    }
    return node;
}

// 查找方法返回类型节点（AST_TYPE）。
static AstNode *find_method_return_type(AstNode *method) {
    if (!method) {
        return NULL;
    }
    for (size_t i = 0; i < method->child_count; ++i) {
        AstNode *child = method->children[i];
        if (child && child->kind == AST_TYPE) {
            return child;
        }
    }
    return NULL;
}

// 将修饰符末尾的注解移动到返回类型上（public @A int -> @A 归到 Type）。
static void move_trailing_type_annotations(AstNode *mods, AstNode *method) {
    if (!mods || !method || mods->kind != AST_MODIFIER_LIST) {
        return;
    }
    AstNode *result_type = find_method_return_type(method);
    if (!result_type) {
        return;
    }

    int last_non_annot = -1;
    for (size_t i = 0; i < mods->child_count; ++i) {
        AstNode *child = mods->children[i];
        if (child && child->kind != AST_ANNOTATION) {
            last_non_annot = (int)i;
        }
    }
    if (last_non_annot < 0) {
        return; // 只有注解，保持为方法修饰符
    }

    size_t start = (size_t)last_non_annot + 1u;
    if (start >= mods->child_count) {
        return;
    }

    AstNode *ann_list = NULL;
    for (size_t i = 0; i < result_type->child_count; ++i) {
        AstNode *child = result_type->children[i];
        if (child && child->kind == AST_ANNOTATION_LIST) {
            ann_list = child;
            break;
        }
    }
    if (!ann_list) {
        YYLTYPE dummy_loc = {0};
        ann_list = make_list_node(AST_ANNOTATION_LIST, dummy_loc);
        ast_prepend_child(result_type, ann_list);
    }

    for (size_t i = start; i < mods->child_count; ++i) {
        AstNode *ann = mods->children[i];
        if (ann) {
            ast_add_child(ann_list, ann);
        }
    }

    for (size_t i = start; i < mods->child_count; ++i) {
        mods->children[i] = NULL;
    }
    mods->child_count = start;
}

// 构造增强 for 语句节点。
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

// 构造 try 语句节点（含 catch/finally）。
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

// 构造 class 字面量节点。
static AstNode *make_class_literal_node(AstNode *target, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_CLASS_LITERAL, loc, 0);
    if (target) {
        ast_add_child(node, target);
    }
    return node;
}

// 构造成员访问节点。
static AstNode *make_field_access_node(AstNode *base, AstNode *member, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_MEMBER_ACCESS, loc, 0);
    node->scope = base;
    if (base) {
        ast_add_child(node, base);
    }
    if (member) {
        ast_add_child(node, member);
    }
    return node;
}

// 构造数组访问节点。
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

// 构造方法调用节点。
static AstNode *make_method_invocation_node(AstNode *qualifier, AstNode *type_args, AstNode *name, AstNode *args, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_METHOD_INVOCATION, loc, 0);
    node->scope = qualifier;
    if (qualifier) {
        ast_add_child(node, qualifier);
    }
    if (name) {
        ast_add_child(node, name);
    }
    if (type_args) {
        ast_add_child(node, type_args);
    }
    if (args) {
        ast_add_child(node, args);
    } else {
        ast_add_child(node, make_list_node(AST_ARGUMENT_LIST, loc));
    }
    return node;
}

// 构造方法引用节点。
static AstNode *make_method_reference_node(AstNode *target, AstNode *type_args, AstNode *name, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_METHOD_REFERENCE, loc, 0);
    node->scope = target;
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

// 构造 new 类实例节点（含参数与类体）。
static AstNode *make_new_class_core(AstNode *type, AstNode *args, AstNode *body, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_NEW_CLASS, loc, 0);
    node->scope = NULL;
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

// 构造 lambda 表达式节点。
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

// 构造赋值表达式节点。
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

// 构造二元表达式节点。
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

// 构造 instanceof 表达式节点。
static AstNode *make_instanceof_expr(AstNode *lhs, AstNode *rhs, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_INSTANCEOF, loc, 0);
    if (lhs) {
        ast_add_child(node, lhs);
    }
    if (rhs) {
        ast_add_child(node, rhs);
    }
    return node;
}

// 构造一元表达式节点（前/后缀）。
static AstNode *make_unary_expr(const char *op, AstNode *expr, YYLTYPE loc, bool postfix) {
    AstNode *node = AST_BRANCH_AT(AST_UNARY_EXPR, loc, 0);
    ast_set_text(node, postfix ? "postfix" : "prefix");
    if (postfix) {
        if (expr) {
            ast_add_child(node, expr);
        }
        if (op) {
            ast_add_child(node, make_keyword_leaf(op, loc));
        }
    } else {
        if (op) {
            ast_add_child(node, make_keyword_leaf(op, loc));
        }
        if (expr) {
            ast_add_child(node, expr);
        }
    }
    return node;
}

// 构造三元条件表达式节点。
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

// 构造数组创建表达式节点。
static AstNode *make_array_creation_node(AstNode *type, AstNode *dim_exprs, AstNode *dims, AstNode *initializer, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_ARRAY_CREATION, loc, 0);
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

// 构造类型转换表达式节点。
static AstNode *make_cast_expr(AstNode *type, AstNode *expr, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_CAST, loc, 0);
    if (type) {
        ast_add_child(node, type);
    }
    if (expr) {
        ast_add_child(node, expr);
    }
    return node;
}

// 构造参数节点。
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

// 构造类型匹配模式节点。
static AstNode *make_type_pattern_node(AstNode *mods, AstNode *type_node,
                                       AstNode *decl_node, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_TYPE_PATTERN, loc, 0);
    if (mods) {
        ast_add_child(node, mods);
    }
    if (type_node) {
        ast_add_child(node, type_node);
    }
    if (decl_node) {
        ast_add_child(node, decl_node);
    }
    return node;
}

// 构造 try-with-resources 资源声明节点。
static AstNode *make_resource_decl(AstNode *mods, AstNode *type_node,
                                   AstNode *name_node, AstNode *expr,
                                   YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_RESOURCE_DECL, loc, 0);
    if (mods) {
        ast_add_child(node, mods);
    }
    if (type_node || name_node) {
        AstNode *param = make_parameter_node(type_node, name_node, loc);
        ast_add_child(node, param);
    }
    if (expr) {
        ast_add_child(node, expr);
    }
    return node;
}

// 构造参数/实参列表节点并加入首个参数。
static AstNode *make_params_list(AstNode *first, YYLTYPE loc) {
    AstNode *list = ast_branch(AST_ARGUMENT_LIST, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (first) {
        ast_add_child(list, first);
    }
    return list;
}

// 构造方法签名节点（名称 + 参数）。
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

// 构造注解元素声明节点。
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

// 构造构造器声明节点。
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

// 构造显式构造器调用节点（this/super）。
static AstNode *make_explicit_ctor_invocation(AstNode *qualifier, AstNode *type_args,
                                              const char *name, AstNode *args, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_EXPLICIT_CTOR_INVOCATION, loc, 0);
    if (name) {
        ast_set_text(node, name);
    }
    if (qualifier) {
        ast_add_child(node, qualifier);
    }
    if (type_args) {
        ast_add_child(node, type_args);
    }
    if (args) {
        ast_add_child(node, args);
    } else {
        ast_add_child(node, make_list_node(AST_ARGUMENT_LIST, loc));
    }
    return node;
}

// 构造 for 语句的初始化/条件/更新部分节点。
static AstNode *make_for_part_node(AstKind kind, AstNode *payload, YYLTYPE loc) {
    if (!payload) {
        payload = make_simple_stmt(AST_EMPTY, loc);
    }
    return make_clause_node(kind, payload, AST_LOC_LINE(loc), AST_LOC_COL(loc));
}

// 构造传统 for 语句节点。
static AstNode *make_for_stmt(AstNode *init, AstNode *cond, AstNode *update,
                              AstNode *body, YYLTYPE loc) {
    AstNode *node = AST_BRANCH_AT(AST_FOR, loc, 0);
    ast_add_child(node, make_for_part_node(AST_FOR_INIT, init, loc));
    ast_add_child(node, make_for_part_node(AST_FOR_COND, cond, loc));
    ast_add_child(node, make_for_part_node(AST_FOR_UPDATE, update, loc));
    if (body) {
        ast_add_child(node, body);
    }
    return node;
}

// 构造 throws 列表节点。
static AstNode *make_throws_node(AstNode *types, YYLTYPE loc) {
    if (!types) {
        return NULL;
    }
    if (types->kind == AST_EXCEPTION_TYPE_LIST) {
        return types;
    }
    AstNode *node = ast_branch(AST_EXCEPTION_TYPE_LIST, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    ast_add_child(node, types);
    return node;
}

// 构造枚举常量节点。
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

// 构造字段声明节点。
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
        ast_add_child(field, make_list_node(AST_VAR_DECL_LIST, loc));
    } 
    return field;
}



// 构造本地变量声明节点。
static AstNode *make_local_variable_node(AstNode *mods, AstNode *type_node, AstNode *vars_node, YYLTYPE loc) {
    AstNode *stmt = ast_branch(AST_LOCAL_VAR_DECL, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (mods) {
        ast_add_child(stmt, mods);
    }
    if (type_node) {
        ast_add_child(stmt, type_node);
    }
    if (vars_node) {
        ast_add_child(stmt, vars_node);
    } else {
        ast_add_child(stmt, make_list_node(AST_VAR_DECL_LIST, loc));
    } 
    return stmt;
}

/* annotation / type / type-parameter / type-argument / dims */
static AstNode *make_annotation_node      (AstNode *name, YYLTYPE loc);
static AstNode *make_type_node            (AstNode *core, YYLTYPE loc);
static AstNode *make_type_parameter_node  (AstNode *name, AstNode *bounds, YYLTYPE loc);
static AstNode *make_type_argument_type_node(AstNode *type_node, YYLTYPE loc);
static AstNode *make_type_argument_wildcard_node(AstNode *wildcard_node, YYLTYPE loc);
static AstNode *make_type_bound_node(const char *kind, AstNode *primary, AstNode *additional, YYLTYPE loc);
static AstNode *make_dim_node             (YYLTYPE loc);

/* simple stmt / unary stmt / binary stmt / foreach / try */
// 构造无子节点的语句节点。
static AstNode *make_simple_stmt(AstKind kind, YYLTYPE loc);
// 构造一元语句节点。
static AstNode *make_unary_stmt(AstKind kind, AstNode *child, YYLTYPE loc);
// 构造二元语句节点。
static AstNode *make_binary_stmt(AstKind kind, AstNode *left, AstNode *right, YYLTYPE loc);
// 构造增强 for 语句节点。
static AstNode *make_foreach_stmt         (AstNode *param, AstNode *expr, AstNode *body, YYLTYPE loc);
// 构造 try 语句节点（含 catch/finally）。
static AstNode *make_try_stmt             (AstNode *block, AstNode *catches, AstNode *finally_block, YYLTYPE loc);

/* 参数、变量、本地变?*/
// 构造参数并附加维度。
static AstNode *make_parameter_with_dims  (AstNode *type_node, AstNode *name_node,
                                           AstNode *dims, YYLTYPE loc);
// 构造参数节点。
static AstNode *make_parameter_node       (AstNode *type_node, AstNode *name_node, YYLTYPE loc);
// 构造本地变量声明节点。
static AstNode *make_local_variable_node  (AstNode *mods, AstNode *type_node, AstNode *vars_node, YYLTYPE loc);
// 构造类型匹配模式节点。
static AstNode *make_type_pattern_node (AstNode *mods, AstNode *type_node, AstNode *decl_node, YYLTYPE loc);
static AstNode *make_clause_node(AstKind kind, AstNode *payload, int line, int column) {
    AstNode *node = ast_branch(kind, line, column, 0);
    if (payload) {
        ast_add_child(node, payload);
    }
    return node;
}

// 构造类声明基础节点。
static AstNode *make_class_basic(int line, int column, 
                               AstNode *modifiers, 
                               AstNode *name, 
                               AstNode *type_params,
                               AstNode *super_class,
                               AstNode *super_interfaces,
                               AstNode *permits,
                               AstNode *body);
// 构造注解类型声明基础节点。
static AstNode *make_annotation_decl_basic(int line, int column,
                                           AstNode *modifiers,
                                           AstNode *name,
                                           AstNode *type_params,
                                           AstNode *extends_interfaces,
                                           AstNode *permits,
                                           AstNode *body);
// 构造接口声明基础节点。
static AstNode *make_interface_basic(int line, int column, 
                                   AstNode *modifiers,
                                   AstNode *name, 
                                   AstNode *type_params,
                                   AstNode *extends_interfaces,
                                   AstNode *permits,
                                   AstNode *body);
// 构造注解元素声明节点。
static AstNode *make_annotation_element_decl(AstNode *modifiers, AstNode *type_node,
                                             AstNode *name_node, AstNode *dims_node,
                                             AstNode *default_value, YYLTYPE loc);

/* 访问相关：field / class-literal / dims / new-class / array */
// 构造成员访问节点。
static AstNode *make_field_access_node    (AstNode *qualifier, AstNode *name, YYLTYPE loc);
// 构造 class 字面量节点。
static AstNode *make_class_literal_node   (AstNode *type_or_kw, YYLTYPE loc);
// 构造 new 类实例节点（含参数与类体）。
static AstNode *make_new_class_core       (AstNode *type, AstNode *args, AstNode *body, YYLTYPE loc);
// 构造数组访问节点。
static AstNode *make_array_access_node    (AstNode *array_expr, AstNode *index_expr, YYLTYPE loc);
// 构造数组创建表达式节点。
static AstNode *make_array_creation_node  (AstNode *type, AstNode *dim_exprs,
                                           AstNode *dims, AstNode *initializer, YYLTYPE loc);

/* 调用 / 方法引用 / lambda / 赋?/ 条件表达?/ 一元二元表达式 / 强制类型转换 */
// 构造方法调用节点。
static AstNode *make_method_invocation_node (AstNode *qualifier, AstNode *type_args,
                                             AstNode *name, AstNode *args, YYLTYPE loc);
// 构造方法引用节点。
static AstNode *make_method_reference_node  (AstNode *qualifier, AstNode *type_args,
                                             AstNode *name, YYLTYPE loc);
// 构造 lambda 表达式节点。
static AstNode *make_lambda_node            (AstNode *params, AstNode *body, YYLTYPE loc);
// 构造赋值表达式节点。
static AstNode *make_assignment_node        (AstNode *lhs, AstNode *op, AstNode *rhs, YYLTYPE loc);
// 构造三元条件表达式节点。
static AstNode *make_conditional_expr       (AstNode *cond, AstNode *then_expr,
                                             AstNode *else_expr, YYLTYPE loc);
// 构造二元表达式节点。
static AstNode *make_binary_expr            (AstNode *lhs, const char *op,
                                             AstNode *rhs, YYLTYPE loc);
// 构造 instanceof 表达式节点。
static AstNode *make_instanceof_expr        (AstNode *lhs, AstNode *rhs, YYLTYPE loc);
// 构造一元表达式节点（前/后缀）。
static AstNode *make_unary_expr             (const char *op, AstNode *expr,
                                             YYLTYPE loc, bool is_postfix);
// 构造类型转换表达式节点。
static AstNode *make_cast_expr              (AstNode *type, AstNode *expr, YYLTYPE loc);

/* 通用构造：列表、关键字叶子 */
// 生成指定种类的空列表节点。
static AstNode *make_list_node(AstKind kind, YYLTYPE loc);
// 用关键字文本生成标识符叶子节点。
static AstNode *make_keyword_leaf           (const char *kw, YYLTYPE loc);
static AstNode *wrap_labeled_block(const char *label, AstNode *block, YYLTYPE loc);
%}

%code requires {
#include "java_ast.h"
}

%union {
    char* str;
    int val;
    AstNode* node;
}

%locations

// %token LPAREN_LambdaParameters            // LambdaParameters 中的 '(' 符号（之前有冲突，调试后发现冲突意外没了，故建议保留以便后续研究）

// %expect 0

%token AMPERSAND RPAREN_CastExpression // 辅助处理按位与与类型转换右括号的 token

// %token IDENTIFIER
%token <node> TYPE_IDENTIFIER_IdentifierforLambdaParameterList // 向后检查两个是 '(' ARROW，或者向后检查一个是 ARROW，或者向后检查一个是 ','，就在向后检查两个 token。
%token <node> VAR_IdentifierComplement_IdentifierforLambdaParameterList // lambda 参数列表中的 var 标识符补全
%token <node> YIELD_IdentifierComplement_IdentifierforLambdaParameterList // lambda 参数列表中的 yield 标识符补全
%token <node> TYPE_IDENTIFIER_MethodDeclarator // 方法声明中的类型标识符
%token <node> VAR_MethodDeclarator // 方法声明中的 var 标识符
%token <node> YIELD_MethodDeclarator // 方法声明中的 yield 标识符
%token <node> TYPE_IDENTIFIER_AnnotationIdentifiers // 注解名中的类型标识符
%token <node> VAR_AnnotationIdentifiers // 注解名中的 var 标识符
%token <node> YIELD_AnnotationIdentifiers // 注解名中的 yield 标识符
%token <node> TYPE_IDENTIFIER // 类型标识符
%token <node> TYPE_IDENTIFIER_EnumDeclaration // 枚举声明中的类型标识符

%token LANGLE // TypeArguments / TypeParameters 中尖括号的左半边
%token AT_Dims // Dims 中的 '@' 符号
%token AT_Modifier // Modifier 中的 '@' 符号
%token AT_AnnotationTypeDeclaration // AnnotationTypeDeclaration 中的 '@' 符号
%token LBRACK // DimsNoAnnotations 中的 '[' 符号
%token LBRACK_ArrayAccess // 向后判断一个 token 不为 ']' 的 '['
// %token LPAREN_LambdaParameters            // LambdaParameters 中的 '(' 符号（之前有冲突，调试后发现冲突意外没了，故建议保留以便后续研究）
// %token RPAREN_LambdaParameters            // LambdaParameters 中的 ')' 符号

%token DEFAULT_SwitchLabel // SwitchLabel 中的 Default 关键字（根据后续 ARROW 或 ':' 判定）
%token DOT // '.' 符号
%token DOT_CommonName // CommonName 中的 '.' 符号（向前检测一个为 TYPE_IDENTIFIER 或 IdentifierComplement，向后也是）
%token <node> VAR_IdentifierComplement // 标识符补全：var
%token <node> YIELD_IdentifierComplement // 标识符补全：yield
%token CLASS // class 关键字
%token BYTE INT SHORT LONG CHAR // 整型/字符型关键字
%token FLOAT DOUBLE // 浮点类型关键字
%token EXTENDS SUPER // extends/super 关键字
%token OPEN OPENS // 模块 open/opens 关键字
%token REQUIRES EXPORTS USES PROVIDES // 模块指令关键字
%token PACKAGE // package 关键字
%token IMPORT // import 关键字
%token PUBLIC PROTECTED PRIVATE ABSTRACT STATIC FINAL STRICTFP TRANSITIVE // 修饰符关键字
%token IMPLEMENTS // implements 关键字
%token TRANSIENT VOLATILE // 字段修饰符关键字
%token SYNCHRONIZED NATIVE // 方法修饰符关键字
%token BOOLEAN // boolean 关键字
%token VOID // void 关键字
%token THROW THROWS // throw/throws 关键字
%token THIS // this 关键字
%token ENUM // enum 关键字
%token INTERFACE // interface 关键字
%token DEFAULT // default 关键字
%token VAR // var 关键字
%token IF ELSE ASSERT SWITCH // 控制流关键字
%token CASE // case 关键字
%token DO WHILE FOR // 循环关键字
%token BREAK YIELD CONTINUE RETURN // 跳转/返回关键字
%token TRY CATCH // try/catch 关键字
%token FINALLY // finally 关键字
%token NEW // new 关键字
%token MODULE // module 关键字
%token <node> UnqualifiedMethodIdentifier // 不带限定的方法标识符
%token <node> NUMBER // 数值字面量（词法归并）
%token <node> TRUE FALSE // 布尔字面量
%token <node> INTEGERLITERAL // 整数字面量
%token <node> FLOATINGPOINTLITERAL // 浮点字面量
%token <node> CHARACTERLITERAL // 字符字面量
%token <node> STRINGLITERAL // 字符串字面量
%token <node> MY_NULL // null 字面量

// 定义多字符运算符 Token
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
%token PREFIX_INC PREFIX_DEC // 前缀 ++/-- 运算符
%token ELLIPSIS     // "..."
%token TRAILING_COMMA  // ","

%token <node> TextBlock // 文本块
%token SEALED NON_SEALED // sealed 修饰符关键字
%token <node> SEALED_IdentifierComplement // 标识符补全：sealed
%token <node> NON_SEALED_IdentifierComplement // 标识符补全：non-sealed
%token PERMITS // permits 关键字
%token EMPTY_STMT // 空语句

// 待确定以下顺序的唯一正确方案。
// 以下为优先级标记（不直接对应词法 token）。
%nonassoc PREC_ConditionalExpression_1 // 条件表达式优先级（用于禁止递归扩展）
%nonassoc PREC_ConditionalExpression // 条件表达式优先级（完整 ?: 规则）
%nonassoc PREC_ConditionalOrExpression_1 // 条件或表达式优先级（用于禁止递归扩展）
%nonassoc PREC_ConditionalAndExpression // 条件与表达式优先级（完整规则）
%nonassoc PREC_ConditionalAndExpression_1 // 条件与表达式优先级（用于禁止递归扩展）
%nonassoc PREC_InclusiveOrExpression // 按位或表达式优先级（完整规则）
%nonassoc PREC_InclusiveOrExpression_1 // 按位或表达式优先级（用于禁止递归扩展）
%nonassoc PREC_ExclusiveOrExpression // 按位异或表达式优先级（完整规则）
%nonassoc PREC_ExclusiveOrExpression_1 // 按位异或表达式优先级（用于禁止递归扩展）
%nonassoc PREC_AndExpression // 按位与表达式优先级（完整规则）
%nonassoc PREC_AndExpression_1 // 按位与表达式优先级（用于禁止递归扩展）
%nonassoc PREC_EqualityExpression // 相等性表达式优先级
%nonassoc PREC_RelationalExpression // 关系表达式优先级（含 instanceof）
%nonassoc PREC_RelationalExpression_1 // 关系表达式优先级（用于禁止递归扩展）
%nonassoc PREC_SimpleRelationalExpression // 简单关系表达式优先级（不递归扩展）
%nonassoc PREC_ShiftExpression // 移位表达式优先级（完整规则）
%nonassoc PREC_ShiftExpression_1 // 移位表达式优先级（用于禁止递归扩展）
%nonassoc PREC_AdditiveExpression // 加减表达式优先级（完整规则）
%nonassoc PREC_AdditiveExpression_1 // 加减表达式优先级（用于禁止递归扩展）
%nonassoc PREC_MultiplicativeExpression // 乘除模表达式优先级

%left ','                        // ,（逗号运算符） 
%right '=' ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN OR_ASSIGN XOR_ASSIGN AND_ASSIGN SHL_ASSIGN SHR_ASSIGN USHR_ASSIGN   // = += -= *= /= %= |= ^= &= <<= >>= >>>=
%right ARROW DIAMOND             // -> <>
%right '?' ':'                   // ? :
// %right ':'                   // ? :
%left OR                         // ||（逻辑或）
%left AND                        // &&（逻辑与）
%left '|' // |（按位或）
%left '^'                        // ^（按位异或）
%left '&' // &（按位与）
%left EQ NE                      // == !=（相等比较）
%left TO WITH                    // to with
%nonassoc INSTANCEOF             // instanceof（类型检查）
%left '<' '>' LE GE LT_RelationalExpression             // < > <= >=
%left SHL_OP SHR_OP USHR_OP      // << >> >>>（位移运算）
%left '+' '-' // + -（加减号）
%left '*' '/' '%'                // * / %
%left '.' DOT DOUBLE_COLON // . ::（DOUBLE_COLON_MethodReference_COI / DOUBLE_COLON_MethodReference_AT）
%right '!' '~' // !(逻辑非) ~（按位取反）
%nonassoc PREFIX_INC PREFIX_DEC // ++ --（前缀形式）（UPLUS UMINUS 作为正负号）
%left INC_OP DEC_OP              // ++ -- (后缀形式) 
%left '[' ']' LBRACK RBRACK '(' ')' // [] () LPAREN_LambdaParameters RPAREN_LambdaParameters

%left '@'  AT_Dims AT_Modifier AT_AnnotationTypeDeclaration // 注解相关优先级

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
%type <node> FormalParameterList FormalParameter VariableArityParameter ReceiverParameter
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

/* 语法规则：各产生式构建 AST 节点。 */
%%

// 标识符补全关键字集合（var/yield/sealed 等）。
IdentifierComplement:
    VAR_IdentifierComplement        { $$ = $1; }
    | SEALED_IdentifierComplement   { $$ = $1; }
    | NON_SEALED_IdentifierComplement { $$ = $1; }
    | YIELD_IdentifierComplement    { $$ = $1; }
;

// 方法声明用的标识符入口（含 var/yield 补全）。
IDENTIFIER_MethodDeclarator:
    TYPE_IDENTIFIER_MethodDeclarator { $$ = $1; }
    | VAR_MethodDeclarator { $$ = $1; }
    | YIELD_MethodDeclarator { $$ = $1; }
;

// 注解标识符入口（含 var/yield 补全）。
IDENTIFIER_AnnotationIdentifiers:
    TYPE_IDENTIFIER_AnnotationIdentifiers { $$ = $1; }
    | VAR_AnnotationIdentifiers { $$ = $1; }
    | YIELD_AnnotationIdentifiers { $$ = $1; }
;

// 词法定义
// 字面量：整数/浮点/字符/字符串/布尔/null。
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

// 修饰符：访问控制与声明修饰关键字。
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
// 修饰符列表。
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

// 以下 4 条语法是专门为 Modifier 服务的 Annotation。
// 作为修饰符使用的注解三种形态。
Annotation_Modifier:
    NormalAnnotation_Modifier { $$ = $1; }
    | MarkerAnnotation_Modifier { $$ = $1; }
    | SingleElementAnnotation_Modifier { $$ = $1; }
;

// 普通注解作为修饰符的形式。
NormalAnnotation_Modifier:
    AT_Modifier TypeName_ModifierOrDims '(' ElementValuePairList ')' {
        AstNode *node = make_annotation_node($2, @1);
        ast_add_child(node, $4);
        AstNode *list = make_list_node(AST_ANNOTATION_LIST, @$);
        ast_add_child(list, node);
        $$ = list;
    }
    | AT_Modifier TypeName_ModifierOrDims '(' ')' {
        AstNode *node = make_annotation_node($2, @1);
        AstNode *list = make_list_node(AST_ANNOTATION_LIST, @$);
        ast_add_child(list, node);
        $$ = list;
    }
;

// 标记注解作为修饰符的形式。
MarkerAnnotation_Modifier:
    AT_Modifier TypeName {
        AstNode *node = make_annotation_node($2, @1);
        AstNode *list = make_list_node(AST_ANNOTATION_LIST, @$);
        ast_add_child(list, node);
        $$ = list;
    }
;

// 单元素注解作为修饰符的形式。
SingleElementAnnotation_Modifier:
    AT_Modifier TypeName_ModifierOrDims '(' ElementValue ')' {
        AstNode *node = make_annotation_node($2, @1);
        AstNode *args = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child(args, $4);
        ast_add_child(node, args);
        AstNode *list = make_list_node(AST_ANNOTATION_LIST, @$);
        ast_add_child(list, node);
        $$ = list;
    }
;

// 修饰符注解中允许的类型名/带限定名。
TypeName_ModifierOrDims:
    TYPE_IDENTIFIER_MethodDeclarator { $$ = $1; }
  | CommonName DOT_CommonName TYPE_IDENTIFIER_MethodDeclarator {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        node->scope = $1;
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
;


//-------------------Types, Values, and Variables-----------------------
// 类型、值与变量：字面量、类型、注解、修饰符、变量/参数等。
/*
// 类型入口：基本类型或引用类型。
Type:
    PrimitiveType
  | ReferenceType
;
*/

// 这里将 PrimitiveType 缩减为必须被一个或多个 Annotation 修饰的 NumericType 或 BOOLEAN，然后在以下所有用到 PrimitiveType 的地方替换为 UnannPrimitiveType 以保证语法正确。
// 基本类型：数值/布尔。
PrimitiveType:
//    Annotations NumericType
//    | Annotations BOOLEAN
    Annotations UnannPrimitiveType {
        AstNode *type = make_type_node($2, @$);
        if ($1) {
            ast_prepend_child(type, $1);
        }
        $$ = type;
    }
//    | NumericType
//    | BOOLEAN
;

// 数值类型入口（整型/浮点）。
NumericType:
    IntegralType { $$ = $1; }
  | FloatingPointType { $$ = $1; }
;

// 整型/字符类型。
IntegralType:
    BYTE { $$ = make_keyword_leaf("byte", @1); }
  | SHORT { $$ = make_keyword_leaf("short", @1); }
  | INT { $$ = make_keyword_leaf("int", @1); }
  | LONG { $$ = make_keyword_leaf("long", @1); }
  | CHAR { $$ = make_keyword_leaf("char", @1); }
;

// 浮点类型。
FloatingPointType:
    FLOAT { $$ = make_keyword_leaf("float", @1); }
  | DOUBLE { $$ = make_keyword_leaf("double", @1); }
;

// 原 ClassOrInterfaceType 中实际包含 TypeVariable 的具体语法，这里将原定义中的 TypeVariable 去掉。
// 引用类型：类/接口/数组。
ReferenceType:
    ClassOrInterfaceType { $$ = $1; }
//  | TypeVariable
    | ArrayType { $$ = $1; }
//    | UnannArrayType
;

// 将 ClassOrInterfaceType/ClassType/InterfaceType 合并，使用 Annotations TYPE_IDENTIFIER 替换 TypeVariable。
/* 
这样定义实际上在扩宽语义。
因为 ClassOrInterfaceType: Annotations PackageName . {Annotation} TypeIdentifier [TypeArguments] 不应该规约成 ClassOrInterfaceType
*/
// 类或接口类型（含类型实参）。
ClassOrInterfaceType:
    UnannClassOrInterfaceType {
        $$ = make_type_node($1, @$);
    }
    | Annotations UnannClassOrInterfaceType {
        AstNode *type = make_type_node($2, @$);
        if ($1) {
            ast_prepend_child(type, $1);
        }
        $$ = type;
    }
;

// 因为 ClassOrInterfaceType 中实际包含语法上的 TypeVariable，则 TypeVariable Dims 与 ClassOrInterfaceType Dims 有一定重叠，故删除。
// 数组类型：元素类型 + 维度。
ArrayType:
    Annotations UnannPrimitiveType Dims {
        AstNode *type = make_type_node($2, @$);
        if ($1) {
            ast_prepend_child(type, $1);
        }
        if ($3) {
            ast_add_child(type, $3);
        }
        $$ = type;
    }
    | Annotations UnannClassOrInterfaceType Dims {
        AstNode *type = make_type_node($2, @$);
        if ($1) {
            ast_prepend_child(type, $1);
        }
        if ($3) {
            ast_add_child(type, $3);
        }
        $$ = type;
    }
    | UnannArrayType {
        $$ = make_type_node($1, @$);
    }
;

// 数组维度列表（可带注解）。
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

// 以下五条语法是专门为 Dims 服务的 Annotations。
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
        $$ = make_list_node(AST_ANNOTATION_LIST, @$);
        ast_add_child($$, $1);
    }
;

// 维度上的普通注解。
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

// 维度上的标记注解。
MarkerAnnotation_Dims:
    AT_Dims TypeName {
        $$ = make_annotation_node($2, @1);
    }
;

// 维度上的单元素注解。
SingleElementAnnotation_Dims:
    AT_Dims TypeName_ModifierOrDims '(' ElementValue ')' {
        AstNode *node = make_annotation_node($2, @1);
        AstNode *args = make_list_node(AST_ARGUMENT_LIST, @$);
        ast_add_child(args, $4);
        ast_add_child(node, args);
        $$ = node;
    }
;

// TypeParameterModifier 纯粹由 Annotation 构成，故而这里把 TypeParameterModifier 直接替换为 Annotation。
TypeParameter:
    Annotations TYPE_IDENTIFIER {
        AstNode *node = make_type_parameter_node($2, NULL, @$);
        if ($1) {
            ast_prepend_child(node, $1);
        }
        $$ = node;
    }
    | Annotations TYPE_IDENTIFIER TypeBound {
        AstNode *node = make_type_parameter_node($2, $3, @$);
        if ($1) {
            ast_prepend_child(node, $1);
        }
        $$ = node;
    }
    | TYPE_IDENTIFIER {
        $$ = make_type_parameter_node($1, NULL, @$);
    }
    | TYPE_IDENTIFIER TypeBound {
        $$ = make_type_parameter_node($1, $2, @$);
    }
;

// 泛型上界/下界约束。
TypeBound:
    EXTENDS ClassOrInterfaceType AdditionalBounds {
        $$ = make_type_bound_node("extends", $2, $3, @$);
    }
;

// 这里允许 AdditionalBounds 为空，这是代码中极少数允许为空的地方。
AdditionalBounds:
//    AMPERSAND ClassOrInterfaceType
    AdditionalBounds AMPERSAND ClassOrInterfaceType {
        AstNode *list = $1;
        if (!list) {
            list = make_list_node(AST_ADDITIONAL_BOUNDS, @$);
        }
        ast_add_child(list, $3);
        $$ = list;
    }
    |  { $$ = NULL; }
;

// 泛型实参列表：<String, ? extends ...>。
TypeArguments:
    '<' TypeArgumentList '>' { $$ = $2; }
    | LANGLE TypeArgumentList '>' { $$ = $2; }
;

// 未注解类/接口类型中的泛型实参。
TypeArguments_UnannClassOrInterfaceType:
//    '<' TypeArgumentList '>'
    LANGLE TypeArgumentList '>' { $$ = $2; }
;

// 类型实参列表。
TypeArgumentList:
    TypeArgument {
        $$ = make_list_node(AST_TYPE_ARGUMENT_LIST, @$);
        ast_add_child($$, $1);
    }
    | TypeArgumentList ',' TypeArgument {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

// 单个类型实参。
TypeArgument:
    ReferenceType {
        $$ = make_type_argument_type_node($1, @$);
    }
  | Wildcard {
        $$ = make_type_argument_wildcard_node($1, @$);
    }
;

// 通配符类型参数。
Wildcard:
    Annotations '?' WildcardBounds {
        $$ = make_wildcard_node($1, $3, @$);
    }
  | Annotations '?' {
        $$ = make_wildcard_node($1, NULL, @$);
    }
  | '?' WildcardBounds {
        $$ = make_wildcard_node(NULL, $2, @$);
    }
  | '?' {
        $$ = make_wildcard_node(NULL, NULL, @$);
    }
;

// 通配符边界（extends/super）。
WildcardBounds:
    EXTENDS ReferenceType {
        AstNode *bound = make_type_bound_node("extends", $2, NULL, @$);
        $$ = make_wildcard_bound_node("extends", bound, @1);
    }
  | SUPER ReferenceType {
        AstNode *bound = make_type_bound_node("super", $2, NULL, @$);
        $$ = make_wildcard_bound_node("super", bound, @1);
    }
;


//-------------------------------Names------------------------------
// 名称相关：包名、类型名、点分名称等。


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
        $$->scope = $1;
        ast_add_children($$, children, 2);
    }
    | ModuleName DOT_CommonName IdentifierComplement {
        AstNode *children[] = { $1, $3 };
        $$ = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        $$->scope = $1;
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

// 类型名称（可带限定前缀）。
TypeName:
    TYPE_IDENTIFIER { $$ = $1; }
  | CommonName DOT_CommonName TYPE_IDENTIFIER {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        node->scope = $1;
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        $$ = node;
    }
;
// 类型名称列表。
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
   如上四条合并?CommonName
   有点地方好像还掺杂一?ModuleName
*/
/*
// 通用点分名称（表达式名/包名/类型名公用）。
CommonName:
    IDENTIFIER
    | CommonName '.' IDENTIFIER
;
*/
// 通用点分名称（表达式名/包名/类型名公用）。
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
        $$->scope = $1;
        ast_add_children($$, children, 2);
    }
    | CommonName DOT_CommonName IdentifierComplement {
        AstNode *children[] = { $1, $3 };
        $$ = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        $$->scope = $1;
        ast_add_children($$, children, 2);
    }
;


//----------------------------Packages and Modules-----------------------------
// 包与模块：package/import/module 声明及其指令。
// 根规则：编译单元。
CompilationUnit:
    OrdinaryCompilationUnit { $$ = $1; root_ast = $$; }
  | ModularCompilationUnit {
        $$ = $1;
        root_ast = $$;
    }
;

// 普通编译单元。
// 普通编译单元：固定 3 个子节点（package, imports, types）。
OrdinaryCompilationUnit:
    PackageDeclaration ImportDeclarations TypeDeclarations {
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @$, 3, $1, $2, $3);
    }
  | ImportDeclarations TypeDeclarations {
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @$, 2, $1, $2);
    }
  | PackageDeclaration TypeDeclarations {
        AstNode *empty_imports = AST_EMPTY_NODE(AST_IMPORT_LIST, @$);
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @$, 3, $1, empty_imports, $2);
    }
  | TypeDeclarations {
        AstNode *empty_imports = AST_EMPTY_NODE(AST_IMPORT_LIST, @$);
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @$, 2, empty_imports, $1);
    }
;


// 模块编译单元。
ModularCompilationUnit:
    ImportDeclarations ModuleDeclaration {
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @1, 2, $1, $2);
    }
    | ModuleDeclaration {
        $$ = AST_BRANCH_AT(AST_COMPILATION_UNIT, @1, 1, $1);
    }
;

// 包声明：包修饰符 PackageModifier 完全由 Annotation 构成，故而这里把 PackageModifier 直接替换为 Annotation。
// package 声明（可选，默认包）。
PackageDeclaration:
    Annotations PACKAGE CommonName ';' {
        $$ = AST_BRANCH_AT(AST_PACKAGE_DECL, @2, 2, $1, $3);
    }
  | PACKAGE CommonName ';' {
        AstNode *empty_ann = make_list_node(AST_ANNOTATION_LIST, @$);   // 与 Annotations 的 list kind 保持一致。
        $$ = AST_BRANCH_AT(AST_PACKAGE_DECL, @1, 2, empty_ann, $2);
    }
;


// 导入声明列表。
// import 列表（普通/静态/按需）。
ImportDeclarations:
    ImportDeclaration {
        $$ = AST_BRANCH_AT(AST_IMPORT_LIST, @$, 1, $1);
    }
  | ImportDeclarations ImportDeclaration {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

// import 声明入口（单条）。
ImportDeclaration:
    SingleTypeImportDeclaration          { $$ = $1; }
  | TypeImportOnDemandDeclaration        { $$ = $1; }
  | SingleStaticImportDeclaration        { $$ = $1; }
  | StaticImportOnDemandDeclaration      { $$ = $1; }
;

// 单一类型导入声明。
SingleTypeImportDeclaration:
    IMPORT TypeName ';' {
        $$ = make_import_node(false, false, $2, @1);
    }
;

// 按需类型导入声明。
TypeImportOnDemandDeclaration:
    IMPORT CommonName '.' '*' ';' {
        $$ = make_import_node(false, true, $2, @1);
    }
;

// 单一静态导入声明。
// 这里为避免冲突将 TypeName 换成 CommonName，实际上扩大了语义，后续可升级。
SingleStaticImportDeclaration:
//   IMPORT STATIC TypeName '.' TYPE_IDENTIFIER ';'
//    | IMPORT STATIC TypeName '.' IdentifierComplement ';'
    IMPORT STATIC CommonName ';' {
        $$ = make_import_node(true, false, $3, @1);
    }
;

// 静态按需导入声明。
// 这里为避免冲突将 TypeName 换成 CommonName，实际上扩大了语义，后续可升级。
StaticImportOnDemandDeclaration:
//    IMPORT STATIC TypeName '.' '*' ';'
    IMPORT STATIC CommonName '.' '*' ';' {
        $$ = make_import_node(true, true, $3, @1);
    }
;

// 类型声明列表。
TypeDeclarations:
    TypeDeclaration {
        $$ = AST_BRANCH_AT(AST_TYPE_DECL_LIST, @$, 1, $1);
    }
  | TypeDeclarations TypeDeclaration {
        ast_add_child($1, $2);
        $$ = $1;
    }
;


// 类型声明入口。
TypeDeclaration:
    ClassDeclaration { $$ = $1; }
    | InterfaceDeclaration { $$ = $1; }
    | EMPTY_STMT { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
    | ';' { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
;

// 模块声明。
// module 声明与指令块。
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

// 模块指令列表。
ModuleDirectives:
    ModuleDirective {
        $$ = AST_BRANCH_AT(AST_STATEMENT_LIST, @1, 1, $1);
    }
    | ModuleDirectives ModuleDirective {
        ast_add_child($1, $2);
        $$ = $1;
    }
//  | /*/
;

// 这里把原 ModuleDirective 中的 RequiresModifier 替换为 TRANSITIVE/STATIC 及其排列组合。
// module 指令：requires/exports/opens/uses/provides。
ModuleDirective:
    REQUIRES TRANSITIVE ModuleName ';' {
        AstNode *node = make_module_directive_node(AST_REQUIRES_DIRECTIVE, @1);
        ast_add_child(node, make_keyword_leaf("transitive", @2));
        ast_add_child(node, $3);
        $$ = node;
    }
    | REQUIRES ModuleName ';' {
        AstNode *node = make_module_directive_node(AST_REQUIRES_DIRECTIVE, @1);
        ast_add_child(node, $2);
        $$ = node;
    }
    | REQUIRES STATIC ModuleName ';' {
        AstNode *node = make_module_directive_node(AST_REQUIRES_DIRECTIVE, @1);
        ast_add_child(node, make_keyword_leaf("static", @2));
        ast_add_child(node, $3);
        $$ = node;
    }
    | EXPORTS CommonName ';' {
        AstNode *node = make_module_directive_node(AST_EXPORTS_DIRECTIVE, @1);
        ast_add_child(node, $2);
        $$ = node;
    }
    | EXPORTS CommonName TO ModuleNames ';' {
        AstNode *node = make_module_directive_node(AST_EXPORTS_DIRECTIVE, @1);
        ast_add_child(node, $2);
        ast_add_child(node, make_keyword_leaf("to", @3));
        ast_add_child(node, $4);
        $$ = node;
    }
    | OPENS CommonName ';' {
        AstNode *node = make_module_directive_node(AST_OPENS_DIRECTIVE, @1);
        ast_add_child(node, $2);
        $$ = node;
    }
    | OPENS CommonName TO ModuleNames ';' {
        AstNode *node = make_module_directive_node(AST_OPENS_DIRECTIVE, @1);
        ast_add_child(node, $2);
        ast_add_child(node, make_keyword_leaf("to", @3));
        ast_add_child(node, $4);
        $$ = node;
    }
    | USES TypeName ';' {
        AstNode *node = make_module_directive_node(AST_USES_DIRECTIVE, @1);
        ast_add_child(node, $2);
        $$ = node;
    }
    | PROVIDES TypeName WITH TypeNames ';' {
        AstNode *node = make_module_directive_node(AST_PROVIDES_DIRECTIVE, @1);
        ast_add_child(node, $2);
        ast_add_child(node, make_keyword_leaf("with", @3));
        ast_add_child(node, $4);
        $$ = node;
    }
;


//------------------------Arrays------------------------------------
// 数组相关：数组类型、数组初始化与维度表达式。


// 数组初始化器。
// 数组初始化器：{ ... }。
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

// 变量初始化器列表。
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
// 类相关：类声明、成员、构造器、枚举等。


// 类声明入口：普通类或枚举。
ClassDeclaration:
    NormalClassDeclaration { $$ = $1; }
    | EnumDeclaration { $$ = $1; }
;

// 普通类声明。
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
// 可选泛型形参。
OptTypeParameters:
    TypeParameters { $$ = $1; }
    | /* empty */ { $$ = NULL; }
;

// 可选父类。
OptSuperclass:
    Superclass { $$ = $1; }
    | /* empty */ { $$ = NULL; }
;

// 可选接口列表。
OptSuperinterfaces:
    Superinterfaces { $$ = $1; }
    | /* empty */ { $$ = NULL; }
;

// 可选 permits 列表。
OptClassOrInterfacePermits:
    ClassOrInterfacePermits { $$ = $1; }
    | /* empty */ { $$ = NULL; }
;

// 泛型形参列表：<T, U extends ...>。
TypeParameters:
    '<' TypeParameterList '>'  { $$ = $2; }
  | LANGLE TypeParameterList '>' { $$ = $2; }
;

// 泛型形参项列表。
TypeParameterList:
    TypeParameter {
        $$ = make_list_node(AST_TYPE_PARAMETER_LIST, @$);
        ast_add_child($$, $1);
    }
  | TypeParameterList ',' TypeParameter {
        ast_add_child($1, $3);
        $$ = $1;
    }
;


// extends 父类。
Superclass:
    EXTENDS ClassOrInterfaceType {
        $$ = $2;
    }
;

// implements 接口列表。
Superinterfaces:
    IMPLEMENTS InterfaceTypeList {
        $$ = $2;
    }
;

// 接口类型列表。
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

// permits 类型列表。
ClassOrInterfacePermits:
    PERMITS TypeNames { $$ = $2; }
;

// 类体与成员声明列表。
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

// 类体单条声明。
ClassBodyDeclaration:
    ClassMemberDeclaration { $$ = $1; }
    | InstanceInitializer { $$ = $1; }
    | StaticInitializer { $$ = $1; }
    | ConstructorDeclaration { $$ = $1; }
;

// 类成员：字段/方法/构造器/初始化块/嵌套类型。
ClassMemberDeclaration:
    FieldDeclaration { $$ = $1; }
    | MethodDeclaration { $$ = $1; }
    | ClassDeclaration { $$ = $1; }
    | InterfaceDeclaration { $$ = $1; }
    | ';' { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
    | EMPTY_STMT { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
;
// 字段声明。
FieldDeclaration:
    Modifiers UnannType VariableDeclaratorList ';' {
        $$ = make_field_node($1, $2, $3, @2);
    }
  | UnannType VariableDeclaratorList ';' {
        $$ = make_field_node(NULL, $1, $2, @1);
    }
;





// 变量声明列表。
VariableDeclaratorList:
    VariableDeclarator {
        $$ = make_list_node(AST_VAR_DECL_LIST, @$);
        ast_add_child($$, $1);
    }
    | VariableDeclaratorList ',' VariableDeclarator {
        ast_add_child($1, $3);
        $$ = $1;
    }
;

// 变量声明项。
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

// 变量名与数组维度。
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

// 变量初始化器（表达式或数组初始化器）。
VariableInitializer:
    Expression {
        $$ = $<node>1;
    }
    | ArrayInitializer {
        $$ = $1;
    }
;

// 未注解类型入口。
UnannType:
    UnannReferenceType { $$ = make_type_node($1, @$); }
    | UnannPrimitiveType { $$ = make_type_node($1, @$); }
;

// ?FloatingPointType IntegralType NumericType UnannPrimitiveType 合并
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

// UnannClassOrInterfaceType 中实际包?UnannTypeVariable ，这里予以去?
UnannReferenceType:
    UnannArrayType { $$ = $1; }    //结尾?]'
//    | UnannTypeVariable
    | UnannClassOrInterfaceType { $$ = $1; }
;

// UnannClassOrInterfaceType、UnannClassType、UnannInterfaceType 合并在一?
/*
   请注意，这里?CommonName 替代 TypeName 替代?UnannClassOrInterfaceType: TYPE_IDENTIFIER 和原 UnannClassOrInterfaceType: PackageName . TypeIdentifier 的行?
   实际上扩宽了语义
*/
// 未注解类或接口类型。
UnannClassOrInterfaceType:
    TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType {
        if ($2) {
            ast_add_child($1, $2);
        }
        $$ = $1;
    }
    | CommonName { $$ = $1; }
//    | TYPE_IDENTIFIER
//    | TYPE_IDENTIFIER '.' TYPE_IDENTIFIER
//    | IdentifierComplement '.' TYPE_IDENTIFIER
    | CommonName '.' Annotations TYPE_IDENTIFIER {
        AstNode *member = $4;
        if ($3) {
            ast_prepend_child(member, $3);
        }
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        node->scope = $1;
        ast_add_child(node, $1);
        ast_add_child(node, member);
        $$ = node;
    }
    | CommonName '.' Annotations TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType {
        AstNode *member = $4;
        if ($3) {
            ast_prepend_child(member, $3);
        }
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        node->scope = $1;
        ast_add_child(node, $1);
        ast_add_child(node, member);
        if ($5) {
            ast_add_child(node, $5);
        }
        $$ = node;
    }
    | CommonName DOT_CommonName TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        node->scope = $1;
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        if ($4) {
            ast_add_child(node, $4);
        }
        $$ = node;
    }
//    | UnannClassOrInterfaceType '.' TYPE_IDENTIFIER
    | UnannClassOrInterfaceType DOT TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        node->scope = $1;
        ast_add_child(node, $1);
        ast_add_child(node, $3);
        if ($4) {
            ast_add_child(node, $4);
        }
        $$ = node;
    }
    | UnannClassOrInterfaceType DOT Annotations TYPE_IDENTIFIER {
        AstNode *member = $4;
        if ($3) {
            ast_prepend_child(member, $3);
        }
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        node->scope = $1;
        ast_add_child(node, $1);
        ast_add_child(node, member);
        $$ = node;
    }
    | UnannClassOrInterfaceType DOT Annotations TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType {
        AstNode *member = $4;
        if ($3) {
            ast_prepend_child(member, $3);
        }
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, @$);
        node->scope = $1;
        ast_add_child(node, $1);
        ast_add_child(node, member);
        if ($5) {
            ast_add_child(node, $5);
        }
        $$ = node;
    }
;

// UnannClassOrInterfaceType 中实际包?UnannTypeVariable ，这里予以去?
UnannArrayType:
    UnannPrimitiveType Dims {
        AstNode *type = make_type_node($1, @$);
        if ($2) {
            ast_add_child(type, $2);
        }
        $$ = type;
    }
    | UnannClassOrInterfaceType Dims {
        AstNode *type = make_type_node($1, @$);
        if ($2) {
            ast_add_child(type, $2);
        }
        $$ = type;
    }
//    | UnannTypeVariable Dims
;

// 方法声明。
MethodDeclaration:
    Modifiers MethodHeader MethodBody {
        move_trailing_type_annotations($1, $2);
        if ($1 && $1->child_count > 0) {
            ast_prepend_child($2, $1);
        }
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

// 方法头：修饰符/类型/名称/参数/throws。
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
        if ($3 && $2) {
            ast_prepend_child($3, $2);
        }
        if ($3) {
            ast_prepend_child($4, $3);
        }
        if ($1) {
            ast_prepend_child($4, $1);
        }
        if ($5) {
            ast_add_child($4, $5);
        }
        $$ = $4;
    }
    | TypeParameters Annotations Result MethodDeclarator {
        if ($3 && $2) {
            ast_prepend_child($3, $2);
        }
        if ($3) {
            ast_prepend_child($4, $3);
        }
        if ($1) {
            ast_prepend_child($4, $1);
        }
        $$ = $4;
    }
    | TypeParameters Result MethodDeclarator Throws {
        if ($2) {
            ast_prepend_child($3, $2);
        }
        if ($1) {
            ast_prepend_child($3, $1);
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
        if ($1) {
            ast_prepend_child($3, $1);
        }
        $$ = $3;
    }
;

// 方法返回类型（类型/void）。
Result:
    UnannType { $$ = $1; }
    | VOID { $$ = make_type_node(make_keyword_leaf("void", @1), @1); }
;

// 方法声明子句：名称与参数列表。
MethodDeclarator:
    IDENTIFIER_MethodDeclarator '(' ReceiverParameter ',' FormalParameterList ')' Dims {
        AstNode *params = $5 ? $5 : make_list_node(AST_ARGUMENT_LIST, @$);
        ast_prepend_child(params, $3);
        $$ = make_method_signature($1, params, @$);
    }
    | IDENTIFIER_MethodDeclarator '(' ReceiverParameter ')' Dims {
        $$ = make_method_signature($1, make_params_list($3, @$), @$);
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
        AstNode *params = $5 ? $5 : make_list_node(AST_ARGUMENT_LIST, @$);
        ast_prepend_child(params, $3);
        $$ = make_method_signature($1, params, @$);
    }
    | IDENTIFIER_MethodDeclarator '(' ReceiverParameter ')' {
        $$ = make_method_signature($1, make_params_list($3, @$), @$);
    }
    | IDENTIFIER_MethodDeclarator '(' FormalParameterList ')' {
        $$ = make_method_signature($1, $3, @$);
    }
    | IDENTIFIER_MethodDeclarator '(' ')' {
        $$ = make_method_signature($1, NULL, @$);
    }
;

// 接收者参数（this）。
ReceiverParameter:
    Annotations UnannType TYPE_IDENTIFIER '.' THIS {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = make_receiver_parameter_node(mods, $2, $3, @5, @$);
    }
    | Annotations UnannType IdentifierComplement '.' THIS {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = make_receiver_parameter_node(mods, $2, $3, @5, @$);
    }
    | Annotations UnannType THIS {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = make_receiver_parameter_node(mods, $2, NULL, @3, @$);
    }
    | UnannType TYPE_IDENTIFIER '.' THIS {
        $$ = make_receiver_parameter_node(NULL, $1, $2, @4, @$);
    }
    | UnannType IdentifierComplement '.' THIS {
        $$ = make_receiver_parameter_node(NULL, $1, $2, @4, @$);
    }
    | UnannType THIS {
        $$ = make_receiver_parameter_node(NULL, $1, NULL, @2, @$);
    }
;

// 形参列表（含可变参数）。
FormalParameterList:
    FormalParameter {
        $$ = make_params_list($1, @$);
    }
    | FormalParameter ',' FormalParameterList {
        ast_prepend_child($3, $1);
        $$ = $3;
    }
;

// 这里为了避免冲突?VariableDeclaratorId 直接诠释为了 IDENTIFIER Dims / IDENTIFIER ;
FormalParameter:
//    VariableModifiers UnannType VariableDeclaratorId
//    | Annotations UnannType VariableDeclaratorId
//    | UnannType VariableDeclaratorId
    VariableModifiers UnannType TYPE_IDENTIFIER Dims {
        $$ = make_parameter_with_mods($1, $2, $3, $4, @$);
    }
    | Annotations UnannType TYPE_IDENTIFIER Dims {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = make_parameter_with_mods(mods, $2, $3, $4, @$);
    }
    | UnannType TYPE_IDENTIFIER Dims {
        $$ = make_parameter_with_dims($1, $2, $3, @$);
    }
    | VariableModifiers UnannType TYPE_IDENTIFIER {
        $$ = make_parameter_with_mods($1, $2, $3, NULL, @$);
    }
    | Annotations UnannType TYPE_IDENTIFIER {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = make_parameter_with_mods(mods, $2, $3, NULL, @$);
    }
    | UnannType TYPE_IDENTIFIER {
        $$ = make_parameter_node($1, $2, @$);
    }
    | VariableModifiers UnannType IdentifierComplement Dims {
        $$ = make_parameter_with_mods($1, $2, $3, $4, @$);
    }
    | Annotations UnannType IdentifierComplement Dims {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = make_parameter_with_mods(mods, $2, $3, $4, @$);
    }
    | UnannType IdentifierComplement Dims {
        $$ = make_parameter_with_dims($1, $2, $3, @$);
    }
    | VariableModifiers UnannType IdentifierComplement {
        $$ = make_parameter_with_mods($1, $2, $3, NULL, @$);
    }
    | Annotations UnannType IdentifierComplement {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = make_parameter_with_mods(mods, $2, $3, NULL, @$);
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

// 可变参数。
VariableArityParameter:
    VariableModifiers UnannType Annotations ELLIPSIS TYPE_IDENTIFIER {
        AstNode *mods = merge_param_modifiers($1, $3, @3);
        $$ = mark_varargs_parameter(make_parameter_with_mods(mods, $2, $5, NULL, @$));
    }
    | VariableModifiers UnannType ELLIPSIS TYPE_IDENTIFIER {
        $$ = mark_varargs_parameter(make_parameter_with_mods($1, $2, $4, NULL, @$));
    }
    | Annotations UnannType Annotations ELLIPSIS TYPE_IDENTIFIER {
        AstNode *mods = merge_param_modifiers(make_modifiers_from_annotations($1, @1), $3, @3);
        $$ = mark_varargs_parameter(make_parameter_with_mods(mods, $2, $5, NULL, @$));
    }
    | Annotations UnannType ELLIPSIS TYPE_IDENTIFIER {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = mark_varargs_parameter(make_parameter_with_mods(mods, $2, $4, NULL, @$));
    }
    | UnannType Annotations ELLIPSIS TYPE_IDENTIFIER {
        AstNode *mods = make_modifiers_from_annotations($2, @2);
        $$ = mark_varargs_parameter(make_parameter_with_mods(mods, $1, $4, NULL, @$));
    }
    | UnannType ELLIPSIS TYPE_IDENTIFIER {
        $$ = mark_varargs_parameter(make_parameter_node($1, $3, @$));
    }
    | VariableModifiers UnannType Annotations ELLIPSIS IdentifierComplement {
        AstNode *mods = merge_param_modifiers($1, $3, @3);
        $$ = mark_varargs_parameter(make_parameter_with_mods(mods, $2, $5, NULL, @$));
    }
    | VariableModifiers UnannType ELLIPSIS IdentifierComplement {
        $$ = mark_varargs_parameter(make_parameter_with_mods($1, $2, $4, NULL, @$));
    }
    | Annotations UnannType Annotations ELLIPSIS IdentifierComplement {
        AstNode *mods = merge_param_modifiers(make_modifiers_from_annotations($1, @1), $3, @3);
        $$ = mark_varargs_parameter(make_parameter_with_mods(mods, $2, $5, NULL, @$));
    }
    | Annotations UnannType ELLIPSIS IdentifierComplement {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = mark_varargs_parameter(make_parameter_with_mods(mods, $2, $4, NULL, @$));
    }
    | UnannType Annotations ELLIPSIS IdentifierComplement {
        AstNode *mods = make_modifiers_from_annotations($2, @2);
        $$ = mark_varargs_parameter(make_parameter_with_mods(mods, $1, $4, NULL, @$));
    }
    | UnannType ELLIPSIS IdentifierComplement {
        $$ = mark_varargs_parameter(make_parameter_node($1, $3, @$));
    }
;

// 变量修饰符列表。
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

// throws 子句。
Throws:
    THROWS ExceptionTypeList { $$ = make_throws_node($2, @$); }
;

// 异常类型列表。
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


// 异常类型。
ExceptionType:
    ClassOrInterfaceType
//    | TypeVariable      // 因为 ClassOrInterfaceType 实际包括 TypeVariable 的内容（ ClassOrInterfaceType: TypeVariable ），故而这里将这一行语句去?
;

// 方法体或分号。
MethodBody:
    Block { $$ = $1; }
    | ';' { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
;

// 实例初始化块。
InstanceInitializer:
    Block { $$ = wrap_labeled_block("instance-init", $1, @1); }
;

// 静态初始化块。
StaticInitializer:
    STATIC Block { $$ = wrap_labeled_block("static-init", $2, @1); }
;


// 构造器声明。
ConstructorDeclaration:
    Modifiers ConstructorDeclarator Throws ConstructorBody {
        if ($1) {
            ast_prepend_child($2, $1);
        }
        if ($3) {
            ast_add_child($2, $3);
        }
        if ($4) {
            ast_add_child($2, $4);
        }
        $$ = $2;
    }
    | Modifiers ConstructorDeclarator ConstructorBody {
        if ($1) {
            ast_prepend_child($2, $1);
        }
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

// 构造器声明子句。
ConstructorDeclarator:
    TypeParameters SimpleTypeName '(' ReceiverParameter ',' FormalParameterList ')' {
        AstNode *params = $6 ? $6 : make_list_node(AST_ARGUMENT_LIST, @$);
        ast_prepend_child(params, $4);
        AstNode *ctor = make_constructor_node($2, params, @$);
        if ($1) {
            ast_prepend_child(ctor, $1);
        }
        $$ = ctor;
    }
    | TypeParameters SimpleTypeName '(' ReceiverParameter ')' {
        AstNode *ctor = make_constructor_node($2, make_params_list($4, @$), @$);
        if ($1) {
            ast_prepend_child(ctor, $1);
        }
        $$ = ctor;
    }
    | TypeParameters SimpleTypeName '(' FormalParameterList ')' {
        AstNode *ctor = make_constructor_node($2, $4, @$);
        if ($1) {
            ast_prepend_child(ctor, $1);
        }
        $$ = ctor;
    }
    | TypeParameters SimpleTypeName '(' ')' {
        AstNode *ctor = make_constructor_node($2, NULL, @$);
        if ($1) {
            ast_prepend_child(ctor, $1);
        }
        $$ = ctor;
    }
    | SimpleTypeName '(' ReceiverParameter ',' FormalParameterList ')' {
        AstNode *params = $5 ? $5 : make_list_node(AST_ARGUMENT_LIST, @$);
        ast_prepend_child(params, $3);
        $$ = make_constructor_node($1, params, @$);
    }
    | SimpleTypeName '(' ReceiverParameter ')' {
        $$ = make_constructor_node($1, make_params_list($3, @$), @$);
    }
    | SimpleTypeName '(' FormalParameterList ')' {
        $$ = make_constructor_node($1, $3, @$);
    }
    | SimpleTypeName '(' ')' {
        $$ = make_constructor_node($1, NULL, @$);
    }
;

// 简单类型名。
SimpleTypeName:
    TYPE_IDENTIFIER_MethodDeclarator { $$ = $1; }
;

// 构造器体。
ConstructorBody:
    '{' ExplicitConstructorInvocation BlockStatements '}' {
        if ($2) {
            ast_prepend_child($3, $2);
        }
        $$ = make_block_node(@1, $3);
    }
    | '{' ExplicitConstructorInvocation '}' {
        AstNode *stmts = make_list_node(AST_STATEMENT_LIST, @1);
        if ($2) {
            ast_add_child(stmts, $2);
        }
        $$ = make_block_node(@1, stmts);
    }
    | '{' BlockStatements '}' {
        $$ = make_block_node(@1, $2);
    }
    | '{' '}' {
        $$ = make_block_node(@1, NULL);
    }
;

// 显式构造器调用（this/super）。
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

// 枚举声明与枚举常量。
EnumDeclaration:
    Modifiers ENUM TYPE_IDENTIFIER_EnumDeclaration Superinterfaces EnumBody {
        /* 4 个孩? 修饰? 名字, superinterfaces, body */
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


// 枚举体。
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

// 枚举常量列表。
EnumConstantList:
    EnumConstant {
        $$ = ast_branch(AST_STATEMENT_LIST, @1.first_line, @1.first_column, 1, $1);
    }
    | EnumConstantList ',' EnumConstant {
        ast_add_child($1, $3);
        $$ = $1;
    }
;


// 在新一次的迭代中，我们认为可以直接?EnumConstantModifiers 替换?Annotations
// 这里为了避免冲突?TYPE_IDENTIFIER 直接换成?SimpleTypeName
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

// 枚举体声明（常量列表后可选声明）。
EnumBodyDeclarations:
    ';' ClassBodyDeclarationList {
        $$ = $2;
    }
;



//-----------------------------------Interfaces-------------------------------------
// 接口与注解类型：接口声明、注解类型声明及成员。



// 接口声明入口。
InterfaceDeclaration:
    NormalInterfaceDeclaration { $$ = $1; }
    | AnnotationTypeDeclaration {
        $$ = $1;
    }
;

// 普通接口声明。
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


// 接口继承列表（extends ...）。
ExtendsInterfaces:
    EXTENDS InterfaceTypeList {
        $$ = $2;
    }
;

// 可选 extends 接口列表。
OptExtendsInterfaces:
    ExtendsInterfaces { $$ = $1; }
    | /* empty */ { $$ = NULL; }
;

// 接口体与成员列表。
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

// 接口成员声明。
InterfaceMemberDeclaration:
    ConstantDeclaration { $$ = $1; }
    | InterfaceMethodDeclaration { $$ = $1; }
    | ClassDeclaration { $$ = $1; }
    | InterfaceDeclaration { $$ = $1; }
    | ';' { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
    | EMPTY_STMT { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
;

// 接口常量声明。
ConstantDeclaration:
    Modifiers UnannType VariableDeclaratorList ';' {
        $$ = make_field_node($1, $2, $3, @2);
    }
  | UnannType VariableDeclaratorList ';' {
        $$ = make_field_node(NULL, $1, $2, @1);
    }
;


// 接口方法声明。
InterfaceMethodDeclaration:
    Modifiers MethodHeader MethodBody {
        if ($1) {
            ast_prepend_child($2, $1);
        }
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

// 在词法里区分 AnnotationTypeDeclaration ?'@' ?Annotation ?'@'
// 注解类型声明。
AnnotationTypeDeclaration:
    /* 原有规则保持不变 */
    Modifiers AT_AnnotationTypeDeclaration INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
        $$ = make_annotation_decl_basic(@3.first_line, @3.first_column, $1, $4, NULL, NULL, NULL, $5 ? $5 : ast_branch(AST_BLOCK, @3.first_line, @3.first_column, 0));
    }
    | AT_AnnotationTypeDeclaration INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
        $$ = make_annotation_decl_basic(@2.first_line, @2.first_column, NULL, $3, NULL, NULL, NULL, $4 ? $4 : ast_branch(AST_BLOCK, @2.first_line, @2.first_column, 0));
    }
    /* === 新增以下规则以兼?Lexer 返回的普?AT Token === */
    | Modifiers AT_Modifier INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
        $$ = make_annotation_decl_basic(@3.first_line, @3.first_column, $1, $4, NULL, NULL, NULL, $5 ? $5 : ast_branch(AST_BLOCK, @3.first_line, @3.first_column, 0));
    }
    | AT_Modifier INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
        $$ = make_annotation_decl_basic(@2.first_line, @2.first_column, NULL, $3, NULL, NULL, NULL, $4 ? $4 : ast_branch(AST_BLOCK, @2.first_line, @2.first_column, 0));
    }
    /* 如果你的 Lexer 返回的是字符 '@' 而不是宏定义，可以把下面这行也加?*/
    | Modifiers '@' INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
         $$ = make_annotation_decl_basic(@3.first_line, @3.first_column, $1, $4, NULL, NULL, NULL, $5 ? $5 : ast_branch(AST_BLOCK, @3.first_line, @3.first_column, 0));
    }
    | '@' INTERFACE TYPE_IDENTIFIER AnnotationTypeBody {
         $$ = make_annotation_decl_basic(@2.first_line, @2.first_column, NULL, $3, NULL, NULL, NULL, $4 ? $4 : ast_branch(AST_BLOCK, @2.first_line, @2.first_column, 0));
    }
;
// 注解类型体。
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

// 注解类型成员声明。
AnnotationTypeMemberDeclaration:
    AnnotationTypeElementDeclaration { $$ = $1; }
    | ConstantDeclaration { $$ = $1; }
    | ClassDeclaration { $$ = $1; }
    | InterfaceDeclaration { $$ = $1; }
    | ';' { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
    | EMPTY_STMT { $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column); }
;

// 注解元素声明。
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

// 注解元素默认值。
DefaultValue:
    DEFAULT ElementValue {
        $$ = AST_BRANCH_AT(AST_DEFAULT_VALUE, @1, 1, $2);
    }
;

// 注解入口。
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
        $$ = make_list_node(AST_ANNOTATION_LIST, @$);
        ast_add_child($$, $1);
    }
;

// 普通注解。
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

// 注解键值对列表。
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

// 注解键值对。
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

// 注解元素值。
ElementValue:
    ConditionalExpression
//    | ConditionalOrExpression
    | ElementValueArrayInitializer { $$ = $1; }
    | Annotation { $$ = $1; }
;

// 注解数组值初始化器。
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

// 注解元素值列表。
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

// 标记注解。
MarkerAnnotation:
    '@' TypeName {
        $$ = make_annotation_node($2, @1);
    }
;

// 单元素注解。
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
// 语句与块：控制流、声明语句、跳转语句、try/with/switch 等。
// 语句块与本地声明。
Block:
    '{' BlockStatements '}' {
        $$ = make_block_node(@1, $2);
    }
    | '{' '}' {
        $$ = make_block_node(@1, NULL);
    }
;

// 代码块语句列表。
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

// 代码块内的单条语句/声明。
BlockStatement:
    LocalVariableDeclarationStatement { $$ = $1; }
    | LocalClassOrInterfaceDeclaration { $$ = $1; }
    | Statement { $$ = $1; }
;

// 本地变量声明语句。
LocalVariableDeclarationStatement:
    LocalVariableDeclaration ';' { $$ = $1; }
;

// 本地变量声明。
LocalVariableDeclaration:
    VariableModifiers LocalVariableType VariableDeclaratorList {
        $$ = make_local_variable_node($1, $2, $3, @2);
    }
    | Annotations LocalVariableType VariableDeclaratorList {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = make_local_variable_node(mods, $2, $3, @2);
    }
    | LocalVariableType VariableDeclaratorList {
        $$ = make_local_variable_node(NULL, $1, $2, @1);
    }
;

// 本地变量类型。
LocalVariableType:
    UnannType { $$ = $1; }
    | VAR { $$ = make_type_node(make_keyword_leaf("var", @1), @1); }
;

// 本地类/接口声明。
LocalClassOrInterfaceDeclaration:
    ClassDeclaration { $$ = $1; }
    | NormalInterfaceDeclaration { $$ = $1; }
;

// 通用语句入口（含短 if 处理）。
Statement:
    StatementWithoutTrailingSubstatement { $$ = $1; }
    | LabeledStatement { $$ = $1; }
    | IfThenStatement { $$ = $1; }
    | IfThenElseStatement { $$ = $1; }
    | WhileStatement { $$ = $1; }
    | ForStatement { $$ = $1; }
;

// 无短 if 语句入口。
StatementNoShortIf:
    StatementWithoutTrailingSubstatement { $$ = $1; }
    | LabeledStatementNoShortIf { $$ = $1; }
    | IfThenElseStatementNoShortIf { $$ = $1; }
    | WhileStatementNoShortIf { $$ = $1; }
    | ForStatementNoShortIf { $$ = $1; }
;

// 无尾随子语句入口。
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

// 空语句。
EmptyStatement:
    EMPTY_STMT {
        $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column);
    }
    | ';' {
        $$ = ast_leaf(AST_EMPTY, ";", @1.first_line, @1.first_column);
    }
;

// 带标签语句。
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

// 无短 if 的带标签语句。
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

// 表达式语句。
ExpressionStatement:
    StatementExpression ';' {
        $$ = $1;
    }
;

// 语句中的表达式。
StatementExpression:
    Assignment { $$ = $1; }
    | PreIncrementExpression { $$ = $1; }
    | PreDecrementExpression { $$ = $1; }
    | PostIncrementExpression { $$ = $1; }
    | PostDecrementExpression { $$ = $1; }
    | MethodInvocation { $$ = $1; }
    | ClassInstanceCreationExpression { $$ = $1; }
;

// if/else 条件语句。
IfThenStatement:
    IF '(' Expression ')' Statement {
        $$ = make_ternary_stmt(AST_IF, $3, $5, NULL, @1);
    }
;

// if-else 语句。
IfThenElseStatement:
    IF '(' Expression ')' StatementNoShortIf ELSE Statement {
        AstNode *else_clause = make_else_clause_node($7, @6);
        $$ = make_ternary_stmt(AST_IF, $3, $5, else_clause, @1);
    }
;

// 无短 if 的 if-else 语句。
IfThenElseStatementNoShortIf:
    IF '(' Expression ')' StatementNoShortIf ELSE StatementNoShortIf {
        AstNode *else_clause = make_else_clause_node($7, @6);
        $$ = make_ternary_stmt(AST_IF, $3, $5, else_clause, @1);
    }
;

// assert 断言语句。
AssertStatement:
    ASSERT Expression ';' {
        $$ = make_unary_stmt(AST_ASSERT, $2, @1);
    }
    | ASSERT Expression ':' Expression ';' {
        AstNode *node = make_binary_stmt(AST_ASSERT, $2, $4, @1);
        $$ = node;
    }
;

// switch 语句与规则。
SwitchStatement:
    SWITCH '(' Expression ')' SwitchBlock {
        AstNode *node = make_unary_stmt(AST_SWITCH, $3, @1);
        if ($5) {
            ast_add_child(node, $5);
        }
        $$ = node;
    }
;

// switch 块。
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

// switch 规则（箭头形式）。
SwitchRule:
    SwitchLabel ARROW Expression ';' {
        AstNode *labels = make_list_node(AST_SWITCH_LABEL_LIST, @1);
        ast_add_child(labels, $1);
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_RULE, @1, 0);
        ast_add_child(node, labels);
        if ($3) {
            ast_add_child(node, $3);
        }
        $$ = node;
    }
    | SwitchLabel ARROW Block {
        AstNode *labels = make_list_node(AST_SWITCH_LABEL_LIST, @1);
        ast_add_child(labels, $1);
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_RULE, @1, 0);
        ast_add_child(node, labels);
        if ($3) {
            ast_add_child(node, $3);
        }
        $$ = node;
    }
    | SwitchLabel ARROW ThrowStatement {
        AstNode *labels = make_list_node(AST_SWITCH_LABEL_LIST, @1);
        ast_add_child(labels, $1);
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_RULE, @1, 0);
        ast_add_child(node, labels);
        if ($3) {
            ast_add_child(node, $3);
        }
        $$ = node;
    }
;

// 允许使用一个或者多个SwitchRule
SwitchRules:
    SwitchRule {
        $$ = make_list_node(AST_SWITCH_RULE_LIST, @$);
        ast_add_child($$, $1);
    }
    | SwitchRules SwitchRule {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

// switch 语句组。
SwitchBlockStatementGroup:
    SwitchLabels BlockStatements {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_GROUP, @1, 0);
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

// switch 标签。
SwitchLabel:
    CASE CaseConstants {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_LABEL, @1, 0);
        ast_add_child(node, $2);
        $$ = node;
    }
    | DEFAULT_SwitchLabel {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_LABEL, @1, 0);
        ast_add_child(node, make_keyword_leaf("default", @1));
        $$ = node;
    }
;

// 允许使用一个或者多个SwitchLabel
SwitchLabels:
    SwitchLabel ':' {
        AstNode *node = make_list_node(AST_SWITCH_LABEL_LIST, @$);
        ast_add_child(node, $1);
        $$ = node;
    }
    | SwitchLabels SwitchLabel ':' {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

// case 常量。
CaseConstant:
    ConditionalExpression { $$ = $1; }
;

// case 常量列表。
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

// while 循环。
WhileStatement:
    WHILE '(' Expression ')' Statement {
        $$ = make_binary_stmt(AST_WHILE, $3, $5, @1);
    }
;

// 无短 if 的 while 语句。
WhileStatementNoShortIf:
    WHILE '(' Expression ')' StatementNoShortIf {
        $$ = make_binary_stmt(AST_WHILE, $3, $5, @1);
    }
;

// do-while 循环。
DoStatement:
    DO Statement WHILE '(' Expression ')' ';' {
        AstNode *node = make_binary_stmt(AST_DO_WHILE, $2, $5, @1);
        $$ = node;
    }
;

// for 循环（基本/增强）。
ForStatement:
    BasicForStatement { $$ = $1; }
    | EnhancedForStatement { $$ = $1; }
;

// 无短 if 的 for 语句入口。
ForStatementNoShortIf:
    BasicForStatementNoShortIf { $$ = $1; }
    | EnhancedForStatementNoShortIf { $$ = $1; }
;

// 传统 for 语句。
BasicForStatement:
    FOR '(' ForInit ';' Expression ';' ForUpdate ')' Statement {
        $$ = make_for_stmt($3, $5, $7, $9, @1);
    }
    | FOR '(' ForInit ';' Expression ';' ')' Statement {
        $$ = make_for_stmt($3, $5, NULL, $8, @1);
    }
    | FOR '(' ForInit ';' ';' ForUpdate ')' Statement {
        $$ = make_for_stmt($3, NULL, $6, $8, @1);
    }
    | FOR '(' ForInit ';' ';' ')' Statement {
        $$ = make_for_stmt($3, NULL, NULL, $7, @1);
    }
    | FOR '(' ';' Expression ';' ForUpdate ')' Statement {
        $$ = make_for_stmt(NULL, $4, $6, $8, @1);
    }
    | FOR '(' ';' Expression ';' ')' Statement {
        $$ = make_for_stmt(NULL, $4, NULL, $7, @1);
    }
    | FOR '(' ';' ';' ForUpdate ')' Statement {
        $$ = make_for_stmt(NULL, NULL, $5, $7, @1);
    }
    | FOR '(' ';' ';' ')' Statement {
        $$ = make_for_stmt(NULL, NULL, NULL, $6, @1);
    }
;

// 无短 if 的传统 for 语句。
BasicForStatementNoShortIf:
    FOR '(' ForInit ';' Expression ';' ForUpdate ')' StatementNoShortIf {
        $$ = make_for_stmt($3, $5, $7, $9, @1);
    }
    | FOR '(' ForInit ';' Expression ';' ')' StatementNoShortIf {
        $$ = make_for_stmt($3, $5, NULL, $8, @1);
    }
    | FOR '(' ForInit ';' ';' ForUpdate ')' StatementNoShortIf {
        $$ = make_for_stmt($3, NULL, $6, $8, @1);
    }
    | FOR '(' ForInit ';' ';' ')' StatementNoShortIf {
        $$ = make_for_stmt($3, NULL, NULL, $7, @1);
    }
    | FOR '(' ';' Expression ';' ForUpdate ')' StatementNoShortIf {
        $$ = make_for_stmt(NULL, $4, $6, $8, @1);
    }
    | FOR '(' ';' Expression ';' ')' StatementNoShortIf {
        $$ = make_for_stmt(NULL, $4, NULL, $7, @1);
    }
    | FOR '(' ';' ';' ForUpdate ')' StatementNoShortIf {
        $$ = make_for_stmt(NULL, NULL, $5, $7, @1);
    }
    | FOR '(' ';' ';' ')' StatementNoShortIf {
        $$ = make_for_stmt(NULL, NULL, NULL, $6, @1);
    }
;

// for 初始化部分。
ForInit:
    StatementExpressionList { $$ = relabel_list($1, AST_FOR_INIT_LIST); }
    | LocalVariableDeclaration { $$ = $1; }
;

// for 更新部分。
ForUpdate:
    StatementExpressionList { $$ = relabel_list($1, AST_FOR_UPDATE_LIST); }
;

// 语句表达式列表。
StatementExpressionList:
    StatementExpression {
        $$ = make_list_node(AST_EXPRESSION_LIST, @$);
        if ($1) ast_add_child($$, $1);
    }
    | StatementExpressionList ',' StatementExpression {
        if ($3) {
            ast_add_child($1, $3);
        }
        $$ = $1;
    }
;

// 这里为了避免冲突?VariableDeclaratorId 直接诠释为了 IDENTIFIER Dims / IDENTIFIER ;
// 增强 for (for-each) 循环。
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

// 这里为了避免冲突?VariableDeclaratorId 直接诠释为了 IDENTIFIER Dims / IDENTIFIER ;
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

// 跳转语句：break/continue/return/throw/yield。
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

// yield 语句。
YieldStatement:
    YIELD Expression ';' {
        $$ = make_unary_stmt(AST_YIELD, $2, @1);
    }
;

// continue 语句。
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

// return 语句。
ReturnStatement:
    RETURN Expression ';' {
        $$ = make_unary_stmt(AST_RETURN, $2, @1);
    }
    | RETURN ';' {
        $$ = make_simple_stmt(AST_RETURN, @1);
    }
;

// throw 语句。
ThrowStatement:
    THROW Expression ';' {
        $$ = make_unary_stmt(AST_THROW, $2, @1);
    }
;

// synchronized 语句。
SynchronizedStatement:
    SYNCHRONIZED '(' Expression ')' Block {
        $$ = make_binary_stmt(AST_SYNCHRONIZED, $3, $5, @1);
    }
;

// try/catch/finally/with-resources。
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

// catch 列表。
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

// 单个 catch 子句。
CatchClause:
    CATCH '(' CatchFormalParameter ')' Block {
        AstNode *node = AST_BRANCH_AT(AST_CATCH, @1, 0);
        if ($3) ast_add_child(node, $3);
        if ($5) ast_add_child(node, $5);
        $$ = node;
    }
;

// 这里为了避免冲突?VariableDeclaratorId 直接诠释为了 IDENTIFIER Dims / IDENTIFIER ;
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

// catch 异常类型。
CatchType:
    UnannClassOrInterfaceType {
        AstNode *list = make_list_node(AST_EXCEPTION_TYPE_LIST, @$);
        ast_add_child(list, make_type_node($1, @$));
        $$ = list;
    }
    | CatchType '|' UnannClassOrInterfaceType {
        if ($1) {
            ast_add_child($1, make_type_node($3, @3));
        }
        $$ = $1;
    }
;

// finally 子句。
Finally:
    FINALLY Block {
        AstNode *node = AST_BRANCH_AT(AST_FINALLY, @1, 0);
        if ($2) ast_add_child(node, $2);
        $$ = node;
    }
;

// try-with-resources 语句。
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

// 资源声明列表（try-with-resources）。
ResourceSpecification:
    '(' ResourceList ')' {
        $$ = AST_BRANCH_AT(AST_RESOURCE_SPEC, @$, 1, $2);
    }
  | '(' ResourceList ';' ')' {
        $$ = AST_BRANCH_AT(AST_RESOURCE_SPEC, @$, 1, $2);
    }
;


// 资源列表。
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


// 单个资源声明。
Resource:
    VariableModifiers LocalVariableType TYPE_IDENTIFIER '=' Expression {
        AstNode *decl = make_resource_decl($1, $2, $3, $5, @$);
        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, decl);
    }
  | Annotations LocalVariableType TYPE_IDENTIFIER '=' Expression {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        AstNode *decl = make_resource_decl(mods, $2, $3, $5, @$);
        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, decl);
    }
  | LocalVariableType TYPE_IDENTIFIER '=' Expression {
        AstNode *decl = make_resource_decl(NULL, $1, $2, $4, @$);
        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, decl);
    }
  | VariableModifiers LocalVariableType IdentifierComplement '=' Expression {
        AstNode *decl = make_resource_decl($1, $2, $3, $5, @$);
        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, decl);
    }
  | Annotations LocalVariableType IdentifierComplement '=' Expression {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        AstNode *decl = make_resource_decl(mods, $2, $3, $5, @$);
        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, decl);
    }
  | LocalVariableType IdentifierComplement '=' Expression {
        AstNode *decl = make_resource_decl(NULL, $1, $2, $4, @$);
        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, decl);
    }
  | VariableAccess {
        $$ = AST_BRANCH_AT(AST_RESOURCE, @$, 1, $1);
    }
;

// 变量访问表达式。
VariableAccess:
    CommonName { $$ = $1; }
    | FieldAccess { $$ = $1; }
;

// 模式匹配入口。
Pattern:
    TypePattern { $$ = $1; }
//    | RecordPattern
;

/*
`TypePattern`是一个局部变量声明，但它不允许包含多个变量声明（即不允许使用逗号分隔的变量声明列表）
*/
// 类型模式（instanceof 模式）。
TypePattern:
    LocalVariableDeclaration_TypePattern { $$ = $1; }
;

// 本地变量声明的类型模式。
LocalVariableDeclaration_TypePattern:
    VariableModifiers LocalVariableType VariableDeclarator {
        $$ = make_type_pattern_node($1, $2, $3, @$);
    }
    | Annotations LocalVariableType VariableDeclarator {
        AstNode *mods = make_modifiers_from_annotations($1, @1);
        $$ = make_type_pattern_node(mods, $2, $3, @$);
    }
    | LocalVariableType VariableDeclarator {
        $$ = make_type_pattern_node(NULL, $1, $2, @$);
    }
;

//------------------------------------Expressions-----------------------------------
// 表达式：赋值、逻辑、方法调用、构造、lambda 等。
// 基本表达式：字面量/this/super/括号等。
Primary:
    PrimaryNoNewArray { $$ = $1; }
    | ArrayCreationExpression { $$ = $1; }
;

// 基本表达式（不含新建数组）。
PrimaryNoNewArray:
    Literal { $$ = $1; }
    | ClassLiteral { $$ = $1; }
    | THIS {
        $$ = make_this_expr_node(NULL, @1);
    }
    | CommonName '.' THIS {
        $$ = make_this_expr_node($1, @3);
    }
//    | TypeName '.' THIS
    | '(' Expression ')' { $$ = $2; }
    | ClassInstanceCreationExpression { $$ = $1; }
    | FieldAccess { $$ = $1; }
    | ArrayAccess { $$ = $1; }
    | MethodInvocation { $$ = $1; }
    | MethodReference { $$ = $1; }
;

// class 字面量表达式。
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

// 专门为了ClassLiteral 引进一个或者多'[]'
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

// new 创建对象与匿名类。
ClassInstanceCreationExpression:
    UnqualifiedClassInstanceCreationExpression { $$ = $1; }
    | CommonName '.' UnqualifiedClassInstanceCreationExpression {
        ast_prepend_child($3, $1);
        $3->scope = $1;
        $$ = $3;
    }
    | Primary '.' UnqualifiedClassInstanceCreationExpression {
        ast_prepend_child($3, $1);
        $3->scope = $1;
        $$ = $3;
    }
;

// 非限定类实例创建。
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

// 可实例化的类/接口类型。
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

// 专门为了ClassOrInterfaceTypeToInstantiate 中引入一个或者多Annotations Identifier ，用'.'隔开
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

// 类型实参或菱形 <>。
TypeArgumentsOrDiamond:
    TypeArguments {
        $$ = $1;
    }  //显式指定类型参数
    | DIAMOND {
        AstNode *list = make_list_node(AST_TYPE_ARGUMENT_LIST, @1);
        ast_set_text(list, "<>");
        $$ = list;
    }      //钻石操作?
;

// 字段/成员访问。
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

// 数组访问 a[i]。
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
// 方法调用（含链式与超类调用）。
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
// 这里CommonName 替换 TypeName 实际上扩宽了语义
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

// 实参列表。
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

// 方法引用 :: 形式。
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
// 以下注释掉的部分是一个扩宽语义的部分，如果上面的审查方法不好使可以考虑注释掉从本行起上数三行（不含本行），并采用如下的扩宽语义法（经检测效果一致，应该不会导致新的冲突已采用此备案)
    | ReferenceType DOUBLE_COLON TypeArguments NEW {
        $$ = make_method_reference_node($1, $3, make_keyword_leaf("new", @4), @2);
    }
    | ReferenceType DOUBLE_COLON NEW {
        $$ = make_method_reference_node($1, NULL, make_keyword_leaf("new", @3), @2);
    }
;

// new 创建数组。
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
        $$ = make_list_node(AST_DIM_EXPR_LIST, @$);
        ast_add_child($$, $1);
    }
    | DimExprs DimExpr {
        ast_add_child($1, $2);
        $$ = $1;
    }
;

// 维度表达式。
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

/* 表达式与 lambda 相关产生式。 */
// 表达式入口（含 lambda/赋值/条件等）。
Expression:
    LambdaExpression { $$ = $1; }
    | AssignmentExpression { $$ = $1; }
//    ExpressionWithoutTernary
//    | ConditionalExpression
;

// lambda 表达式：参数 -> 体。
LambdaExpression:
    LambdaParameters ARROW LambdaBody {
        $$ = make_lambda_node($1, $3, @2);
    }
;

// %token LPAREN_LambdaParameters            // LambdaParameters 中的 '(' 符号（之前有冲突，调试后发现冲突意外没了，故建议保留以便后续研究）
LambdaParameters:
    '(' LambdaParameterList ')' {
        $$ = $2;
    } // LambdaParameters 中的 ')' 符号
    | '(' ')' {
        $$ = make_list_node(AST_ARGUMENT_LIST, @$);
    } // LambdaParameters 中的 ')' 符号
    | TYPE_IDENTIFIER_IdentifierforLambdaParameterList { $$ = $1; }
    | IdentifierComplement_IdentifierforLambdaParameterList { $$ = $1; }
// %token LPAREN_LambdaParameters            // LambdaParameters 中的 '(' 符号（之前有冲突，调试后发现冲突意外没了，故建议保留以便后续研究）
// %token LPAREN_LambdaParameters            // LambdaParameters 中的 '(' 符号（之前有冲突，调试后发现冲突意外没了，故建议保留以便后续研究）
//    | IDENTIFIER_LambdaParameters
;

// lambda 参数列表。
LambdaParameterList:
    LambdaParameterforLambdaParameterList { $$ = $1; }
    | IdentifierforLambdaParameterList { $$ = $1; }
;

// lambda 标识符参数列表。
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

// lambda 标识符补全列表。
IdentifierComplement_IdentifierforLambdaParameterList:
    VAR_IdentifierComplement_IdentifierforLambdaParameterList { $$ = $1; }
    | YIELD_IdentifierComplement_IdentifierforLambdaParameterList { $$ = $1; }
;

// lambda 参数列表项。
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

// 单个 lambda 参数。
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

// lambda 参数类型。
LambdaParameterType:
    UnannType { $$ = $1; }
    | VAR { $$ = make_keyword_leaf("var", @1); }
;

// lambda 体（表达式或块）。
LambdaBody:
    Expression { $$ = $1; }
    | Block { $$ = $1; }
;

// 赋值表达式入口（含条件表达式）。
AssignmentExpression:
    ConditionalExpression { $$ = $1; }
    | Assignment { $$ = $1; }
;

// 赋值表达式。
Assignment:
    LeftHandSide AssignmentOperator Expression {
        $$ = make_assignment_node($1, $2, $3, @2);
    }
;

// 赋值左值。
LeftHandSide:
    CommonName { $$ = $1; }
    | FieldAccess { $$ = $1; }
    | ArrayAccess { $$ = $1; }
;

// 赋值运算符。
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

// 三元条件表达式。
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

// 逻辑或表达式。
ConditionalOrExpression:
    ConditionalAndExpression %prec PREC_ConditionalOrExpression_1 {
        $$ = $1;
    }
    | ConditionalOrExpression OR ConditionalAndExpression   // %prec PREC_ConditionalOrExpression
    {
        $$ = make_binary_expr($1, "||", $3, @2);
    }
;

// 逻辑与表达式。
ConditionalAndExpression:
    InclusiveOrExpression %prec PREC_ConditionalAndExpression_1 {
        $$ = $1;
    }
    | ConditionalAndExpression AND InclusiveOrExpression %prec PREC_ConditionalAndExpression {
        $$ = make_binary_expr($1, "&&", $3, @2);
    }
;

// 按位或表达式。
InclusiveOrExpression:
    ExclusiveOrExpression %prec PREC_InclusiveOrExpression_1 {
        $$ = $1;
    }
    | InclusiveOrExpression '|' ExclusiveOrExpression %prec PREC_InclusiveOrExpression {
        $$ = make_binary_expr($1, "|", $3, @2);
    }
;

// 按位异或表达式。
ExclusiveOrExpression:
    AndExpression %prec PREC_ExclusiveOrExpression_1 {
        $$ = $1;
    }
    | ExclusiveOrExpression '^' AndExpression %prec PREC_ExclusiveOrExpression {
        $$ = make_binary_expr($1, "^", $3, @2);
    }
;

// 按位与表达式。
AndExpression:
    EqualityExpression %prec PREC_AndExpression_1 {
        $$ = $1;
    }
    | AndExpression '&' EqualityExpression %prec PREC_AndExpression {
        $$ = make_binary_expr($1, "&", $3, @2);
    }
;

// 相等性表达式。
EqualityExpression:
    RelationalExpression { $$ = $1; }
    | EqualityExpression EQ RelationalExpression %prec PREC_EqualityExpression {
        $$ = make_binary_expr($1, "==", $3, @2);
    }
    | EqualityExpression NE RelationalExpression %prec PREC_EqualityExpression {
        $$ = make_binary_expr($1, "!=", $3, @2);
    }
;

// 基础部分（不可递归扩展
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
        $$ = make_instanceof_expr($1, $3, @2);
    } // 仅允许单?`instanceof`，下?
    | SimpleRelationalExpression INSTANCEOF Pattern %prec PREC_RelationalExpression {
        $$ = make_instanceof_expr($1, $3, @2);
    } // 仅允许单?`instanceof`，下?
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

// 移位表达式。
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

// 加减表达式。
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

// 乘除模表达式。
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

// 一元表达式。
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

// 前置自增表达式。
PreIncrementExpression:
    PREFIX_INC UnaryExpression %prec PREFIX_INC {
        $$ = make_unary_expr("++", $2, @1, false);
    }
;

// 前置自减表达式。
PreDecrementExpression:
    PREFIX_DEC UnaryExpression %prec PREFIX_DEC {
        $$ = make_unary_expr("--", $2, @1, false);
    }
;

// 非正负号一元表达式。
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

// 后缀表达式。
PostfixExpression:
    Primary { $$ = $1; }
    | CommonName      // ?ExpressionName
    {
        $$ = $1;
    }
    | PostIncrementExpression { $$ = $1; }
    | PostDecrementExpression { $$ = $1; }
;

// 后置自增表达式。
PostIncrementExpression:
    PostfixExpression INC_OP %prec INC_OP {
        $$ = make_unary_expr("++", $1, @2, true);
    }
;

// 后置自减表达式。
PostDecrementExpression:
    PostfixExpression DEC_OP %prec DEC_OP {
        $$ = make_unary_expr("--", $1, @2, true);
    }
;

// 强制类型转换。
CastExpression:
    '(' PrimitiveType ')' UnaryExpression {
        $$ = make_cast_expr($2, $4, @1);
    }
    | '(' UnannPrimitiveType ')' UnaryExpression {
        $$ = make_cast_expr(make_type_node($2, @2), $4, @1);
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

// switch 表达式。
SwitchExpression:
    SWITCH '(' Expression ')' SwitchBlock {
        AstNode *node = make_unary_stmt(AST_SWITCH_EXPR, $3, @1);
        if ($5) {
            ast_add_child(node, $5);
        }
        $$ = node;
    }
;


%%

/* 尾部实现：语法动作中引用的辅助函数。 */
// 修改 make_class_basic 以接收所有组
// 构造类声明基础节点。
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
        // 可以选择在这里加一keyword leaf "extends" 或者直接加节点
        AstNode *clause = make_clause_node(AST_EXTENDS, super_class, line, column);
        ast_add_child(node, clause);
    }
    // 5. 添加接口 (implements A, B)
    if (super_interfaces) {
        AstNode *clause = make_clause_node(AST_IMPLEMENTS, super_interfaces, line, column);
        ast_add_child(node, clause);
    }
    // 6. 添加 permits (sealed class)
    if (permits) {
        AstNode *clause = make_clause_node(AST_PERMITS, permits, line, column);
        ast_add_child(node, clause);
    }
    // 7. 添加类体 ({...})
    if (body) {
        ast_add_child(node, body);
    }
    return node;
}

// 根据标签创建对应的块节点（普通/静态/实例初始化）。
static AstNode *wrap_labeled_block(const char *label, AstNode *block, YYLTYPE loc) {
    AstKind kind = AST_BLOCK;
    if (label) {
        if (strcmp(label, "static-init") == 0) {
            kind = AST_STATIC_INIT;
        } else if (strcmp(label, "instance-init") == 0) {
            kind = AST_INSTANCE_INIT;
        }
    }
    if (kind == AST_BLOCK) {
        return block ? block : AST_BRANCH_AT(AST_BLOCK, loc, 0);
    }
    AstNode *n = AST_BRANCH_AT(kind, loc, 0);
    if (block) {
        ast_add_child(n, block);
    }
    return n;
}



// 构造接口声明基础节点。
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
    if (extends_interfaces) {
        AstNode *clause = make_clause_node(AST_EXTENDS, extends_interfaces, line, column);
        ast_add_child(node, clause);
    }
    if (permits) {
        AstNode *clause = make_clause_node(AST_PERMITS, permits, line, column);
        ast_add_child(node, clause);
    }
    if (body) ast_add_child(node, body);
    
    return node;
}

// 构造注解类型声明基础节点。
static AstNode *make_annotation_decl_basic(int line, int column, 
                                           AstNode *modifiers,
                                           AstNode *name, 
                                           AstNode *type_params,
                                           AstNode *extends_interfaces,
                                           AstNode *permits,
                                           AstNode *body) {
    AstNode *node = make_interface_basic(line, column,
                                         modifiers,
                                         name,
                                         type_params,
                                         extends_interfaces,
                                         permits,
                                         body);
    if (node) {
        node->kind = AST_ANNOTATION_DECL;
    }
    return node;
}

// 构造注解节点。
static AstNode *make_annotation_node(AstNode *name, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_ANNOTATION, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (name) {
        ast_add_child(node, name);
    }
    return node;
}

// 构造类型节点（必要时包一层 AST_TYPE）。
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

// 构造类型形参节点。
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

// 构造类型实参：类型。
static AstNode *make_type_argument_type_node(AstNode *type_node, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_TYPE_ARGUMENT_TYPE, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (type_node) {
        ast_add_child(node, type_node);
    }
    return node;
}

// 构造类型实参：通配符。
static AstNode *make_type_argument_wildcard_node(AstNode *wildcard_node, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_TYPE_ARGUMENT_WILDCARD, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (wildcard_node) {
        ast_add_child(node, wildcard_node);
    }
    return node;
}

// 构造泛型边界节点（extends）。
static AstNode *make_type_bound_node(const char *kind, AstNode *primary, AstNode *additional, YYLTYPE loc) {
    AstNode *node = ast_branch(AST_TYPE_BOUND, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
    if (kind) {
        ast_set_text(node, kind);
    }
    if (primary) {
        ast_add_child(node, primary);
    }
    if (additional) {
        ast_add_child(node, additional);
    }
    return node;
}

// 构造维度节点。
static AstNode *make_dim_node(YYLTYPE loc) {
    return ast_branch(AST_DIM, AST_LOC_LINE(loc), AST_LOC_COL(loc), 0);
}
