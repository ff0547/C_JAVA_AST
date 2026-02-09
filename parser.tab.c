/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 2 "parser.y"

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

#line 983 "parser.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_AMPERSAND = 3,                  /* AMPERSAND  */
  YYSYMBOL_RPAREN_CastExpression = 4,      /* RPAREN_CastExpression  */
  YYSYMBOL_TYPE_IDENTIFIER_IdentifierforLambdaParameterList = 5, /* TYPE_IDENTIFIER_IdentifierforLambdaParameterList  */
  YYSYMBOL_VAR_IdentifierComplement_IdentifierforLambdaParameterList = 6, /* VAR_IdentifierComplement_IdentifierforLambdaParameterList  */
  YYSYMBOL_YIELD_IdentifierComplement_IdentifierforLambdaParameterList = 7, /* YIELD_IdentifierComplement_IdentifierforLambdaParameterList  */
  YYSYMBOL_TYPE_IDENTIFIER_MethodDeclarator = 8, /* TYPE_IDENTIFIER_MethodDeclarator  */
  YYSYMBOL_VAR_MethodDeclarator = 9,       /* VAR_MethodDeclarator  */
  YYSYMBOL_YIELD_MethodDeclarator = 10,    /* YIELD_MethodDeclarator  */
  YYSYMBOL_TYPE_IDENTIFIER_AnnotationIdentifiers = 11, /* TYPE_IDENTIFIER_AnnotationIdentifiers  */
  YYSYMBOL_VAR_AnnotationIdentifiers = 12, /* VAR_AnnotationIdentifiers  */
  YYSYMBOL_YIELD_AnnotationIdentifiers = 13, /* YIELD_AnnotationIdentifiers  */
  YYSYMBOL_TYPE_IDENTIFIER = 14,           /* TYPE_IDENTIFIER  */
  YYSYMBOL_TYPE_IDENTIFIER_EnumDeclaration = 15, /* TYPE_IDENTIFIER_EnumDeclaration  */
  YYSYMBOL_LANGLE = 16,                    /* LANGLE  */
  YYSYMBOL_AT_Dims = 17,                   /* AT_Dims  */
  YYSYMBOL_AT_Modifier = 18,               /* AT_Modifier  */
  YYSYMBOL_AT_AnnotationTypeDeclaration = 19, /* AT_AnnotationTypeDeclaration  */
  YYSYMBOL_LBRACK = 20,                    /* LBRACK  */
  YYSYMBOL_LBRACK_ArrayAccess = 21,        /* LBRACK_ArrayAccess  */
  YYSYMBOL_DEFAULT_SwitchLabel = 22,       /* DEFAULT_SwitchLabel  */
  YYSYMBOL_DOT = 23,                       /* DOT  */
  YYSYMBOL_DOT_CommonName = 24,            /* DOT_CommonName  */
  YYSYMBOL_VAR_IdentifierComplement = 25,  /* VAR_IdentifierComplement  */
  YYSYMBOL_YIELD_IdentifierComplement = 26, /* YIELD_IdentifierComplement  */
  YYSYMBOL_CLASS = 27,                     /* CLASS  */
  YYSYMBOL_BYTE = 28,                      /* BYTE  */
  YYSYMBOL_INT = 29,                       /* INT  */
  YYSYMBOL_SHORT = 30,                     /* SHORT  */
  YYSYMBOL_LONG = 31,                      /* LONG  */
  YYSYMBOL_CHAR = 32,                      /* CHAR  */
  YYSYMBOL_FLOAT = 33,                     /* FLOAT  */
  YYSYMBOL_DOUBLE = 34,                    /* DOUBLE  */
  YYSYMBOL_EXTENDS = 35,                   /* EXTENDS  */
  YYSYMBOL_SUPER = 36,                     /* SUPER  */
  YYSYMBOL_OPEN = 37,                      /* OPEN  */
  YYSYMBOL_OPENS = 38,                     /* OPENS  */
  YYSYMBOL_REQUIRES = 39,                  /* REQUIRES  */
  YYSYMBOL_EXPORTS = 40,                   /* EXPORTS  */
  YYSYMBOL_USES = 41,                      /* USES  */
  YYSYMBOL_PROVIDES = 42,                  /* PROVIDES  */
  YYSYMBOL_PACKAGE = 43,                   /* PACKAGE  */
  YYSYMBOL_IMPORT = 44,                    /* IMPORT  */
  YYSYMBOL_PUBLIC = 45,                    /* PUBLIC  */
  YYSYMBOL_PROTECTED = 46,                 /* PROTECTED  */
  YYSYMBOL_PRIVATE = 47,                   /* PRIVATE  */
  YYSYMBOL_ABSTRACT = 48,                  /* ABSTRACT  */
  YYSYMBOL_STATIC = 49,                    /* STATIC  */
  YYSYMBOL_FINAL = 50,                     /* FINAL  */
  YYSYMBOL_STRICTFP = 51,                  /* STRICTFP  */
  YYSYMBOL_TRANSITIVE = 52,                /* TRANSITIVE  */
  YYSYMBOL_IMPLEMENTS = 53,                /* IMPLEMENTS  */
  YYSYMBOL_TRANSIENT = 54,                 /* TRANSIENT  */
  YYSYMBOL_VOLATILE = 55,                  /* VOLATILE  */
  YYSYMBOL_SYNCHRONIZED = 56,              /* SYNCHRONIZED  */
  YYSYMBOL_NATIVE = 57,                    /* NATIVE  */
  YYSYMBOL_BOOLEAN = 58,                   /* BOOLEAN  */
  YYSYMBOL_VOID = 59,                      /* VOID  */
  YYSYMBOL_THROW = 60,                     /* THROW  */
  YYSYMBOL_THROWS = 61,                    /* THROWS  */
  YYSYMBOL_THIS = 62,                      /* THIS  */
  YYSYMBOL_ENUM = 63,                      /* ENUM  */
  YYSYMBOL_INTERFACE = 64,                 /* INTERFACE  */
  YYSYMBOL_DEFAULT = 65,                   /* DEFAULT  */
  YYSYMBOL_VAR = 66,                       /* VAR  */
  YYSYMBOL_IF = 67,                        /* IF  */
  YYSYMBOL_ELSE = 68,                      /* ELSE  */
  YYSYMBOL_ASSERT = 69,                    /* ASSERT  */
  YYSYMBOL_SWITCH = 70,                    /* SWITCH  */
  YYSYMBOL_CASE = 71,                      /* CASE  */
  YYSYMBOL_DO = 72,                        /* DO  */
  YYSYMBOL_WHILE = 73,                     /* WHILE  */
  YYSYMBOL_FOR = 74,                       /* FOR  */
  YYSYMBOL_BREAK = 75,                     /* BREAK  */
  YYSYMBOL_YIELD = 76,                     /* YIELD  */
  YYSYMBOL_CONTINUE = 77,                  /* CONTINUE  */
  YYSYMBOL_RETURN = 78,                    /* RETURN  */
  YYSYMBOL_TRY = 79,                       /* TRY  */
  YYSYMBOL_CATCH = 80,                     /* CATCH  */
  YYSYMBOL_FINALLY = 81,                   /* FINALLY  */
  YYSYMBOL_NEW = 82,                       /* NEW  */
  YYSYMBOL_MODULE = 83,                    /* MODULE  */
  YYSYMBOL_UnqualifiedMethodIdentifier = 84, /* UnqualifiedMethodIdentifier  */
  YYSYMBOL_NUMBER = 85,                    /* NUMBER  */
  YYSYMBOL_TRUE = 86,                      /* TRUE  */
  YYSYMBOL_FALSE = 87,                     /* FALSE  */
  YYSYMBOL_INTEGERLITERAL = 88,            /* INTEGERLITERAL  */
  YYSYMBOL_FLOATINGPOINTLITERAL = 89,      /* FLOATINGPOINTLITERAL  */
  YYSYMBOL_CHARACTERLITERAL = 90,          /* CHARACTERLITERAL  */
  YYSYMBOL_STRINGLITERAL = 91,             /* STRINGLITERAL  */
  YYSYMBOL_MY_NULL = 92,                   /* MY_NULL  */
  YYSYMBOL_LE = 93,                        /* LE  */
  YYSYMBOL_GE = 94,                        /* GE  */
  YYSYMBOL_EQ = 95,                        /* EQ  */
  YYSYMBOL_NE = 96,                        /* NE  */
  YYSYMBOL_AND = 97,                       /* AND  */
  YYSYMBOL_OR = 98,                        /* OR  */
  YYSYMBOL_TO = 99,                        /* TO  */
  YYSYMBOL_WITH = 100,                     /* WITH  */
  YYSYMBOL_ARROW = 101,                    /* ARROW  */
  YYSYMBOL_DIAMOND = 102,                  /* DIAMOND  */
  YYSYMBOL_DOUBLE_COLON = 103,             /* DOUBLE_COLON  */
  YYSYMBOL_MUL_ASSIGN = 104,               /* MUL_ASSIGN  */
  YYSYMBOL_DIV_ASSIGN = 105,               /* DIV_ASSIGN  */
  YYSYMBOL_MOD_ASSIGN = 106,               /* MOD_ASSIGN  */
  YYSYMBOL_ADD_ASSIGN = 107,               /* ADD_ASSIGN  */
  YYSYMBOL_SUB_ASSIGN = 108,               /* SUB_ASSIGN  */
  YYSYMBOL_SHL_ASSIGN = 109,               /* SHL_ASSIGN  */
  YYSYMBOL_SHR_ASSIGN = 110,               /* SHR_ASSIGN  */
  YYSYMBOL_USHR_ASSIGN = 111,              /* USHR_ASSIGN  */
  YYSYMBOL_AND_ASSIGN = 112,               /* AND_ASSIGN  */
  YYSYMBOL_XOR_ASSIGN = 113,               /* XOR_ASSIGN  */
  YYSYMBOL_OR_ASSIGN = 114,                /* OR_ASSIGN  */
  YYSYMBOL_INSTANCEOF = 115,               /* INSTANCEOF  */
  YYSYMBOL_SHL_OP = 116,                   /* SHL_OP  */
  YYSYMBOL_SHR_OP = 117,                   /* SHR_OP  */
  YYSYMBOL_USHR_OP = 118,                  /* USHR_OP  */
  YYSYMBOL_INC_OP = 119,                   /* INC_OP  */
  YYSYMBOL_DEC_OP = 120,                   /* DEC_OP  */
  YYSYMBOL_PREFIX_INC = 121,               /* PREFIX_INC  */
  YYSYMBOL_PREFIX_DEC = 122,               /* PREFIX_DEC  */
  YYSYMBOL_ELLIPSIS = 123,                 /* ELLIPSIS  */
  YYSYMBOL_TRAILING_COMMA = 124,           /* TRAILING_COMMA  */
  YYSYMBOL_TextBlock = 125,                /* TextBlock  */
  YYSYMBOL_SEALED = 126,                   /* SEALED  */
  YYSYMBOL_NON_SEALED = 127,               /* NON_SEALED  */
  YYSYMBOL_SEALED_IdentifierComplement = 128, /* SEALED_IdentifierComplement  */
  YYSYMBOL_NON_SEALED_IdentifierComplement = 129, /* NON_SEALED_IdentifierComplement  */
  YYSYMBOL_PERMITS = 130,                  /* PERMITS  */
  YYSYMBOL_EMPTY_STMT = 131,               /* EMPTY_STMT  */
  YYSYMBOL_PREC_ConditionalExpression_1 = 132, /* PREC_ConditionalExpression_1  */
  YYSYMBOL_PREC_ConditionalExpression = 133, /* PREC_ConditionalExpression  */
  YYSYMBOL_PREC_ConditionalOrExpression_1 = 134, /* PREC_ConditionalOrExpression_1  */
  YYSYMBOL_PREC_ConditionalAndExpression = 135, /* PREC_ConditionalAndExpression  */
  YYSYMBOL_PREC_ConditionalAndExpression_1 = 136, /* PREC_ConditionalAndExpression_1  */
  YYSYMBOL_PREC_InclusiveOrExpression = 137, /* PREC_InclusiveOrExpression  */
  YYSYMBOL_PREC_InclusiveOrExpression_1 = 138, /* PREC_InclusiveOrExpression_1  */
  YYSYMBOL_PREC_ExclusiveOrExpression = 139, /* PREC_ExclusiveOrExpression  */
  YYSYMBOL_PREC_ExclusiveOrExpression_1 = 140, /* PREC_ExclusiveOrExpression_1  */
  YYSYMBOL_PREC_AndExpression = 141,       /* PREC_AndExpression  */
  YYSYMBOL_PREC_AndExpression_1 = 142,     /* PREC_AndExpression_1  */
  YYSYMBOL_PREC_EqualityExpression = 143,  /* PREC_EqualityExpression  */
  YYSYMBOL_PREC_RelationalExpression = 144, /* PREC_RelationalExpression  */
  YYSYMBOL_PREC_RelationalExpression_1 = 145, /* PREC_RelationalExpression_1  */
  YYSYMBOL_PREC_SimpleRelationalExpression = 146, /* PREC_SimpleRelationalExpression  */
  YYSYMBOL_PREC_ShiftExpression = 147,     /* PREC_ShiftExpression  */
  YYSYMBOL_PREC_ShiftExpression_1 = 148,   /* PREC_ShiftExpression_1  */
  YYSYMBOL_PREC_AdditiveExpression = 149,  /* PREC_AdditiveExpression  */
  YYSYMBOL_PREC_AdditiveExpression_1 = 150, /* PREC_AdditiveExpression_1  */
  YYSYMBOL_PREC_MultiplicativeExpression = 151, /* PREC_MultiplicativeExpression  */
  YYSYMBOL_152_ = 152,                     /* ','  */
  YYSYMBOL_153_ = 153,                     /* '='  */
  YYSYMBOL_154_ = 154,                     /* '?'  */
  YYSYMBOL_155_ = 155,                     /* ':'  */
  YYSYMBOL_156_ = 156,                     /* '|'  */
  YYSYMBOL_157_ = 157,                     /* '^'  */
  YYSYMBOL_158_ = 158,                     /* '&'  */
  YYSYMBOL_159_ = 159,                     /* '<'  */
  YYSYMBOL_160_ = 160,                     /* '>'  */
  YYSYMBOL_LT_RelationalExpression = 161,  /* LT_RelationalExpression  */
  YYSYMBOL_162_ = 162,                     /* '+'  */
  YYSYMBOL_163_ = 163,                     /* '-'  */
  YYSYMBOL_164_ = 164,                     /* '*'  */
  YYSYMBOL_165_ = 165,                     /* '/'  */
  YYSYMBOL_166_ = 166,                     /* '%'  */
  YYSYMBOL_167_ = 167,                     /* '.'  */
  YYSYMBOL_168_ = 168,                     /* '!'  */
  YYSYMBOL_169_ = 169,                     /* '~'  */
  YYSYMBOL_170_ = 170,                     /* '['  */
  YYSYMBOL_171_ = 171,                     /* ']'  */
  YYSYMBOL_RBRACK = 172,                   /* RBRACK  */
  YYSYMBOL_173_ = 173,                     /* '('  */
  YYSYMBOL_174_ = 174,                     /* ')'  */
  YYSYMBOL_175_ = 175,                     /* '@'  */
  YYSYMBOL_176_ = 176,                     /* ';'  */
  YYSYMBOL_177_ = 177,                     /* '{'  */
  YYSYMBOL_178_ = 178,                     /* '}'  */
  YYSYMBOL_YYACCEPT = 179,                 /* $accept  */
  YYSYMBOL_IdentifierComplement = 180,     /* IdentifierComplement  */
  YYSYMBOL_IDENTIFIER_MethodDeclarator = 181, /* IDENTIFIER_MethodDeclarator  */
  YYSYMBOL_IDENTIFIER_AnnotationIdentifiers = 182, /* IDENTIFIER_AnnotationIdentifiers  */
  YYSYMBOL_Literal = 183,                  /* Literal  */
  YYSYMBOL_Modifier = 184,                 /* Modifier  */
  YYSYMBOL_Modifiers = 185,                /* Modifiers  */
  YYSYMBOL_Annotation_Modifier = 186,      /* Annotation_Modifier  */
  YYSYMBOL_NormalAnnotation_Modifier = 187, /* NormalAnnotation_Modifier  */
  YYSYMBOL_MarkerAnnotation_Modifier = 188, /* MarkerAnnotation_Modifier  */
  YYSYMBOL_SingleElementAnnotation_Modifier = 189, /* SingleElementAnnotation_Modifier  */
  YYSYMBOL_TypeName_ModifierOrDims = 190,  /* TypeName_ModifierOrDims  */
  YYSYMBOL_PrimitiveType = 191,            /* PrimitiveType  */
  YYSYMBOL_NumericType = 192,              /* NumericType  */
  YYSYMBOL_IntegralType = 193,             /* IntegralType  */
  YYSYMBOL_FloatingPointType = 194,        /* FloatingPointType  */
  YYSYMBOL_ReferenceType = 195,            /* ReferenceType  */
  YYSYMBOL_ClassOrInterfaceType = 196,     /* ClassOrInterfaceType  */
  YYSYMBOL_ArrayType = 197,                /* ArrayType  */
  YYSYMBOL_Dims = 198,                     /* Dims  */
  YYSYMBOL_Annotation_Dims = 199,          /* Annotation_Dims  */
  YYSYMBOL_Annotations_Dims = 200,         /* Annotations_Dims  */
  YYSYMBOL_NormalAnnotation_Dims = 201,    /* NormalAnnotation_Dims  */
  YYSYMBOL_MarkerAnnotation_Dims = 202,    /* MarkerAnnotation_Dims  */
  YYSYMBOL_SingleElementAnnotation_Dims = 203, /* SingleElementAnnotation_Dims  */
  YYSYMBOL_TypeParameter = 204,            /* TypeParameter  */
  YYSYMBOL_TypeBound = 205,                /* TypeBound  */
  YYSYMBOL_AdditionalBounds = 206,         /* AdditionalBounds  */
  YYSYMBOL_TypeArguments = 207,            /* TypeArguments  */
  YYSYMBOL_TypeArguments_UnannClassOrInterfaceType = 208, /* TypeArguments_UnannClassOrInterfaceType  */
  YYSYMBOL_TypeArgumentList = 209,         /* TypeArgumentList  */
  YYSYMBOL_TypeArgument = 210,             /* TypeArgument  */
  YYSYMBOL_Wildcard = 211,                 /* Wildcard  */
  YYSYMBOL_WildcardBounds = 212,           /* WildcardBounds  */
  YYSYMBOL_ModuleName = 213,               /* ModuleName  */
  YYSYMBOL_ModuleNames = 214,              /* ModuleNames  */
  YYSYMBOL_TypeName = 215,                 /* TypeName  */
  YYSYMBOL_TypeNames = 216,                /* TypeNames  */
  YYSYMBOL_CommonName = 217,               /* CommonName  */
  YYSYMBOL_CompilationUnit = 218,          /* CompilationUnit  */
  YYSYMBOL_OrdinaryCompilationUnit = 219,  /* OrdinaryCompilationUnit  */
  YYSYMBOL_ModularCompilationUnit = 220,   /* ModularCompilationUnit  */
  YYSYMBOL_PackageDeclaration = 221,       /* PackageDeclaration  */
  YYSYMBOL_ImportDeclarations = 222,       /* ImportDeclarations  */
  YYSYMBOL_ImportDeclaration = 223,        /* ImportDeclaration  */
  YYSYMBOL_SingleTypeImportDeclaration = 224, /* SingleTypeImportDeclaration  */
  YYSYMBOL_TypeImportOnDemandDeclaration = 225, /* TypeImportOnDemandDeclaration  */
  YYSYMBOL_SingleStaticImportDeclaration = 226, /* SingleStaticImportDeclaration  */
  YYSYMBOL_StaticImportOnDemandDeclaration = 227, /* StaticImportOnDemandDeclaration  */
  YYSYMBOL_TypeDeclarations = 228,         /* TypeDeclarations  */
  YYSYMBOL_TypeDeclaration = 229,          /* TypeDeclaration  */
  YYSYMBOL_ModuleDeclaration = 230,        /* ModuleDeclaration  */
  YYSYMBOL_ModuleDirectives = 231,         /* ModuleDirectives  */
  YYSYMBOL_ModuleDirective = 232,          /* ModuleDirective  */
  YYSYMBOL_ArrayInitializer = 233,         /* ArrayInitializer  */
  YYSYMBOL_VariableInitializerList = 234,  /* VariableInitializerList  */
  YYSYMBOL_ClassDeclaration = 235,         /* ClassDeclaration  */
  YYSYMBOL_NormalClassDeclaration = 236,   /* NormalClassDeclaration  */
  YYSYMBOL_OptTypeParameters = 237,        /* OptTypeParameters  */
  YYSYMBOL_OptSuperclass = 238,            /* OptSuperclass  */
  YYSYMBOL_OptSuperinterfaces = 239,       /* OptSuperinterfaces  */
  YYSYMBOL_OptClassOrInterfacePermits = 240, /* OptClassOrInterfacePermits  */
  YYSYMBOL_TypeParameters = 241,           /* TypeParameters  */
  YYSYMBOL_TypeParameterList = 242,        /* TypeParameterList  */
  YYSYMBOL_Superclass = 243,               /* Superclass  */
  YYSYMBOL_Superinterfaces = 244,          /* Superinterfaces  */
  YYSYMBOL_InterfaceTypeList = 245,        /* InterfaceTypeList  */
  YYSYMBOL_ClassOrInterfacePermits = 246,  /* ClassOrInterfacePermits  */
  YYSYMBOL_ClassBody = 247,                /* ClassBody  */
  YYSYMBOL_ClassBodyDeclarationList = 248, /* ClassBodyDeclarationList  */
  YYSYMBOL_ClassBodyDeclaration = 249,     /* ClassBodyDeclaration  */
  YYSYMBOL_ClassMemberDeclaration = 250,   /* ClassMemberDeclaration  */
  YYSYMBOL_FieldDeclaration = 251,         /* FieldDeclaration  */
  YYSYMBOL_VariableDeclaratorList = 252,   /* VariableDeclaratorList  */
  YYSYMBOL_VariableDeclarator = 253,       /* VariableDeclarator  */
  YYSYMBOL_VariableDeclaratorId = 254,     /* VariableDeclaratorId  */
  YYSYMBOL_VariableInitializer = 255,      /* VariableInitializer  */
  YYSYMBOL_UnannType = 256,                /* UnannType  */
  YYSYMBOL_UnannPrimitiveType = 257,       /* UnannPrimitiveType  */
  YYSYMBOL_UnannReferenceType = 258,       /* UnannReferenceType  */
  YYSYMBOL_UnannClassOrInterfaceType = 259, /* UnannClassOrInterfaceType  */
  YYSYMBOL_UnannArrayType = 260,           /* UnannArrayType  */
  YYSYMBOL_MethodDeclaration = 261,        /* MethodDeclaration  */
  YYSYMBOL_MethodHeader = 262,             /* MethodHeader  */
  YYSYMBOL_Result = 263,                   /* Result  */
  YYSYMBOL_MethodDeclarator = 264,         /* MethodDeclarator  */
  YYSYMBOL_ReceiverParameter = 265,        /* ReceiverParameter  */
  YYSYMBOL_FormalParameterList = 266,      /* FormalParameterList  */
  YYSYMBOL_FormalParameter = 267,          /* FormalParameter  */
  YYSYMBOL_VariableArityParameter = 268,   /* VariableArityParameter  */
  YYSYMBOL_VariableModifiers = 269,        /* VariableModifiers  */
  YYSYMBOL_Throws = 270,                   /* Throws  */
  YYSYMBOL_ExceptionTypeList = 271,        /* ExceptionTypeList  */
  YYSYMBOL_ExceptionType = 272,            /* ExceptionType  */
  YYSYMBOL_MethodBody = 273,               /* MethodBody  */
  YYSYMBOL_InstanceInitializer = 274,      /* InstanceInitializer  */
  YYSYMBOL_StaticInitializer = 275,        /* StaticInitializer  */
  YYSYMBOL_ConstructorDeclaration = 276,   /* ConstructorDeclaration  */
  YYSYMBOL_ConstructorDeclarator = 277,    /* ConstructorDeclarator  */
  YYSYMBOL_SimpleTypeName = 278,           /* SimpleTypeName  */
  YYSYMBOL_ConstructorBody = 279,          /* ConstructorBody  */
  YYSYMBOL_ExplicitConstructorInvocation = 280, /* ExplicitConstructorInvocation  */
  YYSYMBOL_EnumDeclaration = 281,          /* EnumDeclaration  */
  YYSYMBOL_EnumBody = 282,                 /* EnumBody  */
  YYSYMBOL_EnumConstantList = 283,         /* EnumConstantList  */
  YYSYMBOL_EnumConstant = 284,             /* EnumConstant  */
  YYSYMBOL_EnumBodyDeclarations = 285,     /* EnumBodyDeclarations  */
  YYSYMBOL_InterfaceDeclaration = 286,     /* InterfaceDeclaration  */
  YYSYMBOL_NormalInterfaceDeclaration = 287, /* NormalInterfaceDeclaration  */
  YYSYMBOL_ExtendsInterfaces = 288,        /* ExtendsInterfaces  */
  YYSYMBOL_OptExtendsInterfaces = 289,     /* OptExtendsInterfaces  */
  YYSYMBOL_InterfaceBody = 290,            /* InterfaceBody  */
  YYSYMBOL_InterfaceMemberDeclarationList = 291, /* InterfaceMemberDeclarationList  */
  YYSYMBOL_InterfaceMemberDeclaration = 292, /* InterfaceMemberDeclaration  */
  YYSYMBOL_ConstantDeclaration = 293,      /* ConstantDeclaration  */
  YYSYMBOL_InterfaceMethodDeclaration = 294, /* InterfaceMethodDeclaration  */
  YYSYMBOL_AnnotationTypeDeclaration = 295, /* AnnotationTypeDeclaration  */
  YYSYMBOL_AnnotationTypeBody = 296,       /* AnnotationTypeBody  */
  YYSYMBOL_AnnotationTypeMemberDeclarationList = 297, /* AnnotationTypeMemberDeclarationList  */
  YYSYMBOL_AnnotationTypeMemberDeclaration = 298, /* AnnotationTypeMemberDeclaration  */
  YYSYMBOL_AnnotationTypeElementDeclaration = 299, /* AnnotationTypeElementDeclaration  */
  YYSYMBOL_DefaultValue = 300,             /* DefaultValue  */
  YYSYMBOL_Annotation = 301,               /* Annotation  */
  YYSYMBOL_Annotations = 302,              /* Annotations  */
  YYSYMBOL_NormalAnnotation = 303,         /* NormalAnnotation  */
  YYSYMBOL_ElementValuePairList = 304,     /* ElementValuePairList  */
  YYSYMBOL_ElementValuePair = 305,         /* ElementValuePair  */
  YYSYMBOL_ElementValue = 306,             /* ElementValue  */
  YYSYMBOL_ElementValueArrayInitializer = 307, /* ElementValueArrayInitializer  */
  YYSYMBOL_ElementValueList = 308,         /* ElementValueList  */
  YYSYMBOL_MarkerAnnotation = 309,         /* MarkerAnnotation  */
  YYSYMBOL_SingleElementAnnotation = 310,  /* SingleElementAnnotation  */
  YYSYMBOL_Block = 311,                    /* Block  */
  YYSYMBOL_BlockStatements = 312,          /* BlockStatements  */
  YYSYMBOL_BlockStatement = 313,           /* BlockStatement  */
  YYSYMBOL_LocalVariableDeclarationStatement = 314, /* LocalVariableDeclarationStatement  */
  YYSYMBOL_LocalVariableDeclaration = 315, /* LocalVariableDeclaration  */
  YYSYMBOL_LocalVariableType = 316,        /* LocalVariableType  */
  YYSYMBOL_LocalClassOrInterfaceDeclaration = 317, /* LocalClassOrInterfaceDeclaration  */
  YYSYMBOL_Statement = 318,                /* Statement  */
  YYSYMBOL_StatementNoShortIf = 319,       /* StatementNoShortIf  */
  YYSYMBOL_StatementWithoutTrailingSubstatement = 320, /* StatementWithoutTrailingSubstatement  */
  YYSYMBOL_EmptyStatement = 321,           /* EmptyStatement  */
  YYSYMBOL_LabeledStatement = 322,         /* LabeledStatement  */
  YYSYMBOL_LabeledStatementNoShortIf = 323, /* LabeledStatementNoShortIf  */
  YYSYMBOL_ExpressionStatement = 324,      /* ExpressionStatement  */
  YYSYMBOL_StatementExpression = 325,      /* StatementExpression  */
  YYSYMBOL_IfThenStatement = 326,          /* IfThenStatement  */
  YYSYMBOL_IfThenElseStatement = 327,      /* IfThenElseStatement  */
  YYSYMBOL_IfThenElseStatementNoShortIf = 328, /* IfThenElseStatementNoShortIf  */
  YYSYMBOL_AssertStatement = 329,          /* AssertStatement  */
  YYSYMBOL_SwitchStatement = 330,          /* SwitchStatement  */
  YYSYMBOL_SwitchBlock = 331,              /* SwitchBlock  */
  YYSYMBOL_SwitchRule = 332,               /* SwitchRule  */
  YYSYMBOL_SwitchRules = 333,              /* SwitchRules  */
  YYSYMBOL_SwitchBlockStatementGroup = 334, /* SwitchBlockStatementGroup  */
  YYSYMBOL_SwitchBlockStatementGroups = 335, /* SwitchBlockStatementGroups  */
  YYSYMBOL_SwitchLabel = 336,              /* SwitchLabel  */
  YYSYMBOL_SwitchLabels = 337,             /* SwitchLabels  */
  YYSYMBOL_CaseConstant = 338,             /* CaseConstant  */
  YYSYMBOL_CaseConstants = 339,            /* CaseConstants  */
  YYSYMBOL_WhileStatement = 340,           /* WhileStatement  */
  YYSYMBOL_WhileStatementNoShortIf = 341,  /* WhileStatementNoShortIf  */
  YYSYMBOL_DoStatement = 342,              /* DoStatement  */
  YYSYMBOL_ForStatement = 343,             /* ForStatement  */
  YYSYMBOL_ForStatementNoShortIf = 344,    /* ForStatementNoShortIf  */
  YYSYMBOL_BasicForStatement = 345,        /* BasicForStatement  */
  YYSYMBOL_BasicForStatementNoShortIf = 346, /* BasicForStatementNoShortIf  */
  YYSYMBOL_ForInit = 347,                  /* ForInit  */
  YYSYMBOL_ForUpdate = 348,                /* ForUpdate  */
  YYSYMBOL_StatementExpressionList = 349,  /* StatementExpressionList  */
  YYSYMBOL_EnhancedForStatement = 350,     /* EnhancedForStatement  */
  YYSYMBOL_EnhancedForStatementNoShortIf = 351, /* EnhancedForStatementNoShortIf  */
  YYSYMBOL_BreakStatement = 352,           /* BreakStatement  */
  YYSYMBOL_YieldStatement = 353,           /* YieldStatement  */
  YYSYMBOL_ContinueStatement = 354,        /* ContinueStatement  */
  YYSYMBOL_ReturnStatement = 355,          /* ReturnStatement  */
  YYSYMBOL_ThrowStatement = 356,           /* ThrowStatement  */
  YYSYMBOL_SynchronizedStatement = 357,    /* SynchronizedStatement  */
  YYSYMBOL_TryStatement = 358,             /* TryStatement  */
  YYSYMBOL_Catches = 359,                  /* Catches  */
  YYSYMBOL_CatchClause = 360,              /* CatchClause  */
  YYSYMBOL_CatchFormalParameter = 361,     /* CatchFormalParameter  */
  YYSYMBOL_CatchType = 362,                /* CatchType  */
  YYSYMBOL_Finally = 363,                  /* Finally  */
  YYSYMBOL_TryWithResourcesStatement = 364, /* TryWithResourcesStatement  */
  YYSYMBOL_ResourceSpecification = 365,    /* ResourceSpecification  */
  YYSYMBOL_ResourceList = 366,             /* ResourceList  */
  YYSYMBOL_Resource = 367,                 /* Resource  */
  YYSYMBOL_VariableAccess = 368,           /* VariableAccess  */
  YYSYMBOL_Pattern = 369,                  /* Pattern  */
  YYSYMBOL_TypePattern = 370,              /* TypePattern  */
  YYSYMBOL_LocalVariableDeclaration_TypePattern = 371, /* LocalVariableDeclaration_TypePattern  */
  YYSYMBOL_Primary = 372,                  /* Primary  */
  YYSYMBOL_PrimaryNoNewArray = 373,        /* PrimaryNoNewArray  */
  YYSYMBOL_ClassLiteral = 374,             /* ClassLiteral  */
  YYSYMBOL_DimsNoAnnotations = 375,        /* DimsNoAnnotations  */
  YYSYMBOL_ClassInstanceCreationExpression = 376, /* ClassInstanceCreationExpression  */
  YYSYMBOL_UnqualifiedClassInstanceCreationExpression = 377, /* UnqualifiedClassInstanceCreationExpression  */
  YYSYMBOL_ClassOrInterfaceTypeToInstantiate = 378, /* ClassOrInterfaceTypeToInstantiate  */
  YYSYMBOL_AnnotationIdentifiers = 379,    /* AnnotationIdentifiers  */
  YYSYMBOL_TypeArgumentsOrDiamond = 380,   /* TypeArgumentsOrDiamond  */
  YYSYMBOL_FieldAccess = 381,              /* FieldAccess  */
  YYSYMBOL_ArrayAccess = 382,              /* ArrayAccess  */
  YYSYMBOL_MethodInvocation = 383,         /* MethodInvocation  */
  YYSYMBOL_ArgumentList = 384,             /* ArgumentList  */
  YYSYMBOL_MethodReference = 385,          /* MethodReference  */
  YYSYMBOL_ArrayCreationExpression = 386,  /* ArrayCreationExpression  */
  YYSYMBOL_DimExprs = 387,                 /* DimExprs  */
  YYSYMBOL_DimExpr = 388,                  /* DimExpr  */
  YYSYMBOL_Expression = 389,               /* Expression  */
  YYSYMBOL_LambdaExpression = 390,         /* LambdaExpression  */
  YYSYMBOL_LambdaParameters = 391,         /* LambdaParameters  */
  YYSYMBOL_LambdaParameterList = 392,      /* LambdaParameterList  */
  YYSYMBOL_IdentifierforLambdaParameterList = 393, /* IdentifierforLambdaParameterList  */
  YYSYMBOL_IdentifierComplement_IdentifierforLambdaParameterList = 394, /* IdentifierComplement_IdentifierforLambdaParameterList  */
  YYSYMBOL_LambdaParameterforLambdaParameterList = 395, /* LambdaParameterforLambdaParameterList  */
  YYSYMBOL_LambdaParameter = 396,          /* LambdaParameter  */
  YYSYMBOL_LambdaParameterType = 397,      /* LambdaParameterType  */
  YYSYMBOL_LambdaBody = 398,               /* LambdaBody  */
  YYSYMBOL_AssignmentExpression = 399,     /* AssignmentExpression  */
  YYSYMBOL_Assignment = 400,               /* Assignment  */
  YYSYMBOL_LeftHandSide = 401,             /* LeftHandSide  */
  YYSYMBOL_AssignmentOperator = 402,       /* AssignmentOperator  */
  YYSYMBOL_ConditionalExpression = 403,    /* ConditionalExpression  */
  YYSYMBOL_ConditionalOrExpression = 404,  /* ConditionalOrExpression  */
  YYSYMBOL_ConditionalAndExpression = 405, /* ConditionalAndExpression  */
  YYSYMBOL_InclusiveOrExpression = 406,    /* InclusiveOrExpression  */
  YYSYMBOL_ExclusiveOrExpression = 407,    /* ExclusiveOrExpression  */
  YYSYMBOL_AndExpression = 408,            /* AndExpression  */
  YYSYMBOL_EqualityExpression = 409,       /* EqualityExpression  */
  YYSYMBOL_SimpleRelationalExpression = 410, /* SimpleRelationalExpression  */
  YYSYMBOL_RelationalExpression = 411,     /* RelationalExpression  */
  YYSYMBOL_ShiftExpression = 412,          /* ShiftExpression  */
  YYSYMBOL_AdditiveExpression = 413,       /* AdditiveExpression  */
  YYSYMBOL_MultiplicativeExpression = 414, /* MultiplicativeExpression  */
  YYSYMBOL_UnaryExpression = 415,          /* UnaryExpression  */
  YYSYMBOL_PreIncrementExpression = 416,   /* PreIncrementExpression  */
  YYSYMBOL_PreDecrementExpression = 417,   /* PreDecrementExpression  */
  YYSYMBOL_UnaryExpressionNotPlusMinus = 418, /* UnaryExpressionNotPlusMinus  */
  YYSYMBOL_PostfixExpression = 419,        /* PostfixExpression  */
  YYSYMBOL_PostIncrementExpression = 420,  /* PostIncrementExpression  */
  YYSYMBOL_PostDecrementExpression = 421,  /* PostDecrementExpression  */
  YYSYMBOL_CastExpression = 422,           /* CastExpression  */
  YYSYMBOL_SwitchExpression = 423          /* SwitchExpression  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  89
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   14030

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  179
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  245
/* YYNRULES -- Number of rules.  */
#define YYNRULES  812
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1595

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   408


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   168,     2,     2,     2,   166,   158,     2,
     173,   174,   164,   162,   152,   163,   167,   165,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   155,   176,
     159,   153,   160,   154,   175,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   170,     2,   171,   157,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   177,   156,   178,   169,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   161,   172
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,  1128,  1128,  1129,  1130,  1131,  1136,  1137,  1138,  1143,
    1144,  1145,  1151,  1152,  1153,  1154,  1155,  1156,  1157,  1158,
    1163,  1165,  1166,  1167,  1168,  1169,  1170,  1171,  1172,  1173,
    1174,  1175,  1176,  1177,  1178,  1182,  1186,  1195,  1196,  1197,
    1202,  1209,  1219,  1229,  1242,  1243,  1268,  1281,  1282,  1287,
    1288,  1289,  1290,  1291,  1296,  1297,  1303,  1305,  1316,  1319,
    1331,  1341,  1351,  1358,  1367,  1375,  1380,  1389,  1390,  1391,
    1396,  1400,  1408,  1413,  1420,  1427,  1438,  1445,  1452,  1455,
    1462,  1470,  1478,  1483,  1484,  1490,  1495,  1499,  1507,  1510,
    1517,  1520,  1523,  1526,  1533,  1537,  1550,  1553,  1556,  1562,
    1572,  1576,  1584,  1585,  1595,  1599,  1623,  1626,  1629,  1635,
    1648,  1649,  1658,  1661,  1664,  1668,  1677,  1680,  1688,  1691,
    1701,  1704,  1712,  1713,  1714,  1715,  1720,  1727,  1737,  1746,
    1753,  1756,  1765,  1766,  1767,  1768,  1774,  1778,  1781,  1785,
    1788,  1792,  1795,  1799,  1806,  1809,  1819,  1825,  1830,  1836,
    1841,  1848,  1853,  1860,  1865,  1882,  1885,  1889,  1896,  1900,
    1914,  1915,  1920,  1931,  1944,  1945,  1950,  1951,  1956,  1957,
    1962,  1963,  1968,  1969,  1974,  1978,  1987,  1994,  2001,  2005,
    2013,  2018,  2019,  2024,  2027,  2035,  2036,  2037,  2038,  2043,
    2044,  2045,  2046,  2047,  2048,  2052,  2055,  2066,  2070,  2078,
    2086,  2095,  2101,  2107,  2108,  2113,  2116,  2123,  2124,  2137,
    2138,  2143,  2145,  2155,  2161,  2165,  2176,  2190,  2201,  2211,
    2222,  2240,  2247,  2259,  2269,  2279,  2288,  2294,  2309,  2321,
    2333,  2346,  2347,  2352,  2357,  2360,  2363,  2369,  2374,  2377,
    2380,  2387,  2391,  2395,  2399,  2402,  2405,  2412,  2415,  2426,
    2429,  2433,  2436,  2439,  2443,  2446,  2449,  2453,  2456,  2459,
    2463,  2466,  2467,  2471,  2479,  2483,  2486,  2490,  2494,  2498,
    2501,  2505,  2508,  2512,  2516,  2520,  2527,  2531,  2536,  2541,
    2551,  2556,  2560,  2569,  2575,  2576,  2581,  2586,  2592,  2604,
    2613,  2622,  2632,  2641,  2648,  2655,  2662,  2667,  2670,  2673,
    2680,  2685,  2691,  2698,  2701,  2708,  2711,  2714,  2717,  2720,
    2723,  2726,  2729,  2732,  2735,  2738,  2741,  2744,  2747,  2750,
    2753,  2760,  2770,  2779,  2788,  2802,  2807,  2812,  2815,  2819,
    2822,  2829,  2832,  2842,  2845,  2848,  2851,  2854,  2857,  2860,
    2863,  2866,  2869,  2872,  2875,  2878,  2881,  2884,  2887,  2894,
    2908,  2909,  2916,  2925,  2939,  2946,  2947,  2952,  2953,  2958,
    2961,  2969,  2970,  2971,  2972,  2973,  2974,  2979,  2982,  2990,
    2999,  3011,  3014,  3018,  3021,  3025,  3028,  3034,  3037,  3044,
    3047,  3055,  3056,  3057,  3058,  3059,  3060,  3069,  3072,  3075,
    3078,  3081,  3084,  3087,  3090,  3097,  3104,  3105,  3106,  3111,
    3115,  3123,  3128,  3135,  3139,  3147,  3153,  3163,  3165,  3166,
    3171,  3175,  3178,  3185,  3189,  3197,  3204,  3219,  3222,  3229,
    3235,  3245,  3246,  3247,  3252,  3257,  3260,  3264,  3271,  3272,
    3277,  3278,  3283,  3284,  3285,  3286,  3287,  3288,  3293,  3294,
    3295,  3296,  3297,  3302,  3303,  3304,  3305,  3306,  3307,  3308,
    3309,  3310,  3311,  3312,  3313,  3314,  3319,  3322,  3329,  3334,
    3343,  3348,  3357,  3364,  3365,  3366,  3367,  3368,  3369,  3370,
    3375,  3382,  3390,  3398,  3401,  3409,  3420,  3424,  3427,  3434,
    3444,  3454,  3468,  3472,  3480,  3492,  3496,  3504,  3509,  3518,
    3523,  3531,  3536,  3540,  3548,  3555,  3562,  3570,  3571,  3576,
    3577,  3582,  3585,  3588,  3591,  3594,  3597,  3600,  3603,  3610,
    3613,  3616,  3619,  3622,  3625,  3628,  3631,  3638,  3639,  3644,
    3649,  3653,  3667,  3671,  3675,  3679,  3683,  3687,  3691,  3695,
    3699,  3703,  3707,  3711,  3722,  3726,  3730,  3734,  3738,  3742,
    3746,  3750,  3754,  3758,  3762,  3766,  3774,  3778,  3782,  3789,
    3796,  3799,  3803,  3811,  3814,  3821,  3828,  3835,  3838,  3841,
    3844,  3851,  3855,  3863,  3876,  3879,  3882,  3885,  3888,  3891,
    3894,  3897,  3900,  3903,  3906,  3909,  3916,  3921,  3931,  3940,
    3947,  3954,  3961,  3972,  3975,  3983,  3987,  3996,  4000,  4005,
    4009,  4013,  4018,  4022,  4029,  4030,  4035,  4044,  4049,  4052,
    4056,  4065,  4066,  4071,  4072,  4073,  4076,  4080,  4081,  4082,
    4083,  4084,  4085,  4091,  4095,  4099,  4106,  4109,  4112,  4117,
    4125,  4130,  4138,  4139,  4144,  4153,  4160,  4167,  4174,  4181,
    4184,  4187,  4190,  4197,  4203,  4210,  4214,  4215,  4219,  4227,
    4230,  4239,  4242,  4246,  4250,  4253,  4257,  4265,  4269,  4272,
    4278,  4284,  4323,  4326,  4333,  4336,  4339,  4342,  4345,  4348,
    4351,  4354,  4357,  4360,  4363,  4366,  4373,  4377,  4381,  4385,
    4393,  4399,  4411,  4414,  4417,  4420,  4423,  4426,  4431,  4435,
    4441,  4444,  4447,  4450,  4453,  4456,  4461,  4465,  4473,  4476,
    4483,  4487,  4490,  4494,  4497,  4501,  4504,  4508,  4512,  4520,
    4524,  4532,  4540,  4550,  4551,  4558,  4565,  4568,  4571,  4572,
    4580,  4581,  4586,  4590,  4594,  4598,  4606,  4607,  4612,  4616,
    4624,  4627,  4630,  4633,  4636,  4639,  4642,  4645,  4648,  4651,
    4654,  4657,  4660,  4665,  4666,  4671,  4672,  4677,  4678,  4683,
    4690,  4691,  4692,  4697,  4698,  4699,  4700,  4701,  4702,  4703,
    4704,  4705,  4706,  4707,  4708,  4713,  4716,  4719,  4726,  4729,
    4737,  4740,  4747,  4750,  4757,  4760,  4767,  4770,  4777,  4778,
    4781,  4788,  4795,  4798,  4801,  4804,  4807,  4810,  4813,  4820,
    4823,  4826,  4829,  4836,  4839,  4842,  4849,  4850,  4853,  4856,
    4863,  4864,  4865,  4868,  4873,  4878,  4885,  4892,  4893,  4896,
    4899,  4900,  4905,  4906,  4910,  4911,  4916,  4923,  4930,  4933,
    4938,  4941,  4948
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "AMPERSAND",
  "RPAREN_CastExpression",
  "TYPE_IDENTIFIER_IdentifierforLambdaParameterList",
  "VAR_IdentifierComplement_IdentifierforLambdaParameterList",
  "YIELD_IdentifierComplement_IdentifierforLambdaParameterList",
  "TYPE_IDENTIFIER_MethodDeclarator", "VAR_MethodDeclarator",
  "YIELD_MethodDeclarator", "TYPE_IDENTIFIER_AnnotationIdentifiers",
  "VAR_AnnotationIdentifiers", "YIELD_AnnotationIdentifiers",
  "TYPE_IDENTIFIER", "TYPE_IDENTIFIER_EnumDeclaration", "LANGLE",
  "AT_Dims", "AT_Modifier", "AT_AnnotationTypeDeclaration", "LBRACK",
  "LBRACK_ArrayAccess", "DEFAULT_SwitchLabel", "DOT", "DOT_CommonName",
  "VAR_IdentifierComplement", "YIELD_IdentifierComplement", "CLASS",
  "BYTE", "INT", "SHORT", "LONG", "CHAR", "FLOAT", "DOUBLE", "EXTENDS",
  "SUPER", "OPEN", "OPENS", "REQUIRES", "EXPORTS", "USES", "PROVIDES",
  "PACKAGE", "IMPORT", "PUBLIC", "PROTECTED", "PRIVATE", "ABSTRACT",
  "STATIC", "FINAL", "STRICTFP", "TRANSITIVE", "IMPLEMENTS", "TRANSIENT",
  "VOLATILE", "SYNCHRONIZED", "NATIVE", "BOOLEAN", "VOID", "THROW",
  "THROWS", "THIS", "ENUM", "INTERFACE", "DEFAULT", "VAR", "IF", "ELSE",
  "ASSERT", "SWITCH", "CASE", "DO", "WHILE", "FOR", "BREAK", "YIELD",
  "CONTINUE", "RETURN", "TRY", "CATCH", "FINALLY", "NEW", "MODULE",
  "UnqualifiedMethodIdentifier", "NUMBER", "TRUE", "FALSE",
  "INTEGERLITERAL", "FLOATINGPOINTLITERAL", "CHARACTERLITERAL",
  "STRINGLITERAL", "MY_NULL", "LE", "GE", "EQ", "NE", "AND", "OR", "TO",
  "WITH", "ARROW", "DIAMOND", "DOUBLE_COLON", "MUL_ASSIGN", "DIV_ASSIGN",
  "MOD_ASSIGN", "ADD_ASSIGN", "SUB_ASSIGN", "SHL_ASSIGN", "SHR_ASSIGN",
  "USHR_ASSIGN", "AND_ASSIGN", "XOR_ASSIGN", "OR_ASSIGN", "INSTANCEOF",
  "SHL_OP", "SHR_OP", "USHR_OP", "INC_OP", "DEC_OP", "PREFIX_INC",
  "PREFIX_DEC", "ELLIPSIS", "TRAILING_COMMA", "TextBlock", "SEALED",
  "NON_SEALED", "SEALED_IdentifierComplement",
  "NON_SEALED_IdentifierComplement", "PERMITS", "EMPTY_STMT",
  "PREC_ConditionalExpression_1", "PREC_ConditionalExpression",
  "PREC_ConditionalOrExpression_1", "PREC_ConditionalAndExpression",
  "PREC_ConditionalAndExpression_1", "PREC_InclusiveOrExpression",
  "PREC_InclusiveOrExpression_1", "PREC_ExclusiveOrExpression",
  "PREC_ExclusiveOrExpression_1", "PREC_AndExpression",
  "PREC_AndExpression_1", "PREC_EqualityExpression",
  "PREC_RelationalExpression", "PREC_RelationalExpression_1",
  "PREC_SimpleRelationalExpression", "PREC_ShiftExpression",
  "PREC_ShiftExpression_1", "PREC_AdditiveExpression",
  "PREC_AdditiveExpression_1", "PREC_MultiplicativeExpression", "','",
  "'='", "'?'", "':'", "'|'", "'^'", "'&'", "'<'", "'>'",
  "LT_RelationalExpression", "'+'", "'-'", "'*'", "'/'", "'%'", "'.'",
  "'!'", "'~'", "'['", "']'", "RBRACK", "'('", "')'", "'@'", "';'", "'{'",
  "'}'", "$accept", "IdentifierComplement", "IDENTIFIER_MethodDeclarator",
  "IDENTIFIER_AnnotationIdentifiers", "Literal", "Modifier", "Modifiers",
  "Annotation_Modifier", "NormalAnnotation_Modifier",
  "MarkerAnnotation_Modifier", "SingleElementAnnotation_Modifier",
  "TypeName_ModifierOrDims", "PrimitiveType", "NumericType",
  "IntegralType", "FloatingPointType", "ReferenceType",
  "ClassOrInterfaceType", "ArrayType", "Dims", "Annotation_Dims",
  "Annotations_Dims", "NormalAnnotation_Dims", "MarkerAnnotation_Dims",
  "SingleElementAnnotation_Dims", "TypeParameter", "TypeBound",
  "AdditionalBounds", "TypeArguments",
  "TypeArguments_UnannClassOrInterfaceType", "TypeArgumentList",
  "TypeArgument", "Wildcard", "WildcardBounds", "ModuleName",
  "ModuleNames", "TypeName", "TypeNames", "CommonName", "CompilationUnit",
  "OrdinaryCompilationUnit", "ModularCompilationUnit",
  "PackageDeclaration", "ImportDeclarations", "ImportDeclaration",
  "SingleTypeImportDeclaration", "TypeImportOnDemandDeclaration",
  "SingleStaticImportDeclaration", "StaticImportOnDemandDeclaration",
  "TypeDeclarations", "TypeDeclaration", "ModuleDeclaration",
  "ModuleDirectives", "ModuleDirective", "ArrayInitializer",
  "VariableInitializerList", "ClassDeclaration", "NormalClassDeclaration",
  "OptTypeParameters", "OptSuperclass", "OptSuperinterfaces",
  "OptClassOrInterfacePermits", "TypeParameters", "TypeParameterList",
  "Superclass", "Superinterfaces", "InterfaceTypeList",
  "ClassOrInterfacePermits", "ClassBody", "ClassBodyDeclarationList",
  "ClassBodyDeclaration", "ClassMemberDeclaration", "FieldDeclaration",
  "VariableDeclaratorList", "VariableDeclarator", "VariableDeclaratorId",
  "VariableInitializer", "UnannType", "UnannPrimitiveType",
  "UnannReferenceType", "UnannClassOrInterfaceType", "UnannArrayType",
  "MethodDeclaration", "MethodHeader", "Result", "MethodDeclarator",
  "ReceiverParameter", "FormalParameterList", "FormalParameter",
  "VariableArityParameter", "VariableModifiers", "Throws",
  "ExceptionTypeList", "ExceptionType", "MethodBody",
  "InstanceInitializer", "StaticInitializer", "ConstructorDeclaration",
  "ConstructorDeclarator", "SimpleTypeName", "ConstructorBody",
  "ExplicitConstructorInvocation", "EnumDeclaration", "EnumBody",
  "EnumConstantList", "EnumConstant", "EnumBodyDeclarations",
  "InterfaceDeclaration", "NormalInterfaceDeclaration",
  "ExtendsInterfaces", "OptExtendsInterfaces", "InterfaceBody",
  "InterfaceMemberDeclarationList", "InterfaceMemberDeclaration",
  "ConstantDeclaration", "InterfaceMethodDeclaration",
  "AnnotationTypeDeclaration", "AnnotationTypeBody",
  "AnnotationTypeMemberDeclarationList", "AnnotationTypeMemberDeclaration",
  "AnnotationTypeElementDeclaration", "DefaultValue", "Annotation",
  "Annotations", "NormalAnnotation", "ElementValuePairList",
  "ElementValuePair", "ElementValue", "ElementValueArrayInitializer",
  "ElementValueList", "MarkerAnnotation", "SingleElementAnnotation",
  "Block", "BlockStatements", "BlockStatement",
  "LocalVariableDeclarationStatement", "LocalVariableDeclaration",
  "LocalVariableType", "LocalClassOrInterfaceDeclaration", "Statement",
  "StatementNoShortIf", "StatementWithoutTrailingSubstatement",
  "EmptyStatement", "LabeledStatement", "LabeledStatementNoShortIf",
  "ExpressionStatement", "StatementExpression", "IfThenStatement",
  "IfThenElseStatement", "IfThenElseStatementNoShortIf", "AssertStatement",
  "SwitchStatement", "SwitchBlock", "SwitchRule", "SwitchRules",
  "SwitchBlockStatementGroup", "SwitchBlockStatementGroups", "SwitchLabel",
  "SwitchLabels", "CaseConstant", "CaseConstants", "WhileStatement",
  "WhileStatementNoShortIf", "DoStatement", "ForStatement",
  "ForStatementNoShortIf", "BasicForStatement",
  "BasicForStatementNoShortIf", "ForInit", "ForUpdate",
  "StatementExpressionList", "EnhancedForStatement",
  "EnhancedForStatementNoShortIf", "BreakStatement", "YieldStatement",
  "ContinueStatement", "ReturnStatement", "ThrowStatement",
  "SynchronizedStatement", "TryStatement", "Catches", "CatchClause",
  "CatchFormalParameter", "CatchType", "Finally",
  "TryWithResourcesStatement", "ResourceSpecification", "ResourceList",
  "Resource", "VariableAccess", "Pattern", "TypePattern",
  "LocalVariableDeclaration_TypePattern", "Primary", "PrimaryNoNewArray",
  "ClassLiteral", "DimsNoAnnotations", "ClassInstanceCreationExpression",
  "UnqualifiedClassInstanceCreationExpression",
  "ClassOrInterfaceTypeToInstantiate", "AnnotationIdentifiers",
  "TypeArgumentsOrDiamond", "FieldAccess", "ArrayAccess",
  "MethodInvocation", "ArgumentList", "MethodReference",
  "ArrayCreationExpression", "DimExprs", "DimExpr", "Expression",
  "LambdaExpression", "LambdaParameters", "LambdaParameterList",
  "IdentifierforLambdaParameterList",
  "IdentifierComplement_IdentifierforLambdaParameterList",
  "LambdaParameterforLambdaParameterList", "LambdaParameter",
  "LambdaParameterType", "LambdaBody", "AssignmentExpression",
  "Assignment", "LeftHandSide", "AssignmentOperator",
  "ConditionalExpression", "ConditionalOrExpression",
  "ConditionalAndExpression", "InclusiveOrExpression",
  "ExclusiveOrExpression", "AndExpression", "EqualityExpression",
  "SimpleRelationalExpression", "RelationalExpression", "ShiftExpression",
  "AdditiveExpression", "MultiplicativeExpression", "UnaryExpression",
  "PreIncrementExpression", "PreDecrementExpression",
  "UnaryExpressionNotPlusMinus", "PostfixExpression",
  "PostIncrementExpression", "PostDecrementExpression", "CastExpression",
  "SwitchExpression", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1221)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-806)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    4006,  1460,    81,   127,    68,  1197,  1813, -1221, -1221, -1221,
   -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221,   176,   167,
   -1221,  1197, -1221, -1221, -1221,  1484, -1221, -1221,  4574, -1221,
   -1221, -1221, -1221,   398, -1221, -1221,  6385, 13854, -1221, -1221,
   -1221, -1221, -1221,  3766, -1221, -1221, -1221, -1221, -1221, -1221,
   -1221, -1221, -1221,   720, -1221, -1221, -1221, -1221,    55, -1221,
   -1221,   511, -1221, -1221, -1221,   274, -1221,   568,   583,   188,
    1197, -1221,   137,  1197,   385,   380,   330,   188,   186,   601,
     473, -1221,  1603,   637,   741,   735,   762,   740, -1221, -1221,
     768,  6385,  3766, -1221,  3766, -1221,   433, -1221,   755,  1197,
    1197,  1269, -1221,   686, 11642,  1681,   686,    46,    46,   843,
   -1221,   276,  1601, -1221,   409, -1221,  1762,   758,   924,    77,
     705, -1221,   894,   635,   686, 11764,   880,   932,   188,   330,
     188,   946,  3766,  1197,   166,   294, 13628, -1221, -1221, -1221,
   -1221,   381, -1221, -1221, -1221, -1221, -1221, -1221, -1221,     2,
      74,   804, -1221,   803,  1123, -1221, -1221, -1221, -1221, -1221,
   -1221, -1221, 12344, 12344, -1221, 12344, 12344, 12344, 12344, 10662,
   -1221, 10784,   857,   872, -1221,   261, -1221, -1221,   927, -1221,
   -1221,   586,   367,   171, -1221,   750,  3514,   736, -1221,   877,
   -1221,    15,  1041, -1221, -1221, -1221, -1221, -1221, -1221, -1221,
   -1221, -1221,   -15,   975,   921,   925,   929,    12,   819, -1221,
     979,   489,  1000, -1221, -1221, -1221, -1221,   753, -1221, -1221,
   -1221, -1221, -1221,   406, -1221, -1221,  1085, -1221,   481,    70,
     562,   924,  1058, -1221,   743, -1221,   958, -1221,   965,  1151,
   -1221,   411,  1018,  1157,   924, -1221,  1005, 11582, -1221,   592,
    1021,   553, -1221,  1013,    67, -1221,   924, -1221,  1067,  1197,
    1684,  1197,  1805,  1805, -1221,  1001, -1221, -1221, -1221,   749,
    1029,   686,   686,   843,   705, -1221,   894,   686,   383, -1221,
    1261, -1221, -1221, -1221, -1221, 13820, -1221, -1221,  1306,   367,
   -1221,   171, -1221, -1221, -1221, 13681, -1221, -1221,  1504, 12008,
   -1221,  1090,  1216,  1039,  1187,   318,  1194, 10662, -1221, -1221,
   -1221,  1504,  1504, -1221,    50,    50,    57,    50,  1713,  1054,
     640, -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221,
    8162,  1043,   927,  3388,   150,  3514,  2760,  3105,  1055, -1221,
    1136, -1221, -1221, -1221,  3630, -1221, -1221, -1221,   -32, 12008,
    8287,  1213,   320,  1176, 10662,  1807,   384,   503,  1269,  1086,
   -1221, -1221,   425, -1221, -1221, -1221,    73, -1221,   367,   171,
    1809, -1221, -1221,  1117,  1652, 10662, 12344, 10662, 12344, 12344,
   12344, 12344, 12344, 12344, 12344, 12344,  1826, 12344, 12344, 12344,
   12344, 12344, 12344, 12344, 12344, 12344, 12344, -1221, -1221,   924,
   -1221,    46, -1221,  1085, -1221, -1221,  1067, -1221, -1221,  1298,
    1077, -1221,  1088,   924,  1157,  3668, -1221,  1083, -1221, -1221,
   -1221,  7497, 12429, -1221,  1060, 11582, -1221, -1221, -1221,  1868,
   -1221,   814,  1280, -1221, -1221, -1221,   361,  1094, -1221, -1221,
   10662, -1221, 10662,   639, -1221,  1091, -1221,  1005,   629,  1111,
    1018,  1805,  1119, -1221,    41, -1221,  1918,  1918, -1221,   210,
     570,  1141,  1297,  1223, -1221, -1221, -1221, -1221, -1221, -1221,
    1058, -1221,  1067, -1221,  1309, -1221,  1559,  1306,   367,   367,
    1156,   325, -1221,  1188, -1221, -1221,   967, -1221,   669, -1221,
   -1221,  2981, -1221, -1221, -1221,  1925, -1221, -1221,  1170,  1280,
   -1221, -1221,  1191,  1325, -1221,  1199,   795,   841, 10662,  1203,
     132,    50, -1221,  1203,    50,    57,  1195,  1203,    50, -1221,
   -1221,  8412, -1221,    57, -1221, -1221,  1275,  1088, -1221, -1221,
     -33,   150,   942,   665, -1221,  2082,  2475,  1210,  1234,  1287,
    1239, -1221,  1948, 12344,  1181, 12344,   367, -1221,  6456, -1221,
   -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221,
   -1221, 10662, 10906, -1221, -1221, -1221,   787, -1221, -1221,  1372,
   -1221, -1221, -1221,  1464,  1232,  1151, -1221,   513, -1221,   120,
    1236,  1280,    82, -1221,  1378,  1242, -1221,   367,  1246, -1221,
    1151,    92, -1221, -1221,  1271,   857, -1221, -1221, -1221,  2010,
   -1221, -1221,  1256,  1280, -1221,  1259,   975,  1252,   921,   925,
     929,    12, -1221, -1221,   979,   979, -1221, -1221, -1221,  3002,
    2485,  1868, -1221, -1221, -1221,   979,   979,   489,   489,   489,
    1000,  1000, -1221, -1221, -1221, -1221, -1221, -1221,  1005, -1221,
   -1221, -1221, -1221,  5801, -1221,   372,  1269,  2596,  1263, 10662,
    1264, 10662,  1267, 11325,  1278,  1282,   145, 10662,   346,  6706,
     660, -1221, 10662, -1221, -1221,  1277,  5076, -1221,  3002, -1221,
    2485, -1221,  7631, -1221, -1221,  1266,  1868, -1221, -1221, -1221,
   -1221, -1221, -1221,  1283, -1221, -1221, -1221, -1221, -1221, -1221,
   -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221,
   -1221,   288,   722, -1221, -1221, -1221,   753,  1154,  1173,  1868,
     814,   361, -1221,  1280,  1291,  3063, -1221,   643, -1221, -1221,
   -1221,  1293,  1392,   924,  7362,  1279, -1221,  1839,   796,   801,
   -1221, -1221,  1295, -1221, -1221, 10662, -1221, 10662, -1221,  1308,
   13459, -1221,  1918, -1221,   223,   337,  2013, -1221,  1918, -1221,
   -1221,  1805,  1067,  1119, -1221,  1583, -1221,  1296,   646, -1221,
   -1221,  1310,  1868, -1221,  6581,  3514,  3514, -1221,  1504, -1221,
     967, -1221, -1221,  8537,  1307, -1221, -1221,  1294, -1221, -1221,
    1311,  4394,  1467, 10662, -1221, -1221,  1472, -1221,  8662,  1479,
   -1221,  1005,   821, -1221,    57,  1088,  2017,   227,   306,  2030,
    1088,   532,   160,   942,  2034, -1221,  1303,  2876,   367,   367,
   -1221,   924,  6219, -1221, -1221, -1221, -1221, -1221, -1221, -1221,
   10662, -1221, -1221, -1221, -1221, -1221, -1221, -1221,  1254,  1655,
    8787,  1329,  1151, -1221, 11886, -1221,   367, -1221,  1151, -1221,
   -1221,  8912,  1332, -1221, 10662,  1868,   367,  1868, -1221,  1503,
   -1221, -1221, 11325, 10662,  1336, 10662,   -54, 10662,  1263,  1435,
   10662, 12130,  1341, -1221,  1344,  1349,  1350, -1221,  1352, -1221,
    1355,  5304,  1231,  1083, 11325,  1868,  1868, -1221, -1221, -1221,
    1388, -1221,   684, -1221,  1279, -1221,  1392,  2417,  1280, -1221,
    2889, -1221, -1221,  1389, -1221,     7,  1374, -1221,   418,  1424,
    7765,  7899,   537, -1221, -1221,   442,   840,  1375,  1404, -1221,
    3345,  3170,  1005,  1005, -1221,   844,   852,  1805, -1221, -1221,
   -1221, 13768, -1221,  3063,  1868,   814, -1221, 13512, -1221, -1221,
   -1221,  1527,   715, -1221, -1221, -1221, -1221,   729,   757,  1005,
   -1221, -1221,  1383, -1221,   107, -1221, -1221, -1221, -1221, -1221,
   -1221, -1221, -1221, -1221,   859,  9037,   112, -1221, -1221, -1221,
     595, -1221, 10662,  1390, 10662,  1005,   907, 10662, -1221,  1005,
   -1221, -1221, -1221,  2036,  2044,   564,   367,   367,  1088,  2092,
     589,   702,   761,   367,   367, -1221, -1221,  2475, -1221, -1221,
   -1221, -1221, -1221, -1221, -1221, -1221, -1221,  2110, -1221, -1221,
    1387,  1280, -1221,   951,  9162, -1221, -1221,   956,  1391, -1221,
   -1221, -1221,   971,  9287, -1221, -1221, -1221, -1221, -1221,  1394,
   -1221,  1400, 10662, -1221,  1402,  1393,  1403,  6831,  3002,  2485,
   -1221,  2142, -1221,  1397,  1411, -1221, -1221, -1221, -1221, -1221,
   -1221,   357,  3002,  2485,  2168,   539, -1221, -1221,    15,   732,
    1406,  1083,  1231, -1221, -1221,  1231, -1221,  1388,  1388, -1221,
   -1221, -1221, -1221,   972,  1410,  1392,   367,   976,  1412,   924,
    9412,  9537,  1432,  1433,   416, -1221,  8033, -1221,  1594,   388,
   -1221,   246,  3407, -1221, -1221,  3407,   802,   531, -1221, -1221,
    1005,  1005, -1221,  1868,   814, -1221, -1221, -1221,  1918, -1221,
   -1221, -1221, -1221,   110, 12008, -1221,   509,  1438, -1221, -1221,
     986, -1221, 12344, -1221, -1221,   138, -1221,   305,    -3, 11068,
    6206, -1221,  1463, -1221,  1466, -1221,  1005,  1486, -1221, -1221,
   -1221, -1221, -1221,  2178, -1221, -1221, -1221, -1221,  2191, -1221,
   -1221, -1221, -1221,  9662,  1497, -1221, -1221,   988, -1221, -1221,
   -1221, -1221,   995,  1083, 11449,  1496,  1294, 10662, 11325, 12534,
    1498,  2211,  2218,   220,   295,  6956, 13374,  2251,  2261,  1530,
    1533, -1221,  1890,   938, -1221, -1221, -1221,  1231, -1221,  3407,
   -1221, -1221, -1221, -1221,  3407,   367,   367, -1221,  1511,  1009,
    1512,  1019,  9787,  9912,   593,   909, -1221,  1518,   933,  1550,
     718, -1221,  1634, -1221,   998,  1525,  3170, -1221,   367,   367,
     309, -1221,   365, -1221, -1221, -1221,  1527, -1221,   521,  1524,
   -1221, -1221,  1526, -1221, -1221, -1221,  1549, -1221, -1221, -1221,
    1602, -1221, -1221,  1553,  6331, -1221, 11201,  1557, -1221, -1221,
   -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221,  1022,
   10037, -1221, -1221, -1221,   387,  1531,  1532,  1542,  1562, -1221,
    1651,  1660, -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221,
    1546, -1221, 11325,  1561,  1411, 12639,   341,   350,   414,   428,
   10662,  1585, 10662,  1593, 12744,  1573, -1221,  1597,  1605,  1606,
    1609, 10662, 10662, -1221, -1221,  1157,  2286,  1030,  1580,   973,
   -1221,  1581,  1582, -1221, -1221, -1221,  1587, -1221,  1588,  1589,
    1024,  1590,  1026, 10162,  1596, 10287,  1599, -1221, -1221, -1221,
   -1221,   420,   367, -1221,  1084, -1221, -1221,  1715, -1221,  1720,
   -1221, -1221,  1610, -1221, -1221, 12344, -1221, -1221,  1613, -1221,
   -1221, -1221,  1057, 11449, 10662, 10662, 12235, 11449, 11325,  1614,
   -1221, 11325, 11325,  1611, 10662,  1620, 10662,  1636, 10662,  1638,
   10662,  1639,  1623, 10662,  1625, 10662, 11325,  1626, 12849, 10662,
   10662, 10662, 10662, -1221, -1221,  1190,  1386,  1083,   367,  2286,
     367, -1221,   367, -1221, -1221, -1221,  1628, -1221,  1630,  1631,
    1076, 10412,  1632,  1081, 10537,  1787,   367,   367, -1221, -1221,
   -1221, -1221, -1221, -1221, -1221,  1641,  1642,  7081,  3002,  2485,
    2299,  1644, -1221, -1221, -1221, -1221, -1221, 11325,  1643, 10662,
    1650, 10662,  1669, 10662,  1670, 10662, 11325,  1671, 11325,  1672,
   -1221, 11325, 11325,  1673, -1221, -1221, -1221, -1221,   367,   367,
     367,   367, -1221, -1221,  1157, -1221, -1221, -1221, -1221, -1221,
    1649,  1653,  1082, -1221,  1690,  1705,  1112, 11449, 11449, 12954,
    1709,  2344,  2411,   434,   444,  7206, -1221, 11325,  1675, 11325,
    1722, 11325,  1727, 11325,  1728, -1221, 11325, -1221, 11325, -1221,
   -1221, 11325, -1221, -1221, -1221, -1221, -1221, -1221,  1731, -1221,
   -1221,  1732,  1734, -1221, 11449,  1737, 13059,   469,   480,   523,
     547, 10662,  1757, 10662,  1759, 13164,  1749, -1221, 11325, -1221,
   11325, -1221, 11325, -1221, 11325, -1221, -1221, -1221, -1221, -1221,
   11449, -1221, 11449, 11449,  1753, 10662,  1774, 10662,  1791, 10662,
    1792, 10662,  1798,  1783, 10662,  1786, 10662, 11449,  1789, 13269,
   -1221, -1221, -1221, -1221, -1221, -1221, -1221, 11449,  1795, 10662,
    1796, 10662,  1797, 10662,  1814, 10662, 11449,  1816, 11449,  1819,
   -1221, 11449, 11449,  1820, -1221, 11449,  1821, 11449,  1824, 11449,
    1825, 11449,  1828, -1221, 11449, -1221, 11449, -1221, -1221, 11449,
   -1221, 11449, -1221, 11449, -1221, 11449, -1221, 11449, -1221, -1221,
   -1221, -1221, -1221, -1221, -1221
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,     0,     0,
      32,     0,    33,    34,   134,     0,   135,    35,     0,    20,
      37,    38,    39,     0,   110,   111,     0,     0,   120,   122,
     123,   124,   125,   115,   130,   117,   132,   160,   161,   133,
     350,   351,   400,     0,   396,   397,   398,    44,   102,     2,
       5,     0,     3,     4,   107,     0,    42,     0,     0,   165,
       0,   106,     0,     0,     0,     0,     0,   165,     0,     0,
       0,   415,     0,     0,     0,     0,     0,     0,    36,     1,
       0,     0,   114,   121,   113,   116,     0,   131,     0,     0,
       0,     0,   399,     0,     0,     0,     0,     0,     0,   167,
     164,     0,     0,   119,     0,   126,     0,     0,     0,     0,
       0,   324,   356,     0,     0,     0,     0,     0,   165,     0,
     165,     0,   112,     0,     0,     0,     0,   374,     6,     7,
       8,   106,    49,    51,    50,    52,    53,    54,    55,     0,
     210,     0,   605,     0,     0,    14,    15,    12,    13,    16,
      17,    19,     0,     0,    18,     0,     0,     0,     0,     0,
      41,     0,   107,     0,   603,   209,    47,    48,     0,    56,
      57,   803,     0,    58,    62,   400,     0,     0,   403,     0,
     408,   802,   601,   604,   608,   622,   609,   610,   611,   612,
     602,   407,   755,   758,   760,   762,   764,   766,   772,   768,
     771,   779,   783,   786,   790,   791,   794,   797,   804,   805,
     800,   801,    45,   103,   109,   372,    78,   174,     0,     0,
       0,     0,   169,   166,     0,   108,     0,   128,     0,   106,
     178,   214,   177,    58,     0,   300,   340,     0,   330,   348,
       0,     0,   331,     0,     0,   323,     0,   355,   171,     0,
       0,     0,     0,     0,   143,     0,   144,   376,   402,     0,
       0,     0,     0,   167,     0,   322,   356,     0,     0,   118,
       0,   210,   386,   385,   378,     0,   209,   383,     0,   208,
     207,   212,   211,   384,   382,     0,   379,   381,     0,     0,
     213,     0,     0,     0,     0,     0,     0,     0,     9,    10,
      11,     0,     0,   636,     0,     0,     0,     0,     0,     0,
     634,   795,   796,   792,   793,   799,   798,   708,   716,   717,
       0,     0,    82,   803,     0,     0,   609,   610,     0,   703,
       0,   709,   704,   738,     0,   737,   412,   413,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     221,    71,     0,    67,    68,    69,     0,   222,     0,    59,
       0,    40,    43,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   806,   807,     0,
      79,     0,   173,    76,   172,   176,   171,   168,   142,     0,
       0,   127,     0,     0,    59,     0,   339,    25,   232,   194,
     193,     0,     0,   191,     0,   349,   183,   185,   189,   231,
     190,     0,     0,   186,   187,   188,     0,     0,   192,   286,
       0,   347,     0,     0,   327,     0,   329,   336,   344,     0,
     354,     0,     0,   170,     0,    96,     0,     0,    97,     0,
       0,     0,     0,     0,   139,   145,   401,   416,   373,   371,
     169,   321,   171,   375,     0,   141,     0,     0,   203,   204,
       0,     0,   197,   200,   377,   380,    93,    88,     0,    86,
      89,     0,   405,   677,   685,     0,   642,   645,     0,     0,
     620,   618,     0,     0,   619,     0,     0,     0,     0,     0,
       0,   691,   699,     0,   695,     0,     0,     0,   693,   635,
      46,     0,   640,     0,   639,   633,   712,   276,   734,   707,
     733,   208,    58,    62,   732,     0,     0,     0,   711,   713,
     710,   718,     0,     0,     0,     0,    46,   607,     0,   744,
     745,   746,   747,   748,   749,   750,   751,   752,   753,   754,
     743,     0,     0,   411,   406,   653,     0,   670,   617,     0,
     675,   689,   683,     0,     0,   108,   616,     0,   606,     0,
       0,     0,     0,   623,     0,     0,    74,    65,     0,    70,
       0,     0,    60,    61,     0,     0,   404,   673,   681,     0,
     641,   644,     0,     0,   624,     0,   759,     0,   761,   763,
     765,   767,   769,   770,   777,   778,   429,   773,   428,     0,
       0,     0,   774,   596,   597,   775,   776,   780,   781,   782,
     784,   785,   787,   788,   789,    82,   175,    77,     0,   138,
     129,   179,   182,     0,   287,   106,     0,    26,    30,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   456,     0,   457,   418,   107,     0,   430,     0,   431,
       0,   443,     0,   419,   421,     0,     0,   422,   423,   432,
     444,   433,   445,     0,   434,   435,   446,   447,   436,   448,
     437,   497,   498,   449,   455,   450,   451,   453,   452,   454,
     560,   608,   611,   463,   464,   465,     0,   466,   467,   231,
       0,     0,   231,     0,     0,     0,   184,     0,   285,   224,
     284,     0,   226,     0,     0,     0,   291,     0,     0,     0,
     328,   332,     0,   326,   335,     0,   343,     0,   104,   180,
       0,   353,     0,   151,     0,     0,     0,   147,     0,   149,
     153,     0,   171,     0,   140,     0,   137,     0,     0,   201,
     202,     0,     0,   368,     0,     0,     0,    92,     0,    85,
      91,   676,   684,     0,     0,   621,   615,     0,    84,    83,
       0,     0,   696,     0,   690,   700,   698,   694,     0,   697,
     692,   632,     0,   638,     0,   278,     0,     0,   733,     0,
     277,   733,   208,    59,     0,   706,     0,     0,   725,   731,
     808,     0,     0,   809,   736,   735,   705,   739,   410,   414,
       0,   652,   614,   674,   688,   682,   647,   217,     0,     0,
       0,     0,   215,   613,     0,    66,    63,   218,   219,   672,
     680,     0,     0,   648,     0,     0,   208,     0,   600,    80,
     163,   181,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   548,     0,     0,     0,   550,     0,   554,
       0,     0,     0,     0,     0,     0,     0,   417,   420,   424,
     427,   462,     0,   223,     0,   289,   230,     0,     0,   196,
       0,   225,   283,   280,   281,     0,   605,   304,     0,   740,
       0,     0,   802,   290,   299,     0,     0,     0,   247,   261,
       0,     0,   346,   338,   325,     0,     0,     0,   366,   365,
     358,     0,   363,     0,   231,     0,   364,     0,   359,   361,
     362,   100,     0,   148,   146,    98,    99,     0,     0,     0,
     352,   136,     0,   367,     0,   198,   206,   199,   205,    94,
      95,    87,    90,   665,     0,     0,     0,   812,   702,   157,
       0,   158,     0,     0,     0,   628,     0,     0,   631,   630,
     637,   269,   275,     0,     0,     0,   723,   729,   279,     0,
       0,    60,    61,   724,   730,   714,   715,     0,   719,   722,
     728,    81,   811,   810,   671,   679,   687,     0,   643,   646,
       0,     0,   657,     0,     0,   216,    73,     0,     0,    64,
     220,   661,     0,     0,   757,   756,   598,   599,   458,     0,
     555,     0,     0,   473,     0,     0,     0,     0,     0,     0,
     518,     0,   520,     0,   517,   546,   547,   549,   551,   552,
     553,   214,     0,     0,     0,     0,   585,   593,     0,   609,
       0,     0,   557,   561,   559,   582,   459,   425,   426,   195,
     288,   229,   295,     0,     0,   228,   240,     0,     0,     0,
       0,     0,     0,     0,     0,   302,     0,   303,     0,   254,
     246,   260,     0,   297,   298,     0,     0,     0,   345,   337,
     342,   334,   105,   231,     0,   370,   357,   360,     0,   152,
     150,   154,   162,     0,     0,   394,     0,     0,   664,   663,
       0,   488,     0,   478,   482,     0,   485,     0,     0,     0,
       0,   155,     0,   701,     0,   627,   626,     0,   629,   268,
     274,   265,   271,     0,   720,   726,   267,   273,     0,   721,
     727,   678,   686,     0,     0,   656,   655,     0,    72,    75,
     660,   659,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   203,   204,     0,     0,     0,     0,     0,
       0,   583,     0,     0,   578,   562,   558,   580,   581,     0,
     293,   294,   227,   236,     0,   238,   239,   282,     0,     0,
       0,     0,     0,     0,     0,     0,   301,     0,     0,     0,
       0,   251,     0,   257,     0,     0,     0,   248,   252,   258,
     253,   243,   259,   341,   333,   369,   101,   390,     0,     0,
     395,   392,     0,   393,   662,   492,   487,   491,   476,   483,
       0,   477,   486,     0,     0,   489,   484,     0,   156,   159,
     649,   651,   625,   650,   264,   270,   266,   272,   669,     0,
       0,   654,   658,   556,   106,     0,     0,     0,   107,   470,
       0,   432,   439,   440,   441,   442,   499,   500,   474,   475,
       0,   494,     0,     0,   519,     0,   203,   204,   203,   204,
       0,   201,     0,   202,     0,     0,   521,     0,     0,     0,
       0,     0,     0,   584,   586,   576,     0,     0,     0,     0,
     579,     0,     0,   234,   235,   312,     0,   308,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   262,   263,   244,
     245,   254,   260,   296,     0,   249,   255,     0,   250,     0,
     256,   388,     0,   389,   391,     0,   480,   481,     0,   490,
     668,   667,     0,     0,     0,     0,     0,     0,     0,     0,
     508,     0,     0,     0,     0,   201,     0,   202,     0,   201,
       0,   202,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   589,   592,     0,     0,     0,   569,     0,
     575,   292,   237,   311,   307,   310,     0,   306,     0,     0,
       0,     0,     0,     0,     0,     0,   253,   259,   241,   242,
     387,   493,   479,   666,   460,     0,     0,     0,     0,     0,
       0,     0,   461,   471,   496,   507,   506,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     504,     0,     0,     0,   587,   590,   588,   591,   567,   573,
     568,   574,   563,   566,   577,   572,   233,   309,   305,   316,
       0,     0,     0,   320,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   203,   204,     0,   505,     0,     0,     0,
       0,     0,     0,     0,     0,   527,     0,   533,     0,   503,
     502,     0,   564,   570,   565,   571,   315,   314,     0,   319,
     318,     0,     0,   495,     0,     0,     0,   203,   204,   203,
     204,     0,   201,     0,   202,     0,     0,   525,     0,   531,
       0,   526,     0,   532,     0,   524,   530,   501,   313,   317,
       0,   516,     0,     0,     0,     0,   201,     0,   202,     0,
     201,     0,   202,     0,     0,     0,     0,     0,     0,     0,
     522,   528,   523,   529,   472,   515,   514,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     512,     0,     0,     0,   513,     0,     0,     0,     0,     0,
       0,     0,     0,   539,     0,   545,     0,   511,   510,     0,
     537,     0,   543,     0,   538,     0,   544,     0,   536,   542,
     509,   534,   540,   535,   541
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1221,  -104,  -167,  -285, -1221,   -21,  -122, -1221, -1221, -1221,
   -1221,    22,  1811,    -7, -1221, -1221,    24,  -112, -1221,  1399,
    -296,  -152, -1221, -1221, -1221,  1425,  1558,  1331,    -5,  -501,
    1044,  1235, -1221,  1237,  -232,  1257,    -2,  1253,  1260, -1221,
   -1221, -1221, -1221,  1970,   369, -1221, -1221, -1221, -1221,   101,
     594,  1971,  -185,  -241,   449, -1221,   -92, -1221,    36,  1736,
    1547,  -363,   -42,  1912, -1221,    28,  1767, -1221,  -229,  1615,
    -371, -1221, -1221,  -275,  -563, -1221,  -735,  3969,  2401, -1221,
    2070,  2433, -1221,  -391,  -421,  -657,   159,  -814, -1221,  -301,
    -308,  -670, -1221,   960,  -671, -1221, -1221, -1221,  1612,   -69,
    -648, -1221, -1221,   267, -1221,  1608,  -200,  -117,  -410, -1221,
    1756,  1284, -1221,  1113,  -683, -1221, -1221,   -44, -1221,  1738,
   -1221, -1006,   -43,  1462, -1221,  -116,  1679,  -123, -1221, -1221,
   -1221, -1221,  -202,  -692,  -655, -1221,  -843,  -361, -1221,  4623,
    1185,   837, -1221, -1221, -1221, -1221,  -802, -1221, -1221, -1221,
   -1221, -1221,   889,   937, -1221,   940, -1221,  -660, -1221,   731,
   -1221, -1221, -1221, -1221, -1221, -1221, -1221, -1221,   727, -1220,
    -831, -1221, -1221, -1221, -1221, -1221, -1221,   834, -1221, -1221,
    1023,  -534, -1221,    98,  -433, -1221, -1221, -1221,   908, -1221,
   -1221, -1221, -1221,  -684, -1221, -1221,   446,  2408,  -340,  1766,
   -1221, -1221,   463,  2787,  2729,   766, -1221, -1221,   692,   565,
     730,  -686, -1221, -1221, -1221,  -294, -1221,  1276,  -498, -1221,
   -1221,  3185, -1221, -1221,   -99, -1221,  1711,  1707,  1710,  1712,
    1716, -1221,  1038,   632,  1106,   978,   -51,  3441,  3687,  1286,
    3943,  4189,  4445, -1221, -1221
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    64,   173,   313,   174,    27,    28,    29,    30,    31,
      32,    65,   331,   175,   176,   177,   178,   179,   180,   360,
     361,   362,   363,   364,   365,   227,   400,   544,   316,   300,
     488,   489,   490,   767,   931,   932,    66,   739,   333,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,   265,   266,   946,   960,    46,    47,   109,   232,
     406,   452,   424,   228,   233,   407,   242,   453,   416,   425,
     426,   427,   428,   880,   482,   483,   947,   618,   182,   290,
     183,   184,   430,   431,   432,   722,   906,   907,   908,   909,
     668,   725,   893,   894,   719,   433,   434,   435,   436,   437,
     726,   900,    48,   121,   251,   252,   253,    49,    50,   257,
     258,   741,   927,   928,   294,   930,    51,   137,   295,   296,
     297,  1107,    52,   186,    54,   187,   188,   189,   190,   348,
      55,    56,   671,   672,   673,   674,   675,   676,   677,   678,
    1260,   679,   680,   681,  1262,   682,   683,   684,   685,  1263,
     686,   687,   957,  1114,  1115,  1116,  1117,  1118,  1119,  1225,
    1226,   688,  1264,   689,   690,  1265,   691,  1266,  1033,  1273,
    1274,   692,  1267,   693,   694,   695,   696,   697,   698,   699,
    1052,  1053,  1298,  1299,  1054,   700,   873,  1045,  1046,  1047,
     622,   623,   624,   191,   192,   193,   357,   194,   195,   319,
     320,   525,   336,   337,   198,   566,   199,   200,   511,   512,
     567,   339,   340,   537,   538,   341,   540,   541,   542,   816,
     342,   343,   344,   561,   345,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     172,   224,   270,   713,    74,   201,   240,    88,   224,   269,
     102,   669,   224,   481,   285,   249,   583,   878,  1030,   293,
     441,   172,   535,    81,   465,   621,   201,   110,   459,   534,
    1034,   710,   901,   519,   604,   110,   539,   799,   804,   883,
     902,   884,   315,   638,   287,   439,   961,    80,   347,   409,
     250,   445,   891,   102,   716,  1353,   886,   929,   848,  1032,
     226,   185,   225,   885,  1367,   112,   589,   358,   308,   309,
     310,   508,   201,  1064,   827,   245,  1068,   903,   619,  -106,
     267,   447,   185,   376,   403,   245,   110,   590,   110,   837,
     796,   246,    59,    60,   303,   476,   832,  1219,  1234,    81,
    1222,  1022,    59,    60,   120,   301,   838,   382,   383,   753,
     301,   321,   322,   122,   323,   324,   325,   326,   373,   405,
     562,   480,  1023,    80,   358,   422,   992,   358,   185,   286,
     438,   308,   309,   310,  1111,   498,   311,    92,    94,   377,
     742,    69,   101,   102,   240,    68,   563,   286,  1433,   358,
     448,    70,  1235,   783,   717,   423,   458,   274,  1014,   862,
    1111,   112,   510,   510,   273,   510,   276,   358,   465,   302,
      59,    60,  1104,   285,   302,  1104,   492,   358,   293,   286,
    1070,    77,   374,  1112,   479,   449,   102,  1048,   358,   580,
     112,    76,   132,   332,   366,    62,    63,   494,   497,   945,
     201,   102,   758,   287,   107,    62,    63,   602,  1076,  1112,
     112,   102,  1332,   439,   589,   644,  1061,   743,   734,   736,
     359,   101,  -106,   439,   744,   745,   564,   468,   469,   720,
     519,  1065,   101,   473,   746,   465,  1060,   358,   793,  1495,
     286,   304,   101,   732,   929,   101,   878,   746,   101,   572,
     201,   224,   101,   247,  1095,   248,   185,   101,   845,   847,
     461,   463,   669,   358,    88,   721,   595,   101,  1205,   598,
     601,  1207,   716,    62,    63,   102,  1524,   359,   286,   312,
     359,   303,  1016,  1105,  1017,  1538,  1217,   635,   286,   755,
    1113,   286,   102,   422,   888,   101,   495,   499,   438,   666,
     112,   641,   588,   422,   286,   286,   185,   875,   438,   876,
     757,   286,   358,   113,   669,   524,  1228,   665,   112,  1563,
     359,   863,   487,   423,   545,   479,   358,  1111,   286,   667,
     359,  1005,   774,   423,   -46,   487,   487,  1010,   502,   249,
     502,   359,   279,   632,   633,   634,   814,   108,   573,   925,
     973,   581,   458,   458,   332,   714,   586,  1032,   358,   510,
     866,   746,   510,   123,  1286,  1301,   510,   358,   599,   603,
    1302,    59,    60,   479,   250,  1280,  1112,   303,   354,   286,
     585,   355,   358,   118,   358,  1239,   747,   255,   298,   939,
     359,   772,   138,   139,   140,  1182,   275,   298,    89,   933,
     311,    88,   101,   298,   116,   358,    93,   112,   286,   850,
     617,   576,  1199,  1202,   831,   286,   359,   286,   286,   910,
     577,   878,   723,  1215,   138,   139,   140,  1236,   351,   974,
    -108,   358,   311,   112,   882,   355,   842,   358,   809,   819,
    -469,   439,   358,   576,  1199,   358,   578,   104,   102,   738,
    1282,   358,  1194,   234,  1072,  1230,  1079,  1233,   872,  1237,
      93,   358,  -469,   201,  -469,   359,   579,    59,    60,   825,
      98,   280,   102,  1032,    62,    63,  1327,   762,   578,   359,
    1073,   101,  1032,  1231,   286,   503,   358,   569,  1048,   804,
     669,   669,   810,   102,   813,   840,  1354,   358,   579,   535,
    1031,   763,   713,  1030,  1080,  1356,   534,   119,   720,   970,
    1044,   359,   986,   934,   465,  1034,   100,   479,  1175,   185,
     359,   422,   867,   502,   356,   103,   438,   852,   286,   286,
    1094,  -594,  1329,  -594,   299,   359,   925,   359,   724,   102,
     358,   471,  1343,   312,  1032,  1210,   721,   117,   102,   665,
     666,   423,   864,  1028,   868,  1200,    59,    60,   359,   101,
     474,   115,   968,  1042,   358,   796,  1032,   196,   665,  1358,
      62,    63,   479,  -108,  1104,   312,   236,   102,   412,   910,
     667,   878,   910,  1360,   359,   237,  1104,  1395,   196,  1501,
     359,   101,   105,  1211,   112,   588,   305,   106,   359,  1503,
    1057,  1058,   666,  -214,   359,   479,   303,   354,   101,  -214,
     355,   892,   286,   286,   359,   124,   828,   101,   921,  1176,
     665,   352,  1178,   926,  1525,   196,   196,   102,   196,   196,
     196,   196,   667,   401,   196,  1527,   286,    97,   458,   359,
     373,   402,   936,  1175,   458,    88,   125,   245,   922,   481,
     359,   392,   393,   246,   979,   979,   311,  1032,   479,    62,
      63,   286,  1000,   286,    59,    60,   669,  1161,  1162,   748,
     584,  1055,   102,   259,   260,   261,   262,   263,  1529,  -211,
     829,  1167,  1168,  1088,  1089,  1221,    97,  1133,    97,  -214,
    -211,  -211,   972,   359,  1032,   977,   828,  1331,   923,   991,
     984,   127,  1531,  1032,  1078,   443,   101,   101,   286,   669,
    1102,  1008,  1138,  1171,   401,  1172,  -221,   359,  1007,   898,
     286,   721,   404,   720,   996,   999,    97,  -221,  -221,   247,
     172,   444,  1318,   286,   583,   201,  1125,  1032,   604,   101,
    1128,   479,   522,   479,  1300,  1015,   749,  1120,   665,   738,
     129,   102,   102,   356,   102,   128,  -214,    98,   286,   286,
     829,   286,   196,    99,   101,   440,  1313,    62,    63,   415,
     665,   479,   479,  1121,   910,  -222,   130,   910,   666,   666,
    1319,   259,   260,   261,   262,   263,  -222,  -222,  -211,   949,
     950,   185,   487,  -211,  -211,   762,   665,   665,   762,   312,
     286,  1081,   735,   100,   131,   921,   415,   523,   667,   667,
     926,  1044,   196,   264,   101,   247,  1208,   730,   758,   889,
     479,   768,   943,   997,  1001,  -221,   669,    59,    60,   769,
    -221,  -221,    79,   871,  1144,   922,   762,   421,   133,   196,
    -211,   196,   196,   196,   196,   196,   196,   196,   196,  1174,
     196,   196,   196,   196,   196,   196,   196,   196,   196,   196,
    1059,  1213,  1214,   136,  1042,  1296,  1216,  1098,   102,  1130,
    1132,   910,   397,   398,  -468,  1137,   910,  -221,   231,   923,
     286,  1098,   119,   286,  -222,   923,  1057,  1058,   370,  -222,
    -222,  1099,   720,  1142,   271,   101,  -468,  1242,  -468,   338,
      88,   370,  -409,   286,   286,  1100,  -595,   580,  -595,   917,
     371,   602,   384,   385,   286,  1092,   286,   138,   139,   140,
     286,   408,   238,   466,  -409,   974,  -409,  1164,  -409,   256,
      62,    63,   102,  1101,   386,   102,  -222,   102,   239,   820,
    1170,   138,   139,   140,   102,  1314,   272,   768,   820,    59,
      60,  1253,   239,   820,   666,   778,  -212,   892,   782,   358,
     277,   821,   786,    59,    60,   366,   789,  -212,  -212,  1316,
     912,   306,   665,   820,   601,   913,   307,   101,   387,   388,
     286,  1220,  1209,  1212,   667,  1410,   102,  1378,   527,   479,
     718,   421,  1082,   768,   458,   969,   820,   666,    59,    60,
     102,   779,   765,   766,   820,   201,   196,   514,   196,   518,
     349,   820,  1321,  1227,  1083,   665,   614,   615,  1090,   625,
     626,   286,   286,    59,    60,   196,  1091,   667,   831,  1245,
     353,   842,  1336,  1108,  1247,   286,   286,   505,  1408,   259,
     260,   261,   262,   263,   239,   350,  1063,  1461,  1462,  1067,
    1258,   372,    62,    63,   665,    59,    60,  1277,  1279,   820,
     338,   185,   375,  1288,  1290,  -212,    62,    63,   245,  1195,
    -212,  -212,   378,  1198,   239,   286,   785,   379,   286,   785,
     800,  1126,   380,   785,   574,    59,    60,   381,   142,   143,
     144,   145,   146,   147,   148,   389,   390,   391,  1396,   101,
    1322,    62,    63,   820,   493,   605,   311,   607,   370,    59,
      60,   118,   359,   101,   666,    59,    60,  -212,   281,   418,
     399,   796,   410,   820,  1179,  1145,    62,    63,  1184,  1379,
    1148,   597,   665,   311,   308,   309,   310,   239,   820,   311,
     820,   411,    59,    60,   667,  1150,  1180,   820,    59,    60,
    1185,   142,   143,   144,   145,   146,   147,   148,    62,    63,
    1224,   820,  1251,   102,   394,   395,   396,   298,   665,  1252,
     413,   820,   286,   101,   820,  1442,   820,   286,   820,   464,
     366,   281,   415,  1306,   811,   812,  1057,  1058,    62,    63,
     570,   446,   311,  1308,   442,  1380,  1340,   451,  1386,   286,
    1388,    59,    60,   467,  1438,   101,   728,   979,   729,   820,
     500,    71,    62,    63,   501,    59,    60,   543,    62,    63,
    1397,   504,    59,    60,   138,   139,   140,   521,   820,   547,
     496,  1403,   311,   820,   820,   101,  1227,   548,   780,  1258,
     568,    59,    60,  1258,   665,    62,    63,   665,   665,   312,
    1450,    62,    63,   640,   102,  1454,  1488,   587,   571,   101,
     421,    67,   665,   101,   820,    72,    75,   727,   995,   733,
     311,  1439,  1441,  -804,  -804,   196,   312,    57,   815,    59,
      60,    78,   312,    58,   737,    67,  1491,   792,   138,   139,
     140,   817,  -805,  -805,    59,    60,   740,   196,   101,   259,
     260,   261,   262,   263,    62,    63,  1464,   196,   985,   328,
     329,  1050,  1051,   665,   138,   139,   140,   750,    62,    63,
     478,   116,   665,   751,   665,    62,    63,   665,   665,   761,
     111,    59,    60,   114,  1049,   312,   259,   260,   261,   262,
     263,   764,    67,   773,    62,    63,  1379,   259,   260,   261,
     262,   263,   776,  1258,  1258,   506,   507,  1498,  1500,   134,
     135,    67,   775,   665,   181,   665,   102,   665,   788,   665,
     630,   631,   665,   777,   665,   312,  -708,   665,   241,   854,
     781,   856,    62,    63,   805,   181,   806,   865,  -709,   870,
    1258,   807,   338,   278,  1375,  1376,   241,    62,    63,   822,
    1440,   286,   286,   826,   665,   833,   665,   844,   665,   830,
     665,    59,    60,   312,   241,   834,  1258,   836,  1258,  1258,
     612,   613,   181,   181,   299,   181,   181,   181,   181,   841,
     843,   181,   874,  1258,    62,    63,   853,   855,  -214,   475,
     857,  -214,   879,  1258,   303,   354,   241,  -214,   355,  -214,
    -214,   860,  1258,   723,  1258,   861,   724,  1258,  1258,   881,
     917,  1258,    53,  1258,   887,  1258,   890,  1258,    57,   942,
    1258,   956,  1258,   914,    58,  1258,   639,  1258,   823,  1258,
     955,  1258,   958,  1258,   944,    59,    60,   754,   962,    59,
      60,   241,    57,   964,   948,   627,   628,   629,    58,    96,
     967,   915,  1004,   916,   241,  1013,   811,   241,  1025,    59,
      60,   948,  1020,   963,    62,    63,   241,  1035,   239,   454,
    1036,   460,   462,   462,    61,  1037,  1038,  -214,  1039,    59,
      60,  1040,   142,   143,   144,   145,   146,   147,   148,   954,
     762,  1069,  1379,  -803,  -803,   241,   824,  1071,    79,  1084,
     994,   746,  -214,  -214,   966,   241,  1085,  1103,   241,   181,
    1143,  1123,   281,  1166,  1317,  1149,  1157,   196,  1153,   229,
     229,   241,   241,  1165,  1154,   196,  1156,  1158,   241,  1173,
     244,   254,   367,  1019,  1181,  1021,  1186,  1024,    62,    63,
    1026,  1074,    62,    63,  -214,   241,  1003,   259,   260,   261,
     262,   263,   138,   139,   140,  1192,  1193,  1012,   600,   181,
     311,    57,    62,    63,  1223,   235,   318,    58,    67,    59,
      60,   259,   260,   261,   262,   263,    59,    60,    59,    60,
    1197,   335,    62,    63,  1240,  1049,   181,  1241,   181,   181,
     181,   181,   181,   181,   181,   181,   241,   181,   181,   181,
     181,   181,   181,   181,   181,   181,   181,  1243,   486,   241,
     138,   139,   140,   138,   139,   140,   600,   126,   311,   998,
    1250,   311,  1268,   241,  1275,   241,   579,    59,    60,   101,
      59,    60,   241,  1291,   241,   241,  1292,  1305,  1307,   222,
     367,  1315,  1122,   244,  1124,   223,  1320,  1127,   455,  1323,
    1333,  1335,  1334,  1234,  1344,  1345,    59,    60,  1235,    59,
      60,   462,  1339,   509,   513,  1346,   517,  1347,   244,  1348,
    1349,  1110,    62,    63,   308,   309,   310,   239,  -438,    62,
      63,    62,    63,   456,   579,  1351,   457,   756,    59,    60,
    1363,   142,   143,   144,   145,   146,   147,   148,  1365,  1368,
    1369,   241,  1155,   312,  1377,  1381,  1382,  1160,  1370,  1371,
     491,   941,  1372,  1383,  1384,  1385,  1387,   592,   593,  1391,
    1147,   281,  1394,   491,   491,  1419,   223,  1398,   515,  1152,
      62,    63,  1399,    62,    63,  1417,  1400,    59,    60,  1402,
    1414,  1421,   536,  1423,  1425,   241,   241,  1426,   196,  1428,
    1431,  1318,  1520,   181,  1447,   181,  1448,  1449,  1453,    62,
      63,   312,    62,    63,   312,  1457,  1458,  1467,   582,    58,
    1465,   575,   181,   594,  1469,  1486,   636,    58,   591,  1487,
      59,    60,    59,    60,    59,    60,  1189,  1191,    59,    60,
     239,    62,    63,  1471,  1473,  1476,  1478,  1481,   620,  1508,
     948,    59,    60,   239,   142,   143,   144,   145,   146,   147,
     148,   244,    73,   229,    59,    60,  1489,   142,   143,   144,
     145,   146,   147,   148,   582,   244,   527,   759,   760,   241,
     241,  1490,   478,   670,   281,  1496,   715,  1270,   101,   527,
      62,    63,   616,    59,    60,  1285,  1510,   281,   138,   139,
     140,  1512,  1514,   241,   239,   254,    67,  1518,  1519,  1249,
     784,  1522,  1534,   787,  1536,    59,    60,   790,   142,   143,
     144,   145,   146,   147,   148,  1539,   149,  1547,   241,  1549,
     241,   367,   455,    62,    63,    62,    63,    62,    63,   771,
     527,    62,    63,    59,    60,   592,  1551,  1553,   150,   151,
      59,    60,   152,  1555,    62,    63,   616,  1556,  1310,  1312,
    1558,   637,   808,  1561,  1338,   314,   849,    62,    63,  1565,
    1567,  1569,   154,    59,    60,   241,   155,   156,   157,   158,
     159,   160,   161,   241,   899,   794,   835,   241,  1571,   795,
    1574,  1261,   797,  1576,  1579,  1581,    62,    63,  1583,  1585,
     241,   101,  1587,   951,   938,   937,    91,   952,    95,   470,
    1362,   462,  1364,   904,   101,   164,  1342,   752,    62,    63,
     230,  1373,  1374,   450,   839,   241,   241,   935,   241,  1187,
     643,   971,   472,   485,   711,    59,    60,   940,    59,    60,
    1097,   515,    59,    60,   976,  1269,    62,    63,   983,   596,
    1129,   731,  1229,    62,    63,    59,    60,  1232,  1131,    59,
      60,    59,    60,   662,  1293,   101,  1401,   241,  1337,    59,
      60,   241,   181,  1411,  1405,  1406,    62,    63,  1177,  1390,
    1294,  1393,   516,   988,  1418,   608,  1420,   606,  1422,   609,
    1424,     0,   610,  1427,   181,  1429,   239,   611,   993,  1434,
    1435,  1436,  1437,     0,   181,     0,  1136,    59,    60,   795,
     142,   143,   144,   145,   146,   147,   148,    59,    60,     0,
       0,     0,     0,     0,  1141,     0,     0,     0,     0,     0,
       0,  1041,     0,     0,   670,    59,    60,  1460,    62,    63,
     281,    62,    63,     0,     0,    62,    63,   241,   528,  1468,
     241,  1470,     0,  1472,     0,  1474,  1163,  1452,    62,    63,
    1456,     0,    62,    63,    62,    63,     0,    59,    60,     0,
     241,   241,    62,    63,     0,     0,     0,   462,     0,     0,
    1261,   241,  1169,   241,  1261,   244,   670,   241,   243,   911,
       0,     0,  1244,    59,    60,  1506,     0,     0,     0,     0,
       0,   981,   982,    59,    60,  1246,   291,   989,   990,     0,
      62,    63,     0,     0,     0,     0,    59,    60,     0,     0,
      62,    63,     0,     0,   243,  1276,     0,     0,     0,     0,
     491,  1533,  1278,  1535,     0,  1009,    59,    60,    62,    63,
       0,     0,     0,    59,    60,   981,     0,   241,     0,     0,
       0,     0,     0,     0,     0,  1548,   369,  1550,     0,  1552,
     975,  1554,   978,   980,  1557,  1287,  1559,     0,     0,   987,
      62,    63,     0,   244,     0,  1289,    59,    60,     0,  1566,
       0,  1568,     0,  1570,     0,  1572,    59,    60,   241,   241,
       0,     0,     0,     0,  1261,  1261,    62,    63,     0,     0,
     239,   243,   241,   241,     0,     0,    62,    63,     0,     0,
       0,    59,    60,  1463,   414,     0,     0,   291,     0,    62,
      63,     0,     0,  1029,    59,    60,   243,     0,     0,   241,
       0,  1261,     0,  1043,     0,     0,     0,     0,     0,    62,
      63,     0,   241,  1106,     0,   241,    62,    63,     0,   911,
       0,     0,   911,     0,     0,   291,     0,  1261,  1497,  1261,
    1261,     0,   670,   670,   181,   291,     0,   797,     0,    59,
      60,     0,   181,     0,  1261,  1134,  1135,     0,     0,    62,
      63,     0,  1139,  1140,  1261,   715,     0,     0,   414,    62,
      63,     0,     0,  1261,     0,  1261,     0,     0,  1261,  1261,
     532,     0,  1261,     0,  1261,   369,  1261,     0,  1261,     0,
       0,  1261,     0,  1261,    62,    63,  1261,     0,  1261,     0,
    1261,     0,  1261,     0,  1261,  1499,     0,    62,    63,     0,
       0,   239,  1041,   241,     0,     0,    59,    60,     0,   241,
       0,     0,    59,    60,   241,   142,   143,   144,   145,   146,
     147,   148,     0,     0,     0,     0,   532,     0,     0,     0,
       0,     0,     0,     0,     0,  1183,   241,   527,     0,   243,
       0,     0,    62,    63,     0,   281,     0,     0,  1201,     0,
    1203,     0,     0,   243,     0,   291,     0,     0,     0,   239,
       0,   532,   291,     0,   291,   291,     0,     0,     0,   239,
      59,    60,  1218,   142,   143,   144,   145,   146,   147,   148,
      59,    60,     0,   142,   143,   144,   145,   146,   147,   148,
       0,     0,     0,     0,     0,   800,     0,     0,  1404,     0,
       0,   244,  1412,   281,     0,   800,   582,   289,   670,    62,
      63,   528,     0,   281,  1206,    62,    63,  1206,   975,   980,
       0,   616,     0,     0,     0,   317,   241,   241,     0,     0,
       0,   369,  1281,  1283,     0,     0,     0,     0,     0,   292,
     334,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   670,     0,     0,  1303,  1304,     0,   368,     0,     0,
       0,  1062,   101,     0,     0,   181,     0,     0,     0,     0,
       0,     0,     0,    62,    63,   291,   803,  1325,  1326,  1328,
    -276,  1330,     0,    62,    63,     0,     0,     0,     0,     0,
       0,  -276,  -276,     0,  -276,  -276,  -276,  -276,  -276,  -276,
    -276,     0,     0,     0,  1043,  1297,     0,     0,     0,   241,
       0,  1206,  1492,  1493,     0,     0,  1206,     0,   289,     0,
     101,     0,     0,     0,  -276,     0,     0,     0,     0,     0,
     101,     0,  -276,     0,     0,     0,   797,     0,   241,   241,
       0,     0,     0,     0,     0,  1355,  1357,  1359,  1361,  1521,
     292,     0,     0,     0,     0,     0,   289,     0,     0,   291,
     803,     0,     0,     0,     0,     0,   289,     0,   670,     0,
       0,     0,     0,     0,     0,  1544,     0,  1545,  1546,     0,
       0,     0,     0,   291,     0,     0,     0,     0,   292,   520,
    1201,  1203,  1560,     0,  -276,  -276,     0,     0,   292,     0,
       0,   531,  1564,     0,     0,     0,   546,     0,   291,     0,
     803,  1573,   532,  1575,     0,     0,  1577,  1578,     0,     0,
    1580,     0,  1582,     0,  1584,     0,  1586,     0,     0,  1588,
       0,  1589,     0,   533,  1590,     0,  1591,     0,  1592,     0,
    1593,   101,  1594,     0,     0,     0,     0,  1443,     0,  1445,
       0,  1446,     0,     0,     0,   291,   980,   289,     0,     0,
       0,     0,     0,   243,   532,  1328,  1330,   291,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1409,     0,
     291,     0,     0,     0,     0,     0,   289,     0,     0,   533,
       0,     0,   289,   289,     0,   289,   289,     0,     0,   701,
       0,     0,     0,     0,     0,     0,     0,  1482,  1483,  1484,
    1485,     0,     0,     0,     0,     0,     0,     0,   292,     0,
       0,     0,     0,     0,   533,   292,     0,   292,   292,     0,
       0,     0,  1502,  1504,  -741,  -741,  -741,  -741,  -741,  -741,
    -741,  -741,  -741,  -741,  -741,     0,     0,   291,     0,     0,
       0,   243,     0,     0,     0,     0,     0,     0,     0,     0,
     239,   197,   368,     0,     0,     0,  1526,  1528,  1530,  1532,
       0,    59,    60,   239,   142,   143,   144,   145,   146,   147,
     148,     0,   197,  -741,    59,    60,     0,   142,   143,   144,
     145,   146,   147,   148,     0,     0,   527,     0,     0,     0,
       0,   532,     0,     0,   281,     0,   289,   802,     0,   527,
       0,   532,   528,     0,     0,     0,     0,   281,     0,   197,
     197,     0,   197,   197,   197,   197,     0,   291,   197,     0,
     291,     0,     0,     0,     0,     0,     0,     0,   292,   292,
     532,   532,     0,     0,     0,     0,     0,     0,     0,     0,
     291,   291,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   291,     0,   291,     0,   239,     0,   291,     0,     0,
       0,     0,     0,     0,    62,    63,    59,    60,     0,   142,
     143,   144,   145,   146,   147,   148,   239,    62,    63,     0,
     289,   846,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,     0,   281,
       0,     0,     0,     0,   289,     0,     0,     0,     0,     0,
       0,   101,   292,   292,     0,     0,     0,   291,     0,     0,
     281,   701,     0,  1066,   101,     0,     0,     0,   616,   289,
       0,   846,     0,   289,     0,     0,   292,   239,     0,     0,
     701,     0,     0,     0,     0,     0,   197,     0,    59,    60,
       0,   142,   143,   144,   145,   146,   147,   148,   291,   803,
       0,   292,     0,   292,     0,   533,     0,     0,     0,    62,
      63,     0,   291,   803,     0,     0,   289,     0,     0,     0,
       0,   281,   418,     0,     0,   289,     0,     0,   289,     0,
      62,    63,   701,     0,     0,   770,   197,     0,     0,   243,
       0,   289,     0,     0,     0,     0,   532,     0,   292,     0,
     702,     0,   291,     0,     0,   291,   101,   533,     0,     0,
     292,     0,     0,   197,     0,   197,   197,   197,   197,   197,
     197,   197,   197,   292,   197,   197,   197,   197,   197,   197,
     197,   197,   197,   197,   239,     0,     0,     0,     0,   532,
       0,    62,    63,     0,     0,    59,    60,     0,   142,   143,
     144,   145,   146,   147,   148,     0,     0,     0,   289,  -742,
    -742,  -742,  -742,  -742,  -742,  -742,  -742,  -742,  -742,  -742,
     800,     0,     0,     0,     0,     0,     0,     0,   281,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   101,     0,
     292,     0,   532,  1295,     0,     0,     0,     0,     0,   291,
       0,     0,     0,     0,   291,     0,     0,     0,  -742,     0,
     701,     0,   289,     0,     0,     0,     0,     0,     0,   701,
       0,     0,   289,     0,     0,     0,   291,     0,     0,     0,
       0,     0,   701,     0,     0,     0,     0,     0,   289,     0,
       0,   289,     0,     0,   533,     0,     0,     0,    62,    63,
       0,   289,   289,     0,   533,     0,   532,     0,   701,   701,
       0,   289,   289,     0,     0,     0,     0,     0,     0,     0,
     292,     0,   289,   292,   289,     0,     0,     0,   289,     0,
     197,     0,   197,   533,   533,     0,     0,     0,     0,     0,
       0,     0,     0,   292,   292,   101,     0,     0,     0,   197,
       0,     0,     0,     0,   292,     0,   292,     0,     0,   239,
     292,     0,     0,     0,     0,     0,  1295,  1295,     0,     0,
      59,    60,     0,   142,   143,   144,   145,   146,   147,   148,
       0,     0,   702,     0,     0,     0,     0,     0,   289,     0,
       0,  -214,  -214,     0,     0,     0,     0,     0,     0,     0,
       0,   702,  -214,   281,     0,  -214,     0,     0,   303,   354,
       0,  -214,   355,  -214,  -214,     0,   532,     0,     0,     0,
     292,   239,     0,     0,     0,     0,     0,     0,     0,   289,
     846,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   289,   846,     0,     0,     0,     0,  1444,
       0,     0,     0,   702,     0,     0,     0,   527,     0,     0,
       0,   292,   292,     0,     0,   281,     0,     0,     0,     0,
       0,     0,     0,    62,    63,   292,   292,   289,   291,   803,
       0,     0,     0,   289,   701,     0,   289,     0,     0,     0,
       0,  -214,  -740,  -740,  -740,  -740,  -740,  -740,  -740,  -740,
    -740,  -740,  -740,     0,     0,     0,     0,     0,     0,   533,
       0,  -214,     0,     0,     0,   292,  -214,  -214,   292,     0,
     289,     0,     0,     0,     0,     0,     0,   701,   239,     0,
       0,     0,     0,     0,     0,    62,    63,     0,     0,    59,
      60,  -740,   142,   143,   144,   145,   146,   147,   148,     0,
       0,     0,   533,     0,     0,   356,     0,     0,  -214,     0,
       0,     0,   701,  -214,     0,     0,   701,   701,     0,     0,
       0,     0,   281,   289,   701,     0,     0,     0,     0,     0,
     289,   702,   101,     0,     0,   289,     0,     0,     0,     0,
     702,     0,     0,     0,     0,     0,     0,     0,     0,   197,
       0,     0,     0,   702,     0,   533,   703,   289,     0,     0,
       0,     0,   292,     0,     0,     0,     0,   292,     0,     0,
       0,   197,     0,     0,     0,     0,     0,     0,     0,   702,
     702,   197,     0,     0,     0,     0,     0,   289,     0,   292,
       0,     0,    62,    63,   701,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   197,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   533,
       0,     0,     0,     0,     0,     0,   245,     0,     0,     0,
     701,     0,   239,   701,   107,     0,     1,     2,     0,   101,
       0,     0,   701,    59,    60,     3,   142,   143,   144,   145,
     146,   147,   148,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     7,     8,     9,    10,   417,    12,    13,
       0,     0,    14,    15,    16,    17,   281,   418,     0,     0,
       0,    18,    19,    20,   549,   550,   551,   552,   553,   554,
     555,   556,   557,   558,   559,     0,     0,   289,     0,     0,
       0,   701,     0,     0,   701,   701,   701,     0,     0,   701,
     701,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   701,     0,   701,     0,     0,   533,
       0,     0,     0,   560,     1,     2,     0,     0,     0,     0,
       0,     0,     0,     3,    22,    23,    62,    63,     0,   419,
       0,     0,     0,     0,     0,   702,     0,     0,     0,   289,
     846,     7,     8,     9,    10,    11,    12,    13,     0,     0,
      14,    15,    16,    17,     0,   701,     0,   108,     0,    18,
      19,    20,     0,     0,   701,     0,   701,     0,   703,   701,
     701,   292,   292,    90,   420,   421,   642,     0,   702,     0,
       0,     0,     0,     0,     0,     0,     0,   703,     0,     0,
       0,     0,   704,     0,     0,   701,   701,   701,     0,     0,
       0,     0,     0,     0,     0,   701,     0,   701,     0,   701,
       0,   701,     0,   702,   701,     0,   701,   702,   702,   701,
       0,   197,    22,    23,     0,   702,     0,    24,     0,   197,
       0,     0,   701,     0,   701,     0,     0,     0,     0,   703,
       0,     0,     0,   701,     0,     0,   701,     0,   701,     0,
     701,     0,   701,     0,     0,     0,     0,     0,   701,     0,
     701,   701,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    90,    26,     0,     0,   701,     0,   701,     0,     0,
       0,     0,     0,     0,     0,   701,     0,     0,     0,   197,
       0,     0,     0,     0,   701,   702,   701,     0,     0,   701,
     701,     0,     0,   701,     0,   701,     0,   701,     0,   701,
       0,     0,   701,     0,   701,     0,     0,   701,     0,   701,
       0,   701,     0,   701,     0,   701,     0,     0,     0,     0,
       0,   702,     0,     0,   702,     0,     0,     0,     0,     0,
       0,     0,     0,   702,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     1,     2,     0,     0,     0,     0,
       0,     0,     0,     3,     0,     0,     0,   703,     0,     0,
       0,     0,     0,     4,     0,     0,   703,     0,     0,     5,
       6,     7,     8,     9,    10,    11,    12,    13,     0,   703,
      14,    15,    16,    17,     0,     0,     0,     0,     0,    18,
      19,    20,   702,     0,     0,   702,   702,   702,     0,     0,
     702,   702,     0,     0,     0,   703,   703,     0,     0,    21,
       0,     0,     0,     0,   704,   702,     0,   702,     0,     0,
       0,     0,     0,     0,     0,   288,     0,     0,   705,     0,
       0,     0,     0,   704,     0,     0,     0,     0,     0,     0,
       0,     0,   197,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    22,    23,     0,     0,     0,    24,     0,     0,
       0,     0,     0,     0,     0,     0,   702,     0,     0,     0,
       0,     0,     0,     0,     0,   702,     0,   702,     0,     0,
     702,   702,     0,     0,     0,   704,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    25,    26,     0,     0,     0,   702,   702,   702,     0,
       0,     0,     0,     0,     0,     0,   702,     0,   702,     0,
     702,     0,   702,     0,     0,   702,     0,   702,     0,     0,
     702,     0,     0,     0,     0,     0,   429,     0,     0,     0,
       0,     0,     0,   702,     0,   702,     0,     0,     0,     0,
       0,     0,     0,     0,   702,     0,     0,   702,     0,   702,
       0,   702,     0,   702,     0,     0,     0,     0,     0,   702,
       0,   702,   702,     0,   477,     0,     0,     0,     0,     0,
       0,   703,     0,     0,   288,     0,   702,     0,   702,     0,
       0,     0,     0,     0,     0,     0,   702,     0,     0,     0,
       0,     0,     0,     0,     0,   702,     0,   702,     0,     0,
     702,   702,     0,   704,   702,     0,   702,     0,   702,   530,
     702,     0,   704,   702,   703,   702,     0,     0,   702,     0,
     702,     0,   702,     0,   702,   704,   702,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   703,
     705,   704,   704,   703,   703,     0,     0,     0,     0,     0,
       0,   703,     0,     0,     0,     0,     0,     0,     0,   705,
       0,     0,     0,     0,   706,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   429,     0,     0,     0,     0,     0,
       0,   709,     0,   712,   429,     0,     0,     0,     0,   327,
     328,   329,   138,   139,   140,     0,     0,     0,   239,     0,
       0,   705,     0,     0,     0,     0,     0,     0,     0,    59,
      60,   703,   142,   143,   144,   145,   146,   147,   148,     0,
     149,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   150,   151,     0,     0,   152,   703,     0,     0,
     703,     0,     0,     0,   153,     0,     0,     0,     0,   703,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   798,   801,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   162,   163,   704,     0,   164,
       0,     0,    62,    63,     0,     0,     0,     0,   703,     0,
       0,   703,   703,   703,     0,     0,   703,   703,     0,   705,
       0,     0,     0,     0,     0,     0,     0,     0,   705,     0,
       0,   703,     0,   703,     0,     0,   165,   166,     0,     0,
     704,   705,   167,   168,     0,     0,     0,   330,     0,   101,
       0,   781,   959,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   705,   705,     0,
       0,     0,    82,    83,     0,   704,   706,     0,     0,   704,
     704,    84,   703,     0,     0,     0,     0,   704,     0,     0,
     707,   703,   429,   703,     0,   706,   703,   703,     0,     7,
       8,     9,    10,    11,    12,    13,     0,     0,    14,    15,
      16,    17,     0,     0,     0,     0,     0,    85,    86,    20,
       0,     0,   703,   703,   703,     0,     0,     0,     0,     0,
       0,     0,   703,     0,   703,     0,   703,     0,   703,     0,
       0,   703,     0,   703,     0,     0,   703,   706,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   704,     0,   703,
       0,   703,     0,     0,   712,     0,     0,     0,     0,     0,
     703,     0,     0,   703,     0,   703,   905,   703,     0,   703,
      22,    23,     0,     0,     0,   703,     0,   703,   703,   924,
       0,     0,     0,   704,     0,     0,   704,     0,     0,     0,
       0,     0,   703,     0,   703,   704,     0,     0,     0,     0,
       0,     0,   703,     0,     0,     0,     0,     0,     0,     0,
       0,   703,     0,   703,     0,     0,   703,   703,     0,    87,
     703,     0,   703,     0,   703,     0,   703,     0,     0,   703,
       0,   703,     0,   705,   703,     0,   703,     0,   703,     0,
     703,     0,   703,     0,     0,     0,   530,     0,     0,     0,
       0,     0,     0,     0,   704,     0,     0,   704,   704,   704,
       0,     0,   704,   704,     0,   706,     0,     0,     0,     0,
       0,     0,     0,     0,   706,     0,   705,   704,     0,   704,
       0,     0,     0,     0,     0,     0,     0,   706,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   705,   707,   706,   706,   705,   705,     0,     0,     0,
       0,     0,     0,   705,     0,     0,   905,     0,   704,   905,
       0,   707,     0,     0,     0,     0,   708,   704,     0,   704,
       0,     0,   704,   704,     0,     0,     0,     0,     0,  1086,
    1087,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1093,     0,   712,     0,     0,     0,   924,     0,   704,   704,
     704,     0,     0,     0,     0,     0,     0,     0,   704,     0,
     704,     0,   704,   707,   704,     0,     0,   704,     0,   704,
       0,     0,   704,   705,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   704,     0,   704,     0,     0,
       0,     0,     0,     0,     0,     0,   704,     0,     0,   704,
       0,   704,     0,   704,     0,   704,   801,     0,     0,   705,
       0,   704,   705,   704,   704,     0,     0,     0,     0,     0,
       0,   705,     0,     0,     0,     0,     0,     0,   704,     0,
     704,     0,     0,     0,     0,     0,     0,     0,   704,     0,
       0,     0,     0,     0,     0,     0,     0,   704,     0,   704,
       0,     0,   704,   704,     0,     0,   704,     0,   704,     0,
     704,     0,   704,     0,     0,   704,     0,   704,     0,   706,
     704,     0,   704,     0,   704,     0,   704,     0,   704,     0,
     705,     0,     0,   705,   705,   705,     0,     0,   705,   705,
       0,   707,     0,     0,     0,     0,     0,     0,     0,     0,
     707,  1204,     0,   705,  1204,   705,     0,     0,     0,     0,
       0,     0,   706,   707,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   707,
     707,     0,     0,     0,   646,     0,     0,   706,   708,     0,
       0,   706,   706,    84,   705,     0,     0,     0,     0,   706,
       0,     0,     0,   705,     0,   705,     0,   708,   705,   705,
       0,     7,     8,     9,    10,    11,    12,    13,     0,     0,
      14,    15,    16,    17,     0,     0,     0,     0,     0,    85,
      86,    20,     0,     0,   705,   705,   705,     0,  1204,     0,
       0,     0,     0,  1204,   705,     0,   705,     0,   705,     0,
     705,     0,     0,   705,     0,   705,     0,     0,   705,   708,
       0,     0,     0,     0,     0,  1324,     0,     0,     0,   706,
       0,   705,     0,   705,     0,     0,     0,     0,     0,     0,
       0,     0,   705,     0,     0,   705,     0,   705,     0,   705,
       0,   705,    22,    23,     0,     0,     0,   705,     0,   705,
     705,     0,     0,     0,     0,   706,     0,     0,   706,     0,
       0,     0,     0,     0,   705,     0,   705,   706,     0,     0,
       0,     0,     0,     0,   705,     0,     0,     0,     0,     0,
       0,     0,     0,   705,     0,   705,     0,     0,   705,   705,
       0,     0,   705,     0,   705,     0,   705,     0,   705,     0,
       0,   705,     0,   705,     0,   707,   705,     0,   705,     0,
     705,     0,   705,     0,   705,     0,   859,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   706,     0,     0,   706,
     706,   706,     0,     0,   706,   706,     0,   708,     0,     0,
       0,     0,     0,     0,     0,     0,   708,     0,   707,   706,
       0,   706,   138,   139,   140,     0,     0,     0,   239,   708,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    59,
      60,     0,   142,   143,   144,   145,   146,   147,   148,     0,
     149,     0,     0,   707,     0,   708,   708,   707,   707,     0,
       0,     0,     0,     0,   527,   707,     0,     0,     0,     0,
     706,     0,   150,   151,     0,     0,   152,     0,     0,   706,
     616,   706,     0,     0,   706,   706,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,     0,     0,     0,
     706,   706,   706,     0,     0,     0,     0,     0,     0,     0,
     706,     0,   706,     0,   706,     0,   706,     0,     0,   706,
       0,   706,     0,     0,   706,   707,     0,     0,     0,   164,
       0,     0,    62,    63,     0,     0,     0,   706,     0,   706,
       0,     0,     0,     0,     0,     0,     0,     0,   706,     0,
       0,   706,     0,   706,     0,   706,     0,   706,     0,     0,
       0,   707,     0,   706,   707,   706,   706,     0,     0,     0,
       0,     0,     0,   707,     0,  1018,     0,   662,     0,   101,
     706,     0,   706,     0,     0,     0,     0,     0,     0,     0,
     706,     0,     0,     0,     0,     0,     0,  1056,     0,   706,
       0,   706,     0,     0,   706,   706,     0,     0,   706,     0,
     706,     0,   706,     0,   706,     0,     0,   706,     0,   706,
       0,   708,   706,     0,   706,     0,   706,     0,   706,     0,
     706,     0,   707,     0,     0,   707,   707,   707,     0,     0,
     707,   707,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   707,     0,   707,     0,     0,
       0,     0,     0,     0,   708,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   708,
       0,     0,     0,   708,   708,     0,   707,     0,     0,     0,
       0,   708,     0,     0,     0,   707,     0,   707,     0,     0,
     707,   707,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   707,   707,   707,     0,
       0,     0,     0,     0,     0,     0,   707,     0,   707,     0,
     707,     0,   707,     0,     0,   707,     0,   707,     0,     0,
     707,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   708,     0,   707,     0,   707,     0,     0,     0,     0,
       0,     0,     0,     0,   707,     0,     0,   707,     0,   707,
       0,   707,     0,   707,     0,     0,     0,     0,     0,   707,
       0,   707,   707,     0,     0,     0,     0,   708,     0,     0,
     708,     0,     0,     0,     0,     0,   707,     0,   707,   708,
       0,     0,     0,     0,     0,     0,   707,     0,     0,     0,
       0,     0,     0,     0,     0,   707,     0,   707,     0,     0,
     707,   707,     0,     0,   707,     0,   707,     0,   707,     0,
     707,     0,     0,   707,     0,   707,     0,     0,   707,     0,
     707,     0,   707,     0,   707,     0,   707,  1259,     0,     0,
       0,  1271,     0,     0,     0,     0,     0,     0,   708,     0,
       0,   708,   708,   708,     0,     0,   708,   708,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   245,
       0,   708,     0,   708,     0,   239,     0,   107,     0,     1,
       2,     0,     0,     0,     0,     0,    59,    60,     3,   142,
     143,   144,   145,   146,   147,   148,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     7,     8,     9,    10,
     417,    12,    13,     0,     0,    14,    15,    16,    17,   281,
     418,     0,   708,     0,    18,    19,    20,     0,     0,     0,
       0,   708,     0,   708,     0,     0,   708,   708,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1350,     0,     0,     0,     0,
       0,     0,   708,   708,   708,     0,     0,     0,     0,     0,
       0,     0,   708,     0,   708,     0,   708,     0,   708,     0,
       0,   708,     0,   708,     0,     0,   708,    22,    23,    62,
      63,     0,   419,     0,     0,     0,     0,     0,     0,   708,
       0,   708,     0,     0,     0,     0,     0,     0,     0,     0,
     708,     0,     0,   708,     0,   708,     0,   708,     0,   708,
     108,     0,     0,     0,     0,   708,  1018,   708,   708,     0,
    1056,  1413,     0,     0,  1415,  1416,    90,   420,   421,   851,
       0,     0,   708,     0,   708,     0,     0,     0,     0,  1430,
       0,     0,   708,     0,     0,     0,     0,     0,     0,     0,
       0,   708,     0,   708,     0,     0,   708,   708,     0,     0,
     708,     0,   708,     0,   708,     0,   708,     0,     0,   708,
       0,   708,     0,     0,   708,     0,   708,     0,   708,     0,
     708,     0,   708,     0,     0,     0,     0,     0,     0,     0,
    1466,     0,     0,     0,     0,     0,     0,     0,     0,  1475,
       0,  1477,     0,     0,  1479,  1480,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1259,  1271,     0,     0,     0,     0,     0,     0,     0,     0,
    1507,     0,  1509,     0,  1511,     0,  1513,     0,     0,  1515,
       0,  1516,     0,     0,  1517,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1350,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1540,     0,  1541,     0,  1542,     0,  1543,     0,     0,
       0,     0,     0,  1413,     0,  1415,  1416,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1430,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1466,     0,     0,     0,     0,     0,     0,     0,     0,  1475,
       0,  1477,     0,     0,  1479,  1480,     0,     0,  1507,     0,
    1509,     0,  1511,     0,  1513,     0,     0,  1515,     0,  1516,
       0,     0,  1517,     0,  1540,     0,  1541,     0,  1542,     0,
    1543,   327,   328,   329,   138,   139,   140,     0,     0,     0,
     239,     0,     0,     0,   327,   328,   329,   138,   139,   140,
       0,    59,    60,   239,   142,   143,   144,   145,   146,   147,
     148,     0,   149,     0,    59,    60,     0,   142,   143,   144,
     145,   146,   147,   148,     0,   149,     0,     0,     0,     0,
       0,     0,     0,     0,   150,   151,     0,     0,   152,     0,
       0,     0,     0,     0,     0,     0,   153,   150,   151,     0,
       0,   152,     0,     0,     0,     0,     0,     0,   154,   153,
       0,     0,   155,   156,   157,   158,   159,   160,   161,     0,
       0,   154,     0,     0,     0,   155,   156,   157,   158,   159,
     160,   161,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   162,   163,     0,
       0,   164,     0,     0,    62,    63,   327,   328,   329,   138,
     139,   140,     0,     0,   164,   239,     0,    62,    63,     0,
       0,     0,     0,     0,     0,     0,    59,    60,     0,   142,
     143,   144,   145,   146,   147,   148,     0,   149,   165,   166,
       0,     0,     0,     0,   167,   168,     0,     0,     0,   330,
       0,   101,     0,   781,  1238,     0,     0,   167,   168,   150,
     151,   649,   330,   152,   101,     0,     0,     0,     0,     0,
       0,   153,     0,     1,     2,     0,     0,     0,     0,     0,
       0,     0,     3,   154,     0,     0,     0,   155,   156,   157,
     158,   159,   160,   161,     0,     0,     0,     0,     0,     6,
       7,     8,     9,    10,    11,    12,    13,     0,     0,    14,
      15,    16,    17,     0,     0,     0,     0,     0,    18,    19,
      20,     0,   162,   163,     0,     0,   164,     0,     0,    62,
      63,   327,   328,   329,   138,   139,   140,     0,     0,     0,
     239,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,    60,     0,   142,   143,   144,   145,   146,   147,
     148,     0,   149,   165,   166,     0,     0,     0,     0,   167,
     168,     0,     0,     0,   330,     0,   101,     0,   421,     0,
       0,    22,    23,     0,   150,   151,    24,     0,   152,     0,
       0,     0,     0,     0,     0,     0,   153,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   154,     0,
       0,     0,   155,   156,   157,   158,   159,   160,   161,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      90,    26,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   162,   163,     0,
       0,   164,     0,     0,    62,    63,   327,   328,   329,   138,
     139,   140,     0,     0,     0,   239,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    59,    60,     0,   142,
     143,   144,   145,   146,   147,   148,     0,   149,   165,   166,
       0,     0,     0,     0,   167,   168,     0,     0,     0,   330,
       0,   101,     0,   421,     0,     0,     0,     0,     0,   150,
     151,     0,     0,   152,     0,     0,     0,     0,     0,     0,
       0,   153,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   154,     0,     0,     0,   155,   156,   157,
     158,   159,   160,   161,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   162,   163,     0,     0,   164,     0,     0,    62,
      63,   327,   328,   329,   138,   139,   140,     0,     0,     0,
     239,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,    60,     0,   142,   143,   144,   145,   146,   147,
     148,     0,   149,   165,   166,     0,     0,     0,     0,   167,
     168,     0,     0,     0,   330,     0,   101,     0,   781,     0,
       0,     0,     0,     0,   150,   151,     0,     0,   152,     0,
       0,     0,     0,     0,     0,     0,   153,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   154,     0,
       0,     0,   155,   156,   157,   158,   159,   160,   161,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   162,   163,     0,
       0,   164,     0,     0,    62,    63,   327,   328,   329,   138,
     139,   140,     0,     0,     0,   239,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    59,    60,     0,   142,
     143,   144,   145,   146,   147,   148,     0,   149,   165,   166,
       0,     0,     0,     0,   167,   168,     0,     0,     0,   330,
       0,   101,   869,     0,     0,     0,     0,     0,     0,   150,
     151,     0,     0,   152,     0,     0,     0,     0,     0,     0,
       0,   153,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   154,     0,     0,     0,   155,   156,   157,
     158,   159,   160,   161,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   162,   163,     0,     0,   164,     0,     0,    62,
      63,   327,   328,   329,   138,   139,   140,     0,     0,     0,
     239,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,    60,     0,   142,   143,   144,   145,   146,   147,
     148,     0,   149,   165,   166,     0,     0,     0,     0,   167,
     168,     0,     0,     0,   330,     0,   101,  1159,     0,     0,
       0,     0,     0,     0,   150,   151,     0,     0,   152,     0,
       0,     0,     0,     0,     0,     0,   153,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   154,     0,
       0,     0,   155,   156,   157,   158,   159,   160,   161,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   162,   163,     0,
       0,   164,     0,     0,    62,    63,   327,   328,   329,   138,
     139,   140,     0,     0,     0,   239,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    59,    60,     0,   142,
     143,   144,   145,   146,   147,   148,     0,   149,   165,   166,
       0,     0,     0,     0,   167,   168,     0,     0,     0,   330,
       0,   101,  1284,     0,     0,     0,     0,     0,     0,   150,
     151,     0,     0,   152,     0,     0,     0,     0,     0,     0,
       0,   153,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   154,     0,     0,     0,   155,   156,   157,
     158,   159,   160,   161,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   162,   163,     0,     0,   164,     0,     0,    62,
      63,   327,   328,   329,   138,   139,   140,     0,     0,     0,
     239,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,    60,     0,   142,   143,   144,   145,   146,   147,
     148,     0,   149,   165,   166,     0,     0,     0,     0,   167,
     168,     0,     0,     0,   330,     0,   101,  1459,     0,     0,
       0,     0,     0,     0,   150,   151,     0,     0,   152,     0,
       0,     0,     0,     0,     0,     0,   153,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   154,     0,
       0,     0,   155,   156,   157,   158,   159,   160,   161,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   162,   163,     0,
       0,   164,     0,     0,    62,    63,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   165,   166,
     138,   139,   140,     0,   167,   168,   645,     0,   311,   330,
     646,   101,  1505,     0,     0,     0,     0,    59,    60,     3,
     142,   143,   144,   145,   146,   147,   148,     0,   895,     0,
       0,     0,     0,     0,     0,     0,     0,     7,     8,     9,
      10,    11,   647,    13,     0,     0,    14,    15,   648,    17,
     150,   151,   649,     0,   896,    18,    19,    20,   616,   650,
       0,   651,   652,     0,   653,   654,   655,   656,   657,   658,
     659,   660,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,    22,    23,
      62,    63,     0,   661,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   138,   139,   140,     0,     0,
       0,   645,     0,     0,     0,   646,     0,     0,     0,     0,
       0,   312,    59,    60,     3,   142,   143,   144,   145,   146,
     147,   148,     0,   149,     0,   662,     0,   101,   663,   421,
     897,     0,     7,     8,     9,    10,    11,   647,    13,     0,
       0,    14,    15,   648,    17,   150,   151,   649,     0,   152,
      18,    19,    20,   616,   650,     0,   651,   652,     0,   653,
     654,   655,   656,   657,   658,   659,   660,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,    22,    23,    62,    63,     0,   661,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   138,
     139,   140,     0,     0,     0,   645,     0,     0,     0,   646,
       0,     0,     0,     0,     0,     0,    59,    60,     3,   142,
     143,   144,   145,   146,   147,   148,     0,   149,     0,     0,
     662,     0,   101,   663,   421,   664,     7,     8,     9,    10,
      11,   647,    13,     0,     0,    14,    15,   648,    17,   150,
     151,   649,     0,   152,    18,    19,    20,   616,   650,     0,
     651,   652,     0,   653,   654,   655,   656,   657,   658,   659,
     660,     0,     0,   154,     0,     0,     0,   155,   156,   157,
     158,   159,   160,   161,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   162,   163,     0,     0,   164,    22,    23,    62,
      63,     0,   661,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   138,   139,   140,     0,     0,     0,   645,
       0,     0,     0,   646,     0,     0,     0,     0,     0,     0,
      59,    60,     3,   142,   143,   144,   145,   146,   147,   148,
       0,   149,     0,     0,   662,     0,   101,   663,   421,   877,
       7,     8,     9,    10,    11,   647,    13,     0,     0,    14,
      15,   648,    17,   150,   151,   649,     0,   152,    18,    19,
      20,   616,   650,     0,   651,   652,     0,   653,   654,   655,
     656,   657,   658,   659,   660,     0,     0,   154,     0,     0,
       0,   155,   156,   157,   158,   159,   160,   161,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   162,   163,     0,     0,
     164,    22,    23,    62,    63,     0,   661,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   138,   139,   140,
       0,     0,     0,   645,     0,     0,     0,   646,     0,     0,
       0,     0,     0,     0,    59,    60,     3,   142,   143,   144,
     145,   146,   147,   148,     0,   149,     0,     0,   662,     0,
     101,   663,   421,  1075,     7,     8,     9,    10,    11,   647,
      13,     0,     0,    14,    15,   648,    17,   150,   151,   649,
       0,   152,    18,    19,    20,   616,   650,     0,   651,   652,
       0,   653,   654,   655,   656,   657,   658,   659,   660,     0,
       0,   154,     0,     0,     0,   155,   156,   157,   158,   159,
     160,   161,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     162,   163,     0,     0,   164,    22,    23,    62,    63,     0,
     661,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   138,   139,   140,     0,     0,     0,   645,     0,     0,
       0,   646,     0,     0,     0,     0,     0,     0,    59,    60,
       3,   142,   143,   144,   145,   146,   147,   148,     0,   149,
       0,     0,   662,     0,   101,   663,   421,  1077,     7,     8,
       9,    10,    11,   647,    13,     0,     0,    14,    15,   648,
      17,   150,   151,   649,     0,   152,    18,    19,    20,   616,
     650,     0,   651,   652,     0,   653,   654,   655,   656,   657,
     658,   659,   660,     0,     0,   154,     0,     0,     0,   155,
     156,   157,   158,   159,   160,   161,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   162,   163,     0,     0,   164,    22,
      23,    62,    63,     0,   661,     0,     0,   526,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,     0,
       0,     0,     0,     0,     0,     0,   662,     0,   101,   663,
     421,  1196,   527,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,   528,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   327,   328,   329,   138,   139,   140,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   149,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,   529,   101,     0,     0,
       0,     0,     0,     0,     0,   150,   151,     0,     0,   152,
       0,     0,     0,     0,     0,     0,     0,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,     0,     0,    62,    63,   327,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,   165,
     166,     0,     0,     0,     0,   167,   168,     0,     0,     0,
     330,   565,   101,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,     0,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   327,   328,   329,   138,   139,   140,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   149,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,   791,   101,     0,     0,
       0,     0,     0,     0,     0,   150,   151,     0,     0,   152,
       0,     0,     0,     0,     0,     0,     0,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,     0,     0,    62,    63,   327,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,   165,
     166,     0,     0,     0,     0,   167,   168,     0,     0,     0,
     330,   953,   101,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,     0,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   327,   328,   329,   138,   139,   140,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   149,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,   965,   101,     0,     0,
       0,     0,     0,     0,     0,   150,   151,     0,     0,   152,
       0,     0,     0,     0,     0,     0,     0,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,     0,     0,    62,    63,   327,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,   165,
     166,     0,     0,     0,     0,   167,   168,     0,     0,     0,
     330,  1002,   101,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,     0,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   327,   328,   329,   138,   139,   140,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   149,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,  1011,   101,     0,     0,
       0,     0,     0,     0,     0,   150,   151,     0,     0,   152,
       0,     0,     0,     0,     0,     0,     0,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,     0,     0,    62,    63,   327,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,   165,
     166,     0,     0,     0,     0,   167,   168,     0,     0,     0,
     330,  1109,   101,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,     0,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   327,   328,   329,   138,   139,   140,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   149,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,  1146,   101,     0,     0,
       0,     0,     0,     0,     0,   150,   151,     0,     0,   152,
       0,     0,     0,     0,     0,     0,     0,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,     0,     0,    62,    63,   327,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,   165,
     166,     0,     0,     0,     0,   167,   168,     0,     0,     0,
     330,  1151,   101,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,     0,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   327,   328,   329,   138,   139,   140,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   149,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,  1188,   101,     0,     0,
       0,     0,     0,     0,     0,   150,   151,     0,     0,   152,
       0,     0,     0,     0,     0,     0,     0,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,     0,     0,    62,    63,   327,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,   165,
     166,     0,     0,     0,     0,   167,   168,     0,     0,     0,
     330,  1190,   101,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,     0,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   327,   328,   329,   138,   139,   140,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   149,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,  1248,   101,     0,     0,
       0,     0,     0,     0,     0,   150,   151,     0,     0,   152,
       0,     0,     0,     0,     0,     0,     0,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,     0,     0,    62,    63,   327,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,   165,
     166,     0,     0,     0,     0,   167,   168,     0,     0,     0,
     330,  1309,   101,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,     0,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   327,   328,   329,   138,   139,   140,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   149,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,  1311,   101,     0,     0,
       0,     0,     0,     0,     0,   150,   151,     0,     0,   152,
       0,     0,     0,     0,     0,     0,     0,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,     0,     0,    62,    63,   327,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,   165,
     166,     0,     0,     0,     0,   167,   168,     0,     0,     0,
     330,  1341,   101,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,     0,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   327,   328,   329,   138,   139,   140,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   149,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,  1389,   101,     0,     0,
       0,     0,     0,     0,     0,   150,   151,     0,     0,   152,
       0,     0,     0,     0,     0,     0,     0,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,     0,     0,    62,    63,   327,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,   165,
     166,     0,     0,     0,     0,   167,   168,     0,     0,     0,
     330,  1392,   101,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,     0,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   327,   328,   329,   138,   139,   140,     0,     0,
       0,   239,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    59,    60,     0,   142,   143,   144,   145,   146,
     147,   148,     0,   149,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,  1451,   101,     0,     0,
       0,     0,     0,     0,     0,   150,   151,     0,     0,   152,
       0,     0,     0,     0,     0,     0,     0,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   154,
       0,     0,     0,   155,   156,   157,   158,   159,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,   163,
       0,     0,   164,     0,     0,    62,    63,   327,   328,   329,
     138,   139,   140,     0,     0,     0,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,   165,
     166,     0,     0,     0,     0,   167,   168,     0,     0,     0,
     330,  1455,   101,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,     0,     0,
       0,     0,   153,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   138,   139,   140,     0,     0,     0,   239,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    59,
      60,     0,   142,   143,   144,   145,   146,   147,   148,     0,
     149,     0,     0,     0,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   330,     0,   101,     0,     0,
       0,     0,   150,   151,     0,     0,   152,     0,     0,     0,
       0,     0,     0,     0,   153,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   162,   163,     0,     0,   164,
       0,     0,    62,    63,   138,   139,   140,     0,     0,     0,
     239,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,    60,     0,   142,   143,   144,   145,   146,   147,
     148,     0,   149,     0,     0,     0,   165,   166,     0,     0,
       0,     0,   167,   168,     0,     0,     0,   169,     0,   101,
       0,   171,   346,     0,   150,   151,     0,     0,   152,     0,
       0,     0,     0,     0,     0,     0,   153,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   154,     0,
       0,     0,   155,   156,   157,   158,   159,   160,   161,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   162,   163,     0,
       0,   164,     0,     0,    62,    63,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   165,   166,
       0,     0,     0,     0,   167,   168,   138,   139,   140,   169,
       0,   101,   645,   171,   818,     0,   646,     0,     0,     0,
    1111,     0,     0,    59,    60,     3,   142,   143,   144,   145,
     146,   147,   148,     0,   149,     0,     0,     0,     0,     0,
       0,     0,     0,     7,     8,     9,    10,    11,   647,    13,
       0,     0,    14,    15,   648,    17,   150,   151,   649,     0,
     152,    18,    19,    20,   616,   650,     0,   651,   652,  1112,
     653,   654,   655,   656,   657,   658,   659,   660,     0,     0,
     154,     0,     0,     0,   155,   156,   157,   158,   159,   160,
     161,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   162,
     163,     0,     0,   164,    22,    23,    62,    63,     0,   661,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   138,
     139,   140,     0,     0,     0,   645,     0,     0,     0,   646,
       0,     0,     0,     0,     0,     0,    59,    60,     3,   142,
     143,   144,   145,   146,   147,   148,     0,   149,     0,     0,
       0,   662,     0,   101,   663,   421,     7,     8,     9,    10,
      11,   647,    13,     0,     0,    14,    15,   648,    17,   150,
     151,   649,     0,   152,    18,    19,    20,   616,   650,     0,
     651,   652,     0,   653,   654,   655,   656,   657,   658,   659,
     660,     0,     0,   154,     0,     0,     0,   155,   156,   157,
     158,   159,   160,   161,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   162,   163,     0,     0,   164,    22,    23,    62,
      63,     0,   661,   138,   139,   140,     0,     0,     0,   645,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      59,    60,     0,   142,   143,   144,   145,   146,   147,   148,
       0,   149,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   662,     0,   101,   663,   421,     0,
       0,   858,     0,   150,   151,   649,     0,   152,     0,     0,
       0,     0,   650,     0,   651,   652,     0,   653,   654,   655,
     656,   657,   658,   659,   660,     0,     0,   154,     0,     0,
       0,   155,   156,   157,   158,   159,   160,   161,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   162,   163,     0,     0,
     164,     0,     0,    62,    63,     0,   661,   138,   139,   140,
       0,     0,     0,  1254,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    59,    60,     0,   142,   143,   144,
     145,   146,   147,   148,     0,   149,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   662,     0,
     101,   663,   421,     0,     0,   858,     0,   150,   151,   649,
       0,   152,     0,     0,     0,     0,  1255,     0,   651,   652,
       0,   653,  1256,  1257,   656,   657,   658,   659,   660,     0,
       0,   154,     0,     0,     0,   155,   156,   157,   158,   159,
     160,   161,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     162,   163,     0,     0,   164,     0,     0,    62,    63,     0,
     661,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     245,     0,     0,     0,     0,     0,   239,     0,   107,     0,
       1,     2,     0,     0,     0,     0,     0,    59,    60,     3,
     142,   143,   144,   145,   146,   147,   148,     0,     0,     0,
       0,     0,   662,     0,   101,   663,   421,     7,     8,     9,
      10,   417,    12,    13,     0,     0,    14,    15,    16,    17,
     281,   418,     0,     0,     0,    18,    19,    20,     0,     0,
     138,   139,   140,     0,     0,     0,   141,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,    60,     0,
     142,   143,   144,   145,   146,   147,   148,     0,   149,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     150,   151,     0,     0,   152,     0,     0,     0,    22,    23,
      62,    63,   153,   419,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   154,     0,     0,     0,   155,   156,
     157,   158,   159,   160,   161,     0,     0,     0,     0,     0,
       0,   108,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    90,   420,   421,
       0,     0,     0,   162,   163,     0,     0,   164,     0,     0,
      62,    63,   138,   139,   140,     0,     0,     0,   141,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    59,
      60,     0,   142,   143,   144,   145,   146,   147,   148,     0,
     149,     0,     0,     0,   165,   166,     0,     0,     0,     0,
     167,   168,     0,     0,     0,   169,   170,   101,     0,   171,
       0,     0,   150,   151,     0,     0,   152,     0,     0,     0,
       0,     0,     0,     0,   153,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   162,   163,     0,     0,   164,
       0,     0,    62,    63,   138,   139,   140,     0,     0,     0,
     141,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    59,    60,     0,   142,   143,   144,   145,   146,   147,
     148,     0,   149,     0,     0,     0,   165,   166,     0,     0,
       0,     0,   167,   168,     0,     0,     0,   169,   268,   101,
       0,   171,     0,     0,   150,   151,     0,     0,   152,     0,
       0,     0,     0,     0,     0,     0,   153,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   154,     0,
       0,     0,   155,   156,   157,   158,   159,   160,   161,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   162,   163,     0,
       0,   164,     0,     0,    62,    63,   138,   139,   140,     0,
       0,     0,   239,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    59,    60,     0,   142,   143,   144,   145,
     146,   147,   148,     0,   149,     0,     0,     0,   165,   166,
       0,     0,     0,     0,   167,   168,     0,     0,     0,   169,
    1006,   101,     0,   171,     0,     0,   150,   151,     0,     0,
     152,     0,     0,     0,     0,     0,     0,     0,   153,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     154,     0,     0,     0,   155,   156,   157,   158,   159,   160,
     161,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   162,
     163,     0,     0,   164,     0,     0,    62,    63,   138,   139,
     140,     0,     0,     0,   239,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    59,    60,     0,   142,   143,
     144,   145,   146,   147,   148,     0,   149,     0,     0,     0,
     165,   166,     0,     0,     0,     0,   167,   168,     0,     0,
     527,   169,     0,   101,     0,   171,     0,     0,   150,   151,
       0,     0,   152,     0,     0,     0,   616,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   154,     0,     0,     0,   155,   156,   157,   158,
     159,   160,   161,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   138,   139,   140,     0,     0,     0,   239,
       0,   162,   163,     0,     0,   164,     0,     0,    62,    63,
      59,    60,     0,   142,   143,   144,   145,   146,   147,   148,
       0,   149,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   527,     0,     0,     0,     0,
       0,     0,     0,   150,   151,     0,     0,   152,     0,     0,
       0,   616,     0,   662,     0,   101,  1027,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   154,     0,     0,
       0,   155,   156,   157,   158,   159,   160,   161,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   138,   139,   140,     0,   162,   163,   239,     0,
     164,     0,     0,    62,    63,     0,     0,     0,     0,    59,
      60,     0,   142,   143,   144,   145,   146,   147,   148,     0,
     149,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   150,   151,     0,     0,   152,     0,   662,     0,
     101,  1407,     0,     0,   153,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,   245,     0,     0,
       0,     0,     0,   239,     0,   107,     0,    82,    83,     0,
       0,     0,     0,     0,    59,    60,    84,   142,   143,   144,
     145,   146,   147,   148,     0,   162,   163,     0,     0,   164,
       0,     0,    62,    63,     7,     8,     9,    10,    11,    12,
      13,     0,     0,    14,    15,    16,    17,   281,   418,     0,
       0,     0,    85,    86,    20,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   165,   166,     0,     0,
       0,     0,   167,   168,     0,     0,     0,   169,     0,   101,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   138,   139,   140,     0,     0,     0,   239,     0,
       0,     0,     0,     0,     0,    22,    23,    62,    63,    59,
      60,     0,   142,   143,   144,   145,   146,   147,   148,     0,
     149,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   108,     0,
       0,     0,   150,   151,     0,     0,   152,     0,     0,     0,
       0,     0,     0,     0,    87,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   138,   139,   140,
       0,     0,     0,   239,     0,   162,   163,     0,     0,   164,
       0,     0,    62,    63,    59,    60,     0,   142,   143,   144,
     145,   146,   147,   148,     0,   149,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   150,   151,     0,
       0,   152,     0,     0,     0,     0,     0,   662,  1272,   101,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   154,     0,     0,     0,   155,   156,   157,   158,   159,
     160,   161,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   138,   139,   140,     0,     0,     0,   239,     0,
     162,   163,     0,     0,   164,     0,     0,    62,    63,    59,
      60,     0,   142,   143,   144,   145,   146,   147,   148,     0,
     149,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   150,   151,     0,     0,   152,     0,     0,     0,
       0,     0,   662,  1352,   101,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   138,   139,   140,
       0,     0,     0,   239,     0,   162,   163,     0,     0,   164,
       0,     0,    62,    63,    59,    60,     0,   142,   143,   144,
     145,   146,   147,   148,     0,   149,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   150,   151,     0,
       0,   152,     0,     0,     0,     0,     0,   662,  1366,   101,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   154,     0,     0,     0,   155,   156,   157,   158,   159,
     160,   161,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   138,   139,   140,     0,     0,     0,   239,     0,
     162,   163,     0,     0,   164,     0,     0,    62,    63,    59,
      60,     0,   142,   143,   144,   145,   146,   147,   148,     0,
     149,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   150,   151,     0,     0,   152,     0,     0,     0,
       0,     0,   662,  1432,   101,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   138,   139,   140,
       0,     0,     0,   239,     0,   162,   163,     0,     0,   164,
       0,     0,    62,    63,    59,    60,     0,   142,   143,   144,
     145,   146,   147,   148,     0,   149,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   150,   151,     0,
       0,   152,     0,     0,     0,     0,     0,   662,  1494,   101,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   154,     0,     0,     0,   155,   156,   157,   158,   159,
     160,   161,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   138,   139,   140,     0,     0,     0,   239,     0,
     162,   163,     0,     0,   164,     0,     0,    62,    63,    59,
      60,     0,   142,   143,   144,   145,   146,   147,   148,     0,
     149,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   150,   151,     0,     0,   152,     0,     0,     0,
       0,     0,   662,  1523,   101,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   138,   139,   140,
       0,     0,     0,   239,     0,   162,   163,     0,     0,   164,
       0,     0,    62,    63,    59,    60,     0,   142,   143,   144,
     145,   146,   147,   148,     0,   149,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   150,   151,     0,
       0,   152,     0,     0,     0,     0,     0,   662,  1537,   101,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   154,     0,     0,     0,   155,   156,   157,   158,   159,
     160,   161,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   138,   139,   140,     0,     0,     0,   239,     0,
     162,   163,     0,     0,   164,     0,     0,    62,    63,    59,
      60,     0,   142,   143,   144,   145,   146,   147,   148,     0,
     149,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   150,   151,     0,     0,   152,     0,     0,     0,
       0,     0,   662,  1562,   101,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     155,   156,   157,   158,   159,   160,   161,     0,     0,     0,
       0,     0,     0,   239,     0,   107,     0,     1,     2,     0,
       0,     0,     0,     0,    59,    60,     3,   142,   143,   144,
     145,   146,   147,   148,     0,   162,   163,     0,     0,   164,
       0,     0,    62,    63,     7,     8,     9,    10,    11,    12,
      13,     0,     0,    14,    15,    16,    17,   281,   418,     0,
       0,     0,    18,    19,    20,     0,   239,     0,   107,     0,
       1,     2,     0,     0,     0,     0,     0,    59,    60,     3,
     142,   143,   144,   145,   146,   147,   148,   662,     0,   101,
       0,     0,     0,     0,     0,     0,     0,     7,     8,     9,
      10,    11,    12,    13,     0,     0,    14,    15,    16,    17,
     281,   418,     0,     0,     0,    18,    19,    20,     0,     0,
       0,     0,     0,     0,     0,    22,    23,    62,    63,     0,
     918,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   108,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    90,   919,     0,   920,    22,    23,
      62,    63,   239,   918,     0,     0,     1,     2,     0,     0,
       0,     0,     0,    59,    60,     3,   142,   143,   144,   145,
     146,   147,   148,     0,     0,     0,     0,     0,     0,     0,
       0,   108,     0,     7,     8,     9,    10,    11,    12,    13,
       0,     0,    14,    15,    16,    17,   281,    90,   919,     0,
    1096,    18,    19,    20,     0,   239,     0,     0,     0,     1,
       2,     0,     0,     0,     0,     0,    59,    60,     3,   142,
     143,   144,   145,   146,   147,   148,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     7,     8,     9,    10,
      11,    12,    13,     0,     0,    14,    15,    16,    17,   281,
       0,     0,     0,     0,    18,    19,    20,     0,     0,     0,
       0,     0,     0,     0,    22,    23,    62,    63,     0,   282,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   239,     0,   107,     0,    82,    83,     0,     0,
       0,     0,     0,    59,    60,    84,   142,   143,   144,   145,
     146,   147,   148,    90,   283,     0,   284,    22,    23,    62,
      63,     0,   282,     7,     8,     9,    10,    11,    12,    13,
       0,     0,    14,    15,    16,    17,   281,   418,     0,     0,
       0,    85,    86,    20,   239,     0,     0,     0,    82,    83,
       0,     0,     0,     0,     0,    59,    60,    84,   142,   143,
     144,   145,   146,   147,   148,     0,    90,   283,     0,   484,
       0,     0,     0,     0,     0,     7,     8,     9,    10,    11,
      12,    13,     1,     2,    14,    15,    16,    17,   281,     0,
       0,     3,     0,    85,    86,    20,     0,     0,     0,     0,
       0,     4,     0,     0,    22,    23,    62,    63,     6,     7,
       8,     9,    10,    11,    12,    13,     0,     0,    14,    15,
      16,    17,     0,     0,     0,     0,     0,    18,    19,    20,
       0,     0,     0,     0,     0,     0,     0,   108,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    21,     0,     0,
       0,     0,     0,    87,     0,     0,    22,    23,    62,    63,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      22,    23,     0,     0,     0,    24,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    87,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    25,
      26
};

static const yytype_int16 yycheck[] =
{
     104,   105,   125,   424,     6,   104,   118,    28,   112,   125,
      53,   421,   116,   288,   136,   119,   356,   672,   861,   136,
     249,   125,   330,    25,   265,   386,   125,    69,   260,   330,
     861,   422,   724,   318,   374,    77,   330,   535,   536,   710,
     724,   711,   154,   406,   136,   247,   781,    25,   171,   234,
     119,   251,   722,    96,   425,  1275,   713,   740,   621,   861,
      14,   104,   106,   711,  1284,    24,   362,    17,    11,    12,
      13,    21,   171,   887,   575,     8,   890,   725,   386,    24,
     124,    14,   125,    98,    14,     8,   128,    14,   130,   590,
     123,    14,    25,    26,    20,   280,    14,  1103,   101,   101,
    1106,   155,    25,    26,    76,   103,    14,    95,    96,   472,
     103,   162,   163,    77,   165,   166,   167,   168,   103,   231,
     152,   288,   176,   101,    17,   247,   812,    17,   171,   136,
     247,    11,    12,    13,    22,   302,    16,    36,    37,   154,
      99,    14,   175,   186,   256,    64,   178,   154,  1368,    17,
     254,    83,   155,    21,   429,   247,   260,   129,   844,    14,
      22,    24,   314,   315,   128,   317,   130,    17,   409,   167,
      25,    26,    65,   295,   167,    65,   299,    17,   295,   186,
     173,    14,   167,    71,   288,   254,   229,   871,    17,   356,
      24,    15,    91,   169,    23,   128,   129,   301,   302,   762,
     299,   244,   477,   295,    16,   128,   129,   374,   900,    71,
      24,   254,  1218,   415,   510,   417,   886,   176,   447,   448,
     170,   175,   167,   425,   456,   457,   349,   271,   272,   431,
     515,   888,   175,   277,    24,   476,   884,    17,   523,  1459,
     247,   167,   175,   443,   927,   175,   901,    24,   175,   353,
     349,   355,   175,   176,   925,   178,   299,   175,   619,   620,
     262,   263,   672,    17,   285,   432,   370,   175,  1082,   373,
     374,  1085,   643,   128,   129,   318,  1496,   170,   285,   159,
     170,    20,   845,   176,   847,  1505,   176,   399,   295,   474,
     178,   298,   335,   415,   715,   175,   301,   302,   415,   421,
      24,   413,   170,   425,   311,   312,   349,   668,   425,   670,
     477,   318,    17,   176,   724,   320,   178,   421,    24,  1539,
     170,   176,   298,   415,   174,   429,    17,    22,   335,   421,
     170,   832,   499,   425,   174,   311,   312,   838,    20,   443,
      20,   170,   176,   394,   395,   396,   548,   159,   353,   740,
     123,   356,   456,   457,   330,   424,   358,  1159,    17,   511,
      14,    24,   514,   177,  1166,  1179,   518,    17,   373,   374,
    1184,    25,    26,   477,   443,   155,    71,    20,    21,   386,
     358,    24,    17,    53,    17,  1120,   176,   120,    16,   752,
     170,   495,     8,     9,    10,  1065,   129,    16,     0,   176,
      16,   422,   175,    16,    24,    17,    37,    24,   415,   638,
     386,    27,    24,   167,   581,   422,   170,   424,   425,   727,
      36,  1076,    61,  1094,     8,     9,    10,  1119,   167,   123,
      24,    17,    16,    24,   709,    24,   603,    17,   542,   562,
     152,   643,    17,    27,    24,    17,    62,   173,   491,   451,
     155,    17,    36,   177,    36,  1115,    14,  1117,   660,  1119,
      91,    17,   174,   562,   176,   170,    82,    25,    26,   573,
      37,   177,   515,  1275,   128,   129,   167,   152,    62,   170,
      62,   175,  1284,   178,   491,   167,    17,   167,  1172,   987,
     900,   901,   543,   536,   545,   599,   155,    17,    82,   807,
     861,   176,   923,  1346,    62,   155,   807,   177,   710,   794,
     871,   170,   806,   176,   755,  1346,    83,   621,  1052,   562,
     170,   643,   176,    20,   167,    14,   643,   155,   535,   536,
     921,   174,   167,   176,   153,   170,   927,   170,   177,   582,
      17,   274,   155,   159,  1346,    14,   713,   167,   591,   653,
     672,   643,   656,   861,   658,   167,    25,    26,   170,   175,
     177,   176,   791,   871,    17,   123,  1368,   104,   672,   155,
     128,   129,   676,   167,    65,   159,   167,   620,   167,   887,
     672,  1236,   890,   155,   170,   176,    65,   167,   125,   155,
     170,   175,    24,    62,    24,   170,   150,    14,   170,   155,
     875,   876,   724,    17,   170,   709,    20,    21,   175,    23,
      24,   723,   619,   620,   170,    14,   103,   175,   740,  1052,
     724,   175,  1055,   740,   155,   162,   163,   670,   165,   166,
     167,   168,   724,   152,   171,   155,   643,    43,   742,   170,
     103,   160,   746,  1177,   748,   666,   173,     8,   740,   924,
     170,   162,   163,    14,   123,   123,    16,  1459,   762,   128,
     129,   668,   829,   670,    25,    26,  1076,  1028,  1029,    99,
     167,   873,   715,    38,    39,    40,    41,    42,   155,    14,
     167,  1042,  1043,   912,   913,   176,    92,   123,    94,   103,
      25,    26,   796,   170,  1496,   799,   103,   176,   740,   811,
     804,    64,   155,  1505,   167,   152,   175,   175,   715,  1119,
     939,   834,   123,   174,   152,   176,    14,   170,   834,   724,
     727,   888,   160,   925,   828,   829,   132,    25,    26,   176,
     834,   178,    14,   740,  1074,   834,   965,  1539,  1078,   175,
     969,   845,   102,   847,  1177,   844,   176,   152,   852,   751,
      15,   794,   795,   167,   797,    14,   170,    37,   765,   766,
     167,   768,   299,    43,   175,   173,   173,   128,   129,   177,
     874,   875,   876,   178,  1082,    14,    14,  1085,   900,   901,
      62,    38,    39,    40,    41,    42,    25,    26,   123,   765,
     766,   834,   768,   128,   129,   152,   900,   901,   152,   159,
     807,   905,   173,    83,    64,   927,   177,   167,   900,   901,
     927,  1172,   349,   178,   175,   176,    14,   178,  1093,   176,
     924,   152,   176,   828,   829,   123,  1236,    25,    26,   160,
     128,   129,    64,   173,  1001,   927,   152,   177,    83,   376,
     175,   378,   379,   380,   381,   382,   383,   384,   385,  1051,
     387,   388,   389,   390,   391,   392,   393,   394,   395,   396,
     176,  1090,  1091,   177,  1172,  1173,  1098,   152,   911,   973,
     974,  1179,   119,   120,   152,   979,  1184,   175,    35,   921,
     887,   152,   177,   890,   123,   927,  1161,  1162,   152,   128,
     129,   176,  1094,   997,    14,   175,   174,  1126,   176,   169,
     921,   152,   152,   910,   911,   176,   174,  1074,   176,   152,
     174,  1078,    93,    94,   921,   917,   923,     8,     9,    10,
     927,   178,   164,   174,   174,   123,   176,  1031,   178,    35,
     128,   129,   975,   176,   115,   978,   175,   980,    14,   152,
    1044,     8,     9,    10,   987,    36,    14,   152,   152,    25,
      26,  1153,    14,   152,  1076,   160,    14,  1069,   509,    17,
      14,   174,   513,    25,    26,    23,   517,    25,    26,    36,
     174,   167,  1076,   152,  1078,   174,   173,   175,   159,   160,
     987,  1104,  1086,  1087,  1076,  1346,  1029,    14,    50,  1093,
     176,   177,   152,   152,  1098,   174,   152,  1119,    25,    26,
    1043,   160,    35,    36,   152,  1104,   543,   315,   545,   317,
     153,   152,    14,  1112,   174,  1119,   384,   385,   174,   387,
     388,  1028,  1029,    25,    26,   562,   174,  1119,  1195,  1133,
     103,  1198,  1234,   174,  1138,  1042,  1043,   307,  1346,    38,
      39,    40,    41,    42,    14,   173,   887,  1408,  1409,   890,
    1154,   174,   128,   129,  1158,    25,    26,  1161,  1162,   152,
     330,  1104,    21,  1167,  1168,   123,   128,   129,     8,  1074,
     128,   129,    97,  1078,    14,  1082,   511,   156,  1085,   514,
      50,   174,   157,   518,   354,    25,    26,   158,    28,    29,
      30,    31,    32,    33,    34,   116,   117,   118,    14,   175,
    1204,   128,   129,   152,    14,   375,    16,   377,   152,    25,
      26,    53,   170,   175,  1236,    25,    26,   175,    58,    59,
      35,   123,   164,   152,   152,   174,   128,   129,   152,   156,
     174,    14,  1236,    16,    11,    12,    13,    14,   152,    16,
     152,   176,    25,    26,  1236,   174,   174,   152,    25,    26,
     174,    28,    29,    30,    31,    32,    33,    34,   128,   129,
     174,   152,   174,  1206,   164,   165,   166,    16,  1272,   174,
     152,   152,  1179,   175,   152,  1377,   152,  1184,   152,   178,
      23,    58,   177,   174,     3,     4,  1461,  1462,   128,   129,
      14,   178,    16,   174,   173,  1299,   174,   130,   174,  1206,
     174,    25,    26,   174,    14,   175,   440,   123,   442,   152,
     171,    14,   128,   129,    27,    25,    26,   174,   128,   129,
    1324,    27,    25,    26,     8,     9,    10,   173,   152,   174,
      14,   174,    16,   152,   152,   175,  1335,   101,   508,  1343,
      27,    25,    26,  1347,  1348,   128,   129,  1351,  1352,   159,
     174,   128,   129,   176,  1297,   174,   174,   171,    82,   175,
     177,     1,  1366,   175,   152,     5,     6,   173,    14,   178,
      16,  1375,  1376,   119,   120,   812,   159,     8,   548,    25,
      26,    21,   159,    14,   173,    25,   174,   521,     8,     9,
      10,   561,   119,   120,    25,    26,   177,   834,   175,    38,
      39,    40,    41,    42,   128,   129,  1410,   844,     5,     6,
       7,    80,    81,  1417,     8,     9,    10,   176,   128,   129,
      14,    24,  1426,   100,  1428,   128,   129,  1431,  1432,   173,
      70,    25,    26,    73,   871,   159,    38,    39,    40,    41,
      42,   153,    82,   173,   128,   129,   156,    38,    39,    40,
      41,    42,    27,  1457,  1458,   311,   312,  1461,  1462,    99,
     100,   101,   171,  1467,   104,  1469,  1409,  1471,   173,  1473,
     392,   393,  1476,   174,  1478,   159,   101,  1481,   118,   649,
     177,   651,   128,   129,   174,   125,   152,   657,   101,   659,
    1494,   152,   662,   133,  1296,  1297,   136,   128,   129,    27,
      14,  1408,  1409,   171,  1508,    27,  1510,   155,  1512,   173,
    1514,    25,    26,   159,   154,   173,  1520,   171,  1522,  1523,
     382,   383,   162,   163,   153,   165,   166,   167,   168,   173,
     171,   171,   155,  1537,   128,   129,   173,   173,    14,   178,
     173,    17,   176,  1547,    20,    21,   186,    23,    24,    25,
      26,   173,  1556,    61,  1558,   173,   177,  1561,  1562,   176,
     152,  1565,     0,  1567,   173,  1569,   173,  1571,     8,   173,
    1574,   177,  1576,   178,    14,  1579,   178,  1581,    14,  1583,
     173,  1585,   171,  1587,   174,    25,    26,   178,    21,    25,
      26,   231,     8,    21,   764,   389,   390,   391,    14,    37,
      21,   735,   173,   737,   244,   173,     3,   247,    73,    25,
      26,   781,   176,   783,   128,   129,   256,   176,    14,   259,
     176,   261,   262,   263,    64,   176,   176,   103,   176,    25,
      26,   176,    28,    29,    30,    31,    32,    33,    34,   773,
     152,   152,   156,   119,   120,   285,    82,   173,    64,   174,
     820,    24,   128,   129,   788,   295,   152,   174,   298,   299,
     173,   171,    58,   152,    14,   174,   173,  1104,   174,   107,
     108,   311,   312,   176,   174,  1112,   174,   174,   318,   173,
     118,   119,   183,   853,   174,   855,   174,   857,   128,   129,
     860,   167,   128,   129,   170,   335,   830,    38,    39,    40,
      41,    42,     8,     9,    10,   173,   173,   841,    14,   349,
      16,     8,   128,   129,   176,    14,   154,    14,   358,    25,
      26,    38,    39,    40,    41,    42,    25,    26,    25,    26,
      36,   169,   128,   129,   171,  1172,   376,   171,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   391,   392,   393,   394,   395,   396,   171,   154,   399,
       8,     9,    10,     8,     9,    10,    14,    64,    16,    14,
     173,    16,   176,   413,   176,   415,    82,    25,    26,   175,
      25,    26,   422,   153,   424,   425,   153,   176,   176,     8,
     291,   173,   962,   231,   964,    14,    62,   967,    14,   174,
     176,   152,   176,   101,   173,   173,    25,    26,   155,    25,
      26,   451,   155,   314,   315,   173,   317,   155,   256,    68,
     174,   955,   128,   129,    11,    12,    13,    14,    68,   128,
     129,   128,   129,    49,    82,   174,    52,   178,    25,    26,
     155,    28,    29,    30,    31,    32,    33,    34,   155,   176,
     153,   491,  1022,   159,   174,   174,   174,  1027,   153,   153,
     298,   178,   153,   176,   176,   176,   176,   368,   369,   173,
    1004,    58,   173,   311,   312,   155,    14,    62,   316,  1013,
     128,   129,    62,   128,   129,   174,   176,    25,    26,   176,
     176,   155,   330,   155,   155,   535,   536,   174,  1335,   174,
     174,    14,    68,   543,   176,   545,   176,   176,   176,   128,
     129,   159,   128,   129,   159,   174,   174,   174,   356,    14,
     176,    14,   562,    14,   174,   176,   401,    14,   366,   176,
      25,    26,    25,    26,    25,    26,  1070,  1071,    25,    26,
      14,   128,   129,   174,   174,   174,   174,   174,   386,   174,
    1120,    25,    26,    14,    28,    29,    30,    31,    32,    33,
      34,   399,    49,   401,    25,    26,   176,    28,    29,    30,
      31,    32,    33,    34,   412,   413,    50,   478,   479,   619,
     620,   176,    14,   421,    58,   176,   424,  1157,   175,    50,
     128,   129,    66,    25,    26,  1165,   174,    58,     8,     9,
      10,   174,   174,   643,    14,   443,   646,   176,   176,  1143,
     511,   174,   155,   514,   155,    25,    26,   518,    28,    29,
      30,    31,    32,    33,    34,   176,    36,   174,   668,   155,
     670,   532,    14,   128,   129,   128,   129,   128,   129,    14,
      50,   128,   129,    25,    26,   546,   155,   155,    58,    59,
      25,    26,    62,   155,   128,   129,    66,   174,  1192,  1193,
     174,   403,    14,   174,  1234,   154,   635,   128,   129,   174,
     174,   174,    82,    25,    26,   715,    86,    87,    88,    89,
      90,    91,    92,   723,   724,   523,   587,   727,   174,   527,
     174,  1154,   530,   174,   174,   174,   128,   129,   174,   174,
     740,   175,   174,   768,   751,   748,    36,   770,    37,   273,
    1280,   751,  1282,   174,   175,   125,  1250,   470,   128,   129,
     108,  1291,  1292,   256,    14,   765,   766,    14,   768,  1069,
     415,    14,   276,   295,   422,    25,    26,   753,    25,    26,
     927,   579,    25,    26,    14,  1156,   128,   129,    14,   370,
      14,   443,  1115,   128,   129,    25,    26,  1117,    14,    25,
      26,    25,    26,   173,   174,   175,  1335,   807,  1234,    25,
      26,   811,   812,  1346,  1344,  1345,   128,   129,  1055,  1313,
    1172,  1315,   316,   807,  1354,   378,  1356,   376,  1358,   379,
    1360,    -1,   380,  1363,   834,  1365,    14,   381,   812,  1369,
    1370,  1371,  1372,    -1,   844,    -1,    14,    25,    26,   647,
      28,    29,    30,    31,    32,    33,    34,    25,    26,    -1,
      -1,    -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,
      -1,   871,    -1,    -1,   672,    25,    26,  1407,   128,   129,
      58,   128,   129,    -1,    -1,   128,   129,   887,    66,  1419,
     890,  1421,    -1,  1423,    -1,  1425,    14,  1391,   128,   129,
    1394,    -1,   128,   129,   128,   129,    -1,    25,    26,    -1,
     910,   911,   128,   129,    -1,    -1,    -1,   917,    -1,    -1,
    1343,   921,    14,   923,  1347,   723,   724,   927,   118,   727,
      -1,    -1,    14,    25,    26,  1465,    -1,    -1,    -1,    -1,
      -1,   802,   803,    25,    26,    14,   136,   808,   809,    -1,
     128,   129,    -1,    -1,    -1,    -1,    25,    26,    -1,    -1,
     128,   129,    -1,    -1,   154,    14,    -1,    -1,    -1,    -1,
     768,  1501,    14,  1503,    -1,   836,    25,    26,   128,   129,
      -1,    -1,    -1,    25,    26,   846,    -1,   987,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1525,   186,  1527,    -1,  1529,
     798,  1531,   800,   801,  1534,    14,  1536,    -1,    -1,   807,
     128,   129,    -1,   811,    -1,    14,    25,    26,    -1,  1549,
      -1,  1551,    -1,  1553,    -1,  1555,    25,    26,  1028,  1029,
      -1,    -1,    -1,    -1,  1457,  1458,   128,   129,    -1,    -1,
      14,   231,  1042,  1043,    -1,    -1,   128,   129,    -1,    -1,
      -1,    25,    26,    14,   244,    -1,    -1,   247,    -1,   128,
     129,    -1,    -1,   861,    25,    26,   256,    -1,    -1,  1069,
      -1,  1494,    -1,   871,    -1,    -1,    -1,    -1,    -1,   128,
     129,    -1,  1082,   944,    -1,  1085,   128,   129,    -1,   887,
      -1,    -1,   890,    -1,    -1,   285,    -1,  1520,    14,  1522,
    1523,    -1,   900,   901,  1104,   295,    -1,   905,    -1,    25,
      26,    -1,  1112,    -1,  1537,   976,   977,    -1,    -1,   128,
     129,    -1,   983,   984,  1547,   923,    -1,    -1,   318,   128,
     129,    -1,    -1,  1556,    -1,  1558,    -1,    -1,  1561,  1562,
     330,    -1,  1565,    -1,  1567,   335,  1569,    -1,  1571,    -1,
      -1,  1574,    -1,  1576,   128,   129,  1579,    -1,  1581,    -1,
    1583,    -1,  1585,    -1,  1587,    14,    -1,   128,   129,    -1,
      -1,    14,  1172,  1173,    -1,    -1,    25,    26,    -1,  1179,
      -1,    -1,    25,    26,  1184,    28,    29,    30,    31,    32,
      33,    34,    -1,    -1,    -1,    -1,   386,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1066,  1206,    50,    -1,   399,
      -1,    -1,   128,   129,    -1,    58,    -1,    -1,  1079,    -1,
    1081,    -1,    -1,   413,    -1,   415,    -1,    -1,    -1,    14,
      -1,   421,   422,    -1,   424,   425,    -1,    -1,    -1,    14,
      25,    26,  1103,    28,    29,    30,    31,    32,    33,    34,
      25,    26,    -1,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    -1,    -1,    50,    -1,    -1,  1343,    -1,
      -1,  1069,  1347,    58,    -1,    50,  1074,   136,  1076,   128,
     129,    66,    -1,    58,  1082,   128,   129,  1085,  1086,  1087,
      -1,    66,    -1,    -1,    -1,   154,  1296,  1297,    -1,    -1,
      -1,   491,  1163,  1164,    -1,    -1,    -1,    -1,    -1,   136,
     169,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1119,    -1,    -1,  1185,  1186,    -1,   186,    -1,    -1,
      -1,   174,   175,    -1,    -1,  1335,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   128,   129,   535,   536,  1208,  1209,  1210,
      14,  1212,    -1,   128,   129,    -1,    -1,    -1,    -1,    -1,
      -1,    25,    26,    -1,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    -1,  1172,  1173,    -1,    -1,    -1,  1379,
      -1,  1179,  1457,  1458,    -1,    -1,  1184,    -1,   247,    -1,
     175,    -1,    -1,    -1,    58,    -1,    -1,    -1,    -1,    -1,
     175,    -1,    66,    -1,    -1,    -1,  1204,    -1,  1408,  1409,
      -1,    -1,    -1,    -1,    -1,  1276,  1277,  1278,  1279,  1494,
     247,    -1,    -1,    -1,    -1,    -1,   285,    -1,    -1,   619,
     620,    -1,    -1,    -1,    -1,    -1,   295,    -1,  1236,    -1,
      -1,    -1,    -1,    -1,    -1,  1520,    -1,  1522,  1523,    -1,
      -1,    -1,    -1,   643,    -1,    -1,    -1,    -1,   285,   318,
    1321,  1322,  1537,    -1,   128,   129,    -1,    -1,   295,    -1,
      -1,   330,  1547,    -1,    -1,    -1,   335,    -1,   668,    -1,
     670,  1556,   672,  1558,    -1,    -1,  1561,  1562,    -1,    -1,
    1565,    -1,  1567,    -1,  1569,    -1,  1571,    -1,    -1,  1574,
      -1,  1576,    -1,   330,  1579,    -1,  1581,    -1,  1583,    -1,
    1585,   175,  1587,    -1,    -1,    -1,    -1,  1378,    -1,  1380,
      -1,  1382,    -1,    -1,    -1,   715,  1324,   386,    -1,    -1,
      -1,    -1,    -1,   723,   724,  1396,  1397,   727,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1346,    -1,
     740,    -1,    -1,    -1,    -1,    -1,   415,    -1,    -1,   386,
      -1,    -1,   421,   422,    -1,   424,   425,    -1,    -1,   421,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1438,  1439,  1440,
    1441,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   415,    -1,
      -1,    -1,    -1,    -1,   421,   422,    -1,   424,   425,    -1,
      -1,    -1,  1463,  1464,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,    -1,    -1,   807,    -1,    -1,
      -1,   811,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      14,   104,   491,    -1,    -1,    -1,  1497,  1498,  1499,  1500,
      -1,    25,    26,    14,    28,    29,    30,    31,    32,    33,
      34,    -1,   125,   153,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,    50,    -1,    -1,    -1,
      -1,   861,    -1,    -1,    58,    -1,   535,   536,    -1,    50,
      -1,   871,    66,    -1,    -1,    -1,    -1,    58,    -1,   162,
     163,    -1,   165,   166,   167,   168,    -1,   887,   171,    -1,
     890,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   535,   536,
     900,   901,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     910,   911,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   921,    -1,   923,    -1,    14,    -1,   927,    -1,    -1,
      -1,    -1,    -1,    -1,   128,   129,    25,    26,    -1,    28,
      29,    30,    31,    32,    33,    34,    14,   128,   129,    -1,
     619,   620,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    58,
      -1,    -1,    -1,    -1,   643,    -1,    -1,    -1,    -1,    -1,
      -1,   175,   619,   620,    -1,    -1,    -1,   987,    -1,    -1,
      58,   653,    -1,   174,   175,    -1,    -1,    -1,    66,   668,
      -1,   670,    -1,   672,    -1,    -1,   643,    14,    -1,    -1,
     672,    -1,    -1,    -1,    -1,    -1,   299,    -1,    25,    26,
      -1,    28,    29,    30,    31,    32,    33,    34,  1028,  1029,
      -1,   668,    -1,   670,    -1,   672,    -1,    -1,    -1,   128,
     129,    -1,  1042,  1043,    -1,    -1,   715,    -1,    -1,    -1,
      -1,    58,    59,    -1,    -1,   724,    -1,    -1,   727,    -1,
     128,   129,   724,    -1,    -1,   154,   349,    -1,    -1,  1069,
      -1,   740,    -1,    -1,    -1,    -1,  1076,    -1,   715,    -1,
     421,    -1,  1082,    -1,    -1,  1085,   175,   724,    -1,    -1,
     727,    -1,    -1,   376,    -1,   378,   379,   380,   381,   382,
     383,   384,   385,   740,   387,   388,   389,   390,   391,   392,
     393,   394,   395,   396,    14,    -1,    -1,    -1,    -1,  1119,
      -1,   128,   129,    -1,    -1,    25,    26,    -1,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,   807,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
      50,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,    -1,
     807,    -1,  1172,  1173,    -1,    -1,    -1,    -1,    -1,  1179,
      -1,    -1,    -1,    -1,  1184,    -1,    -1,    -1,   153,    -1,
     852,    -1,   861,    -1,    -1,    -1,    -1,    -1,    -1,   861,
      -1,    -1,   871,    -1,    -1,    -1,  1206,    -1,    -1,    -1,
      -1,    -1,   874,    -1,    -1,    -1,    -1,    -1,   887,    -1,
      -1,   890,    -1,    -1,   861,    -1,    -1,    -1,   128,   129,
      -1,   900,   901,    -1,   871,    -1,  1236,    -1,   900,   901,
      -1,   910,   911,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     887,    -1,   921,   890,   923,    -1,    -1,    -1,   927,    -1,
     543,    -1,   545,   900,   901,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   910,   911,   175,    -1,    -1,    -1,   562,
      -1,    -1,    -1,    -1,   921,    -1,   923,    -1,    -1,    14,
     927,    -1,    -1,    -1,    -1,    -1,  1296,  1297,    -1,    -1,
      25,    26,    -1,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,   653,    -1,    -1,    -1,    -1,    -1,   987,    -1,
      -1,     3,     4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   672,    14,    58,    -1,    17,    -1,    -1,    20,    21,
      -1,    23,    24,    25,    26,    -1,  1346,    -1,    -1,    -1,
     987,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1028,
    1029,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,  1042,  1043,    -1,    -1,    -1,    -1,  1379,
      -1,    -1,    -1,   724,    -1,    -1,    -1,    50,    -1,    -1,
      -1,  1028,  1029,    -1,    -1,    58,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   128,   129,  1042,  1043,  1076,  1408,  1409,
      -1,    -1,    -1,  1082,  1076,    -1,  1085,    -1,    -1,    -1,
      -1,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,    -1,    -1,    -1,    -1,    -1,    -1,  1076,
      -1,   123,    -1,    -1,    -1,  1082,   128,   129,  1085,    -1,
    1119,    -1,    -1,    -1,    -1,    -1,    -1,  1119,    14,    -1,
      -1,    -1,    -1,    -1,    -1,   128,   129,    -1,    -1,    25,
      26,   153,    28,    29,    30,    31,    32,    33,    34,    -1,
      -1,    -1,  1119,    -1,    -1,   167,    -1,    -1,   170,    -1,
      -1,    -1,  1154,   175,    -1,    -1,  1158,  1159,    -1,    -1,
      -1,    -1,    58,  1172,  1166,    -1,    -1,    -1,    -1,    -1,
    1179,   852,   175,    -1,    -1,  1184,    -1,    -1,    -1,    -1,
     861,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   812,
      -1,    -1,    -1,   874,    -1,  1172,   421,  1206,    -1,    -1,
      -1,    -1,  1179,    -1,    -1,    -1,    -1,  1184,    -1,    -1,
      -1,   834,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   900,
     901,   844,    -1,    -1,    -1,    -1,    -1,  1236,    -1,  1206,
      -1,    -1,   128,   129,  1236,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   871,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1236,
      -1,    -1,    -1,    -1,    -1,    -1,     8,    -1,    -1,    -1,
    1272,    -1,    14,  1275,    16,    -1,    18,    19,    -1,   175,
      -1,    -1,  1284,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    45,    46,    47,    48,    49,    50,    51,
      -1,    -1,    54,    55,    56,    57,    58,    59,    -1,    -1,
      -1,    63,    64,    65,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,    -1,    -1,  1346,    -1,    -1,
      -1,  1343,    -1,    -1,  1346,  1347,  1348,    -1,    -1,  1351,
    1352,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1366,    -1,  1368,    -1,    -1,  1346,
      -1,    -1,    -1,   153,    18,    19,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    27,   126,   127,   128,   129,    -1,   131,
      -1,    -1,    -1,    -1,    -1,  1076,    -1,    -1,    -1,  1408,
    1409,    45,    46,    47,    48,    49,    50,    51,    -1,    -1,
      54,    55,    56,    57,    -1,  1417,    -1,   159,    -1,    63,
      64,    65,    -1,    -1,  1426,    -1,  1428,    -1,   653,  1431,
    1432,  1408,  1409,   175,   176,   177,   178,    -1,  1119,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   672,    -1,    -1,
      -1,    -1,   421,    -1,    -1,  1457,  1458,  1459,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1467,    -1,  1469,    -1,  1471,
      -1,  1473,    -1,  1154,  1476,    -1,  1478,  1158,  1159,  1481,
      -1,  1104,   126,   127,    -1,  1166,    -1,   131,    -1,  1112,
      -1,    -1,  1494,    -1,  1496,    -1,    -1,    -1,    -1,   724,
      -1,    -1,    -1,  1505,    -1,    -1,  1508,    -1,  1510,    -1,
    1512,    -1,  1514,    -1,    -1,    -1,    -1,    -1,  1520,    -1,
    1522,  1523,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   175,   176,    -1,    -1,  1537,    -1,  1539,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1547,    -1,    -1,    -1,  1172,
      -1,    -1,    -1,    -1,  1556,  1236,  1558,    -1,    -1,  1561,
    1562,    -1,    -1,  1565,    -1,  1567,    -1,  1569,    -1,  1571,
      -1,    -1,  1574,    -1,  1576,    -1,    -1,  1579,    -1,  1581,
      -1,  1583,    -1,  1585,    -1,  1587,    -1,    -1,    -1,    -1,
      -1,  1272,    -1,    -1,  1275,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1284,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    18,    19,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    27,    -1,    -1,    -1,   852,    -1,    -1,
      -1,    -1,    -1,    37,    -1,    -1,   861,    -1,    -1,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    -1,   874,
      54,    55,    56,    57,    -1,    -1,    -1,    -1,    -1,    63,
      64,    65,  1343,    -1,    -1,  1346,  1347,  1348,    -1,    -1,
    1351,  1352,    -1,    -1,    -1,   900,   901,    -1,    -1,    83,
      -1,    -1,    -1,    -1,   653,  1366,    -1,  1368,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   136,    -1,    -1,   421,    -1,
      -1,    -1,    -1,   672,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1335,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   126,   127,    -1,    -1,    -1,   131,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1417,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1426,    -1,  1428,    -1,    -1,
    1431,  1432,    -1,    -1,    -1,   724,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   175,   176,    -1,    -1,    -1,  1457,  1458,  1459,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1467,    -1,  1469,    -1,
    1471,    -1,  1473,    -1,    -1,  1476,    -1,  1478,    -1,    -1,
    1481,    -1,    -1,    -1,    -1,    -1,   247,    -1,    -1,    -1,
      -1,    -1,    -1,  1494,    -1,  1496,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1505,    -1,    -1,  1508,    -1,  1510,
      -1,  1512,    -1,  1514,    -1,    -1,    -1,    -1,    -1,  1520,
      -1,  1522,  1523,    -1,   285,    -1,    -1,    -1,    -1,    -1,
      -1,  1076,    -1,    -1,   295,    -1,  1537,    -1,  1539,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1547,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1556,    -1,  1558,    -1,    -1,
    1561,  1562,    -1,   852,  1565,    -1,  1567,    -1,  1569,   330,
    1571,    -1,   861,  1574,  1119,  1576,    -1,    -1,  1579,    -1,
    1581,    -1,  1583,    -1,  1585,   874,  1587,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1154,
     653,   900,   901,  1158,  1159,    -1,    -1,    -1,    -1,    -1,
      -1,  1166,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   672,
      -1,    -1,    -1,    -1,   421,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   415,    -1,    -1,    -1,    -1,    -1,
      -1,   422,    -1,   424,   425,    -1,    -1,    -1,    -1,     5,
       6,     7,     8,     9,    10,    -1,    -1,    -1,    14,    -1,
      -1,   724,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,  1236,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    58,    59,    -1,    -1,    62,  1272,    -1,    -1,
    1275,    -1,    -1,    -1,    70,    -1,    -1,    -1,    -1,  1284,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   535,   536,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   121,   122,  1076,    -1,   125,
      -1,    -1,   128,   129,    -1,    -1,    -1,    -1,  1343,    -1,
      -1,  1346,  1347,  1348,    -1,    -1,  1351,  1352,    -1,   852,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   861,    -1,
      -1,  1366,    -1,  1368,    -1,    -1,   162,   163,    -1,    -1,
    1119,   874,   168,   169,    -1,    -1,    -1,   173,    -1,   175,
      -1,   177,   178,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   900,   901,    -1,
      -1,    -1,    18,    19,    -1,  1154,   653,    -1,    -1,  1158,
    1159,    27,  1417,    -1,    -1,    -1,    -1,  1166,    -1,    -1,
     421,  1426,   643,  1428,    -1,   672,  1431,  1432,    -1,    45,
      46,    47,    48,    49,    50,    51,    -1,    -1,    54,    55,
      56,    57,    -1,    -1,    -1,    -1,    -1,    63,    64,    65,
      -1,    -1,  1457,  1458,  1459,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1467,    -1,  1469,    -1,  1471,    -1,  1473,    -1,
      -1,  1476,    -1,  1478,    -1,    -1,  1481,   724,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1236,    -1,  1494,
      -1,  1496,    -1,    -1,   715,    -1,    -1,    -1,    -1,    -1,
    1505,    -1,    -1,  1508,    -1,  1510,   727,  1512,    -1,  1514,
     126,   127,    -1,    -1,    -1,  1520,    -1,  1522,  1523,   740,
      -1,    -1,    -1,  1272,    -1,    -1,  1275,    -1,    -1,    -1,
      -1,    -1,  1537,    -1,  1539,  1284,    -1,    -1,    -1,    -1,
      -1,    -1,  1547,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1556,    -1,  1558,    -1,    -1,  1561,  1562,    -1,   175,
    1565,    -1,  1567,    -1,  1569,    -1,  1571,    -1,    -1,  1574,
      -1,  1576,    -1,  1076,  1579,    -1,  1581,    -1,  1583,    -1,
    1585,    -1,  1587,    -1,    -1,    -1,   807,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1343,    -1,    -1,  1346,  1347,  1348,
      -1,    -1,  1351,  1352,    -1,   852,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   861,    -1,  1119,  1366,    -1,  1368,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   874,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1154,   653,   900,   901,  1158,  1159,    -1,    -1,    -1,
      -1,    -1,    -1,  1166,    -1,    -1,   887,    -1,  1417,   890,
      -1,   672,    -1,    -1,    -1,    -1,   421,  1426,    -1,  1428,
      -1,    -1,  1431,  1432,    -1,    -1,    -1,    -1,    -1,   910,
     911,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     921,    -1,   923,    -1,    -1,    -1,   927,    -1,  1457,  1458,
    1459,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1467,    -1,
    1469,    -1,  1471,   724,  1473,    -1,    -1,  1476,    -1,  1478,
      -1,    -1,  1481,  1236,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1494,    -1,  1496,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1505,    -1,    -1,  1508,
      -1,  1510,    -1,  1512,    -1,  1514,   987,    -1,    -1,  1272,
      -1,  1520,  1275,  1522,  1523,    -1,    -1,    -1,    -1,    -1,
      -1,  1284,    -1,    -1,    -1,    -1,    -1,    -1,  1537,    -1,
    1539,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1547,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1556,    -1,  1558,
      -1,    -1,  1561,  1562,    -1,    -1,  1565,    -1,  1567,    -1,
    1569,    -1,  1571,    -1,    -1,  1574,    -1,  1576,    -1,  1076,
    1579,    -1,  1581,    -1,  1583,    -1,  1585,    -1,  1587,    -1,
    1343,    -1,    -1,  1346,  1347,  1348,    -1,    -1,  1351,  1352,
      -1,   852,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     861,  1082,    -1,  1366,  1085,  1368,    -1,    -1,    -1,    -1,
      -1,    -1,  1119,   874,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   900,
     901,    -1,    -1,    -1,    18,    -1,    -1,  1154,   653,    -1,
      -1,  1158,  1159,    27,  1417,    -1,    -1,    -1,    -1,  1166,
      -1,    -1,    -1,  1426,    -1,  1428,    -1,   672,  1431,  1432,
      -1,    45,    46,    47,    48,    49,    50,    51,    -1,    -1,
      54,    55,    56,    57,    -1,    -1,    -1,    -1,    -1,    63,
      64,    65,    -1,    -1,  1457,  1458,  1459,    -1,  1179,    -1,
      -1,    -1,    -1,  1184,  1467,    -1,  1469,    -1,  1471,    -1,
    1473,    -1,    -1,  1476,    -1,  1478,    -1,    -1,  1481,   724,
      -1,    -1,    -1,    -1,    -1,  1206,    -1,    -1,    -1,  1236,
      -1,  1494,    -1,  1496,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1505,    -1,    -1,  1508,    -1,  1510,    -1,  1512,
      -1,  1514,   126,   127,    -1,    -1,    -1,  1520,    -1,  1522,
    1523,    -1,    -1,    -1,    -1,  1272,    -1,    -1,  1275,    -1,
      -1,    -1,    -1,    -1,  1537,    -1,  1539,  1284,    -1,    -1,
      -1,    -1,    -1,    -1,  1547,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1556,    -1,  1558,    -1,    -1,  1561,  1562,
      -1,    -1,  1565,    -1,  1567,    -1,  1569,    -1,  1571,    -1,
      -1,  1574,    -1,  1576,    -1,  1076,  1579,    -1,  1581,    -1,
    1583,    -1,  1585,    -1,  1587,    -1,   653,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1343,    -1,    -1,  1346,
    1347,  1348,    -1,    -1,  1351,  1352,    -1,   852,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   861,    -1,  1119,  1366,
      -1,  1368,     8,     9,    10,    -1,    -1,    -1,    14,   874,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,  1154,    -1,   900,   901,  1158,  1159,    -1,
      -1,    -1,    -1,    -1,    50,  1166,    -1,    -1,    -1,    -1,
    1417,    -1,    58,    59,    -1,    -1,    62,    -1,    -1,  1426,
      66,  1428,    -1,    -1,  1431,  1432,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
    1457,  1458,  1459,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1467,    -1,  1469,    -1,  1471,    -1,  1473,    -1,    -1,  1476,
      -1,  1478,    -1,    -1,  1481,  1236,    -1,    -1,    -1,   125,
      -1,    -1,   128,   129,    -1,    -1,    -1,  1494,    -1,  1496,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1505,    -1,
      -1,  1508,    -1,  1510,    -1,  1512,    -1,  1514,    -1,    -1,
      -1,  1272,    -1,  1520,  1275,  1522,  1523,    -1,    -1,    -1,
      -1,    -1,    -1,  1284,    -1,   852,    -1,   173,    -1,   175,
    1537,    -1,  1539,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1547,    -1,    -1,    -1,    -1,    -1,    -1,   874,    -1,  1556,
      -1,  1558,    -1,    -1,  1561,  1562,    -1,    -1,  1565,    -1,
    1567,    -1,  1569,    -1,  1571,    -1,    -1,  1574,    -1,  1576,
      -1,  1076,  1579,    -1,  1581,    -1,  1583,    -1,  1585,    -1,
    1587,    -1,  1343,    -1,    -1,  1346,  1347,  1348,    -1,    -1,
    1351,  1352,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1366,    -1,  1368,    -1,    -1,
      -1,    -1,    -1,    -1,  1119,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1154,
      -1,    -1,    -1,  1158,  1159,    -1,  1417,    -1,    -1,    -1,
      -1,  1166,    -1,    -1,    -1,  1426,    -1,  1428,    -1,    -1,
    1431,  1432,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1457,  1458,  1459,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1467,    -1,  1469,    -1,
    1471,    -1,  1473,    -1,    -1,  1476,    -1,  1478,    -1,    -1,
    1481,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1236,    -1,  1494,    -1,  1496,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1505,    -1,    -1,  1508,    -1,  1510,
      -1,  1512,    -1,  1514,    -1,    -1,    -1,    -1,    -1,  1520,
      -1,  1522,  1523,    -1,    -1,    -1,    -1,  1272,    -1,    -1,
    1275,    -1,    -1,    -1,    -1,    -1,  1537,    -1,  1539,  1284,
      -1,    -1,    -1,    -1,    -1,    -1,  1547,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1556,    -1,  1558,    -1,    -1,
    1561,  1562,    -1,    -1,  1565,    -1,  1567,    -1,  1569,    -1,
    1571,    -1,    -1,  1574,    -1,  1576,    -1,    -1,  1579,    -1,
    1581,    -1,  1583,    -1,  1585,    -1,  1587,  1154,    -1,    -1,
      -1,  1158,    -1,    -1,    -1,    -1,    -1,    -1,  1343,    -1,
      -1,  1346,  1347,  1348,    -1,    -1,  1351,  1352,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     8,
      -1,  1366,    -1,  1368,    -1,    14,    -1,    16,    -1,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,    48,
      49,    50,    51,    -1,    -1,    54,    55,    56,    57,    58,
      59,    -1,  1417,    -1,    63,    64,    65,    -1,    -1,    -1,
      -1,  1426,    -1,  1428,    -1,    -1,  1431,  1432,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1272,    -1,    -1,    -1,    -1,
      -1,    -1,  1457,  1458,  1459,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1467,    -1,  1469,    -1,  1471,    -1,  1473,    -1,
      -1,  1476,    -1,  1478,    -1,    -1,  1481,   126,   127,   128,
     129,    -1,   131,    -1,    -1,    -1,    -1,    -1,    -1,  1494,
      -1,  1496,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1505,    -1,    -1,  1508,    -1,  1510,    -1,  1512,    -1,  1514,
     159,    -1,    -1,    -1,    -1,  1520,  1343,  1522,  1523,    -1,
    1347,  1348,    -1,    -1,  1351,  1352,   175,   176,   177,   178,
      -1,    -1,  1537,    -1,  1539,    -1,    -1,    -1,    -1,  1366,
      -1,    -1,  1547,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1556,    -1,  1558,    -1,    -1,  1561,  1562,    -1,    -1,
    1565,    -1,  1567,    -1,  1569,    -1,  1571,    -1,    -1,  1574,
      -1,  1576,    -1,    -1,  1579,    -1,  1581,    -1,  1583,    -1,
    1585,    -1,  1587,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1417,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1426,
      -1,  1428,    -1,    -1,  1431,  1432,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1457,  1458,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1467,    -1,  1469,    -1,  1471,    -1,  1473,    -1,    -1,  1476,
      -1,  1478,    -1,    -1,  1481,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1494,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1508,    -1,  1510,    -1,  1512,    -1,  1514,    -1,    -1,
      -1,    -1,    -1,  1520,    -1,  1522,  1523,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1537,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1547,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1556,
      -1,  1558,    -1,    -1,  1561,  1562,    -1,    -1,  1565,    -1,
    1567,    -1,  1569,    -1,  1571,    -1,    -1,  1574,    -1,  1576,
      -1,    -1,  1579,    -1,  1581,    -1,  1583,    -1,  1585,    -1,
    1587,     5,     6,     7,     8,     9,    10,    -1,    -1,    -1,
      14,    -1,    -1,    -1,     5,     6,     7,     8,     9,    10,
      -1,    25,    26,    14,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,    -1,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    -1,    36,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    58,    59,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    82,    70,
      -1,    -1,    86,    87,    88,    89,    90,    91,    92,    -1,
      -1,    82,    -1,    -1,    -1,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,
      -1,   125,    -1,    -1,   128,   129,     5,     6,     7,     8,
       9,    10,    -1,    -1,   125,    14,    -1,   128,   129,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,    28,
      29,    30,    31,    32,    33,    34,    -1,    36,   162,   163,
      -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,   173,
      -1,   175,    -1,   177,   178,    -1,    -1,   168,   169,    58,
      59,    60,   173,    62,   175,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    18,    19,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    27,    82,    -1,    -1,    -1,    86,    87,    88,
      89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,    44,
      45,    46,    47,    48,    49,    50,    51,    -1,    -1,    54,
      55,    56,    57,    -1,    -1,    -1,    -1,    -1,    63,    64,
      65,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,   128,
     129,     5,     6,     7,     8,     9,    10,    -1,    -1,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    25,    26,    -1,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,   168,
     169,    -1,    -1,    -1,   173,    -1,   175,    -1,   177,    -1,
      -1,   126,   127,    -1,    58,    59,   131,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,
      -1,    -1,    86,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     175,   176,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,
      -1,   125,    -1,    -1,   128,   129,     5,     6,     7,     8,
       9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,    28,
      29,    30,    31,    32,    33,    34,    -1,    36,   162,   163,
      -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,   173,
      -1,   175,    -1,   177,    -1,    -1,    -1,    -1,    -1,    58,
      59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,    88,
      89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,   128,
     129,     5,     6,     7,     8,     9,    10,    -1,    -1,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    25,    26,    -1,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,   168,
     169,    -1,    -1,    -1,   173,    -1,   175,    -1,   177,    -1,
      -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,
      -1,    -1,    86,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,
      -1,   125,    -1,    -1,   128,   129,     5,     6,     7,     8,
       9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,    28,
      29,    30,    31,    32,    33,    34,    -1,    36,   162,   163,
      -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,   173,
      -1,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,    88,
      89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,   128,
     129,     5,     6,     7,     8,     9,    10,    -1,    -1,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    25,    26,    -1,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,   168,
     169,    -1,    -1,    -1,   173,    -1,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,
      -1,    -1,    86,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,
      -1,   125,    -1,    -1,   128,   129,     5,     6,     7,     8,
       9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,    28,
      29,    30,    31,    32,    33,    34,    -1,    36,   162,   163,
      -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,   173,
      -1,   175,   176,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,    88,
      89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,   128,
     129,     5,     6,     7,     8,     9,    10,    -1,    -1,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    25,    26,    -1,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,   168,
     169,    -1,    -1,    -1,   173,    -1,   175,   176,    -1,    -1,
      -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,
      -1,    -1,    86,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,
      -1,   125,    -1,    -1,   128,   129,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,   163,
       8,     9,    10,    -1,   168,   169,    14,    -1,    16,   173,
      18,   175,   176,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    -1,    -1,    54,    55,    56,    57,
      58,    59,    60,    -1,    62,    63,    64,    65,    66,    67,
      -1,    69,    70,    -1,    72,    73,    74,    75,    76,    77,
      78,    79,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,   126,   127,
     128,   129,    -1,   131,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    18,    -1,    -1,    -1,    -1,
      -1,   159,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,    -1,   173,    -1,   175,   176,   177,
     178,    -1,    45,    46,    47,    48,    49,    50,    51,    -1,
      -1,    54,    55,    56,    57,    58,    59,    60,    -1,    62,
      63,    64,    65,    66,    67,    -1,    69,    70,    -1,    72,
      73,    74,    75,    76,    77,    78,    79,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,   126,   127,   128,   129,    -1,   131,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     8,
       9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    36,    -1,    -1,
     173,    -1,   175,   176,   177,   178,    45,    46,    47,    48,
      49,    50,    51,    -1,    -1,    54,    55,    56,    57,    58,
      59,    60,    -1,    62,    63,    64,    65,    66,    67,    -1,
      69,    70,    -1,    72,    73,    74,    75,    76,    77,    78,
      79,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,    88,
      89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   121,   122,    -1,    -1,   125,   126,   127,   128,
     129,    -1,   131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     8,     9,    10,    -1,    -1,    -1,    14,
      -1,    -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    36,    -1,    -1,   173,    -1,   175,   176,   177,   178,
      45,    46,    47,    48,    49,    50,    51,    -1,    -1,    54,
      55,    56,    57,    58,    59,    60,    -1,    62,    63,    64,
      65,    66,    67,    -1,    69,    70,    -1,    72,    73,    74,
      75,    76,    77,    78,    79,    -1,    -1,    82,    -1,    -1,
      -1,    86,    87,    88,    89,    90,    91,    92,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,    -1,
     125,   126,   127,   128,   129,    -1,   131,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     8,     9,    10,
      -1,    -1,    -1,    14,    -1,    -1,    -1,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,    36,    -1,    -1,   173,    -1,
     175,   176,   177,   178,    45,    46,    47,    48,    49,    50,
      51,    -1,    -1,    54,    55,    56,    57,    58,    59,    60,
      -1,    62,    63,    64,    65,    66,    67,    -1,    69,    70,
      -1,    72,    73,    74,    75,    76,    77,    78,    79,    -1,
      -1,    82,    -1,    -1,    -1,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     121,   122,    -1,    -1,   125,   126,   127,   128,   129,    -1,
     131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,
      -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    36,
      -1,    -1,   173,    -1,   175,   176,   177,   178,    45,    46,
      47,    48,    49,    50,    51,    -1,    -1,    54,    55,    56,
      57,    58,    59,    60,    -1,    62,    63,    64,    65,    66,
      67,    -1,    69,    70,    -1,    72,    73,    74,    75,    76,
      77,    78,    79,    -1,    -1,    82,    -1,    -1,    -1,    86,
      87,    88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   121,   122,    -1,    -1,   125,   126,
     127,   128,   129,    -1,   131,    -1,    -1,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   173,    -1,   175,   176,
     177,   178,    50,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    66,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,   162,
     163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,
     173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,   162,
     163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,
     173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,   162,
     163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,
     173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,   162,
     163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,
     173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,   162,
     163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,
     173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,   162,
     163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,
     173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,   162,
     163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,
     173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,   162,
     163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,
     173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,   162,
     163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,
     173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     5,     6,     7,     8,     9,    10,    -1,    -1,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    62,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,     5,     6,     7,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,   162,
     163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,
     173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     8,     9,    10,    -1,    -1,    -1,    14,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,    -1,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,    -1,   175,    -1,    -1,
      -1,    -1,    58,    59,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   121,   122,    -1,    -1,   125,
      -1,    -1,   128,   129,     8,     9,    10,    -1,    -1,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    25,    26,    -1,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,    -1,    -1,    -1,   162,   163,    -1,    -1,
      -1,    -1,   168,   169,    -1,    -1,    -1,   173,    -1,   175,
      -1,   177,   178,    -1,    58,    59,    -1,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,
      -1,    -1,    86,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,
      -1,   125,    -1,    -1,   128,   129,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   162,   163,
      -1,    -1,    -1,    -1,   168,   169,     8,     9,    10,   173,
      -1,   175,    14,   177,   178,    -1,    18,    -1,    -1,    -1,
      22,    -1,    -1,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    36,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    45,    46,    47,    48,    49,    50,    51,
      -1,    -1,    54,    55,    56,    57,    58,    59,    60,    -1,
      62,    63,    64,    65,    66,    67,    -1,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    -1,    -1,
      82,    -1,    -1,    -1,    86,    87,    88,    89,    90,    91,
      92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,
     122,    -1,    -1,   125,   126,   127,   128,   129,    -1,   131,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     8,
       9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    36,    -1,    -1,
      -1,   173,    -1,   175,   176,   177,    45,    46,    47,    48,
      49,    50,    51,    -1,    -1,    54,    55,    56,    57,    58,
      59,    60,    -1,    62,    63,    64,    65,    66,    67,    -1,
      69,    70,    -1,    72,    73,    74,    75,    76,    77,    78,
      79,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,    88,
      89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   121,   122,    -1,    -1,   125,   126,   127,   128,
     129,    -1,   131,     8,     9,    10,    -1,    -1,    -1,    14,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      25,    26,    -1,    28,    29,    30,    31,    32,    33,    34,
      -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   173,    -1,   175,   176,   177,    -1,
      -1,    56,    -1,    58,    59,    60,    -1,    62,    -1,    -1,
      -1,    -1,    67,    -1,    69,    70,    -1,    72,    73,    74,
      75,    76,    77,    78,    79,    -1,    -1,    82,    -1,    -1,
      -1,    86,    87,    88,    89,    90,    91,    92,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,    -1,
     125,    -1,    -1,   128,   129,    -1,   131,     8,     9,    10,
      -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    -1,    36,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   173,    -1,
     175,   176,   177,    -1,    -1,    56,    -1,    58,    59,    60,
      -1,    62,    -1,    -1,    -1,    -1,    67,    -1,    69,    70,
      -1,    72,    73,    74,    75,    76,    77,    78,    79,    -1,
      -1,    82,    -1,    -1,    -1,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     121,   122,    -1,    -1,   125,    -1,    -1,   128,   129,    -1,
     131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       8,    -1,    -1,    -1,    -1,    -1,    14,    -1,    16,    -1,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      -1,    -1,   173,    -1,   175,   176,   177,    45,    46,    47,
      48,    49,    50,    51,    -1,    -1,    54,    55,    56,    57,
      58,    59,    -1,    -1,    -1,    63,    64,    65,    -1,    -1,
       8,     9,    10,    -1,    -1,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    -1,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      58,    59,    -1,    -1,    62,    -1,    -1,    -1,   126,   127,
     128,   129,    70,   131,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    -1,    -1,    -1,
      -1,   159,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,   176,   177,
      -1,    -1,    -1,   121,   122,    -1,    -1,   125,    -1,    -1,
     128,   129,     8,     9,    10,    -1,    -1,    -1,    14,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,    -1,   162,   163,    -1,    -1,    -1,    -1,
     168,   169,    -1,    -1,    -1,   173,   174,   175,    -1,   177,
      -1,    -1,    58,    59,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   121,   122,    -1,    -1,   125,
      -1,    -1,   128,   129,     8,     9,    10,    -1,    -1,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    25,    26,    -1,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,    -1,    -1,    -1,   162,   163,    -1,    -1,
      -1,    -1,   168,   169,    -1,    -1,    -1,   173,   174,   175,
      -1,   177,    -1,    -1,    58,    59,    -1,    -1,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,
      -1,    -1,    86,    87,    88,    89,    90,    91,    92,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,    -1,
      -1,   125,    -1,    -1,   128,   129,     8,     9,    10,    -1,
      -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    -1,    28,    29,    30,    31,
      32,    33,    34,    -1,    36,    -1,    -1,    -1,   162,   163,
      -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,   173,
     174,   175,    -1,   177,    -1,    -1,    58,    59,    -1,    -1,
      62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      82,    -1,    -1,    -1,    86,    87,    88,    89,    90,    91,
      92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,
     122,    -1,    -1,   125,    -1,    -1,   128,   129,     8,     9,
      10,    -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    25,    26,    -1,    28,    29,
      30,    31,    32,    33,    34,    -1,    36,    -1,    -1,    -1,
     162,   163,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,
      50,   173,    -1,   175,    -1,   177,    -1,    -1,    58,    59,
      -1,    -1,    62,    -1,    -1,    -1,    66,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    82,    -1,    -1,    -1,    86,    87,    88,    89,
      90,    91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     8,     9,    10,    -1,    -1,    -1,    14,
      -1,   121,   122,    -1,    -1,   125,    -1,    -1,   128,   129,
      25,    26,    -1,    28,    29,    30,    31,    32,    33,    34,
      -1,    36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    58,    59,    -1,    -1,    62,    -1,    -1,
      -1,    66,    -1,   173,    -1,   175,   176,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,
      -1,    86,    87,    88,    89,    90,    91,    92,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     8,     9,    10,    -1,   121,   122,    14,    -1,
     125,    -1,    -1,   128,   129,    -1,    -1,    -1,    -1,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    58,    59,    -1,    -1,    62,    -1,   173,    -1,
     175,   176,    -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      86,    87,    88,    89,    90,    91,    92,     8,    -1,    -1,
      -1,    -1,    -1,    14,    -1,    16,    -1,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,   121,   122,    -1,    -1,   125,
      -1,    -1,   128,   129,    45,    46,    47,    48,    49,    50,
      51,    -1,    -1,    54,    55,    56,    57,    58,    59,    -1,
      -1,    -1,    63,    64,    65,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   162,   163,    -1,    -1,
      -1,    -1,   168,   169,    -1,    -1,    -1,   173,    -1,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     8,     9,    10,    -1,    -1,    -1,    14,    -1,
      -1,    -1,    -1,    -1,    -1,   126,   127,   128,   129,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   159,    -1,
      -1,    -1,    58,    59,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     8,     9,    10,
      -1,    -1,    -1,    14,    -1,   121,   122,    -1,    -1,   125,
      -1,    -1,   128,   129,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    -1,    36,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    59,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    -1,   173,   174,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    82,    -1,    -1,    -1,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     8,     9,    10,    -1,    -1,    -1,    14,    -1,
     121,   122,    -1,    -1,   125,    -1,    -1,   128,   129,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    58,    59,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,   173,   174,   175,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     8,     9,    10,
      -1,    -1,    -1,    14,    -1,   121,   122,    -1,    -1,   125,
      -1,    -1,   128,   129,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    -1,    36,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    59,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    -1,   173,   174,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    82,    -1,    -1,    -1,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     8,     9,    10,    -1,    -1,    -1,    14,    -1,
     121,   122,    -1,    -1,   125,    -1,    -1,   128,   129,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    58,    59,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,   173,   174,   175,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     8,     9,    10,
      -1,    -1,    -1,    14,    -1,   121,   122,    -1,    -1,   125,
      -1,    -1,   128,   129,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    -1,    36,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    59,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    -1,   173,   174,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    82,    -1,    -1,    -1,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     8,     9,    10,    -1,    -1,    -1,    14,    -1,
     121,   122,    -1,    -1,   125,    -1,    -1,   128,   129,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    58,    59,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,   173,   174,   175,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     8,     9,    10,
      -1,    -1,    -1,    14,    -1,   121,   122,    -1,    -1,   125,
      -1,    -1,   128,   129,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,    -1,    36,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    59,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    -1,   173,   174,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    82,    -1,    -1,    -1,    86,    87,    88,    89,    90,
      91,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     8,     9,    10,    -1,    -1,    -1,    14,    -1,
     121,   122,    -1,    -1,   125,    -1,    -1,   128,   129,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    58,    59,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,   173,   174,   175,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    82,    -1,    -1,    -1,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    14,    -1,    16,    -1,    18,    19,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,   121,   122,    -1,    -1,   125,
      -1,    -1,   128,   129,    45,    46,    47,    48,    49,    50,
      51,    -1,    -1,    54,    55,    56,    57,    58,    59,    -1,
      -1,    -1,    63,    64,    65,    -1,    14,    -1,    16,    -1,
      18,    19,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,   173,    -1,   175,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    -1,    -1,    54,    55,    56,    57,
      58,    59,    -1,    -1,    -1,    63,    64,    65,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   126,   127,   128,   129,    -1,
     131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   159,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   175,   176,    -1,   178,   126,   127,
     128,   129,    14,   131,    -1,    -1,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   159,    -1,    45,    46,    47,    48,    49,    50,    51,
      -1,    -1,    54,    55,    56,    57,    58,   175,   176,    -1,
     178,    63,    64,    65,    -1,    14,    -1,    -1,    -1,    18,
      19,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,    48,
      49,    50,    51,    -1,    -1,    54,    55,    56,    57,    58,
      -1,    -1,    -1,    -1,    63,    64,    65,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   126,   127,   128,   129,    -1,   131,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    14,    -1,    16,    -1,    18,    19,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,   175,   176,    -1,   178,   126,   127,   128,
     129,    -1,   131,    45,    46,    47,    48,    49,    50,    51,
      -1,    -1,    54,    55,    56,    57,    58,    59,    -1,    -1,
      -1,    63,    64,    65,    14,    -1,    -1,    -1,    18,    19,
      -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,   175,   176,    -1,   178,
      -1,    -1,    -1,    -1,    -1,    45,    46,    47,    48,    49,
      50,    51,    18,    19,    54,    55,    56,    57,    58,    -1,
      -1,    27,    -1,    63,    64,    65,    -1,    -1,    -1,    -1,
      -1,    37,    -1,    -1,   126,   127,   128,   129,    44,    45,
      46,    47,    48,    49,    50,    51,    -1,    -1,    54,    55,
      56,    57,    -1,    -1,    -1,    -1,    -1,    63,    64,    65,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   159,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    83,    -1,    -1,
      -1,    -1,    -1,   175,    -1,    -1,   126,   127,   128,   129,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     126,   127,    -1,    -1,    -1,   131,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   175,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   175,
     176
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,    18,    19,    27,    37,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    54,    55,    56,    57,    63,    64,
      65,    83,   126,   127,   131,   175,   176,   184,   185,   186,
     187,   188,   189,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   235,   236,   281,   286,
     287,   295,   301,   302,   303,   309,   310,     8,    14,    25,
      26,    64,   128,   129,   180,   190,   215,   217,    64,    14,
      83,    14,   217,    49,   215,   217,    15,    14,   217,    64,
     190,   215,    18,    19,    27,    63,    64,   175,   184,     0,
     175,   222,   228,   223,   228,   230,   302,   229,    37,    43,
      83,   175,   301,    14,   173,    24,    14,    16,   159,   237,
     241,   217,    24,   176,   217,   176,    24,   167,    53,   177,
     244,   282,   237,   177,    14,   173,    64,    64,    14,    15,
      14,    64,   228,    83,   217,   217,   177,   296,     8,     9,
      10,    14,    28,    29,    30,    31,    32,    33,    34,    36,
      58,    59,    62,    70,    82,    86,    87,    88,    89,    90,
      91,    92,   121,   122,   125,   162,   163,   168,   169,   173,
     174,   177,   180,   181,   183,   192,   193,   194,   195,   196,
     197,   217,   257,   259,   260,   301,   302,   304,   305,   306,
     307,   372,   373,   374,   376,   377,   381,   382,   383,   385,
     386,   403,   404,   405,   406,   407,   408,   409,   410,   411,
     412,   413,   414,   415,   416,   417,   418,   419,   420,   421,
     422,   423,     8,    14,   180,   296,    14,   204,   242,   302,
     242,    35,   238,   243,   177,    14,   167,   176,   164,    14,
     196,   217,   245,   259,   302,     8,    14,   176,   178,   180,
     278,   283,   284,   285,   302,   282,    35,   288,   289,    38,
      39,    40,    41,    42,   178,   231,   232,   296,   174,   304,
     306,    14,    14,   237,   244,   282,   237,    14,   217,   176,
     177,    58,   131,   176,   178,   185,   192,   235,   256,   257,
     258,   259,   260,   286,   293,   297,   298,   299,    16,   153,
     208,   103,   167,    20,   167,   375,   167,   173,    11,    12,
      13,    16,   159,   182,   191,   196,   207,   257,   302,   378,
     379,   415,   415,   415,   415,   415,   415,     5,     6,     7,
     173,   191,   195,   217,   257,   302,   381,   382,   389,   390,
     391,   394,   399,   400,   401,   403,   178,   306,   308,   153,
     173,   167,   375,   103,    21,    24,   167,   375,    17,   170,
     198,   199,   200,   201,   202,   203,    23,   198,   257,   259,
     152,   174,   174,   103,   167,    21,    98,   154,    97,   156,
     157,   158,    95,    96,    93,    94,   115,   159,   160,   116,
     117,   118,   162,   163,   164,   165,   166,   119,   120,    35,
     205,   152,   160,    14,   160,   196,   239,   244,   178,   231,
     164,   176,   167,   152,   259,   177,   247,    49,    59,   131,
     176,   177,   185,   235,   241,   248,   249,   250,   251,   256,
     261,   262,   263,   274,   275,   276,   277,   278,   286,   311,
     173,   247,   173,   152,   178,   285,   178,    14,   180,   278,
     245,   130,   240,   246,   217,    14,    49,    52,   180,   213,
     217,   215,   217,   215,   178,   232,   174,   174,   296,   296,
     238,   282,   289,   296,   177,   178,   231,   256,    14,   180,
     181,   252,   253,   254,   178,   298,   154,   195,   209,   210,
     211,   302,   306,    14,   180,   207,    14,   180,   181,   207,
     171,    27,    20,   167,    27,   389,   209,   209,    21,   198,
     200,   387,   388,   198,   387,   302,   378,   198,   387,   182,
     257,   173,   102,   167,   207,   380,     5,    50,    66,   174,
     256,   257,   259,   260,   268,   269,   302,   392,   393,   394,
     395,   396,   397,   174,   206,   174,   257,   174,   101,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     153,   402,   152,   178,   306,   174,   384,   389,    27,   167,
      14,    82,   180,   207,   389,    14,    27,    36,    62,    82,
     181,   207,   302,   377,   167,   190,   215,   171,   170,   199,
      14,   302,   198,   198,    14,   180,   305,    14,   180,   207,
      14,   180,   181,   207,   377,   389,   405,   389,   406,   407,
     408,   409,   411,   411,   412,   412,    66,   195,   256,   269,
     302,   316,   369,   370,   371,   412,   412,   413,   413,   413,
     414,   414,   415,   415,   415,   196,   204,   205,   240,   178,
     176,   196,   178,   248,   311,    14,    18,    50,    56,    60,
      67,    69,    70,    72,    73,    74,    75,    76,    77,    78,
      79,   131,   173,   176,   178,   180,   185,   235,   269,   287,
     302,   311,   312,   313,   314,   315,   316,   317,   318,   320,
     321,   322,   324,   325,   326,   327,   329,   330,   340,   342,
     343,   345,   350,   352,   353,   354,   355,   356,   357,   358,
     364,   376,   383,   400,   416,   417,   419,   420,   421,   256,
     262,   277,   256,   263,   278,   302,   249,   252,   176,   273,
     311,   181,   264,    61,   177,   270,   279,   173,   384,   384,
     178,   284,   285,   178,   247,   173,   247,   173,   215,   216,
     177,   290,    99,   176,   213,   213,    24,   176,    99,   176,
     176,   100,   239,   240,   178,   231,   178,   181,   252,   198,
     198,   173,   152,   176,   153,    35,    36,   212,   152,   160,
     154,    14,   180,   173,   181,   171,    27,   174,   160,   160,
     389,   177,   233,    21,   198,   388,   233,   198,   173,   233,
     198,   174,   384,   182,   302,   302,   123,   302,   256,   397,
      50,   256,   257,   259,   397,   174,   152,   152,    14,   180,
     415,     3,     4,   415,   311,   389,   398,   389,   178,   306,
     152,   174,    27,    14,    82,   180,   171,   208,   103,   167,
     173,   181,    14,    27,   173,   198,   171,   208,    14,    14,
     180,   173,   181,   171,   155,   316,   257,   316,   253,   206,
     247,   178,   155,   173,   389,   173,   389,   173,    56,   318,
     173,   173,    14,   176,   180,   389,    14,   176,   180,   176,
     389,   173,   311,   365,   155,   316,   316,   178,   313,   176,
     252,   176,   252,   273,   270,   279,   264,   173,   263,   176,
     173,   270,   196,   271,   272,    36,    62,   178,   207,   217,
     280,   312,   372,   279,   174,   256,   265,   266,   267,   268,
     269,   302,   174,   174,   178,   384,   384,   152,   131,   176,
     178,   185,   235,   241,   256,   262,   286,   291,   292,   293,
     294,   213,   214,   176,   176,    14,   180,   214,   216,   240,
     290,   178,   173,   176,   174,   253,   233,   255,   389,   195,
     195,   210,   212,   174,   384,   173,   177,   331,   171,   178,
     234,   255,    21,   389,    21,   174,   384,    21,   247,   174,
     182,    14,   180,   123,   123,   302,    14,   180,   302,   123,
     302,   198,   198,    14,   180,     5,   394,   302,   396,   198,
     198,   196,   390,   418,   389,    14,   180,   207,    14,   180,
     181,   207,   174,   384,   173,   208,   174,   304,   306,   198,
     208,   174,   384,   173,   390,   403,   253,   253,   318,   389,
     176,   389,   155,   176,   389,    73,   389,   176,   269,   302,
     315,   316,   325,   347,   349,   176,   176,   176,   176,   176,
     176,   217,   269,   302,   316,   366,   367,   368,   372,   381,
      80,    81,   359,   360,   363,   311,   318,   252,   252,   176,
     279,   270,   174,   265,   266,   264,   174,   265,   266,   152,
     173,   173,    36,    62,   167,   178,   312,   178,   167,    14,
      62,   180,   152,   174,   174,   152,   256,   256,   247,   247,
     174,   174,   215,   256,   262,   273,   178,   292,   152,   176,
     176,   176,   247,   174,    65,   176,   198,   300,   174,   174,
     384,    22,    71,   178,   332,   333,   334,   335,   336,   337,
     152,   178,   389,   171,   389,   247,   174,   389,   247,    14,
     180,    14,   180,   123,   198,   198,    14,   180,   123,   198,
     198,    14,   180,   173,   181,   174,   174,   384,   174,   174,
     174,   174,   384,   174,   174,   389,   174,   173,   174,   176,
     389,   316,   316,    14,   180,   176,   152,   316,   316,    14,
     180,   174,   176,   173,   311,   360,   363,   359,   363,   152,
     174,   174,   270,   198,   152,   174,   174,   272,   174,   384,
     174,   384,   173,   173,    36,   207,   178,    36,   207,    24,
     167,   198,   167,   198,   256,   266,   302,   266,    14,   180,
      14,    62,   180,   247,   247,   273,   213,   176,   198,   300,
     306,   176,   300,   176,   174,   338,   339,   403,   178,   332,
     336,   178,   334,   336,   101,   155,   312,   336,   178,   255,
     171,   171,   247,   171,    14,   180,    14,   180,   174,   384,
     173,   174,   174,   311,    14,    67,    73,    74,   180,   318,
     319,   320,   323,   328,   341,   344,   346,   351,   176,   331,
     389,   318,   174,   348,   349,   176,    14,   180,    14,   180,
     155,   198,   155,   198,   176,   389,   325,    14,   180,    14,
     180,   153,   153,   174,   367,   259,   269,   302,   361,   362,
     363,   266,   266,   198,   198,   176,   174,   176,   174,   174,
     384,   174,   384,   173,    36,   173,    36,    14,    14,    62,
      62,    14,   180,   174,   256,   198,   198,   167,   198,   167,
     198,   176,   300,   176,   176,   152,   311,   356,   389,   155,
     174,   174,   384,   155,   173,   173,   173,   155,    68,   174,
     318,   174,   174,   348,   155,   198,   155,   198,   155,   198,
     155,   198,   389,   155,   389,   155,   174,   348,   176,   153,
     153,   153,   153,   389,   389,   362,   362,   174,    14,   156,
     180,   174,   174,   176,   176,   176,   174,   176,   174,   174,
     384,   173,   174,   384,   173,   167,    14,   180,    62,    62,
     176,   338,   176,   174,   319,   389,   389,   176,   269,   302,
     316,   347,   319,   318,   176,   318,   318,   174,   389,   155,
     389,   155,   389,   155,   389,   155,   174,   389,   174,   389,
     318,   174,   174,   348,   389,   389,   389,   389,    14,   180,
      14,   180,   311,   198,   259,   198,   198,   176,   176,   176,
     174,   174,   384,   176,   174,   174,   384,   174,   174,   176,
     389,   316,   316,    14,   180,   176,   318,   174,   389,   174,
     389,   174,   389,   174,   389,   318,   174,   318,   174,   318,
     318,   174,   198,   198,   198,   198,   176,   176,   174,   176,
     176,   174,   319,   319,   174,   348,   176,    14,   180,    14,
     180,   155,   198,   155,   198,   176,   389,   318,   174,   318,
     174,   318,   174,   318,   174,   318,   318,   318,   176,   176,
      68,   319,   174,   174,   348,   155,   198,   155,   198,   155,
     198,   155,   198,   389,   155,   389,   155,   174,   348,   176,
     318,   318,   318,   318,   319,   319,   319,   174,   389,   155,
     389,   155,   389,   155,   389,   155,   174,   389,   174,   389,
     319,   174,   174,   348,   319,   174,   389,   174,   389,   174,
     389,   174,   389,   319,   174,   319,   174,   319,   319,   174,
     319,   174,   319,   174,   319,   174,   319,   174,   319,   319,
     319,   319,   319,   319,   319
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   179,   180,   180,   180,   180,   181,   181,   181,   182,
     182,   182,   183,   183,   183,   183,   183,   183,   183,   183,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   185,   185,   186,   186,   186,
     187,   187,   188,   189,   190,   190,   191,   192,   192,   193,
     193,   193,   193,   193,   194,   194,   195,   195,   196,   196,
     197,   197,   197,   198,   198,   198,   198,   199,   199,   199,
     200,   200,   201,   201,   202,   203,   204,   204,   204,   204,
     205,   206,   206,   207,   207,   208,   209,   209,   210,   210,
     211,   211,   211,   211,   212,   212,   213,   213,   213,   213,
     214,   214,   215,   215,   216,   216,   217,   217,   217,   217,
     218,   218,   219,   219,   219,   219,   220,   220,   221,   221,
     222,   222,   223,   223,   223,   223,   224,   225,   226,   227,
     228,   228,   229,   229,   229,   229,   230,   230,   230,   230,
     230,   230,   230,   230,   231,   231,   232,   232,   232,   232,
     232,   232,   232,   232,   232,   233,   233,   233,   234,   234,
     235,   235,   236,   236,   237,   237,   238,   238,   239,   239,
     240,   240,   241,   241,   242,   242,   243,   244,   245,   245,
     246,   247,   247,   248,   248,   249,   249,   249,   249,   250,
     250,   250,   250,   250,   250,   251,   251,   252,   252,   253,
     253,   254,   254,   254,   254,   255,   255,   256,   256,   257,
     257,   258,   258,   259,   259,   259,   259,   259,   259,   259,
     259,   260,   260,   261,   261,   262,   262,   262,   262,   262,
     262,   263,   263,   264,   264,   264,   264,   264,   264,   264,
     264,   265,   265,   265,   265,   265,   265,   266,   266,   267,
     267,   267,   267,   267,   267,   267,   267,   267,   267,   267,
     267,   267,   267,   267,   268,   268,   268,   268,   268,   268,
     268,   268,   268,   268,   268,   268,   269,   269,   269,   269,
     270,   271,   271,   272,   273,   273,   274,   275,   276,   276,
     276,   276,   277,   277,   277,   277,   277,   277,   277,   277,
     278,   279,   279,   279,   279,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   281,   281,   281,   281,   282,   282,   282,   282,   282,
     282,   283,   283,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   285,
     286,   286,   287,   287,   288,   289,   289,   290,   290,   291,
     291,   292,   292,   292,   292,   292,   292,   293,   293,   294,
     294,   295,   295,   295,   295,   295,   295,   296,   296,   297,
     297,   298,   298,   298,   298,   298,   298,   299,   299,   299,
     299,   299,   299,   299,   299,   300,   301,   301,   301,   302,
     302,   303,   303,   304,   304,   305,   305,   306,   306,   306,
     307,   307,   307,   308,   308,   309,   310,   311,   311,   312,
     312,   313,   313,   313,   314,   315,   315,   315,   316,   316,
     317,   317,   318,   318,   318,   318,   318,   318,   319,   319,
     319,   319,   319,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   321,   321,   322,   322,
     323,   323,   324,   325,   325,   325,   325,   325,   325,   325,
     326,   327,   328,   329,   329,   330,   331,   331,   331,   332,
     332,   332,   333,   333,   334,   335,   335,   336,   336,   337,
     337,   338,   339,   339,   340,   341,   342,   343,   343,   344,
     344,   345,   345,   345,   345,   345,   345,   345,   345,   346,
     346,   346,   346,   346,   346,   346,   346,   347,   347,   348,
     349,   349,   350,   350,   350,   350,   350,   350,   350,   350,
     350,   350,   350,   350,   351,   351,   351,   351,   351,   351,
     351,   351,   351,   351,   351,   351,   352,   352,   352,   353,
     354,   354,   354,   355,   355,   356,   357,   358,   358,   358,
     358,   359,   359,   360,   361,   361,   361,   361,   361,   361,
     361,   361,   361,   361,   361,   361,   362,   362,   363,   364,
     364,   364,   364,   365,   365,   366,   366,   367,   367,   367,
     367,   367,   367,   367,   368,   368,   369,   370,   371,   371,
     371,   372,   372,   373,   373,   373,   373,   373,   373,   373,
     373,   373,   373,   374,   374,   374,   374,   374,   374,   374,
     375,   375,   376,   376,   376,   377,   377,   377,   377,   377,
     377,   377,   377,   378,   378,   379,   379,   379,   379,   380,
     380,   381,   381,   381,   381,   381,   381,   382,   382,   382,
     382,   382,   383,   383,   383,   383,   383,   383,   383,   383,
     383,   383,   383,   383,   383,   383,   383,   383,   383,   383,
     384,   384,   385,   385,   385,   385,   385,   385,   385,   385,
     385,   385,   385,   385,   385,   385,   385,   385,   385,   385,
     386,   386,   386,   386,   386,   386,   386,   386,   386,   387,
     387,   388,   388,   389,   389,   390,   391,   391,   391,   391,
     392,   392,   393,   393,   393,   393,   394,   394,   395,   395,
     396,   396,   396,   396,   396,   396,   396,   396,   396,   396,
     396,   396,   396,   397,   397,   398,   398,   399,   399,   400,
     401,   401,   401,   402,   402,   402,   402,   402,   402,   402,
     402,   402,   402,   402,   402,   403,   403,   403,   404,   404,
     405,   405,   406,   406,   407,   407,   408,   408,   409,   409,
     409,   410,   411,   411,   411,   411,   411,   411,   411,   412,
     412,   412,   412,   413,   413,   413,   414,   414,   414,   414,
     415,   415,   415,   415,   415,   416,   417,   418,   418,   418,
     418,   418,   419,   419,   419,   419,   420,   421,   422,   422,
     422,   422,   423
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     1,     1,     1,
       5,     4,     2,     5,     1,     3,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       3,     3,     1,     3,     4,     2,     3,     1,     1,     1,
       2,     1,     5,     4,     2,     5,     2,     3,     1,     2,
       3,     3,     0,     3,     3,     3,     1,     3,     1,     1,
       3,     2,     2,     1,     2,     2,     1,     1,     3,     3,
       1,     3,     1,     3,     1,     3,     1,     1,     3,     3,
       1,     1,     3,     2,     2,     1,     2,     1,     4,     3,
       1,     2,     1,     1,     1,     1,     3,     5,     4,     6,
       1,     2,     1,     1,     1,     1,     7,     6,     6,     5,
       6,     5,     5,     4,     1,     2,     4,     3,     4,     3,
       5,     3,     5,     3,     5,     3,     4,     2,     1,     3,
       1,     1,     8,     7,     1,     0,     1,     0,     1,     0,
       1,     0,     3,     3,     1,     3,     2,     2,     1,     3,
       2,     3,     2,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     4,     3,     1,     3,     3,
       1,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     1,     4,     5,     4,     4,     4,
       5,     2,     2,     3,     2,     3,     2,     5,     4,     4,
       3,     1,     1,     7,     5,     5,     4,     6,     4,     4,
       3,     5,     5,     3,     4,     4,     2,     1,     3,     4,
       4,     3,     3,     3,     2,     4,     4,     3,     3,     3,
       2,     1,     4,     4,     5,     4,     5,     4,     4,     3,
       5,     4,     5,     4,     4,     3,     1,     2,     2,     3,
       2,     1,     3,     1,     1,     1,     1,     2,     4,     3,
       3,     2,     7,     5,     5,     4,     6,     4,     4,     3,
       1,     4,     3,     3,     2,     6,     5,     5,     4,     6,
       5,     5,     4,     8,     7,     7,     6,     8,     7,     7,
       6,     5,     4,     4,     3,     5,     4,     3,     4,     3,
       2,     1,     3,     6,     5,     3,     2,     5,     4,     2,
       1,     6,     5,     3,     2,     5,     4,     2,     1,     2,
       1,     1,     7,     6,     2,     1,     0,     3,     2,     1,
       2,     1,     1,     1,     1,     1,     1,     4,     3,     3,
       2,     5,     4,     5,     4,     5,     4,     3,     2,     1,
       2,     1,     1,     1,     1,     1,     1,     8,     7,     7,
       6,     7,     6,     6,     5,     2,     1,     1,     1,     2,
       1,     5,     4,     1,     3,     3,     3,     1,     1,     1,
       4,     3,     2,     1,     3,     2,     5,     3,     2,     1,
       2,     1,     1,     1,     2,     3,     3,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       3,     3,     2,     1,     1,     1,     1,     1,     1,     1,
       5,     7,     7,     3,     5,     5,     3,     3,     2,     4,
       3,     3,     1,     2,     2,     1,     2,     2,     1,     2,
       3,     1,     1,     3,     5,     5,     7,     1,     1,     1,
       1,     9,     8,     8,     7,     8,     7,     7,     6,     9,
       8,     8,     7,     8,     7,     7,     6,     1,     1,     1,
       1,     3,    10,    10,     9,     9,     9,     8,    10,    10,
       9,     9,     9,     8,    10,    10,     9,     9,     9,     8,
      10,    10,     9,     9,     9,     8,     3,     3,     2,     3,
       2,     3,     3,     3,     2,     3,     5,     3,     4,     3,
       1,     1,     2,     5,     4,     4,     3,     3,     3,     2,
       4,     4,     3,     3,     3,     2,     1,     3,     2,     5,
       4,     4,     3,     3,     4,     1,     3,     5,     5,     4,
       5,     5,     4,     1,     1,     1,     1,     1,     3,     3,
       2,     1,     1,     1,     1,     1,     3,     3,     1,     1,
       1,     1,     1,     4,     4,     4,     3,     3,     3,     3,
       2,     3,     1,     3,     3,     7,     6,     6,     5,     6,
       5,     5,     4,     2,     1,     2,     1,     4,     3,     1,
       1,     3,     3,     5,     3,     3,     5,     4,     4,     7,
       7,     7,     4,     3,     7,     6,     6,     5,     7,     6,
       6,     5,     7,     6,     6,     5,     9,     8,     8,     7,
       1,     3,     4,     3,     4,     3,     4,     3,     6,     5,
       4,     3,     4,     3,     4,     3,     6,     5,     4,     3,
       4,     3,     4,     3,     4,     3,     4,     4,     4,     1,
       2,     4,     3,     1,     1,     3,     3,     2,     1,     1,
       1,     1,     1,     1,     3,     3,     1,     1,     1,     3,
       4,     4,     3,     3,     3,     2,     4,     4,     3,     3,
       3,     2,     1,     1,     1,     1,     1,     1,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     5,     5,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       3,     1,     1,     3,     3,     3,     3,     3,     3,     1,
       3,     3,     3,     1,     3,     3,     1,     3,     3,     3,
       1,     1,     2,     2,     1,     2,     2,     1,     2,     2,
       1,     1,     1,     1,     1,     1,     2,     2,     4,     4,
       5,     5,     5
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* IdentifierComplement: VAR_IdentifierComplement  */
#line 1128 "parser.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 6213 "parser.tab.c"
    break;

  case 3: /* IdentifierComplement: SEALED_IdentifierComplement  */
#line 1129 "parser.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 6219 "parser.tab.c"
    break;

  case 4: /* IdentifierComplement: NON_SEALED_IdentifierComplement  */
#line 1130 "parser.y"
                                      { (yyval.node) = (yyvsp[0].node); }
#line 6225 "parser.tab.c"
    break;

  case 5: /* IdentifierComplement: YIELD_IdentifierComplement  */
#line 1131 "parser.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 6231 "parser.tab.c"
    break;

  case 6: /* IDENTIFIER_MethodDeclarator: TYPE_IDENTIFIER_MethodDeclarator  */
#line 1136 "parser.y"
                                     { (yyval.node) = (yyvsp[0].node); }
#line 6237 "parser.tab.c"
    break;

  case 7: /* IDENTIFIER_MethodDeclarator: VAR_MethodDeclarator  */
#line 1137 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 6243 "parser.tab.c"
    break;

  case 8: /* IDENTIFIER_MethodDeclarator: YIELD_MethodDeclarator  */
#line 1138 "parser.y"
                             { (yyval.node) = (yyvsp[0].node); }
#line 6249 "parser.tab.c"
    break;

  case 9: /* IDENTIFIER_AnnotationIdentifiers: TYPE_IDENTIFIER_AnnotationIdentifiers  */
#line 1143 "parser.y"
                                          { (yyval.node) = (yyvsp[0].node); }
#line 6255 "parser.tab.c"
    break;

  case 10: /* IDENTIFIER_AnnotationIdentifiers: VAR_AnnotationIdentifiers  */
#line 1144 "parser.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 6261 "parser.tab.c"
    break;

  case 11: /* IDENTIFIER_AnnotationIdentifiers: YIELD_AnnotationIdentifiers  */
#line 1145 "parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 6267 "parser.tab.c"
    break;

  case 12: /* Literal: INTEGERLITERAL  */
#line 1151 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 6273 "parser.tab.c"
    break;

  case 13: /* Literal: FLOATINGPOINTLITERAL  */
#line 1152 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 6279 "parser.tab.c"
    break;

  case 14: /* Literal: TRUE  */
#line 1153 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 6285 "parser.tab.c"
    break;

  case 15: /* Literal: FALSE  */
#line 1154 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 6291 "parser.tab.c"
    break;

  case 16: /* Literal: CHARACTERLITERAL  */
#line 1155 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 6297 "parser.tab.c"
    break;

  case 17: /* Literal: STRINGLITERAL  */
#line 1156 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 6303 "parser.tab.c"
    break;

  case 18: /* Literal: TextBlock  */
#line 1157 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 6309 "parser.tab.c"
    break;

  case 19: /* Literal: MY_NULL  */
#line 1158 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 6315 "parser.tab.c"
    break;

  case 20: /* Modifier: Annotation_Modifier  */
#line 1163 "parser.y"
                        { (yyval.node) = (yyvsp[0].node); }
#line 6321 "parser.tab.c"
    break;

  case 21: /* Modifier: PUBLIC  */
#line 1165 "parser.y"
             { (yyval.node) = make_keyword_leaf("public", (yylsp[0])); }
#line 6327 "parser.tab.c"
    break;

  case 22: /* Modifier: PROTECTED  */
#line 1166 "parser.y"
                { (yyval.node) = make_keyword_leaf("protected", (yylsp[0])); }
#line 6333 "parser.tab.c"
    break;

  case 23: /* Modifier: PRIVATE  */
#line 1167 "parser.y"
              { (yyval.node) = make_keyword_leaf("private", (yylsp[0])); }
#line 6339 "parser.tab.c"
    break;

  case 24: /* Modifier: ABSTRACT  */
#line 1168 "parser.y"
               { (yyval.node) = make_keyword_leaf("abstract", (yylsp[0])); }
#line 6345 "parser.tab.c"
    break;

  case 25: /* Modifier: STATIC  */
#line 1169 "parser.y"
             { (yyval.node) = make_keyword_leaf("static", (yylsp[0])); }
#line 6351 "parser.tab.c"
    break;

  case 26: /* Modifier: FINAL  */
#line 1170 "parser.y"
            { (yyval.node) = make_keyword_leaf("final", (yylsp[0])); }
#line 6357 "parser.tab.c"
    break;

  case 27: /* Modifier: STRICTFP  */
#line 1171 "parser.y"
               { (yyval.node) = make_keyword_leaf("strictfp", (yylsp[0])); }
#line 6363 "parser.tab.c"
    break;

  case 28: /* Modifier: TRANSIENT  */
#line 1172 "parser.y"
                { (yyval.node) = make_keyword_leaf("transient", (yylsp[0])); }
#line 6369 "parser.tab.c"
    break;

  case 29: /* Modifier: VOLATILE  */
#line 1173 "parser.y"
               { (yyval.node) = make_keyword_leaf("volatile", (yylsp[0])); }
#line 6375 "parser.tab.c"
    break;

  case 30: /* Modifier: SYNCHRONIZED  */
#line 1174 "parser.y"
                   { (yyval.node) = make_keyword_leaf("synchronized", (yylsp[0])); }
#line 6381 "parser.tab.c"
    break;

  case 31: /* Modifier: NATIVE  */
#line 1175 "parser.y"
             { (yyval.node) = make_keyword_leaf("native", (yylsp[0])); }
#line 6387 "parser.tab.c"
    break;

  case 32: /* Modifier: DEFAULT  */
#line 1176 "parser.y"
              { (yyval.node) = make_keyword_leaf("default", (yylsp[0])); }
#line 6393 "parser.tab.c"
    break;

  case 33: /* Modifier: SEALED  */
#line 1177 "parser.y"
             { (yyval.node) = make_keyword_leaf("sealed", (yylsp[0])); }
#line 6399 "parser.tab.c"
    break;

  case 34: /* Modifier: NON_SEALED  */
#line 1178 "parser.y"
                 { (yyval.node) = make_keyword_leaf("non-sealed", (yylsp[0])); }
#line 6405 "parser.tab.c"
    break;

  case 35: /* Modifiers: Modifier  */
#line 1182 "parser.y"
             {
        (yyval.node) = make_list_node(AST_MODIFIER_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 6414 "parser.tab.c"
    break;

  case 36: /* Modifiers: Modifiers Modifier  */
#line 1186 "parser.y"
                         {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 6423 "parser.tab.c"
    break;

  case 37: /* Annotation_Modifier: NormalAnnotation_Modifier  */
#line 1195 "parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 6429 "parser.tab.c"
    break;

  case 38: /* Annotation_Modifier: MarkerAnnotation_Modifier  */
#line 1196 "parser.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 6435 "parser.tab.c"
    break;

  case 39: /* Annotation_Modifier: SingleElementAnnotation_Modifier  */
#line 1197 "parser.y"
                                       { (yyval.node) = (yyvsp[0].node); }
#line 6441 "parser.tab.c"
    break;

  case 40: /* NormalAnnotation_Modifier: AT_Modifier TypeName_ModifierOrDims '(' ElementValuePairList ')'  */
#line 1202 "parser.y"
                                                                     {
        AstNode *node = make_annotation_node((yyvsp[-3].node), (yylsp[-4]));
        ast_add_child(node, (yyvsp[-1].node));
        AstNode *list = make_list_node(AST_ANNOTATION_LIST, (yyloc));
        ast_add_child(list, node);
        (yyval.node) = list;
    }
#line 6453 "parser.tab.c"
    break;

  case 41: /* NormalAnnotation_Modifier: AT_Modifier TypeName_ModifierOrDims '(' ')'  */
#line 1209 "parser.y"
                                                  {
        AstNode *node = make_annotation_node((yyvsp[-2].node), (yylsp[-3]));
        AstNode *list = make_list_node(AST_ANNOTATION_LIST, (yyloc));
        ast_add_child(list, node);
        (yyval.node) = list;
    }
#line 6464 "parser.tab.c"
    break;

  case 42: /* MarkerAnnotation_Modifier: AT_Modifier TypeName  */
#line 1219 "parser.y"
                         {
        AstNode *node = make_annotation_node((yyvsp[0].node), (yylsp[-1]));
        AstNode *list = make_list_node(AST_ANNOTATION_LIST, (yyloc));
        ast_add_child(list, node);
        (yyval.node) = list;
    }
#line 6475 "parser.tab.c"
    break;

  case 43: /* SingleElementAnnotation_Modifier: AT_Modifier TypeName_ModifierOrDims '(' ElementValue ')'  */
#line 1229 "parser.y"
                                                             {
        AstNode *node = make_annotation_node((yyvsp[-3].node), (yylsp[-4]));
        AstNode *args = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child(args, (yyvsp[-1].node));
        ast_add_child(node, args);
        AstNode *list = make_list_node(AST_ANNOTATION_LIST, (yyloc));
        ast_add_child(list, node);
        (yyval.node) = list;
    }
#line 6489 "parser.tab.c"
    break;

  case 44: /* TypeName_ModifierOrDims: TYPE_IDENTIFIER_MethodDeclarator  */
#line 1242 "parser.y"
                                     { (yyval.node) = (yyvsp[0].node); }
#line 6495 "parser.tab.c"
    break;

  case 45: /* TypeName_ModifierOrDims: CommonName DOT_CommonName TYPE_IDENTIFIER_MethodDeclarator  */
#line 1243 "parser.y"
                                                               {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        node->scope = (yyvsp[-2].node);
        ast_add_child(node, (yyvsp[-2].node));
        ast_add_child(node, (yyvsp[0].node));
        (yyval.node) = node;
    }
#line 6507 "parser.tab.c"
    break;

  case 46: /* PrimitiveType: Annotations UnannPrimitiveType  */
#line 1268 "parser.y"
                                   {
        AstNode *type = make_type_node((yyvsp[0].node), (yyloc));
        if ((yyvsp[-1].node)) {
            ast_prepend_child(type, (yyvsp[-1].node));
        }
        (yyval.node) = type;
    }
#line 6519 "parser.tab.c"
    break;

  case 47: /* NumericType: IntegralType  */
#line 1281 "parser.y"
                 { (yyval.node) = (yyvsp[0].node); }
#line 6525 "parser.tab.c"
    break;

  case 48: /* NumericType: FloatingPointType  */
#line 1282 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 6531 "parser.tab.c"
    break;

  case 49: /* IntegralType: BYTE  */
#line 1287 "parser.y"
         { (yyval.node) = make_keyword_leaf("byte", (yylsp[0])); }
#line 6537 "parser.tab.c"
    break;

  case 50: /* IntegralType: SHORT  */
#line 1288 "parser.y"
          { (yyval.node) = make_keyword_leaf("short", (yylsp[0])); }
#line 6543 "parser.tab.c"
    break;

  case 51: /* IntegralType: INT  */
#line 1289 "parser.y"
        { (yyval.node) = make_keyword_leaf("int", (yylsp[0])); }
#line 6549 "parser.tab.c"
    break;

  case 52: /* IntegralType: LONG  */
#line 1290 "parser.y"
         { (yyval.node) = make_keyword_leaf("long", (yylsp[0])); }
#line 6555 "parser.tab.c"
    break;

  case 53: /* IntegralType: CHAR  */
#line 1291 "parser.y"
         { (yyval.node) = make_keyword_leaf("char", (yylsp[0])); }
#line 6561 "parser.tab.c"
    break;

  case 54: /* FloatingPointType: FLOAT  */
#line 1296 "parser.y"
          { (yyval.node) = make_keyword_leaf("float", (yylsp[0])); }
#line 6567 "parser.tab.c"
    break;

  case 55: /* FloatingPointType: DOUBLE  */
#line 1297 "parser.y"
           { (yyval.node) = make_keyword_leaf("double", (yylsp[0])); }
#line 6573 "parser.tab.c"
    break;

  case 56: /* ReferenceType: ClassOrInterfaceType  */
#line 1303 "parser.y"
                         { (yyval.node) = (yyvsp[0].node); }
#line 6579 "parser.tab.c"
    break;

  case 57: /* ReferenceType: ArrayType  */
#line 1305 "parser.y"
                { (yyval.node) = (yyvsp[0].node); }
#line 6585 "parser.tab.c"
    break;

  case 58: /* ClassOrInterfaceType: UnannClassOrInterfaceType  */
#line 1316 "parser.y"
                              {
        (yyval.node) = make_type_node((yyvsp[0].node), (yyloc));
    }
#line 6593 "parser.tab.c"
    break;

  case 59: /* ClassOrInterfaceType: Annotations UnannClassOrInterfaceType  */
#line 1319 "parser.y"
                                            {
        AstNode *type = make_type_node((yyvsp[0].node), (yyloc));
        if ((yyvsp[-1].node)) {
            ast_prepend_child(type, (yyvsp[-1].node));
        }
        (yyval.node) = type;
    }
#line 6605 "parser.tab.c"
    break;

  case 60: /* ArrayType: Annotations UnannPrimitiveType Dims  */
#line 1331 "parser.y"
                                        {
        AstNode *type = make_type_node((yyvsp[-1].node), (yyloc));
        if ((yyvsp[-2].node)) {
            ast_prepend_child(type, (yyvsp[-2].node));
        }
        if ((yyvsp[0].node)) {
            ast_add_child(type, (yyvsp[0].node));
        }
        (yyval.node) = type;
    }
#line 6620 "parser.tab.c"
    break;

  case 61: /* ArrayType: Annotations UnannClassOrInterfaceType Dims  */
#line 1341 "parser.y"
                                                 {
        AstNode *type = make_type_node((yyvsp[-1].node), (yyloc));
        if ((yyvsp[-2].node)) {
            ast_prepend_child(type, (yyvsp[-2].node));
        }
        if ((yyvsp[0].node)) {
            ast_add_child(type, (yyvsp[0].node));
        }
        (yyval.node) = type;
    }
#line 6635 "parser.tab.c"
    break;

  case 62: /* ArrayType: UnannArrayType  */
#line 1351 "parser.y"
                     {
        (yyval.node) = make_type_node((yyvsp[0].node), (yyloc));
    }
#line 6643 "parser.tab.c"
    break;

  case 63: /* Dims: Annotations_Dims '[' ']'  */
#line 1358 "parser.y"
                             {
        AstNode *list = make_list_node(AST_DIM_LIST, (yyloc));
        AstNode *dim = make_dim_node((yylsp[-1]));
        if ((yyvsp[-2].node)) {
            ast_add_child(dim, (yyvsp[-2].node));
        }
        ast_add_child(list, dim);
        (yyval.node) = list;
    }
#line 6657 "parser.tab.c"
    break;

  case 64: /* Dims: Annotations_Dims '[' ']' Dims  */
#line 1367 "parser.y"
                                    {
        AstNode *dim = make_dim_node((yylsp[-2]));
        if ((yyvsp[-3].node)) {
            ast_add_child(dim, (yyvsp[-3].node));
        }
        ast_prepend_child((yyvsp[0].node), dim);
        (yyval.node) = (yyvsp[0].node);
    }
#line 6670 "parser.tab.c"
    break;

  case 65: /* Dims: '[' ']'  */
#line 1375 "parser.y"
              {
        AstNode *list = make_list_node(AST_DIM_LIST, (yyloc));
        ast_add_child(list, make_dim_node((yylsp[-1])));
        (yyval.node) = list;
    }
#line 6680 "parser.tab.c"
    break;

  case 66: /* Dims: '[' ']' Dims  */
#line 1380 "parser.y"
                   {
        AstNode *dim = make_dim_node((yylsp[-2]));
        ast_prepend_child((yyvsp[0].node), dim);
        (yyval.node) = (yyvsp[0].node);
    }
#line 6690 "parser.tab.c"
    break;

  case 67: /* Annotation_Dims: NormalAnnotation_Dims  */
#line 1389 "parser.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 6696 "parser.tab.c"
    break;

  case 68: /* Annotation_Dims: MarkerAnnotation_Dims  */
#line 1390 "parser.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 6702 "parser.tab.c"
    break;

  case 69: /* Annotation_Dims: SingleElementAnnotation_Dims  */
#line 1391 "parser.y"
                                   { (yyval.node) = (yyvsp[0].node); }
#line 6708 "parser.tab.c"
    break;

  case 70: /* Annotations_Dims: Annotations_Dims Annotation_Dims  */
#line 1396 "parser.y"
                                     {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 6717 "parser.tab.c"
    break;

  case 71: /* Annotations_Dims: Annotation_Dims  */
#line 1400 "parser.y"
                      {
        (yyval.node) = make_list_node(AST_ANNOTATION_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 6726 "parser.tab.c"
    break;

  case 72: /* NormalAnnotation_Dims: AT_Dims TypeName_ModifierOrDims '(' ElementValuePairList ')'  */
#line 1408 "parser.y"
                                                                 {
        AstNode *node = make_annotation_node((yyvsp[-3].node), (yylsp[-4]));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 6736 "parser.tab.c"
    break;

  case 73: /* NormalAnnotation_Dims: AT_Dims TypeName_ModifierOrDims '(' ')'  */
#line 1413 "parser.y"
                                              {
        (yyval.node) = make_annotation_node((yyvsp[-2].node), (yylsp[-3]));
    }
#line 6744 "parser.tab.c"
    break;

  case 74: /* MarkerAnnotation_Dims: AT_Dims TypeName  */
#line 1420 "parser.y"
                     {
        (yyval.node) = make_annotation_node((yyvsp[0].node), (yylsp[-1]));
    }
#line 6752 "parser.tab.c"
    break;

  case 75: /* SingleElementAnnotation_Dims: AT_Dims TypeName_ModifierOrDims '(' ElementValue ')'  */
#line 1427 "parser.y"
                                                         {
        AstNode *node = make_annotation_node((yyvsp[-3].node), (yylsp[-4]));
        AstNode *args = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child(args, (yyvsp[-1].node));
        ast_add_child(node, args);
        (yyval.node) = node;
    }
#line 6764 "parser.tab.c"
    break;

  case 76: /* TypeParameter: Annotations TYPE_IDENTIFIER  */
#line 1438 "parser.y"
                                {
        AstNode *node = make_type_parameter_node((yyvsp[0].node), NULL, (yyloc));
        if ((yyvsp[-1].node)) {
            ast_prepend_child(node, (yyvsp[-1].node));
        }
        (yyval.node) = node;
    }
#line 6776 "parser.tab.c"
    break;

  case 77: /* TypeParameter: Annotations TYPE_IDENTIFIER TypeBound  */
#line 1445 "parser.y"
                                            {
        AstNode *node = make_type_parameter_node((yyvsp[-1].node), (yyvsp[0].node), (yyloc));
        if ((yyvsp[-2].node)) {
            ast_prepend_child(node, (yyvsp[-2].node));
        }
        (yyval.node) = node;
    }
#line 6788 "parser.tab.c"
    break;

  case 78: /* TypeParameter: TYPE_IDENTIFIER  */
#line 1452 "parser.y"
                      {
        (yyval.node) = make_type_parameter_node((yyvsp[0].node), NULL, (yyloc));
    }
#line 6796 "parser.tab.c"
    break;

  case 79: /* TypeParameter: TYPE_IDENTIFIER TypeBound  */
#line 1455 "parser.y"
                                {
        (yyval.node) = make_type_parameter_node((yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 6804 "parser.tab.c"
    break;

  case 80: /* TypeBound: EXTENDS ClassOrInterfaceType AdditionalBounds  */
#line 1462 "parser.y"
                                                  {
        (yyval.node) = make_type_bound_node("extends", (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 6812 "parser.tab.c"
    break;

  case 81: /* AdditionalBounds: AdditionalBounds AMPERSAND ClassOrInterfaceType  */
#line 1470 "parser.y"
                                                    {
        AstNode *list = (yyvsp[-2].node);
        if (!list) {
            list = make_list_node(AST_ADDITIONAL_BOUNDS, (yyloc));
        }
        ast_add_child(list, (yyvsp[0].node));
        (yyval.node) = list;
    }
#line 6825 "parser.tab.c"
    break;

  case 82: /* AdditionalBounds: %empty  */
#line 1478 "parser.y"
       { (yyval.node) = NULL; }
#line 6831 "parser.tab.c"
    break;

  case 83: /* TypeArguments: '<' TypeArgumentList '>'  */
#line 1483 "parser.y"
                             { (yyval.node) = (yyvsp[-1].node); }
#line 6837 "parser.tab.c"
    break;

  case 84: /* TypeArguments: LANGLE TypeArgumentList '>'  */
#line 1484 "parser.y"
                                  { (yyval.node) = (yyvsp[-1].node); }
#line 6843 "parser.tab.c"
    break;

  case 85: /* TypeArguments_UnannClassOrInterfaceType: LANGLE TypeArgumentList '>'  */
#line 1490 "parser.y"
                                { (yyval.node) = (yyvsp[-1].node); }
#line 6849 "parser.tab.c"
    break;

  case 86: /* TypeArgumentList: TypeArgument  */
#line 1495 "parser.y"
                 {
        (yyval.node) = make_list_node(AST_TYPE_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 6858 "parser.tab.c"
    break;

  case 87: /* TypeArgumentList: TypeArgumentList ',' TypeArgument  */
#line 1499 "parser.y"
                                        {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 6867 "parser.tab.c"
    break;

  case 88: /* TypeArgument: ReferenceType  */
#line 1507 "parser.y"
                  {
        (yyval.node) = make_type_argument_type_node((yyvsp[0].node), (yyloc));
    }
#line 6875 "parser.tab.c"
    break;

  case 89: /* TypeArgument: Wildcard  */
#line 1510 "parser.y"
             {
        (yyval.node) = make_type_argument_wildcard_node((yyvsp[0].node), (yyloc));
    }
#line 6883 "parser.tab.c"
    break;

  case 90: /* Wildcard: Annotations '?' WildcardBounds  */
#line 1517 "parser.y"
                                   {
        (yyval.node) = make_wildcard_node((yyvsp[-2].node), (yyvsp[0].node), (yyloc));
    }
#line 6891 "parser.tab.c"
    break;

  case 91: /* Wildcard: Annotations '?'  */
#line 1520 "parser.y"
                    {
        (yyval.node) = make_wildcard_node((yyvsp[-1].node), NULL, (yyloc));
    }
#line 6899 "parser.tab.c"
    break;

  case 92: /* Wildcard: '?' WildcardBounds  */
#line 1523 "parser.y"
                       {
        (yyval.node) = make_wildcard_node(NULL, (yyvsp[0].node), (yyloc));
    }
#line 6907 "parser.tab.c"
    break;

  case 93: /* Wildcard: '?'  */
#line 1526 "parser.y"
        {
        (yyval.node) = make_wildcard_node(NULL, NULL, (yyloc));
    }
#line 6915 "parser.tab.c"
    break;

  case 94: /* WildcardBounds: EXTENDS ReferenceType  */
#line 1533 "parser.y"
                          {
        AstNode *bound = make_type_bound_node("extends", (yyvsp[0].node), NULL, (yyloc));
        (yyval.node) = make_wildcard_bound_node("extends", bound, (yylsp[-1]));
    }
#line 6924 "parser.tab.c"
    break;

  case 95: /* WildcardBounds: SUPER ReferenceType  */
#line 1537 "parser.y"
                        {
        AstNode *bound = make_type_bound_node("super", (yyvsp[0].node), NULL, (yyloc));
        (yyval.node) = make_wildcard_bound_node("super", bound, (yylsp[-1]));
    }
#line 6933 "parser.tab.c"
    break;

  case 96: /* ModuleName: TYPE_IDENTIFIER  */
#line 1550 "parser.y"
                    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 6941 "parser.tab.c"
    break;

  case 97: /* ModuleName: IdentifierComplement  */
#line 1553 "parser.y"
                           {
        (yyval.node) = (yyvsp[0].node);
    }
#line 6949 "parser.tab.c"
    break;

  case 98: /* ModuleName: ModuleName DOT_CommonName TYPE_IDENTIFIER  */
#line 1556 "parser.y"
                                                {
        AstNode *children[] = { (yyvsp[-2].node), (yyvsp[0].node) };
        (yyval.node) = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        (yyval.node)->scope = (yyvsp[-2].node);
        ast_add_children((yyval.node), children, 2);
    }
#line 6960 "parser.tab.c"
    break;

  case 99: /* ModuleName: ModuleName DOT_CommonName IdentifierComplement  */
#line 1562 "parser.y"
                                                     {
        AstNode *children[] = { (yyvsp[-2].node), (yyvsp[0].node) };
        (yyval.node) = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        (yyval.node)->scope = (yyvsp[-2].node);
        ast_add_children((yyval.node), children, 2);
    }
#line 6971 "parser.tab.c"
    break;

  case 100: /* ModuleNames: ModuleName  */
#line 1572 "parser.y"
               {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 6980 "parser.tab.c"
    break;

  case 101: /* ModuleNames: ModuleNames ',' ModuleName  */
#line 1576 "parser.y"
                               {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 6989 "parser.tab.c"
    break;

  case 102: /* TypeName: TYPE_IDENTIFIER  */
#line 1584 "parser.y"
                    { (yyval.node) = (yyvsp[0].node); }
#line 6995 "parser.tab.c"
    break;

  case 103: /* TypeName: CommonName DOT_CommonName TYPE_IDENTIFIER  */
#line 1585 "parser.y"
                                              {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        node->scope = (yyvsp[-2].node);
        ast_add_child(node, (yyvsp[-2].node));
        ast_add_child(node, (yyvsp[0].node));
        (yyval.node) = node;
    }
#line 7007 "parser.tab.c"
    break;

  case 104: /* TypeNames: TypeName  */
#line 1595 "parser.y"
             {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 7016 "parser.tab.c"
    break;

  case 105: /* TypeNames: TypeNames ',' TypeName  */
#line 1599 "parser.y"
                           {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 7025 "parser.tab.c"
    break;

  case 106: /* CommonName: TYPE_IDENTIFIER  */
#line 1623 "parser.y"
                    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 7033 "parser.tab.c"
    break;

  case 107: /* CommonName: IdentifierComplement  */
#line 1626 "parser.y"
                           {
        (yyval.node) = (yyvsp[0].node);
    }
#line 7041 "parser.tab.c"
    break;

  case 108: /* CommonName: CommonName DOT_CommonName TYPE_IDENTIFIER  */
#line 1629 "parser.y"
                                                {
        AstNode *children[] = { (yyvsp[-2].node), (yyvsp[0].node) };
        (yyval.node) = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        (yyval.node)->scope = (yyvsp[-2].node);
        ast_add_children((yyval.node), children, 2);
    }
#line 7052 "parser.tab.c"
    break;

  case 109: /* CommonName: CommonName DOT_CommonName IdentifierComplement  */
#line 1635 "parser.y"
                                                     {
        AstNode *children[] = { (yyvsp[-2].node), (yyvsp[0].node) };
        (yyval.node) = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        (yyval.node)->scope = (yyvsp[-2].node);
        ast_add_children((yyval.node), children, 2);
    }
#line 7063 "parser.tab.c"
    break;

  case 110: /* CompilationUnit: OrdinaryCompilationUnit  */
#line 1648 "parser.y"
                            { (yyval.node) = (yyvsp[0].node); root_ast = (yyval.node); }
#line 7069 "parser.tab.c"
    break;

  case 111: /* CompilationUnit: ModularCompilationUnit  */
#line 1649 "parser.y"
                           {
        (yyval.node) = (yyvsp[0].node);
        root_ast = (yyval.node);
    }
#line 7078 "parser.tab.c"
    break;

  case 112: /* OrdinaryCompilationUnit: PackageDeclaration ImportDeclarations TypeDeclarations  */
#line 1658 "parser.y"
                                                           {
        (yyval.node) = AST_BRANCH_AT(AST_COMPILATION_UNIT, (yyloc), 3, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node));
    }
#line 7086 "parser.tab.c"
    break;

  case 113: /* OrdinaryCompilationUnit: ImportDeclarations TypeDeclarations  */
#line 1661 "parser.y"
                                        {
        (yyval.node) = AST_BRANCH_AT(AST_COMPILATION_UNIT, (yyloc), 2, (yyvsp[-1].node), (yyvsp[0].node));
    }
#line 7094 "parser.tab.c"
    break;

  case 114: /* OrdinaryCompilationUnit: PackageDeclaration TypeDeclarations  */
#line 1664 "parser.y"
                                        {
        AstNode *empty_imports = AST_EMPTY_NODE(AST_IMPORT_LIST, (yyloc));
        (yyval.node) = AST_BRANCH_AT(AST_COMPILATION_UNIT, (yyloc), 3, (yyvsp[-1].node), empty_imports, (yyvsp[0].node));
    }
#line 7103 "parser.tab.c"
    break;

  case 115: /* OrdinaryCompilationUnit: TypeDeclarations  */
#line 1668 "parser.y"
                     {
        AstNode *empty_imports = AST_EMPTY_NODE(AST_IMPORT_LIST, (yyloc));
        (yyval.node) = AST_BRANCH_AT(AST_COMPILATION_UNIT, (yyloc), 2, empty_imports, (yyvsp[0].node));
    }
#line 7112 "parser.tab.c"
    break;

  case 116: /* ModularCompilationUnit: ImportDeclarations ModuleDeclaration  */
#line 1677 "parser.y"
                                         {
        (yyval.node) = AST_BRANCH_AT(AST_COMPILATION_UNIT, (yylsp[-1]), 2, (yyvsp[-1].node), (yyvsp[0].node));
    }
#line 7120 "parser.tab.c"
    break;

  case 117: /* ModularCompilationUnit: ModuleDeclaration  */
#line 1680 "parser.y"
                        {
        (yyval.node) = AST_BRANCH_AT(AST_COMPILATION_UNIT, (yylsp[0]), 1, (yyvsp[0].node));
    }
#line 7128 "parser.tab.c"
    break;

  case 118: /* PackageDeclaration: Annotations PACKAGE CommonName ';'  */
#line 1688 "parser.y"
                                       {
        (yyval.node) = AST_BRANCH_AT(AST_PACKAGE_DECL, (yylsp[-2]), 2, (yyvsp[-3].node), (yyvsp[-1].node));
    }
#line 7136 "parser.tab.c"
    break;

  case 119: /* PackageDeclaration: PACKAGE CommonName ';'  */
#line 1691 "parser.y"
                           {
        AstNode *empty_ann = make_list_node(AST_ANNOTATION_LIST, (yyloc));   // 与 Annotations 的 list kind 保持一致。
        (yyval.node) = AST_BRANCH_AT(AST_PACKAGE_DECL, (yylsp[-2]), 2, empty_ann, (yyvsp[-1].node));
    }
#line 7145 "parser.tab.c"
    break;

  case 120: /* ImportDeclarations: ImportDeclaration  */
#line 1701 "parser.y"
                      {
        (yyval.node) = AST_BRANCH_AT(AST_IMPORT_LIST, (yyloc), 1, (yyvsp[0].node));
    }
#line 7153 "parser.tab.c"
    break;

  case 121: /* ImportDeclarations: ImportDeclarations ImportDeclaration  */
#line 1704 "parser.y"
                                         {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 7162 "parser.tab.c"
    break;

  case 122: /* ImportDeclaration: SingleTypeImportDeclaration  */
#line 1712 "parser.y"
                                         { (yyval.node) = (yyvsp[0].node); }
#line 7168 "parser.tab.c"
    break;

  case 123: /* ImportDeclaration: TypeImportOnDemandDeclaration  */
#line 1713 "parser.y"
                                         { (yyval.node) = (yyvsp[0].node); }
#line 7174 "parser.tab.c"
    break;

  case 124: /* ImportDeclaration: SingleStaticImportDeclaration  */
#line 1714 "parser.y"
                                         { (yyval.node) = (yyvsp[0].node); }
#line 7180 "parser.tab.c"
    break;

  case 125: /* ImportDeclaration: StaticImportOnDemandDeclaration  */
#line 1715 "parser.y"
                                         { (yyval.node) = (yyvsp[0].node); }
#line 7186 "parser.tab.c"
    break;

  case 126: /* SingleTypeImportDeclaration: IMPORT TypeName ';'  */
#line 1720 "parser.y"
                        {
        (yyval.node) = make_import_node(false, false, (yyvsp[-1].node), (yylsp[-2]));
    }
#line 7194 "parser.tab.c"
    break;

  case 127: /* TypeImportOnDemandDeclaration: IMPORT CommonName '.' '*' ';'  */
#line 1727 "parser.y"
                                  {
        (yyval.node) = make_import_node(false, true, (yyvsp[-3].node), (yylsp[-4]));
    }
#line 7202 "parser.tab.c"
    break;

  case 128: /* SingleStaticImportDeclaration: IMPORT STATIC CommonName ';'  */
#line 1737 "parser.y"
                                 {
        (yyval.node) = make_import_node(true, false, (yyvsp[-1].node), (yylsp[-3]));
    }
#line 7210 "parser.tab.c"
    break;

  case 129: /* StaticImportOnDemandDeclaration: IMPORT STATIC CommonName '.' '*' ';'  */
#line 1746 "parser.y"
                                         {
        (yyval.node) = make_import_node(true, true, (yyvsp[-3].node), (yylsp[-5]));
    }
#line 7218 "parser.tab.c"
    break;

  case 130: /* TypeDeclarations: TypeDeclaration  */
#line 1753 "parser.y"
                    {
        (yyval.node) = AST_BRANCH_AT(AST_TYPE_DECL_LIST, (yyloc), 1, (yyvsp[0].node));
    }
#line 7226 "parser.tab.c"
    break;

  case 131: /* TypeDeclarations: TypeDeclarations TypeDeclaration  */
#line 1756 "parser.y"
                                     {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 7235 "parser.tab.c"
    break;

  case 132: /* TypeDeclaration: ClassDeclaration  */
#line 1765 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 7241 "parser.tab.c"
    break;

  case 133: /* TypeDeclaration: InterfaceDeclaration  */
#line 1766 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 7247 "parser.tab.c"
    break;

  case 134: /* TypeDeclaration: EMPTY_STMT  */
#line 1767 "parser.y"
                 { (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column); }
#line 7253 "parser.tab.c"
    break;

  case 135: /* TypeDeclaration: ';'  */
#line 1768 "parser.y"
          { (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column); }
#line 7259 "parser.tab.c"
    break;

  case 136: /* ModuleDeclaration: Annotations OPEN MODULE CommonName '{' ModuleDirectives '}'  */
#line 1774 "parser.y"
                                                                {
        YYLTYPE open_loc = (yylsp[-5]);
        (yyval.node) = make_module_decl_node((yyvsp[-3].node), (yyvsp[-1].node), true, (yylsp[-4]), &open_loc);
    }
#line 7268 "parser.tab.c"
    break;

  case 137: /* ModuleDeclaration: Annotations MODULE CommonName '{' ModuleDirectives '}'  */
#line 1778 "parser.y"
                                                             {
        (yyval.node) = make_module_decl_node((yyvsp[-3].node), (yyvsp[-1].node), false, (yylsp[-4]), NULL);
    }
#line 7276 "parser.tab.c"
    break;

  case 138: /* ModuleDeclaration: OPEN MODULE CommonName '{' ModuleDirectives '}'  */
#line 1781 "parser.y"
                                                      {
        YYLTYPE open_loc = (yylsp[-5]);
        (yyval.node) = make_module_decl_node((yyvsp[-3].node), (yyvsp[-1].node), true, (yylsp[-4]), &open_loc);
    }
#line 7285 "parser.tab.c"
    break;

  case 139: /* ModuleDeclaration: MODULE CommonName '{' ModuleDirectives '}'  */
#line 1785 "parser.y"
                                                 {
        (yyval.node) = make_module_decl_node((yyvsp[-3].node), (yyvsp[-1].node), false, (yylsp[-4]), NULL);
    }
#line 7293 "parser.tab.c"
    break;

  case 140: /* ModuleDeclaration: Annotations OPEN MODULE CommonName '{' '}'  */
#line 1788 "parser.y"
                                                 {
        YYLTYPE open_loc = (yylsp[-4]);
        (yyval.node) = make_module_decl_node((yyvsp[-2].node), NULL, true, (yylsp[-3]), &open_loc);
    }
#line 7302 "parser.tab.c"
    break;

  case 141: /* ModuleDeclaration: Annotations MODULE CommonName '{' '}'  */
#line 1792 "parser.y"
                                            {
        (yyval.node) = make_module_decl_node((yyvsp[-2].node), NULL, false, (yylsp[-3]), NULL);
    }
#line 7310 "parser.tab.c"
    break;

  case 142: /* ModuleDeclaration: OPEN MODULE CommonName '{' '}'  */
#line 1795 "parser.y"
                                     {
        YYLTYPE open_loc = (yylsp[-4]);
        (yyval.node) = make_module_decl_node((yyvsp[-2].node), NULL, true, (yylsp[-3]), &open_loc);
    }
#line 7319 "parser.tab.c"
    break;

  case 143: /* ModuleDeclaration: MODULE CommonName '{' '}'  */
#line 1799 "parser.y"
                                {
        (yyval.node) = make_module_decl_node((yyvsp[-2].node), NULL, false, (yylsp[-3]), NULL);
    }
#line 7327 "parser.tab.c"
    break;

  case 144: /* ModuleDirectives: ModuleDirective  */
#line 1806 "parser.y"
                    {
        (yyval.node) = AST_BRANCH_AT(AST_STATEMENT_LIST, (yylsp[0]), 1, (yyvsp[0].node));
    }
#line 7335 "parser.tab.c"
    break;

  case 145: /* ModuleDirectives: ModuleDirectives ModuleDirective  */
#line 1809 "parser.y"
                                       {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 7344 "parser.tab.c"
    break;

  case 146: /* ModuleDirective: REQUIRES TRANSITIVE ModuleName ';'  */
#line 1819 "parser.y"
                                       {
        AstNode *node = make_module_directive_node(AST_REQUIRES_DIRECTIVE, (yylsp[-3]));
        ast_add_child(node, make_keyword_leaf("transitive", (yylsp[-2])));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 7355 "parser.tab.c"
    break;

  case 147: /* ModuleDirective: REQUIRES ModuleName ';'  */
#line 1825 "parser.y"
                              {
        AstNode *node = make_module_directive_node(AST_REQUIRES_DIRECTIVE, (yylsp[-2]));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 7365 "parser.tab.c"
    break;

  case 148: /* ModuleDirective: REQUIRES STATIC ModuleName ';'  */
#line 1830 "parser.y"
                                     {
        AstNode *node = make_module_directive_node(AST_REQUIRES_DIRECTIVE, (yylsp[-3]));
        ast_add_child(node, make_keyword_leaf("static", (yylsp[-2])));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 7376 "parser.tab.c"
    break;

  case 149: /* ModuleDirective: EXPORTS CommonName ';'  */
#line 1836 "parser.y"
                             {
        AstNode *node = make_module_directive_node(AST_EXPORTS_DIRECTIVE, (yylsp[-2]));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 7386 "parser.tab.c"
    break;

  case 150: /* ModuleDirective: EXPORTS CommonName TO ModuleNames ';'  */
#line 1841 "parser.y"
                                            {
        AstNode *node = make_module_directive_node(AST_EXPORTS_DIRECTIVE, (yylsp[-4]));
        ast_add_child(node, (yyvsp[-3].node));
        ast_add_child(node, make_keyword_leaf("to", (yylsp[-2])));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 7398 "parser.tab.c"
    break;

  case 151: /* ModuleDirective: OPENS CommonName ';'  */
#line 1848 "parser.y"
                           {
        AstNode *node = make_module_directive_node(AST_OPENS_DIRECTIVE, (yylsp[-2]));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 7408 "parser.tab.c"
    break;

  case 152: /* ModuleDirective: OPENS CommonName TO ModuleNames ';'  */
#line 1853 "parser.y"
                                          {
        AstNode *node = make_module_directive_node(AST_OPENS_DIRECTIVE, (yylsp[-4]));
        ast_add_child(node, (yyvsp[-3].node));
        ast_add_child(node, make_keyword_leaf("to", (yylsp[-2])));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 7420 "parser.tab.c"
    break;

  case 153: /* ModuleDirective: USES TypeName ';'  */
#line 1860 "parser.y"
                        {
        AstNode *node = make_module_directive_node(AST_USES_DIRECTIVE, (yylsp[-2]));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 7430 "parser.tab.c"
    break;

  case 154: /* ModuleDirective: PROVIDES TypeName WITH TypeNames ';'  */
#line 1865 "parser.y"
                                           {
        AstNode *node = make_module_directive_node(AST_PROVIDES_DIRECTIVE, (yylsp[-4]));
        ast_add_child(node, (yyvsp[-3].node));
        ast_add_child(node, make_keyword_leaf("with", (yylsp[-2])));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 7442 "parser.tab.c"
    break;

  case 155: /* ArrayInitializer: '{' VariableInitializerList '}'  */
#line 1882 "parser.y"
                                    {
        (yyval.node) = make_array_initializer_node((yyvsp[-1].node), (yylsp[-2]));
    }
#line 7450 "parser.tab.c"
    break;

  case 156: /* ArrayInitializer: '{' VariableInitializerList ',' '}'  */
#line 1885 "parser.y"
                                          {
        (yyval.node) = make_array_initializer_node((yyvsp[-2].node), (yylsp[-3]));
    }
#line 7458 "parser.tab.c"
    break;

  case 157: /* ArrayInitializer: '{' '}'  */
#line 1889 "parser.y"
              {
        (yyval.node) = make_array_initializer_node(NULL, (yylsp[-1]));
    }
#line 7466 "parser.tab.c"
    break;

  case 158: /* VariableInitializerList: VariableInitializer  */
#line 1896 "parser.y"
                        {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 7475 "parser.tab.c"
    break;

  case 159: /* VariableInitializerList: VariableInitializerList ',' VariableInitializer  */
#line 1900 "parser.y"
                                                      {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 7484 "parser.tab.c"
    break;

  case 160: /* ClassDeclaration: NormalClassDeclaration  */
#line 1914 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 7490 "parser.tab.c"
    break;

  case 161: /* ClassDeclaration: EnumDeclaration  */
#line 1915 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 7496 "parser.tab.c"
    break;

  case 162: /* NormalClassDeclaration: Modifiers CLASS TYPE_IDENTIFIER OptTypeParameters OptSuperclass OptSuperinterfaces OptClassOrInterfacePermits ClassBody  */
#line 1920 "parser.y"
                                                                                                                            {
        /* 参数顺序对应修改后的 make_class_basic */
        (yyval.node) = make_class_basic((yylsp[-6]).first_line, (yylsp[-6]).first_column, 
                              (yyvsp[-7].node),  /* Modifiers */
                              (yyvsp[-5].node),  /* Name */
                              (yyvsp[-4].node),  /* TypeParams */
                              (yyvsp[-3].node),  /* SuperClass */
                              (yyvsp[-2].node),  /* Interfaces */
                              (yyvsp[-1].node),  /* Permits */
                              (yyvsp[0].node)); /* Body */
    }
#line 7512 "parser.tab.c"
    break;

  case 163: /* NormalClassDeclaration: CLASS TYPE_IDENTIFIER OptTypeParameters OptSuperclass OptSuperinterfaces OptClassOrInterfacePermits ClassBody  */
#line 1931 "parser.y"
                                                                                                                    {
        (yyval.node) = make_class_basic((yylsp[-6]).first_line, (yylsp[-6]).first_column, 
                              NULL, /* No Modifiers */
                              (yyvsp[-5].node), 
                              (yyvsp[-4].node), 
                              (yyvsp[-3].node), 
                              (yyvsp[-2].node), 
                              (yyvsp[-1].node), 
                              (yyvsp[0].node));
    }
#line 7527 "parser.tab.c"
    break;

  case 164: /* OptTypeParameters: TypeParameters  */
#line 1944 "parser.y"
                   { (yyval.node) = (yyvsp[0].node); }
#line 7533 "parser.tab.c"
    break;

  case 165: /* OptTypeParameters: %empty  */
#line 1945 "parser.y"
                  { (yyval.node) = NULL; }
#line 7539 "parser.tab.c"
    break;

  case 166: /* OptSuperclass: Superclass  */
#line 1950 "parser.y"
               { (yyval.node) = (yyvsp[0].node); }
#line 7545 "parser.tab.c"
    break;

  case 167: /* OptSuperclass: %empty  */
#line 1951 "parser.y"
                  { (yyval.node) = NULL; }
#line 7551 "parser.tab.c"
    break;

  case 168: /* OptSuperinterfaces: Superinterfaces  */
#line 1956 "parser.y"
                    { (yyval.node) = (yyvsp[0].node); }
#line 7557 "parser.tab.c"
    break;

  case 169: /* OptSuperinterfaces: %empty  */
#line 1957 "parser.y"
                  { (yyval.node) = NULL; }
#line 7563 "parser.tab.c"
    break;

  case 170: /* OptClassOrInterfacePermits: ClassOrInterfacePermits  */
#line 1962 "parser.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 7569 "parser.tab.c"
    break;

  case 171: /* OptClassOrInterfacePermits: %empty  */
#line 1963 "parser.y"
                  { (yyval.node) = NULL; }
#line 7575 "parser.tab.c"
    break;

  case 172: /* TypeParameters: '<' TypeParameterList '>'  */
#line 1968 "parser.y"
                               { (yyval.node) = (yyvsp[-1].node); }
#line 7581 "parser.tab.c"
    break;

  case 173: /* TypeParameters: LANGLE TypeParameterList '>'  */
#line 1969 "parser.y"
                                 { (yyval.node) = (yyvsp[-1].node); }
#line 7587 "parser.tab.c"
    break;

  case 174: /* TypeParameterList: TypeParameter  */
#line 1974 "parser.y"
                  {
        (yyval.node) = make_list_node(AST_TYPE_PARAMETER_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 7596 "parser.tab.c"
    break;

  case 175: /* TypeParameterList: TypeParameterList ',' TypeParameter  */
#line 1978 "parser.y"
                                        {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 7605 "parser.tab.c"
    break;

  case 176: /* Superclass: EXTENDS ClassOrInterfaceType  */
#line 1987 "parser.y"
                                 {
        (yyval.node) = (yyvsp[0].node);
    }
#line 7613 "parser.tab.c"
    break;

  case 177: /* Superinterfaces: IMPLEMENTS InterfaceTypeList  */
#line 1994 "parser.y"
                                 {
        (yyval.node) = (yyvsp[0].node);
    }
#line 7621 "parser.tab.c"
    break;

  case 178: /* InterfaceTypeList: ClassOrInterfaceType  */
#line 2001 "parser.y"
                         {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 7630 "parser.tab.c"
    break;

  case 179: /* InterfaceTypeList: InterfaceTypeList ',' ClassOrInterfaceType  */
#line 2005 "parser.y"
                                                 {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 7639 "parser.tab.c"
    break;

  case 180: /* ClassOrInterfacePermits: PERMITS TypeNames  */
#line 2013 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 7645 "parser.tab.c"
    break;

  case 181: /* ClassBody: '{' ClassBodyDeclarationList '}'  */
#line 2018 "parser.y"
                                     { (yyval.node) = (yyvsp[-1].node); }
#line 7651 "parser.tab.c"
    break;

  case 182: /* ClassBody: '{' '}'  */
#line 2019 "parser.y"
              { (yyval.node) = ast_branch(AST_BLOCK, (yylsp[-1]).first_line, (yylsp[-1]).first_column, 0); }
#line 7657 "parser.tab.c"
    break;

  case 183: /* ClassBodyDeclarationList: ClassBodyDeclaration  */
#line 2024 "parser.y"
                         {
        (yyval.node) = ast_branch(AST_BLOCK, (yylsp[0]).first_line, (yylsp[0]).first_column, 1, (yyvsp[0].node));
    }
#line 7665 "parser.tab.c"
    break;

  case 184: /* ClassBodyDeclarationList: ClassBodyDeclarationList ClassBodyDeclaration  */
#line 2027 "parser.y"
                                                    {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 7674 "parser.tab.c"
    break;

  case 185: /* ClassBodyDeclaration: ClassMemberDeclaration  */
#line 2035 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 7680 "parser.tab.c"
    break;

  case 186: /* ClassBodyDeclaration: InstanceInitializer  */
#line 2036 "parser.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 7686 "parser.tab.c"
    break;

  case 187: /* ClassBodyDeclaration: StaticInitializer  */
#line 2037 "parser.y"
                        { (yyval.node) = (yyvsp[0].node); }
#line 7692 "parser.tab.c"
    break;

  case 188: /* ClassBodyDeclaration: ConstructorDeclaration  */
#line 2038 "parser.y"
                             { (yyval.node) = (yyvsp[0].node); }
#line 7698 "parser.tab.c"
    break;

  case 189: /* ClassMemberDeclaration: FieldDeclaration  */
#line 2043 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 7704 "parser.tab.c"
    break;

  case 190: /* ClassMemberDeclaration: MethodDeclaration  */
#line 2044 "parser.y"
                        { (yyval.node) = (yyvsp[0].node); }
#line 7710 "parser.tab.c"
    break;

  case 191: /* ClassMemberDeclaration: ClassDeclaration  */
#line 2045 "parser.y"
                       { (yyval.node) = (yyvsp[0].node); }
#line 7716 "parser.tab.c"
    break;

  case 192: /* ClassMemberDeclaration: InterfaceDeclaration  */
#line 2046 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 7722 "parser.tab.c"
    break;

  case 193: /* ClassMemberDeclaration: ';'  */
#line 2047 "parser.y"
          { (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column); }
#line 7728 "parser.tab.c"
    break;

  case 194: /* ClassMemberDeclaration: EMPTY_STMT  */
#line 2048 "parser.y"
                 { (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column); }
#line 7734 "parser.tab.c"
    break;

  case 195: /* FieldDeclaration: Modifiers UnannType VariableDeclaratorList ';'  */
#line 2052 "parser.y"
                                                   {
        (yyval.node) = make_field_node((yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yylsp[-2]));
    }
#line 7742 "parser.tab.c"
    break;

  case 196: /* FieldDeclaration: UnannType VariableDeclaratorList ';'  */
#line 2055 "parser.y"
                                         {
        (yyval.node) = make_field_node(NULL, (yyvsp[-2].node), (yyvsp[-1].node), (yylsp[-2]));
    }
#line 7750 "parser.tab.c"
    break;

  case 197: /* VariableDeclaratorList: VariableDeclarator  */
#line 2066 "parser.y"
                       {
        (yyval.node) = make_list_node(AST_VAR_DECL_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 7759 "parser.tab.c"
    break;

  case 198: /* VariableDeclaratorList: VariableDeclaratorList ',' VariableDeclarator  */
#line 2070 "parser.y"
                                                    {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 7768 "parser.tab.c"
    break;

  case 199: /* VariableDeclarator: VariableDeclaratorId '=' VariableInitializer  */
#line 2078 "parser.y"
                                                 {
        AstNode *node = ast_branch(AST_VARIABLE_DECL, (yylsp[-2]).first_line, (yylsp[-2]).first_column, 0);
        ast_add_child(node, (yyvsp[-2].node));
        if ((yyvsp[0].node)) {
            ast_add_child(node, (yyvsp[0].node));
        }
        (yyval.node) = node;
    }
#line 7781 "parser.tab.c"
    break;

  case 200: /* VariableDeclarator: VariableDeclaratorId  */
#line 2086 "parser.y"
                           {
        AstNode *node = ast_branch(AST_VARIABLE_DECL, (yylsp[0]).first_line, (yylsp[0]).first_column, 0);
        ast_add_child(node, (yyvsp[0].node));
        (yyval.node) = node;
    }
#line 7791 "parser.tab.c"
    break;

  case 201: /* VariableDeclaratorId: TYPE_IDENTIFIER Dims  */
#line 2095 "parser.y"
                         {
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 7802 "parser.tab.c"
    break;

  case 202: /* VariableDeclaratorId: IdentifierComplement Dims  */
#line 2101 "parser.y"
                                {
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 7813 "parser.tab.c"
    break;

  case 203: /* VariableDeclaratorId: TYPE_IDENTIFIER  */
#line 2107 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 7819 "parser.tab.c"
    break;

  case 204: /* VariableDeclaratorId: IdentifierComplement  */
#line 2108 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 7825 "parser.tab.c"
    break;

  case 205: /* VariableInitializer: Expression  */
#line 2113 "parser.y"
               {
        (yyval.node) = (yyvsp[0].node);
    }
#line 7833 "parser.tab.c"
    break;

  case 206: /* VariableInitializer: ArrayInitializer  */
#line 2116 "parser.y"
                       {
        (yyval.node) = (yyvsp[0].node);
    }
#line 7841 "parser.tab.c"
    break;

  case 207: /* UnannType: UnannReferenceType  */
#line 2123 "parser.y"
                       { (yyval.node) = make_type_node((yyvsp[0].node), (yyloc)); }
#line 7847 "parser.tab.c"
    break;

  case 208: /* UnannType: UnannPrimitiveType  */
#line 2124 "parser.y"
                         { (yyval.node) = make_type_node((yyvsp[0].node), (yyloc)); }
#line 7853 "parser.tab.c"
    break;

  case 209: /* UnannPrimitiveType: NumericType  */
#line 2137 "parser.y"
                { (yyval.node) = (yyvsp[0].node); }
#line 7859 "parser.tab.c"
    break;

  case 210: /* UnannPrimitiveType: BOOLEAN  */
#line 2138 "parser.y"
              { (yyval.node) = make_keyword_leaf("boolean", (yylsp[0])); }
#line 7865 "parser.tab.c"
    break;

  case 211: /* UnannReferenceType: UnannArrayType  */
#line 2143 "parser.y"
                   { (yyval.node) = (yyvsp[0].node); }
#line 7871 "parser.tab.c"
    break;

  case 212: /* UnannReferenceType: UnannClassOrInterfaceType  */
#line 2145 "parser.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 7877 "parser.tab.c"
    break;

  case 213: /* UnannClassOrInterfaceType: TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType  */
#line 2155 "parser.y"
                                                            {
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 7888 "parser.tab.c"
    break;

  case 214: /* UnannClassOrInterfaceType: CommonName  */
#line 2161 "parser.y"
                 { (yyval.node) = (yyvsp[0].node); }
#line 7894 "parser.tab.c"
    break;

  case 215: /* UnannClassOrInterfaceType: CommonName '.' Annotations TYPE_IDENTIFIER  */
#line 2165 "parser.y"
                                                 {
        AstNode *member = (yyvsp[0].node);
        if ((yyvsp[-1].node)) {
            ast_prepend_child(member, (yyvsp[-1].node));
        }
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        node->scope = (yyvsp[-3].node);
        ast_add_child(node, (yyvsp[-3].node));
        ast_add_child(node, member);
        (yyval.node) = node;
    }
#line 7910 "parser.tab.c"
    break;

  case 216: /* UnannClassOrInterfaceType: CommonName '.' Annotations TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType  */
#line 2176 "parser.y"
                                                                                         {
        AstNode *member = (yyvsp[-1].node);
        if ((yyvsp[-2].node)) {
            ast_prepend_child(member, (yyvsp[-2].node));
        }
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        node->scope = (yyvsp[-4].node);
        ast_add_child(node, (yyvsp[-4].node));
        ast_add_child(node, member);
        if ((yyvsp[0].node)) {
            ast_add_child(node, (yyvsp[0].node));
        }
        (yyval.node) = node;
    }
#line 7929 "parser.tab.c"
    break;

  case 217: /* UnannClassOrInterfaceType: CommonName DOT_CommonName TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType  */
#line 2190 "parser.y"
                                                                                        {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        node->scope = (yyvsp[-3].node);
        ast_add_child(node, (yyvsp[-3].node));
        ast_add_child(node, (yyvsp[-1].node));
        if ((yyvsp[0].node)) {
            ast_add_child(node, (yyvsp[0].node));
        }
        (yyval.node) = node;
    }
#line 7944 "parser.tab.c"
    break;

  case 218: /* UnannClassOrInterfaceType: UnannClassOrInterfaceType DOT TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType  */
#line 2201 "parser.y"
                                                                                            {
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        node->scope = (yyvsp[-3].node);
        ast_add_child(node, (yyvsp[-3].node));
        ast_add_child(node, (yyvsp[-1].node));
        if ((yyvsp[0].node)) {
            ast_add_child(node, (yyvsp[0].node));
        }
        (yyval.node) = node;
    }
#line 7959 "parser.tab.c"
    break;

  case 219: /* UnannClassOrInterfaceType: UnannClassOrInterfaceType DOT Annotations TYPE_IDENTIFIER  */
#line 2211 "parser.y"
                                                                {
        AstNode *member = (yyvsp[0].node);
        if ((yyvsp[-1].node)) {
            ast_prepend_child(member, (yyvsp[-1].node));
        }
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        node->scope = (yyvsp[-3].node);
        ast_add_child(node, (yyvsp[-3].node));
        ast_add_child(node, member);
        (yyval.node) = node;
    }
#line 7975 "parser.tab.c"
    break;

  case 220: /* UnannClassOrInterfaceType: UnannClassOrInterfaceType DOT Annotations TYPE_IDENTIFIER TypeArguments_UnannClassOrInterfaceType  */
#line 2222 "parser.y"
                                                                                                        {
        AstNode *member = (yyvsp[-1].node);
        if ((yyvsp[-2].node)) {
            ast_prepend_child(member, (yyvsp[-2].node));
        }
        AstNode *node = AST_EMPTY_NODE(AST_MEMBER_ACCESS, (yyloc));
        node->scope = (yyvsp[-4].node);
        ast_add_child(node, (yyvsp[-4].node));
        ast_add_child(node, member);
        if ((yyvsp[0].node)) {
            ast_add_child(node, (yyvsp[0].node));
        }
        (yyval.node) = node;
    }
#line 7994 "parser.tab.c"
    break;

  case 221: /* UnannArrayType: UnannPrimitiveType Dims  */
#line 2240 "parser.y"
                            {
        AstNode *type = make_type_node((yyvsp[-1].node), (yyloc));
        if ((yyvsp[0].node)) {
            ast_add_child(type, (yyvsp[0].node));
        }
        (yyval.node) = type;
    }
#line 8006 "parser.tab.c"
    break;

  case 222: /* UnannArrayType: UnannClassOrInterfaceType Dims  */
#line 2247 "parser.y"
                                     {
        AstNode *type = make_type_node((yyvsp[-1].node), (yyloc));
        if ((yyvsp[0].node)) {
            ast_add_child(type, (yyvsp[0].node));
        }
        (yyval.node) = type;
    }
#line 8018 "parser.tab.c"
    break;

  case 223: /* MethodDeclaration: Modifiers MethodHeader MethodBody  */
#line 2259 "parser.y"
                                      {
        move_trailing_type_annotations((yyvsp[-2].node), (yyvsp[-1].node));
        if ((yyvsp[-2].node) && (yyvsp[-2].node)->child_count > 0) {
            ast_prepend_child((yyvsp[-1].node), (yyvsp[-2].node));
        }
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 8033 "parser.tab.c"
    break;

  case 224: /* MethodDeclaration: MethodHeader MethodBody  */
#line 2269 "parser.y"
                            {
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 8044 "parser.tab.c"
    break;

  case 225: /* MethodHeader: Result MethodDeclarator Throws  */
#line 2279 "parser.y"
                                   {
        if ((yyvsp[-2].node)) {
            ast_prepend_child((yyvsp[-1].node), (yyvsp[-2].node));
        }
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 8058 "parser.tab.c"
    break;

  case 226: /* MethodHeader: Result MethodDeclarator  */
#line 2288 "parser.y"
                              {
        if ((yyvsp[-1].node)) {
            ast_prepend_child((yyvsp[0].node), (yyvsp[-1].node));
        }
        (yyval.node) = (yyvsp[0].node);
    }
#line 8069 "parser.tab.c"
    break;

  case 227: /* MethodHeader: TypeParameters Annotations Result MethodDeclarator Throws  */
#line 2294 "parser.y"
                                                                {
        if ((yyvsp[-2].node) && (yyvsp[-3].node)) {
            ast_prepend_child((yyvsp[-2].node), (yyvsp[-3].node));
        }
        if ((yyvsp[-2].node)) {
            ast_prepend_child((yyvsp[-1].node), (yyvsp[-2].node));
        }
        if ((yyvsp[-4].node)) {
            ast_prepend_child((yyvsp[-1].node), (yyvsp[-4].node));
        }
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 8089 "parser.tab.c"
    break;

  case 228: /* MethodHeader: TypeParameters Annotations Result MethodDeclarator  */
#line 2309 "parser.y"
                                                         {
        if ((yyvsp[-1].node) && (yyvsp[-2].node)) {
            ast_prepend_child((yyvsp[-1].node), (yyvsp[-2].node));
        }
        if ((yyvsp[-1].node)) {
            ast_prepend_child((yyvsp[0].node), (yyvsp[-1].node));
        }
        if ((yyvsp[-3].node)) {
            ast_prepend_child((yyvsp[0].node), (yyvsp[-3].node));
        }
        (yyval.node) = (yyvsp[0].node);
    }
#line 8106 "parser.tab.c"
    break;

  case 229: /* MethodHeader: TypeParameters Result MethodDeclarator Throws  */
#line 2321 "parser.y"
                                                    {
        if ((yyvsp[-2].node)) {
            ast_prepend_child((yyvsp[-1].node), (yyvsp[-2].node));
        }
        if ((yyvsp[-3].node)) {
            ast_prepend_child((yyvsp[-1].node), (yyvsp[-3].node));
        }
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 8123 "parser.tab.c"
    break;

  case 230: /* MethodHeader: TypeParameters Result MethodDeclarator  */
#line 2333 "parser.y"
                                             {
        if ((yyvsp[-1].node)) {
            ast_prepend_child((yyvsp[0].node), (yyvsp[-1].node));
        }
        if ((yyvsp[-2].node)) {
            ast_prepend_child((yyvsp[0].node), (yyvsp[-2].node));
        }
        (yyval.node) = (yyvsp[0].node);
    }
#line 8137 "parser.tab.c"
    break;

  case 231: /* Result: UnannType  */
#line 2346 "parser.y"
              { (yyval.node) = (yyvsp[0].node); }
#line 8143 "parser.tab.c"
    break;

  case 232: /* Result: VOID  */
#line 2347 "parser.y"
           { (yyval.node) = make_type_node(make_keyword_leaf("void", (yylsp[0])), (yylsp[0])); }
#line 8149 "parser.tab.c"
    break;

  case 233: /* MethodDeclarator: IDENTIFIER_MethodDeclarator '(' ReceiverParameter ',' FormalParameterList ')' Dims  */
#line 2352 "parser.y"
                                                                                       {
        AstNode *params = (yyvsp[-2].node) ? (yyvsp[-2].node) : make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_prepend_child(params, (yyvsp[-4].node));
        (yyval.node) = make_method_signature((yyvsp[-6].node), params, (yyloc));
    }
#line 8159 "parser.tab.c"
    break;

  case 234: /* MethodDeclarator: IDENTIFIER_MethodDeclarator '(' ReceiverParameter ')' Dims  */
#line 2357 "parser.y"
                                                                 {
        (yyval.node) = make_method_signature((yyvsp[-4].node), make_params_list((yyvsp[-2].node), (yyloc)), (yyloc));
    }
#line 8167 "parser.tab.c"
    break;

  case 235: /* MethodDeclarator: IDENTIFIER_MethodDeclarator '(' FormalParameterList ')' Dims  */
#line 2360 "parser.y"
                                                                   {
        (yyval.node) = make_method_signature((yyvsp[-4].node), (yyvsp[-2].node), (yyloc));
    }
#line 8175 "parser.tab.c"
    break;

  case 236: /* MethodDeclarator: IDENTIFIER_MethodDeclarator '(' ')' Dims  */
#line 2363 "parser.y"
                                               {
       AstNode *sig = make_method_signature((yyvsp[-3].node), NULL, (yyloc));
       if ((yyvsp[0].node)) ast_add_child(sig, (yyvsp[0].node));
       (yyval.node) = sig;
    }
#line 8185 "parser.tab.c"
    break;

  case 237: /* MethodDeclarator: IDENTIFIER_MethodDeclarator '(' ReceiverParameter ',' FormalParameterList ')'  */
#line 2369 "parser.y"
                                                                                    {
        AstNode *params = (yyvsp[-1].node) ? (yyvsp[-1].node) : make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_prepend_child(params, (yyvsp[-3].node));
        (yyval.node) = make_method_signature((yyvsp[-5].node), params, (yyloc));
    }
#line 8195 "parser.tab.c"
    break;

  case 238: /* MethodDeclarator: IDENTIFIER_MethodDeclarator '(' ReceiverParameter ')'  */
#line 2374 "parser.y"
                                                            {
        (yyval.node) = make_method_signature((yyvsp[-3].node), make_params_list((yyvsp[-1].node), (yyloc)), (yyloc));
    }
#line 8203 "parser.tab.c"
    break;

  case 239: /* MethodDeclarator: IDENTIFIER_MethodDeclarator '(' FormalParameterList ')'  */
#line 2377 "parser.y"
                                                              {
        (yyval.node) = make_method_signature((yyvsp[-3].node), (yyvsp[-1].node), (yyloc));
    }
#line 8211 "parser.tab.c"
    break;

  case 240: /* MethodDeclarator: IDENTIFIER_MethodDeclarator '(' ')'  */
#line 2380 "parser.y"
                                          {
        (yyval.node) = make_method_signature((yyvsp[-2].node), NULL, (yyloc));
    }
#line 8219 "parser.tab.c"
    break;

  case 241: /* ReceiverParameter: Annotations UnannType TYPE_IDENTIFIER '.' THIS  */
#line 2387 "parser.y"
                                                   {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-4].node), (yylsp[-4]));
        (yyval.node) = make_receiver_parameter_node(mods, (yyvsp[-3].node), (yyvsp[-2].node), (yylsp[0]), (yyloc));
    }
#line 8228 "parser.tab.c"
    break;

  case 242: /* ReceiverParameter: Annotations UnannType IdentifierComplement '.' THIS  */
#line 2391 "parser.y"
                                                          {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-4].node), (yylsp[-4]));
        (yyval.node) = make_receiver_parameter_node(mods, (yyvsp[-3].node), (yyvsp[-2].node), (yylsp[0]), (yyloc));
    }
#line 8237 "parser.tab.c"
    break;

  case 243: /* ReceiverParameter: Annotations UnannType THIS  */
#line 2395 "parser.y"
                                 {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = make_receiver_parameter_node(mods, (yyvsp[-1].node), NULL, (yylsp[0]), (yyloc));
    }
#line 8246 "parser.tab.c"
    break;

  case 244: /* ReceiverParameter: UnannType TYPE_IDENTIFIER '.' THIS  */
#line 2399 "parser.y"
                                         {
        (yyval.node) = make_receiver_parameter_node(NULL, (yyvsp[-3].node), (yyvsp[-2].node), (yylsp[0]), (yyloc));
    }
#line 8254 "parser.tab.c"
    break;

  case 245: /* ReceiverParameter: UnannType IdentifierComplement '.' THIS  */
#line 2402 "parser.y"
                                              {
        (yyval.node) = make_receiver_parameter_node(NULL, (yyvsp[-3].node), (yyvsp[-2].node), (yylsp[0]), (yyloc));
    }
#line 8262 "parser.tab.c"
    break;

  case 246: /* ReceiverParameter: UnannType THIS  */
#line 2405 "parser.y"
                     {
        (yyval.node) = make_receiver_parameter_node(NULL, (yyvsp[-1].node), NULL, (yylsp[0]), (yyloc));
    }
#line 8270 "parser.tab.c"
    break;

  case 247: /* FormalParameterList: FormalParameter  */
#line 2412 "parser.y"
                    {
        (yyval.node) = make_params_list((yyvsp[0].node), (yyloc));
    }
#line 8278 "parser.tab.c"
    break;

  case 248: /* FormalParameterList: FormalParameter ',' FormalParameterList  */
#line 2415 "parser.y"
                                              {
        ast_prepend_child((yyvsp[0].node), (yyvsp[-2].node));
        (yyval.node) = (yyvsp[0].node);
    }
#line 8287 "parser.tab.c"
    break;

  case 249: /* FormalParameter: VariableModifiers UnannType TYPE_IDENTIFIER Dims  */
#line 2426 "parser.y"
                                                     {
        (yyval.node) = make_parameter_with_mods((yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 8295 "parser.tab.c"
    break;

  case 250: /* FormalParameter: Annotations UnannType TYPE_IDENTIFIER Dims  */
#line 2429 "parser.y"
                                                 {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-3].node), (yylsp[-3]));
        (yyval.node) = make_parameter_with_mods(mods, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 8304 "parser.tab.c"
    break;

  case 251: /* FormalParameter: UnannType TYPE_IDENTIFIER Dims  */
#line 2433 "parser.y"
                                     {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 8312 "parser.tab.c"
    break;

  case 252: /* FormalParameter: VariableModifiers UnannType TYPE_IDENTIFIER  */
#line 2436 "parser.y"
                                                  {
        (yyval.node) = make_parameter_with_mods((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 8320 "parser.tab.c"
    break;

  case 253: /* FormalParameter: Annotations UnannType TYPE_IDENTIFIER  */
#line 2439 "parser.y"
                                            {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = make_parameter_with_mods(mods, (yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 8329 "parser.tab.c"
    break;

  case 254: /* FormalParameter: UnannType TYPE_IDENTIFIER  */
#line 2443 "parser.y"
                                {
        (yyval.node) = make_parameter_node((yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 8337 "parser.tab.c"
    break;

  case 255: /* FormalParameter: VariableModifiers UnannType IdentifierComplement Dims  */
#line 2446 "parser.y"
                                                            {
        (yyval.node) = make_parameter_with_mods((yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 8345 "parser.tab.c"
    break;

  case 256: /* FormalParameter: Annotations UnannType IdentifierComplement Dims  */
#line 2449 "parser.y"
                                                      {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-3].node), (yylsp[-3]));
        (yyval.node) = make_parameter_with_mods(mods, (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 8354 "parser.tab.c"
    break;

  case 257: /* FormalParameter: UnannType IdentifierComplement Dims  */
#line 2453 "parser.y"
                                          {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 8362 "parser.tab.c"
    break;

  case 258: /* FormalParameter: VariableModifiers UnannType IdentifierComplement  */
#line 2456 "parser.y"
                                                       {
        (yyval.node) = make_parameter_with_mods((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 8370 "parser.tab.c"
    break;

  case 259: /* FormalParameter: Annotations UnannType IdentifierComplement  */
#line 2459 "parser.y"
                                                 {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = make_parameter_with_mods(mods, (yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 8379 "parser.tab.c"
    break;

  case 260: /* FormalParameter: UnannType IdentifierComplement  */
#line 2463 "parser.y"
                                     {
        (yyval.node) = make_parameter_node((yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 8387 "parser.tab.c"
    break;

  case 261: /* FormalParameter: VariableArityParameter  */
#line 2466 "parser.y"
                             { (yyval.node) = (yyvsp[0].node); }
#line 8393 "parser.tab.c"
    break;

  case 262: /* FormalParameter: UnannType TYPE_IDENTIFIER DOT_CommonName TYPE_IDENTIFIER  */
#line 2467 "parser.y"
                                                               {
        yyerror("Invalid parameter syntax: unexpected '.' in parameter (did you mean String[] args?)");
        YYERROR;
    }
#line 8402 "parser.tab.c"
    break;

  case 263: /* FormalParameter: UnannType TYPE_IDENTIFIER '.' TYPE_IDENTIFIER  */
#line 2471 "parser.y"
                                                    {
        yyerror("Invalid parameter syntax: unexpected '.' in parameter");
        YYERROR;
    }
#line 8411 "parser.tab.c"
    break;

  case 264: /* VariableArityParameter: VariableModifiers UnannType Annotations ELLIPSIS TYPE_IDENTIFIER  */
#line 2479 "parser.y"
                                                                     {
        AstNode *mods = merge_param_modifiers((yyvsp[-4].node), (yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = mark_varargs_parameter(make_parameter_with_mods(mods, (yyvsp[-3].node), (yyvsp[0].node), NULL, (yyloc)));
    }
#line 8420 "parser.tab.c"
    break;

  case 265: /* VariableArityParameter: VariableModifiers UnannType ELLIPSIS TYPE_IDENTIFIER  */
#line 2483 "parser.y"
                                                           {
        (yyval.node) = mark_varargs_parameter(make_parameter_with_mods((yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[0].node), NULL, (yyloc)));
    }
#line 8428 "parser.tab.c"
    break;

  case 266: /* VariableArityParameter: Annotations UnannType Annotations ELLIPSIS TYPE_IDENTIFIER  */
#line 2486 "parser.y"
                                                                 {
        AstNode *mods = merge_param_modifiers(make_modifiers_from_annotations((yyvsp[-4].node), (yylsp[-4])), (yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = mark_varargs_parameter(make_parameter_with_mods(mods, (yyvsp[-3].node), (yyvsp[0].node), NULL, (yyloc)));
    }
#line 8437 "parser.tab.c"
    break;

  case 267: /* VariableArityParameter: Annotations UnannType ELLIPSIS TYPE_IDENTIFIER  */
#line 2490 "parser.y"
                                                     {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-3].node), (yylsp[-3]));
        (yyval.node) = mark_varargs_parameter(make_parameter_with_mods(mods, (yyvsp[-2].node), (yyvsp[0].node), NULL, (yyloc)));
    }
#line 8446 "parser.tab.c"
    break;

  case 268: /* VariableArityParameter: UnannType Annotations ELLIPSIS TYPE_IDENTIFIER  */
#line 2494 "parser.y"
                                                     {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = mark_varargs_parameter(make_parameter_with_mods(mods, (yyvsp[-3].node), (yyvsp[0].node), NULL, (yyloc)));
    }
#line 8455 "parser.tab.c"
    break;

  case 269: /* VariableArityParameter: UnannType ELLIPSIS TYPE_IDENTIFIER  */
#line 2498 "parser.y"
                                         {
        (yyval.node) = mark_varargs_parameter(make_parameter_node((yyvsp[-2].node), (yyvsp[0].node), (yyloc)));
    }
#line 8463 "parser.tab.c"
    break;

  case 270: /* VariableArityParameter: VariableModifiers UnannType Annotations ELLIPSIS IdentifierComplement  */
#line 2501 "parser.y"
                                                                            {
        AstNode *mods = merge_param_modifiers((yyvsp[-4].node), (yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = mark_varargs_parameter(make_parameter_with_mods(mods, (yyvsp[-3].node), (yyvsp[0].node), NULL, (yyloc)));
    }
#line 8472 "parser.tab.c"
    break;

  case 271: /* VariableArityParameter: VariableModifiers UnannType ELLIPSIS IdentifierComplement  */
#line 2505 "parser.y"
                                                                {
        (yyval.node) = mark_varargs_parameter(make_parameter_with_mods((yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[0].node), NULL, (yyloc)));
    }
#line 8480 "parser.tab.c"
    break;

  case 272: /* VariableArityParameter: Annotations UnannType Annotations ELLIPSIS IdentifierComplement  */
#line 2508 "parser.y"
                                                                      {
        AstNode *mods = merge_param_modifiers(make_modifiers_from_annotations((yyvsp[-4].node), (yylsp[-4])), (yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = mark_varargs_parameter(make_parameter_with_mods(mods, (yyvsp[-3].node), (yyvsp[0].node), NULL, (yyloc)));
    }
#line 8489 "parser.tab.c"
    break;

  case 273: /* VariableArityParameter: Annotations UnannType ELLIPSIS IdentifierComplement  */
#line 2512 "parser.y"
                                                          {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-3].node), (yylsp[-3]));
        (yyval.node) = mark_varargs_parameter(make_parameter_with_mods(mods, (yyvsp[-2].node), (yyvsp[0].node), NULL, (yyloc)));
    }
#line 8498 "parser.tab.c"
    break;

  case 274: /* VariableArityParameter: UnannType Annotations ELLIPSIS IdentifierComplement  */
#line 2516 "parser.y"
                                                          {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = mark_varargs_parameter(make_parameter_with_mods(mods, (yyvsp[-3].node), (yyvsp[0].node), NULL, (yyloc)));
    }
#line 8507 "parser.tab.c"
    break;

  case 275: /* VariableArityParameter: UnannType ELLIPSIS IdentifierComplement  */
#line 2520 "parser.y"
                                              {
        (yyval.node) = mark_varargs_parameter(make_parameter_node((yyvsp[-2].node), (yyvsp[0].node), (yyloc)));
    }
#line 8515 "parser.tab.c"
    break;

  case 276: /* VariableModifiers: FINAL  */
#line 2527 "parser.y"
          {
        (yyval.node) = make_list_node(AST_MODIFIER_LIST, (yyloc));
        ast_add_child((yyval.node), make_keyword_leaf("final", (yylsp[0])));
    }
#line 8524 "parser.tab.c"
    break;

  case 277: /* VariableModifiers: Annotations FINAL  */
#line 2531 "parser.y"
                        {
        (yyval.node) = make_list_node(AST_MODIFIER_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[-1].node));
        ast_add_child((yyval.node), make_keyword_leaf("final", (yylsp[0])));
    }
#line 8534 "parser.tab.c"
    break;

  case 278: /* VariableModifiers: FINAL Annotations  */
#line 2536 "parser.y"
                        {
        (yyval.node) = make_list_node(AST_MODIFIER_LIST, (yyloc));
        ast_add_child((yyval.node), make_keyword_leaf("final", (yylsp[-1])));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 8544 "parser.tab.c"
    break;

  case 279: /* VariableModifiers: Annotations FINAL Annotations  */
#line 2541 "parser.y"
                                    {
        (yyval.node) = make_list_node(AST_MODIFIER_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[-2].node));
        ast_add_child((yyval.node), make_keyword_leaf("final", (yylsp[-1])));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 8555 "parser.tab.c"
    break;

  case 280: /* Throws: THROWS ExceptionTypeList  */
#line 2551 "parser.y"
                             { (yyval.node) = make_throws_node((yyvsp[0].node), (yyloc)); }
#line 8561 "parser.tab.c"
    break;

  case 281: /* ExceptionTypeList: ExceptionType  */
#line 2556 "parser.y"
                  {
        (yyval.node) = make_list_node(AST_EXCEPTION_TYPE_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 8570 "parser.tab.c"
    break;

  case 282: /* ExceptionTypeList: ExceptionTypeList ',' ExceptionType  */
#line 2560 "parser.y"
                                          {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 8579 "parser.tab.c"
    break;

  case 284: /* MethodBody: Block  */
#line 2575 "parser.y"
          { (yyval.node) = (yyvsp[0].node); }
#line 8585 "parser.tab.c"
    break;

  case 285: /* MethodBody: ';'  */
#line 2576 "parser.y"
          { (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column); }
#line 8591 "parser.tab.c"
    break;

  case 286: /* InstanceInitializer: Block  */
#line 2581 "parser.y"
          { (yyval.node) = wrap_labeled_block("instance-init", (yyvsp[0].node), (yylsp[0])); }
#line 8597 "parser.tab.c"
    break;

  case 287: /* StaticInitializer: STATIC Block  */
#line 2586 "parser.y"
                 { (yyval.node) = wrap_labeled_block("static-init", (yyvsp[0].node), (yylsp[-1])); }
#line 8603 "parser.tab.c"
    break;

  case 288: /* ConstructorDeclaration: Modifiers ConstructorDeclarator Throws ConstructorBody  */
#line 2592 "parser.y"
                                                           {
        if ((yyvsp[-3].node)) {
            ast_prepend_child((yyvsp[-2].node), (yyvsp[-3].node));
        }
        if ((yyvsp[-1].node)) {
            ast_add_child((yyvsp[-2].node), (yyvsp[-1].node));
        }
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-2].node);
    }
#line 8620 "parser.tab.c"
    break;

  case 289: /* ConstructorDeclaration: Modifiers ConstructorDeclarator ConstructorBody  */
#line 2604 "parser.y"
                                                      {
        if ((yyvsp[-2].node)) {
            ast_prepend_child((yyvsp[-1].node), (yyvsp[-2].node));
        }
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 8634 "parser.tab.c"
    break;

  case 290: /* ConstructorDeclaration: ConstructorDeclarator Throws ConstructorBody  */
#line 2613 "parser.y"
                                                   {
        if ((yyvsp[-1].node)) {
            ast_add_child((yyvsp[-2].node), (yyvsp[-1].node));
        }
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-2].node);
    }
#line 8648 "parser.tab.c"
    break;

  case 291: /* ConstructorDeclaration: ConstructorDeclarator ConstructorBody  */
#line 2622 "parser.y"
                                            {
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 8659 "parser.tab.c"
    break;

  case 292: /* ConstructorDeclarator: TypeParameters SimpleTypeName '(' ReceiverParameter ',' FormalParameterList ')'  */
#line 2632 "parser.y"
                                                                                    {
        AstNode *params = (yyvsp[-1].node) ? (yyvsp[-1].node) : make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_prepend_child(params, (yyvsp[-3].node));
        AstNode *ctor = make_constructor_node((yyvsp[-5].node), params, (yyloc));
        if ((yyvsp[-6].node)) {
            ast_prepend_child(ctor, (yyvsp[-6].node));
        }
        (yyval.node) = ctor;
    }
#line 8673 "parser.tab.c"
    break;

  case 293: /* ConstructorDeclarator: TypeParameters SimpleTypeName '(' ReceiverParameter ')'  */
#line 2641 "parser.y"
                                                              {
        AstNode *ctor = make_constructor_node((yyvsp[-3].node), make_params_list((yyvsp[-1].node), (yyloc)), (yyloc));
        if ((yyvsp[-4].node)) {
            ast_prepend_child(ctor, (yyvsp[-4].node));
        }
        (yyval.node) = ctor;
    }
#line 8685 "parser.tab.c"
    break;

  case 294: /* ConstructorDeclarator: TypeParameters SimpleTypeName '(' FormalParameterList ')'  */
#line 2648 "parser.y"
                                                                {
        AstNode *ctor = make_constructor_node((yyvsp[-3].node), (yyvsp[-1].node), (yyloc));
        if ((yyvsp[-4].node)) {
            ast_prepend_child(ctor, (yyvsp[-4].node));
        }
        (yyval.node) = ctor;
    }
#line 8697 "parser.tab.c"
    break;

  case 295: /* ConstructorDeclarator: TypeParameters SimpleTypeName '(' ')'  */
#line 2655 "parser.y"
                                            {
        AstNode *ctor = make_constructor_node((yyvsp[-2].node), NULL, (yyloc));
        if ((yyvsp[-3].node)) {
            ast_prepend_child(ctor, (yyvsp[-3].node));
        }
        (yyval.node) = ctor;
    }
#line 8709 "parser.tab.c"
    break;

  case 296: /* ConstructorDeclarator: SimpleTypeName '(' ReceiverParameter ',' FormalParameterList ')'  */
#line 2662 "parser.y"
                                                                       {
        AstNode *params = (yyvsp[-1].node) ? (yyvsp[-1].node) : make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_prepend_child(params, (yyvsp[-3].node));
        (yyval.node) = make_constructor_node((yyvsp[-5].node), params, (yyloc));
    }
#line 8719 "parser.tab.c"
    break;

  case 297: /* ConstructorDeclarator: SimpleTypeName '(' ReceiverParameter ')'  */
#line 2667 "parser.y"
                                               {
        (yyval.node) = make_constructor_node((yyvsp[-3].node), make_params_list((yyvsp[-1].node), (yyloc)), (yyloc));
    }
#line 8727 "parser.tab.c"
    break;

  case 298: /* ConstructorDeclarator: SimpleTypeName '(' FormalParameterList ')'  */
#line 2670 "parser.y"
                                                 {
        (yyval.node) = make_constructor_node((yyvsp[-3].node), (yyvsp[-1].node), (yyloc));
    }
#line 8735 "parser.tab.c"
    break;

  case 299: /* ConstructorDeclarator: SimpleTypeName '(' ')'  */
#line 2673 "parser.y"
                             {
        (yyval.node) = make_constructor_node((yyvsp[-2].node), NULL, (yyloc));
    }
#line 8743 "parser.tab.c"
    break;

  case 300: /* SimpleTypeName: TYPE_IDENTIFIER_MethodDeclarator  */
#line 2680 "parser.y"
                                     { (yyval.node) = (yyvsp[0].node); }
#line 8749 "parser.tab.c"
    break;

  case 301: /* ConstructorBody: '{' ExplicitConstructorInvocation BlockStatements '}'  */
#line 2685 "parser.y"
                                                          {
        if ((yyvsp[-2].node)) {
            ast_prepend_child((yyvsp[-1].node), (yyvsp[-2].node));
        }
        (yyval.node) = make_block_node((yylsp[-3]), (yyvsp[-1].node));
    }
#line 8760 "parser.tab.c"
    break;

  case 302: /* ConstructorBody: '{' ExplicitConstructorInvocation '}'  */
#line 2691 "parser.y"
                                            {
        AstNode *stmts = make_list_node(AST_STATEMENT_LIST, (yylsp[-2]));
        if ((yyvsp[-1].node)) {
            ast_add_child(stmts, (yyvsp[-1].node));
        }
        (yyval.node) = make_block_node((yylsp[-2]), stmts);
    }
#line 8772 "parser.tab.c"
    break;

  case 303: /* ConstructorBody: '{' BlockStatements '}'  */
#line 2698 "parser.y"
                              {
        (yyval.node) = make_block_node((yylsp[-2]), (yyvsp[-1].node));
    }
#line 8780 "parser.tab.c"
    break;

  case 304: /* ConstructorBody: '{' '}'  */
#line 2701 "parser.y"
              {
        (yyval.node) = make_block_node((yylsp[-1]), NULL);
    }
#line 8788 "parser.tab.c"
    break;

  case 305: /* ExplicitConstructorInvocation: TypeArguments THIS '(' ArgumentList ')' ';'  */
#line 2708 "parser.y"
                                                {
        (yyval.node) = make_explicit_ctor_invocation(NULL, (yyvsp[-5].node), "this", (yyvsp[-2].node), (yylsp[-4]));
    }
#line 8796 "parser.tab.c"
    break;

  case 306: /* ExplicitConstructorInvocation: TypeArguments THIS '(' ')' ';'  */
#line 2711 "parser.y"
                                     {
        (yyval.node) = make_explicit_ctor_invocation(NULL, (yyvsp[-4].node), "this", NULL, (yylsp[-3]));
    }
#line 8804 "parser.tab.c"
    break;

  case 307: /* ExplicitConstructorInvocation: THIS '(' ArgumentList ')' ';'  */
#line 2714 "parser.y"
                                    {
        (yyval.node) = make_explicit_ctor_invocation(NULL, NULL, "this", (yyvsp[-2].node), (yylsp[-4]));
    }
#line 8812 "parser.tab.c"
    break;

  case 308: /* ExplicitConstructorInvocation: THIS '(' ')' ';'  */
#line 2717 "parser.y"
                       {
        (yyval.node) = make_explicit_ctor_invocation(NULL, NULL, "this", NULL, (yylsp[-3]));
    }
#line 8820 "parser.tab.c"
    break;

  case 309: /* ExplicitConstructorInvocation: TypeArguments SUPER '(' ArgumentList ')' ';'  */
#line 2720 "parser.y"
                                                   {
        (yyval.node) = make_explicit_ctor_invocation(NULL, (yyvsp[-5].node), "super", (yyvsp[-2].node), (yylsp[-4]));
    }
#line 8828 "parser.tab.c"
    break;

  case 310: /* ExplicitConstructorInvocation: TypeArguments SUPER '(' ')' ';'  */
#line 2723 "parser.y"
                                      {
        (yyval.node) = make_explicit_ctor_invocation(NULL, (yyvsp[-4].node), "super", NULL, (yylsp[-3]));
    }
#line 8836 "parser.tab.c"
    break;

  case 311: /* ExplicitConstructorInvocation: SUPER '(' ArgumentList ')' ';'  */
#line 2726 "parser.y"
                                     {
        (yyval.node) = make_explicit_ctor_invocation(NULL, NULL, "super", (yyvsp[-2].node), (yylsp[-4]));
    }
#line 8844 "parser.tab.c"
    break;

  case 312: /* ExplicitConstructorInvocation: SUPER '(' ')' ';'  */
#line 2729 "parser.y"
                        {
        (yyval.node) = make_explicit_ctor_invocation(NULL, NULL, "super", NULL, (yylsp[-3]));
    }
#line 8852 "parser.tab.c"
    break;

  case 313: /* ExplicitConstructorInvocation: CommonName '.' TypeArguments SUPER '(' ArgumentList ')' ';'  */
#line 2732 "parser.y"
                                                                  {
        (yyval.node) = make_explicit_ctor_invocation((yyvsp[-7].node), (yyvsp[-5].node), "super", (yyvsp[-2].node), (yylsp[-4]));
    }
#line 8860 "parser.tab.c"
    break;

  case 314: /* ExplicitConstructorInvocation: CommonName '.' TypeArguments SUPER '(' ')' ';'  */
#line 2735 "parser.y"
                                                     {
        (yyval.node) = make_explicit_ctor_invocation((yyvsp[-6].node), (yyvsp[-4].node), "super", NULL, (yylsp[-3]));
    }
#line 8868 "parser.tab.c"
    break;

  case 315: /* ExplicitConstructorInvocation: CommonName '.' SUPER '(' ArgumentList ')' ';'  */
#line 2738 "parser.y"
                                                    {
        (yyval.node) = make_explicit_ctor_invocation((yyvsp[-6].node), NULL, "super", (yyvsp[-2].node), (yylsp[-4]));
    }
#line 8876 "parser.tab.c"
    break;

  case 316: /* ExplicitConstructorInvocation: CommonName '.' SUPER '(' ')' ';'  */
#line 2741 "parser.y"
                                       {
        (yyval.node) = make_explicit_ctor_invocation((yyvsp[-5].node), NULL, "super", NULL, (yylsp[-3]));
    }
#line 8884 "parser.tab.c"
    break;

  case 317: /* ExplicitConstructorInvocation: Primary '.' TypeArguments SUPER '(' ArgumentList ')' ';'  */
#line 2744 "parser.y"
                                                               {
        (yyval.node) = make_explicit_ctor_invocation((yyvsp[-7].node), (yyvsp[-5].node), "super", (yyvsp[-2].node), (yylsp[-4]));
    }
#line 8892 "parser.tab.c"
    break;

  case 318: /* ExplicitConstructorInvocation: Primary '.' TypeArguments SUPER '(' ')' ';'  */
#line 2747 "parser.y"
                                                  {
        (yyval.node) = make_explicit_ctor_invocation((yyvsp[-6].node), (yyvsp[-4].node), "super", NULL, (yylsp[-3]));
    }
#line 8900 "parser.tab.c"
    break;

  case 319: /* ExplicitConstructorInvocation: Primary '.' SUPER '(' ArgumentList ')' ';'  */
#line 2750 "parser.y"
                                                 {
        (yyval.node) = make_explicit_ctor_invocation((yyvsp[-6].node), NULL, "super", (yyvsp[-2].node), (yylsp[-4]));
    }
#line 8908 "parser.tab.c"
    break;

  case 320: /* ExplicitConstructorInvocation: Primary '.' SUPER '(' ')' ';'  */
#line 2753 "parser.y"
                                    {
        (yyval.node) = make_explicit_ctor_invocation((yyvsp[-5].node), NULL, "super", NULL, (yylsp[-3]));
    }
#line 8916 "parser.tab.c"
    break;

  case 321: /* EnumDeclaration: Modifiers ENUM TYPE_IDENTIFIER_EnumDeclaration Superinterfaces EnumBody  */
#line 2760 "parser.y"
                                                                            {
        /* 4 个孩? 修饰? 名字, superinterfaces, body */
        (yyval.node) = ast_branch(AST_ENUM_DECL,
                        (yylsp[-3]).first_line, (yylsp[-3]).first_column,
                        4,
                        (yyvsp[-4].node),     /* modifiers */
                        (yyvsp[-2].node),     /* name */
                        (yyvsp[-1].node),     /* superinterfaces */
                        (yyvsp[0].node));    /* body (EnumBody) */
    }
#line 8931 "parser.tab.c"
    break;

  case 322: /* EnumDeclaration: Modifiers ENUM TYPE_IDENTIFIER_EnumDeclaration EnumBody  */
#line 2770 "parser.y"
                                                            {
        (yyval.node) = ast_branch(AST_ENUM_DECL,
                        (yylsp[-2]).first_line, (yylsp[-2]).first_column,
                        4,
                        (yyvsp[-3].node),     /* modifiers */
                        (yyvsp[-1].node),     /* name */
                        NULL,   /* no superinterfaces */
                        (yyvsp[0].node));    /* body */
    }
#line 8945 "parser.tab.c"
    break;

  case 323: /* EnumDeclaration: ENUM TYPE_IDENTIFIER_EnumDeclaration Superinterfaces EnumBody  */
#line 2779 "parser.y"
                                                                  {
        (yyval.node) = ast_branch(AST_ENUM_DECL,
                        (yylsp[-3]).first_line, (yylsp[-3]).first_column,
                        4,
                        NULL,   /* no modifiers */
                        (yyvsp[-2].node),     /* name */
                        (yyvsp[-1].node),     /* superinterfaces */
                        (yyvsp[0].node));    /* body */
    }
#line 8959 "parser.tab.c"
    break;

  case 324: /* EnumDeclaration: ENUM TYPE_IDENTIFIER_EnumDeclaration EnumBody  */
#line 2788 "parser.y"
                                                  {
        (yyval.node) = ast_branch(AST_ENUM_DECL,
                        (yylsp[-2]).first_line, (yylsp[-2]).first_column,
                        4,
                        NULL,   /* no modifiers */
                        (yyvsp[-1].node),     /* name */
                        NULL,   /* no superinterfaces */
                        (yyvsp[0].node));    /* body */
    }
#line 8973 "parser.tab.c"
    break;

  case 325: /* EnumBody: '{' EnumConstantList ',' EnumBodyDeclarations '}'  */
#line 2802 "parser.y"
                                                      {
        (yyval.node) = ast_branch(AST_BLOCK, (yylsp[-4]).first_line, (yylsp[-4]).first_column, 0);
        ast_add_child((yyval.node), (yyvsp[-3].node));
        ast_add_child((yyval.node), (yyvsp[-1].node));
    }
#line 8983 "parser.tab.c"
    break;

  case 326: /* EnumBody: '{' EnumConstantList EnumBodyDeclarations '}'  */
#line 2807 "parser.y"
                                                    {
        (yyval.node) = ast_branch(AST_BLOCK, (yylsp[-3]).first_line, (yylsp[-3]).first_column, 0);
        ast_add_child((yyval.node), (yyvsp[-2].node));
        ast_add_child((yyval.node), (yyvsp[-1].node));
    }
#line 8993 "parser.tab.c"
    break;

  case 327: /* EnumBody: '{' EnumConstantList '}'  */
#line 2812 "parser.y"
                               {
        (yyval.node) = ast_branch(AST_BLOCK, (yylsp[-2]).first_line, (yylsp[-2]).first_column, 1, (yyvsp[-1].node));
    }
#line 9001 "parser.tab.c"
    break;

  case 328: /* EnumBody: '{' EnumConstantList ',' '}'  */
#line 2815 "parser.y"
                                   {
        (yyval.node) = ast_branch(AST_BLOCK, (yylsp[-3]).first_line, (yylsp[-3]).first_column, 1, (yyvsp[-2].node));
    }
#line 9009 "parser.tab.c"
    break;

  case 329: /* EnumBody: '{' EnumBodyDeclarations '}'  */
#line 2819 "parser.y"
                                   {
        (yyval.node) = ast_branch(AST_BLOCK, (yylsp[-2]).first_line, (yylsp[-2]).first_column, 1, (yyvsp[-1].node));
    }
#line 9017 "parser.tab.c"
    break;

  case 330: /* EnumBody: '{' '}'  */
#line 2822 "parser.y"
              {
        (yyval.node) = ast_branch(AST_BLOCK, (yylsp[-1]).first_line, (yylsp[-1]).first_column, 0);
    }
#line 9025 "parser.tab.c"
    break;

  case 331: /* EnumConstantList: EnumConstant  */
#line 2829 "parser.y"
                 {
        (yyval.node) = ast_branch(AST_STATEMENT_LIST, (yylsp[0]).first_line, (yylsp[0]).first_column, 1, (yyvsp[0].node));
    }
#line 9033 "parser.tab.c"
    break;

  case 332: /* EnumConstantList: EnumConstantList ',' EnumConstant  */
#line 2832 "parser.y"
                                        {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 9042 "parser.tab.c"
    break;

  case 333: /* EnumConstant: Annotations SimpleTypeName '(' ArgumentList ')' ClassBody  */
#line 2842 "parser.y"
                                                              {
        (yyval.node) = make_enum_constant((yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yyloc));
    }
#line 9050 "parser.tab.c"
    break;

  case 334: /* EnumConstant: Annotations SimpleTypeName '(' ArgumentList ')'  */
#line 2845 "parser.y"
                                                      {
        (yyval.node) = make_enum_constant((yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-1].node), NULL, (yyloc));
    }
#line 9058 "parser.tab.c"
    break;

  case 335: /* EnumConstant: Annotations TYPE_IDENTIFIER ClassBody  */
#line 2848 "parser.y"
                                            {
        (yyval.node) = make_enum_constant((yyvsp[-2].node), (yyvsp[-1].node), NULL, (yyvsp[0].node), (yyloc));
    }
#line 9066 "parser.tab.c"
    break;

  case 336: /* EnumConstant: Annotations TYPE_IDENTIFIER  */
#line 2851 "parser.y"
                                  {
        (yyval.node) = make_enum_constant((yyvsp[-1].node), (yyvsp[0].node), NULL, NULL, (yyloc));
    }
#line 9074 "parser.tab.c"
    break;

  case 337: /* EnumConstant: SimpleTypeName '(' ArgumentList ')' ClassBody  */
#line 2854 "parser.y"
                                                    {
        (yyval.node) = make_enum_constant(NULL, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yyloc));
    }
#line 9082 "parser.tab.c"
    break;

  case 338: /* EnumConstant: SimpleTypeName '(' ArgumentList ')'  */
#line 2857 "parser.y"
                                          {
        (yyval.node) = make_enum_constant(NULL, (yyvsp[-3].node), (yyvsp[-1].node), NULL, (yyloc));
    }
#line 9090 "parser.tab.c"
    break;

  case 339: /* EnumConstant: TYPE_IDENTIFIER ClassBody  */
#line 2860 "parser.y"
                                {
        (yyval.node) = make_enum_constant(NULL, (yyvsp[-1].node), NULL, (yyvsp[0].node), (yyloc));
    }
#line 9098 "parser.tab.c"
    break;

  case 340: /* EnumConstant: TYPE_IDENTIFIER  */
#line 2863 "parser.y"
                      {
        (yyval.node) = make_enum_constant(NULL, (yyvsp[0].node), NULL, NULL, (yyloc));
    }
#line 9106 "parser.tab.c"
    break;

  case 341: /* EnumConstant: Annotations IdentifierComplement '(' ArgumentList ')' ClassBody  */
#line 2866 "parser.y"
                                                                      {
        (yyval.node) = make_enum_constant((yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yyloc));
    }
#line 9114 "parser.tab.c"
    break;

  case 342: /* EnumConstant: Annotations IdentifierComplement '(' ArgumentList ')'  */
#line 2869 "parser.y"
                                                            {
        (yyval.node) = make_enum_constant((yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-1].node), NULL, (yyloc));
    }
#line 9122 "parser.tab.c"
    break;

  case 343: /* EnumConstant: Annotations IdentifierComplement ClassBody  */
#line 2872 "parser.y"
                                                 {
        (yyval.node) = make_enum_constant((yyvsp[-2].node), (yyvsp[-1].node), NULL, (yyvsp[0].node), (yyloc));
    }
#line 9130 "parser.tab.c"
    break;

  case 344: /* EnumConstant: Annotations IdentifierComplement  */
#line 2875 "parser.y"
                                       {
        (yyval.node) = make_enum_constant((yyvsp[-1].node), (yyvsp[0].node), NULL, NULL, (yyloc));
    }
#line 9138 "parser.tab.c"
    break;

  case 345: /* EnumConstant: IdentifierComplement '(' ArgumentList ')' ClassBody  */
#line 2878 "parser.y"
                                                          {
        (yyval.node) = make_enum_constant(NULL, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yyloc));
    }
#line 9146 "parser.tab.c"
    break;

  case 346: /* EnumConstant: IdentifierComplement '(' ArgumentList ')'  */
#line 2881 "parser.y"
                                                {
        (yyval.node) = make_enum_constant(NULL, (yyvsp[-3].node), (yyvsp[-1].node), NULL, (yyloc));
    }
#line 9154 "parser.tab.c"
    break;

  case 347: /* EnumConstant: IdentifierComplement ClassBody  */
#line 2884 "parser.y"
                                     {
        (yyval.node) = make_enum_constant(NULL, (yyvsp[-1].node), NULL, (yyvsp[0].node), (yyloc));
    }
#line 9162 "parser.tab.c"
    break;

  case 348: /* EnumConstant: IdentifierComplement  */
#line 2887 "parser.y"
                           {
        (yyval.node) = make_enum_constant(NULL, (yyvsp[0].node), NULL, NULL, (yyloc));
    }
#line 9170 "parser.tab.c"
    break;

  case 349: /* EnumBodyDeclarations: ';' ClassBodyDeclarationList  */
#line 2894 "parser.y"
                                 {
        (yyval.node) = (yyvsp[0].node);
    }
#line 9178 "parser.tab.c"
    break;

  case 350: /* InterfaceDeclaration: NormalInterfaceDeclaration  */
#line 2908 "parser.y"
                               { (yyval.node) = (yyvsp[0].node); }
#line 9184 "parser.tab.c"
    break;

  case 351: /* InterfaceDeclaration: AnnotationTypeDeclaration  */
#line 2909 "parser.y"
                                {
        (yyval.node) = (yyvsp[0].node);
    }
#line 9192 "parser.tab.c"
    break;

  case 352: /* NormalInterfaceDeclaration: Modifiers INTERFACE TYPE_IDENTIFIER OptTypeParameters OptExtendsInterfaces OptClassOrInterfacePermits InterfaceBody  */
#line 2916 "parser.y"
                                                                                                                        {
        (yyval.node) = make_interface_basic((yylsp[-5]).first_line, (yylsp[-5]).first_column, 
                                  (yyvsp[-6].node), /* Modifiers */
                                  (yyvsp[-4].node), /* Name */
                                  (yyvsp[-3].node), /* TypeParams */
                                  (yyvsp[-2].node), /* Extends */
                                  (yyvsp[-1].node), /* Permits */
                                  (yyvsp[0].node));/* Body */
    }
#line 9206 "parser.tab.c"
    break;

  case 353: /* NormalInterfaceDeclaration: INTERFACE TYPE_IDENTIFIER OptTypeParameters OptExtendsInterfaces OptClassOrInterfacePermits InterfaceBody  */
#line 2925 "parser.y"
                                                                                                                {
        (yyval.node) = make_interface_basic((yylsp[-5]).first_line, (yylsp[-5]).first_column, 
                                  NULL, 
                                  (yyvsp[-4].node), 
                                  (yyvsp[-3].node), 
                                  (yyvsp[-2].node), 
                                  (yyvsp[-1].node), 
                                  (yyvsp[0].node));
    }
#line 9220 "parser.tab.c"
    break;

  case 354: /* ExtendsInterfaces: EXTENDS InterfaceTypeList  */
#line 2939 "parser.y"
                              {
        (yyval.node) = (yyvsp[0].node);
    }
#line 9228 "parser.tab.c"
    break;

  case 355: /* OptExtendsInterfaces: ExtendsInterfaces  */
#line 2946 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 9234 "parser.tab.c"
    break;

  case 356: /* OptExtendsInterfaces: %empty  */
#line 2947 "parser.y"
                  { (yyval.node) = NULL; }
#line 9240 "parser.tab.c"
    break;

  case 357: /* InterfaceBody: '{' InterfaceMemberDeclarationList '}'  */
#line 2952 "parser.y"
                                           { (yyval.node) = (yyvsp[-1].node); }
#line 9246 "parser.tab.c"
    break;

  case 358: /* InterfaceBody: '{' '}'  */
#line 2953 "parser.y"
              { (yyval.node) = ast_branch(AST_BLOCK, (yylsp[-1]).first_line, (yylsp[-1]).first_column, 0); }
#line 9252 "parser.tab.c"
    break;

  case 359: /* InterfaceMemberDeclarationList: InterfaceMemberDeclaration  */
#line 2958 "parser.y"
                               {
        (yyval.node) = ast_branch(AST_BLOCK, (yylsp[0]).first_line, (yylsp[0]).first_column, 1, (yyvsp[0].node));
    }
#line 9260 "parser.tab.c"
    break;

  case 360: /* InterfaceMemberDeclarationList: InterfaceMemberDeclarationList InterfaceMemberDeclaration  */
#line 2961 "parser.y"
                                                                {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 9269 "parser.tab.c"
    break;

  case 361: /* InterfaceMemberDeclaration: ConstantDeclaration  */
#line 2969 "parser.y"
                        { (yyval.node) = (yyvsp[0].node); }
#line 9275 "parser.tab.c"
    break;

  case 362: /* InterfaceMemberDeclaration: InterfaceMethodDeclaration  */
#line 2970 "parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 9281 "parser.tab.c"
    break;

  case 363: /* InterfaceMemberDeclaration: ClassDeclaration  */
#line 2971 "parser.y"
                       { (yyval.node) = (yyvsp[0].node); }
#line 9287 "parser.tab.c"
    break;

  case 364: /* InterfaceMemberDeclaration: InterfaceDeclaration  */
#line 2972 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 9293 "parser.tab.c"
    break;

  case 365: /* InterfaceMemberDeclaration: ';'  */
#line 2973 "parser.y"
          { (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column); }
#line 9299 "parser.tab.c"
    break;

  case 366: /* InterfaceMemberDeclaration: EMPTY_STMT  */
#line 2974 "parser.y"
                 { (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column); }
#line 9305 "parser.tab.c"
    break;

  case 367: /* ConstantDeclaration: Modifiers UnannType VariableDeclaratorList ';'  */
#line 2979 "parser.y"
                                                   {
        (yyval.node) = make_field_node((yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yylsp[-2]));
    }
#line 9313 "parser.tab.c"
    break;

  case 368: /* ConstantDeclaration: UnannType VariableDeclaratorList ';'  */
#line 2982 "parser.y"
                                         {
        (yyval.node) = make_field_node(NULL, (yyvsp[-2].node), (yyvsp[-1].node), (yylsp[-2]));
    }
#line 9321 "parser.tab.c"
    break;

  case 369: /* InterfaceMethodDeclaration: Modifiers MethodHeader MethodBody  */
#line 2990 "parser.y"
                                      {
        if ((yyvsp[-2].node)) {
            ast_prepend_child((yyvsp[-1].node), (yyvsp[-2].node));
        }
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 9335 "parser.tab.c"
    break;

  case 370: /* InterfaceMethodDeclaration: MethodHeader MethodBody  */
#line 2999 "parser.y"
                              {
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 9346 "parser.tab.c"
    break;

  case 371: /* AnnotationTypeDeclaration: Modifiers AT_AnnotationTypeDeclaration INTERFACE TYPE_IDENTIFIER AnnotationTypeBody  */
#line 3011 "parser.y"
                                                                                        {
        (yyval.node) = make_annotation_decl_basic((yylsp[-2]).first_line, (yylsp[-2]).first_column, (yyvsp[-4].node), (yyvsp[-1].node), NULL, NULL, NULL, (yyvsp[0].node) ? (yyvsp[0].node) : ast_branch(AST_BLOCK, (yylsp[-2]).first_line, (yylsp[-2]).first_column, 0));
    }
#line 9354 "parser.tab.c"
    break;

  case 372: /* AnnotationTypeDeclaration: AT_AnnotationTypeDeclaration INTERFACE TYPE_IDENTIFIER AnnotationTypeBody  */
#line 3014 "parser.y"
                                                                                {
        (yyval.node) = make_annotation_decl_basic((yylsp[-2]).first_line, (yylsp[-2]).first_column, NULL, (yyvsp[-1].node), NULL, NULL, NULL, (yyvsp[0].node) ? (yyvsp[0].node) : ast_branch(AST_BLOCK, (yylsp[-2]).first_line, (yylsp[-2]).first_column, 0));
    }
#line 9362 "parser.tab.c"
    break;

  case 373: /* AnnotationTypeDeclaration: Modifiers AT_Modifier INTERFACE TYPE_IDENTIFIER AnnotationTypeBody  */
#line 3018 "parser.y"
                                                                         {
        (yyval.node) = make_annotation_decl_basic((yylsp[-2]).first_line, (yylsp[-2]).first_column, (yyvsp[-4].node), (yyvsp[-1].node), NULL, NULL, NULL, (yyvsp[0].node) ? (yyvsp[0].node) : ast_branch(AST_BLOCK, (yylsp[-2]).first_line, (yylsp[-2]).first_column, 0));
    }
#line 9370 "parser.tab.c"
    break;

  case 374: /* AnnotationTypeDeclaration: AT_Modifier INTERFACE TYPE_IDENTIFIER AnnotationTypeBody  */
#line 3021 "parser.y"
                                                               {
        (yyval.node) = make_annotation_decl_basic((yylsp[-2]).first_line, (yylsp[-2]).first_column, NULL, (yyvsp[-1].node), NULL, NULL, NULL, (yyvsp[0].node) ? (yyvsp[0].node) : ast_branch(AST_BLOCK, (yylsp[-2]).first_line, (yylsp[-2]).first_column, 0));
    }
#line 9378 "parser.tab.c"
    break;

  case 375: /* AnnotationTypeDeclaration: Modifiers '@' INTERFACE TYPE_IDENTIFIER AnnotationTypeBody  */
#line 3025 "parser.y"
                                                                 {
         (yyval.node) = make_annotation_decl_basic((yylsp[-2]).first_line, (yylsp[-2]).first_column, (yyvsp[-4].node), (yyvsp[-1].node), NULL, NULL, NULL, (yyvsp[0].node) ? (yyvsp[0].node) : ast_branch(AST_BLOCK, (yylsp[-2]).first_line, (yylsp[-2]).first_column, 0));
    }
#line 9386 "parser.tab.c"
    break;

  case 376: /* AnnotationTypeDeclaration: '@' INTERFACE TYPE_IDENTIFIER AnnotationTypeBody  */
#line 3028 "parser.y"
                                                       {
         (yyval.node) = make_annotation_decl_basic((yylsp[-2]).first_line, (yylsp[-2]).first_column, NULL, (yyvsp[-1].node), NULL, NULL, NULL, (yyvsp[0].node) ? (yyvsp[0].node) : ast_branch(AST_BLOCK, (yylsp[-2]).first_line, (yylsp[-2]).first_column, 0));
    }
#line 9394 "parser.tab.c"
    break;

  case 377: /* AnnotationTypeBody: '{' AnnotationTypeMemberDeclarationList '}'  */
#line 3034 "parser.y"
                                                {
        (yyval.node) = (yyvsp[-1].node);
    }
#line 9402 "parser.tab.c"
    break;

  case 378: /* AnnotationTypeBody: '{' '}'  */
#line 3037 "parser.y"
              {
        (yyval.node) = ast_branch(AST_BLOCK, (yylsp[-1]).first_line, (yylsp[-1]).first_column, 0);
    }
#line 9410 "parser.tab.c"
    break;

  case 379: /* AnnotationTypeMemberDeclarationList: AnnotationTypeMemberDeclaration  */
#line 3044 "parser.y"
                                    {
        (yyval.node) = ast_branch(AST_BLOCK, (yylsp[0]).first_line, (yylsp[0]).first_column, 1, (yyvsp[0].node));
    }
#line 9418 "parser.tab.c"
    break;

  case 380: /* AnnotationTypeMemberDeclarationList: AnnotationTypeMemberDeclarationList AnnotationTypeMemberDeclaration  */
#line 3047 "parser.y"
                                                                          {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 9427 "parser.tab.c"
    break;

  case 381: /* AnnotationTypeMemberDeclaration: AnnotationTypeElementDeclaration  */
#line 3055 "parser.y"
                                     { (yyval.node) = (yyvsp[0].node); }
#line 9433 "parser.tab.c"
    break;

  case 382: /* AnnotationTypeMemberDeclaration: ConstantDeclaration  */
#line 3056 "parser.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 9439 "parser.tab.c"
    break;

  case 383: /* AnnotationTypeMemberDeclaration: ClassDeclaration  */
#line 3057 "parser.y"
                       { (yyval.node) = (yyvsp[0].node); }
#line 9445 "parser.tab.c"
    break;

  case 384: /* AnnotationTypeMemberDeclaration: InterfaceDeclaration  */
#line 3058 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 9451 "parser.tab.c"
    break;

  case 385: /* AnnotationTypeMemberDeclaration: ';'  */
#line 3059 "parser.y"
          { (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column); }
#line 9457 "parser.tab.c"
    break;

  case 386: /* AnnotationTypeMemberDeclaration: EMPTY_STMT  */
#line 3060 "parser.y"
                 { (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column); }
#line 9463 "parser.tab.c"
    break;

  case 387: /* AnnotationTypeElementDeclaration: Modifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' Dims DefaultValue ';'  */
#line 3069 "parser.y"
                                                                                  {
        (yyval.node) = make_annotation_element_decl((yyvsp[-7].node), (yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyloc));
    }
#line 9471 "parser.tab.c"
    break;

  case 388: /* AnnotationTypeElementDeclaration: Modifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' Dims ';'  */
#line 3072 "parser.y"
                                                                       {
        (yyval.node) = make_annotation_element_decl((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-1].node), NULL, (yyloc));
    }
#line 9479 "parser.tab.c"
    break;

  case 389: /* AnnotationTypeElementDeclaration: Modifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' DefaultValue ';'  */
#line 3075 "parser.y"
                                                                               {
        (yyval.node) = make_annotation_element_decl((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyvsp[-1].node), (yyloc));
    }
#line 9487 "parser.tab.c"
    break;

  case 390: /* AnnotationTypeElementDeclaration: Modifiers UnannType IDENTIFIER_MethodDeclarator '(' ')' ';'  */
#line 3078 "parser.y"
                                                                  {
        (yyval.node) = make_annotation_element_decl((yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), NULL, NULL, (yyloc));
    }
#line 9495 "parser.tab.c"
    break;

  case 391: /* AnnotationTypeElementDeclaration: UnannType IDENTIFIER_MethodDeclarator '(' ')' Dims DefaultValue ';'  */
#line 3081 "parser.y"
                                                                          {
        (yyval.node) = make_annotation_element_decl(NULL, (yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyloc));
    }
#line 9503 "parser.tab.c"
    break;

  case 392: /* AnnotationTypeElementDeclaration: UnannType IDENTIFIER_MethodDeclarator '(' ')' Dims ';'  */
#line 3084 "parser.y"
                                                             {
        (yyval.node) = make_annotation_element_decl(NULL, (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-1].node), NULL, (yyloc));
    }
#line 9511 "parser.tab.c"
    break;

  case 393: /* AnnotationTypeElementDeclaration: UnannType IDENTIFIER_MethodDeclarator '(' ')' DefaultValue ';'  */
#line 3087 "parser.y"
                                                                     {
        (yyval.node) = make_annotation_element_decl(NULL, (yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyvsp[-1].node), (yyloc));
    }
#line 9519 "parser.tab.c"
    break;

  case 394: /* AnnotationTypeElementDeclaration: UnannType IDENTIFIER_MethodDeclarator '(' ')' ';'  */
#line 3090 "parser.y"
                                                        {
        (yyval.node) = make_annotation_element_decl(NULL, (yyvsp[-4].node), (yyvsp[-3].node), NULL, NULL, (yyloc));
    }
#line 9527 "parser.tab.c"
    break;

  case 395: /* DefaultValue: DEFAULT ElementValue  */
#line 3097 "parser.y"
                         {
        (yyval.node) = AST_BRANCH_AT(AST_DEFAULT_VALUE, (yylsp[-1]), 1, (yyvsp[0].node));
    }
#line 9535 "parser.tab.c"
    break;

  case 396: /* Annotation: NormalAnnotation  */
#line 3104 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 9541 "parser.tab.c"
    break;

  case 397: /* Annotation: MarkerAnnotation  */
#line 3105 "parser.y"
                       { (yyval.node) = (yyvsp[0].node); }
#line 9547 "parser.tab.c"
    break;

  case 398: /* Annotation: SingleElementAnnotation  */
#line 3106 "parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 9553 "parser.tab.c"
    break;

  case 399: /* Annotations: Annotations Annotation  */
#line 3111 "parser.y"
                           {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 9562 "parser.tab.c"
    break;

  case 400: /* Annotations: Annotation  */
#line 3115 "parser.y"
                 {
        (yyval.node) = make_list_node(AST_ANNOTATION_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 9571 "parser.tab.c"
    break;

  case 401: /* NormalAnnotation: '@' TypeName_ModifierOrDims '(' ElementValuePairList ')'  */
#line 3123 "parser.y"
                                                             {
        AstNode *node = make_annotation_node((yyvsp[-3].node), (yylsp[-4]));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 9581 "parser.tab.c"
    break;

  case 402: /* NormalAnnotation: '@' TypeName_ModifierOrDims '(' ')'  */
#line 3128 "parser.y"
                                          {
        (yyval.node) = make_annotation_node((yyvsp[-2].node), (yylsp[-3]));
    }
#line 9589 "parser.tab.c"
    break;

  case 403: /* ElementValuePairList: ElementValuePair  */
#line 3135 "parser.y"
                     {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 9598 "parser.tab.c"
    break;

  case 404: /* ElementValuePairList: ElementValuePairList ',' ElementValuePair  */
#line 3139 "parser.y"
                                                {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 9607 "parser.tab.c"
    break;

  case 405: /* ElementValuePair: TYPE_IDENTIFIER '=' ElementValue  */
#line 3147 "parser.y"
                                     {
        AstNode *node = AST_BRANCH_AT(AST_ASSIGN, (yylsp[-1]), 0);
        ast_add_child(node, (yyvsp[-2].node));
        ast_add_child(node, (yyvsp[0].node));
        (yyval.node) = node;
    }
#line 9618 "parser.tab.c"
    break;

  case 406: /* ElementValuePair: IdentifierComplement '=' ElementValue  */
#line 3153 "parser.y"
                                            {
        AstNode *node = AST_BRANCH_AT(AST_ASSIGN, (yylsp[-1]), 0);
        ast_add_child(node, (yyvsp[-2].node));
        ast_add_child(node, (yyvsp[0].node));
        (yyval.node) = node;
    }
#line 9629 "parser.tab.c"
    break;

  case 408: /* ElementValue: ElementValueArrayInitializer  */
#line 3165 "parser.y"
                                   { (yyval.node) = (yyvsp[0].node); }
#line 9635 "parser.tab.c"
    break;

  case 409: /* ElementValue: Annotation  */
#line 3166 "parser.y"
                 { (yyval.node) = (yyvsp[0].node); }
#line 9641 "parser.tab.c"
    break;

  case 410: /* ElementValueArrayInitializer: '{' ElementValueList ',' '}'  */
#line 3171 "parser.y"
                                 {
        (yyval.node) = make_array_initializer_node((yyvsp[-2].node), (yylsp[-3]));
    }
#line 9649 "parser.tab.c"
    break;

  case 411: /* ElementValueArrayInitializer: '{' ElementValueList '}'  */
#line 3175 "parser.y"
                               {
        (yyval.node) = make_array_initializer_node((yyvsp[-1].node), (yylsp[-2]));
    }
#line 9657 "parser.tab.c"
    break;

  case 412: /* ElementValueArrayInitializer: '{' '}'  */
#line 3178 "parser.y"
              {
        (yyval.node) = make_array_initializer_node(NULL, (yylsp[-1]));
    }
#line 9665 "parser.tab.c"
    break;

  case 413: /* ElementValueList: ElementValue  */
#line 3185 "parser.y"
                 {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 9674 "parser.tab.c"
    break;

  case 414: /* ElementValueList: ElementValueList ',' ElementValue  */
#line 3189 "parser.y"
                                        {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 9683 "parser.tab.c"
    break;

  case 415: /* MarkerAnnotation: '@' TypeName  */
#line 3197 "parser.y"
                 {
        (yyval.node) = make_annotation_node((yyvsp[0].node), (yylsp[-1]));
    }
#line 9691 "parser.tab.c"
    break;

  case 416: /* SingleElementAnnotation: '@' TypeName_ModifierOrDims '(' ElementValue ')'  */
#line 3204 "parser.y"
                                                     {
        AstNode *node = make_annotation_node((yyvsp[-3].node), (yylsp[-4]));
        AstNode *args = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child(args, (yyvsp[-1].node));
        ast_add_child(node, args);
        (yyval.node) = node;
    }
#line 9703 "parser.tab.c"
    break;

  case 417: /* Block: '{' BlockStatements '}'  */
#line 3219 "parser.y"
                            {
        (yyval.node) = make_block_node((yylsp[-2]), (yyvsp[-1].node));
    }
#line 9711 "parser.tab.c"
    break;

  case 418: /* Block: '{' '}'  */
#line 3222 "parser.y"
              {
        (yyval.node) = make_block_node((yylsp[-1]), NULL);
    }
#line 9719 "parser.tab.c"
    break;

  case 419: /* BlockStatements: BlockStatement  */
#line 3229 "parser.y"
                   {
        (yyval.node) = make_list_node(AST_STATEMENT_LIST, (yyloc));
        if ((yyvsp[0].node)) {
            ast_add_child((yyval.node), (yyvsp[0].node));
        }
    }
#line 9730 "parser.tab.c"
    break;

  case 420: /* BlockStatements: BlockStatements BlockStatement  */
#line 3235 "parser.y"
                                     {
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 9741 "parser.tab.c"
    break;

  case 421: /* BlockStatement: LocalVariableDeclarationStatement  */
#line 3245 "parser.y"
                                      { (yyval.node) = (yyvsp[0].node); }
#line 9747 "parser.tab.c"
    break;

  case 422: /* BlockStatement: LocalClassOrInterfaceDeclaration  */
#line 3246 "parser.y"
                                       { (yyval.node) = (yyvsp[0].node); }
#line 9753 "parser.tab.c"
    break;

  case 423: /* BlockStatement: Statement  */
#line 3247 "parser.y"
                { (yyval.node) = (yyvsp[0].node); }
#line 9759 "parser.tab.c"
    break;

  case 424: /* LocalVariableDeclarationStatement: LocalVariableDeclaration ';'  */
#line 3252 "parser.y"
                                 { (yyval.node) = (yyvsp[-1].node); }
#line 9765 "parser.tab.c"
    break;

  case 425: /* LocalVariableDeclaration: VariableModifiers LocalVariableType VariableDeclaratorList  */
#line 3257 "parser.y"
                                                               {
        (yyval.node) = make_local_variable_node((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-1]));
    }
#line 9773 "parser.tab.c"
    break;

  case 426: /* LocalVariableDeclaration: Annotations LocalVariableType VariableDeclaratorList  */
#line 3260 "parser.y"
                                                           {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = make_local_variable_node(mods, (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-1]));
    }
#line 9782 "parser.tab.c"
    break;

  case 427: /* LocalVariableDeclaration: LocalVariableType VariableDeclaratorList  */
#line 3264 "parser.y"
                                               {
        (yyval.node) = make_local_variable_node(NULL, (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-1]));
    }
#line 9790 "parser.tab.c"
    break;

  case 428: /* LocalVariableType: UnannType  */
#line 3271 "parser.y"
              { (yyval.node) = (yyvsp[0].node); }
#line 9796 "parser.tab.c"
    break;

  case 429: /* LocalVariableType: VAR  */
#line 3272 "parser.y"
          { (yyval.node) = make_type_node(make_keyword_leaf("var", (yylsp[0])), (yylsp[0])); }
#line 9802 "parser.tab.c"
    break;

  case 430: /* LocalClassOrInterfaceDeclaration: ClassDeclaration  */
#line 3277 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 9808 "parser.tab.c"
    break;

  case 431: /* LocalClassOrInterfaceDeclaration: NormalInterfaceDeclaration  */
#line 3278 "parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 9814 "parser.tab.c"
    break;

  case 432: /* Statement: StatementWithoutTrailingSubstatement  */
#line 3283 "parser.y"
                                         { (yyval.node) = (yyvsp[0].node); }
#line 9820 "parser.tab.c"
    break;

  case 433: /* Statement: LabeledStatement  */
#line 3284 "parser.y"
                       { (yyval.node) = (yyvsp[0].node); }
#line 9826 "parser.tab.c"
    break;

  case 434: /* Statement: IfThenStatement  */
#line 3285 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 9832 "parser.tab.c"
    break;

  case 435: /* Statement: IfThenElseStatement  */
#line 3286 "parser.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 9838 "parser.tab.c"
    break;

  case 436: /* Statement: WhileStatement  */
#line 3287 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 9844 "parser.tab.c"
    break;

  case 437: /* Statement: ForStatement  */
#line 3288 "parser.y"
                   { (yyval.node) = (yyvsp[0].node); }
#line 9850 "parser.tab.c"
    break;

  case 438: /* StatementNoShortIf: StatementWithoutTrailingSubstatement  */
#line 3293 "parser.y"
                                         { (yyval.node) = (yyvsp[0].node); }
#line 9856 "parser.tab.c"
    break;

  case 439: /* StatementNoShortIf: LabeledStatementNoShortIf  */
#line 3294 "parser.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 9862 "parser.tab.c"
    break;

  case 440: /* StatementNoShortIf: IfThenElseStatementNoShortIf  */
#line 3295 "parser.y"
                                   { (yyval.node) = (yyvsp[0].node); }
#line 9868 "parser.tab.c"
    break;

  case 441: /* StatementNoShortIf: WhileStatementNoShortIf  */
#line 3296 "parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 9874 "parser.tab.c"
    break;

  case 442: /* StatementNoShortIf: ForStatementNoShortIf  */
#line 3297 "parser.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 9880 "parser.tab.c"
    break;

  case 443: /* StatementWithoutTrailingSubstatement: Block  */
#line 3302 "parser.y"
          { (yyval.node) = (yyvsp[0].node); }
#line 9886 "parser.tab.c"
    break;

  case 444: /* StatementWithoutTrailingSubstatement: EmptyStatement  */
#line 3303 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 9892 "parser.tab.c"
    break;

  case 445: /* StatementWithoutTrailingSubstatement: ExpressionStatement  */
#line 3304 "parser.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 9898 "parser.tab.c"
    break;

  case 446: /* StatementWithoutTrailingSubstatement: AssertStatement  */
#line 3305 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 9904 "parser.tab.c"
    break;

  case 447: /* StatementWithoutTrailingSubstatement: SwitchStatement  */
#line 3306 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 9910 "parser.tab.c"
    break;

  case 448: /* StatementWithoutTrailingSubstatement: DoStatement  */
#line 3307 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 9916 "parser.tab.c"
    break;

  case 449: /* StatementWithoutTrailingSubstatement: BreakStatement  */
#line 3308 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 9922 "parser.tab.c"
    break;

  case 450: /* StatementWithoutTrailingSubstatement: ContinueStatement  */
#line 3309 "parser.y"
                        { (yyval.node) = (yyvsp[0].node); }
#line 9928 "parser.tab.c"
    break;

  case 451: /* StatementWithoutTrailingSubstatement: ReturnStatement  */
#line 3310 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 9934 "parser.tab.c"
    break;

  case 452: /* StatementWithoutTrailingSubstatement: SynchronizedStatement  */
#line 3311 "parser.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 9940 "parser.tab.c"
    break;

  case 453: /* StatementWithoutTrailingSubstatement: ThrowStatement  */
#line 3312 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 9946 "parser.tab.c"
    break;

  case 454: /* StatementWithoutTrailingSubstatement: TryStatement  */
#line 3313 "parser.y"
                   { (yyval.node) = (yyvsp[0].node); }
#line 9952 "parser.tab.c"
    break;

  case 455: /* StatementWithoutTrailingSubstatement: YieldStatement  */
#line 3314 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 9958 "parser.tab.c"
    break;

  case 456: /* EmptyStatement: EMPTY_STMT  */
#line 3319 "parser.y"
               {
        (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column);
    }
#line 9966 "parser.tab.c"
    break;

  case 457: /* EmptyStatement: ';'  */
#line 3322 "parser.y"
          {
        (yyval.node) = ast_leaf(AST_EMPTY, ";", (yylsp[0]).first_line, (yylsp[0]).first_column);
    }
#line 9974 "parser.tab.c"
    break;

  case 458: /* LabeledStatement: TYPE_IDENTIFIER ':' Statement  */
#line 3329 "parser.y"
                                  {
        AstNode *node = make_unary_stmt(AST_LABELED_STATEMENT, (yyvsp[0].node), (yylsp[-2]));
        ast_prepend_child(node, (yyvsp[-2].node));
        (yyval.node) = node;
    }
#line 9984 "parser.tab.c"
    break;

  case 459: /* LabeledStatement: IdentifierComplement ':' Statement  */
#line 3334 "parser.y"
                                         {
        AstNode *node = make_unary_stmt(AST_LABELED_STATEMENT, (yyvsp[0].node), (yylsp[-2]));
        ast_prepend_child(node, (yyvsp[-2].node));
        (yyval.node) = node;
    }
#line 9994 "parser.tab.c"
    break;

  case 460: /* LabeledStatementNoShortIf: TYPE_IDENTIFIER ':' StatementNoShortIf  */
#line 3343 "parser.y"
                                           {
        AstNode *node = make_unary_stmt(AST_LABELED_STATEMENT, (yyvsp[0].node), (yylsp[-2]));
        ast_prepend_child(node, (yyvsp[-2].node));
        (yyval.node) = node;
    }
#line 10004 "parser.tab.c"
    break;

  case 461: /* LabeledStatementNoShortIf: IdentifierComplement ':' StatementNoShortIf  */
#line 3348 "parser.y"
                                                  {
        AstNode *node = make_unary_stmt(AST_LABELED_STATEMENT, (yyvsp[0].node), (yylsp[-2]));
        ast_prepend_child(node, (yyvsp[-2].node));
        (yyval.node) = node;
    }
#line 10014 "parser.tab.c"
    break;

  case 462: /* ExpressionStatement: StatementExpression ';'  */
#line 3357 "parser.y"
                            {
        (yyval.node) = (yyvsp[-1].node);
    }
#line 10022 "parser.tab.c"
    break;

  case 463: /* StatementExpression: Assignment  */
#line 3364 "parser.y"
               { (yyval.node) = (yyvsp[0].node); }
#line 10028 "parser.tab.c"
    break;

  case 464: /* StatementExpression: PreIncrementExpression  */
#line 3365 "parser.y"
                             { (yyval.node) = (yyvsp[0].node); }
#line 10034 "parser.tab.c"
    break;

  case 465: /* StatementExpression: PreDecrementExpression  */
#line 3366 "parser.y"
                             { (yyval.node) = (yyvsp[0].node); }
#line 10040 "parser.tab.c"
    break;

  case 466: /* StatementExpression: PostIncrementExpression  */
#line 3367 "parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 10046 "parser.tab.c"
    break;

  case 467: /* StatementExpression: PostDecrementExpression  */
#line 3368 "parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 10052 "parser.tab.c"
    break;

  case 468: /* StatementExpression: MethodInvocation  */
#line 3369 "parser.y"
                       { (yyval.node) = (yyvsp[0].node); }
#line 10058 "parser.tab.c"
    break;

  case 469: /* StatementExpression: ClassInstanceCreationExpression  */
#line 3370 "parser.y"
                                      { (yyval.node) = (yyvsp[0].node); }
#line 10064 "parser.tab.c"
    break;

  case 470: /* IfThenStatement: IF '(' Expression ')' Statement  */
#line 3375 "parser.y"
                                    {
        (yyval.node) = make_ternary_stmt(AST_IF, (yyvsp[-2].node), (yyvsp[0].node), NULL, (yylsp[-4]));
    }
#line 10072 "parser.tab.c"
    break;

  case 471: /* IfThenElseStatement: IF '(' Expression ')' StatementNoShortIf ELSE Statement  */
#line 3382 "parser.y"
                                                            {
        AstNode *else_clause = make_else_clause_node((yyvsp[0].node), (yylsp[-1]));
        (yyval.node) = make_ternary_stmt(AST_IF, (yyvsp[-4].node), (yyvsp[-2].node), else_clause, (yylsp[-6]));
    }
#line 10081 "parser.tab.c"
    break;

  case 472: /* IfThenElseStatementNoShortIf: IF '(' Expression ')' StatementNoShortIf ELSE StatementNoShortIf  */
#line 3390 "parser.y"
                                                                     {
        AstNode *else_clause = make_else_clause_node((yyvsp[0].node), (yylsp[-1]));
        (yyval.node) = make_ternary_stmt(AST_IF, (yyvsp[-4].node), (yyvsp[-2].node), else_clause, (yylsp[-6]));
    }
#line 10090 "parser.tab.c"
    break;

  case 473: /* AssertStatement: ASSERT Expression ';'  */
#line 3398 "parser.y"
                          {
        (yyval.node) = make_unary_stmt(AST_ASSERT, (yyvsp[-1].node), (yylsp[-2]));
    }
#line 10098 "parser.tab.c"
    break;

  case 474: /* AssertStatement: ASSERT Expression ':' Expression ';'  */
#line 3401 "parser.y"
                                           {
        AstNode *node = make_binary_stmt(AST_ASSERT, (yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-4]));
        (yyval.node) = node;
    }
#line 10107 "parser.tab.c"
    break;

  case 475: /* SwitchStatement: SWITCH '(' Expression ')' SwitchBlock  */
#line 3409 "parser.y"
                                          {
        AstNode *node = make_unary_stmt(AST_SWITCH, (yyvsp[-2].node), (yylsp[-4]));
        if ((yyvsp[0].node)) {
            ast_add_child(node, (yyvsp[0].node));
        }
        (yyval.node) = node;
    }
#line 10119 "parser.tab.c"
    break;

  case 476: /* SwitchBlock: '{' SwitchRules '}'  */
#line 3420 "parser.y"
                        {
        (yyval.node) = (yyvsp[-1].node);
    }
#line 10127 "parser.tab.c"
    break;

  case 477: /* SwitchBlock: '{' SwitchBlockStatementGroups '}'  */
#line 3424 "parser.y"
                                         {
        (yyval.node) = (yyvsp[-1].node);
    }
#line 10135 "parser.tab.c"
    break;

  case 478: /* SwitchBlock: '{' '}'  */
#line 3427 "parser.y"
             {
        (yyval.node) = make_list_node(AST_STATEMENT_LIST, (yyloc));
    }
#line 10143 "parser.tab.c"
    break;

  case 479: /* SwitchRule: SwitchLabel ARROW Expression ';'  */
#line 3434 "parser.y"
                                     {
        AstNode *labels = make_list_node(AST_SWITCH_LABEL_LIST, (yylsp[-3]));
        ast_add_child(labels, (yyvsp[-3].node));
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_RULE, (yylsp[-3]), 0);
        ast_add_child(node, labels);
        if ((yyvsp[-1].node)) {
            ast_add_child(node, (yyvsp[-1].node));
        }
        (yyval.node) = node;
    }
#line 10158 "parser.tab.c"
    break;

  case 480: /* SwitchRule: SwitchLabel ARROW Block  */
#line 3444 "parser.y"
                              {
        AstNode *labels = make_list_node(AST_SWITCH_LABEL_LIST, (yylsp[-2]));
        ast_add_child(labels, (yyvsp[-2].node));
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_RULE, (yylsp[-2]), 0);
        ast_add_child(node, labels);
        if ((yyvsp[0].node)) {
            ast_add_child(node, (yyvsp[0].node));
        }
        (yyval.node) = node;
    }
#line 10173 "parser.tab.c"
    break;

  case 481: /* SwitchRule: SwitchLabel ARROW ThrowStatement  */
#line 3454 "parser.y"
                                       {
        AstNode *labels = make_list_node(AST_SWITCH_LABEL_LIST, (yylsp[-2]));
        ast_add_child(labels, (yyvsp[-2].node));
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_RULE, (yylsp[-2]), 0);
        ast_add_child(node, labels);
        if ((yyvsp[0].node)) {
            ast_add_child(node, (yyvsp[0].node));
        }
        (yyval.node) = node;
    }
#line 10188 "parser.tab.c"
    break;

  case 482: /* SwitchRules: SwitchRule  */
#line 3468 "parser.y"
               {
        (yyval.node) = make_list_node(AST_SWITCH_RULE_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 10197 "parser.tab.c"
    break;

  case 483: /* SwitchRules: SwitchRules SwitchRule  */
#line 3472 "parser.y"
                             {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 10206 "parser.tab.c"
    break;

  case 484: /* SwitchBlockStatementGroup: SwitchLabels BlockStatements  */
#line 3480 "parser.y"
                                 {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_GROUP, (yylsp[-1]), 0);
        ast_add_child(node, (yyvsp[-1].node));
        if ((yyvsp[0].node)) {
            ast_add_child(node, (yyvsp[0].node));
        }
        (yyval.node) = node;
    }
#line 10219 "parser.tab.c"
    break;

  case 485: /* SwitchBlockStatementGroups: SwitchBlockStatementGroup  */
#line 3492 "parser.y"
                              {
        (yyval.node) = make_list_node(AST_STATEMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 10228 "parser.tab.c"
    break;

  case 486: /* SwitchBlockStatementGroups: SwitchBlockStatementGroups SwitchBlockStatementGroup  */
#line 3496 "parser.y"
                                                           {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 10237 "parser.tab.c"
    break;

  case 487: /* SwitchLabel: CASE CaseConstants  */
#line 3504 "parser.y"
                       {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_LABEL, (yylsp[-1]), 0);
        ast_add_child(node, (yyvsp[0].node));
        (yyval.node) = node;
    }
#line 10247 "parser.tab.c"
    break;

  case 488: /* SwitchLabel: DEFAULT_SwitchLabel  */
#line 3509 "parser.y"
                          {
        AstNode *node = AST_BRANCH_AT(AST_SWITCH_LABEL, (yylsp[0]), 0);
        ast_add_child(node, make_keyword_leaf("default", (yylsp[0])));
        (yyval.node) = node;
    }
#line 10257 "parser.tab.c"
    break;

  case 489: /* SwitchLabels: SwitchLabel ':'  */
#line 3518 "parser.y"
                    {
        AstNode *node = make_list_node(AST_SWITCH_LABEL_LIST, (yyloc));
        ast_add_child(node, (yyvsp[-1].node));
        (yyval.node) = node;
    }
#line 10267 "parser.tab.c"
    break;

  case 490: /* SwitchLabels: SwitchLabels SwitchLabel ':'  */
#line 3523 "parser.y"
                                   {
        ast_add_child((yyvsp[-2].node), (yyvsp[-1].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 10276 "parser.tab.c"
    break;

  case 491: /* CaseConstant: ConditionalExpression  */
#line 3531 "parser.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 10282 "parser.tab.c"
    break;

  case 492: /* CaseConstants: CaseConstant  */
#line 3536 "parser.y"
                 {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 10291 "parser.tab.c"
    break;

  case 493: /* CaseConstants: CaseConstants ',' CaseConstant  */
#line 3540 "parser.y"
                                     {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 10300 "parser.tab.c"
    break;

  case 494: /* WhileStatement: WHILE '(' Expression ')' Statement  */
#line 3548 "parser.y"
                                       {
        (yyval.node) = make_binary_stmt(AST_WHILE, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-4]));
    }
#line 10308 "parser.tab.c"
    break;

  case 495: /* WhileStatementNoShortIf: WHILE '(' Expression ')' StatementNoShortIf  */
#line 3555 "parser.y"
                                                {
        (yyval.node) = make_binary_stmt(AST_WHILE, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-4]));
    }
#line 10316 "parser.tab.c"
    break;

  case 496: /* DoStatement: DO Statement WHILE '(' Expression ')' ';'  */
#line 3562 "parser.y"
                                              {
        AstNode *node = make_binary_stmt(AST_DO_WHILE, (yyvsp[-5].node), (yyvsp[-2].node), (yylsp[-6]));
        (yyval.node) = node;
    }
#line 10325 "parser.tab.c"
    break;

  case 497: /* ForStatement: BasicForStatement  */
#line 3570 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 10331 "parser.tab.c"
    break;

  case 498: /* ForStatement: EnhancedForStatement  */
#line 3571 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 10337 "parser.tab.c"
    break;

  case 499: /* ForStatementNoShortIf: BasicForStatementNoShortIf  */
#line 3576 "parser.y"
                               { (yyval.node) = (yyvsp[0].node); }
#line 10343 "parser.tab.c"
    break;

  case 500: /* ForStatementNoShortIf: EnhancedForStatementNoShortIf  */
#line 3577 "parser.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 10349 "parser.tab.c"
    break;

  case 501: /* BasicForStatement: FOR '(' ForInit ';' Expression ';' ForUpdate ')' Statement  */
#line 3582 "parser.y"
                                                               {
        (yyval.node) = make_for_stmt((yyvsp[-6].node), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10357 "parser.tab.c"
    break;

  case 502: /* BasicForStatement: FOR '(' ForInit ';' Expression ';' ')' Statement  */
#line 3585 "parser.y"
                                                       {
        (yyval.node) = make_for_stmt((yyvsp[-5].node), (yyvsp[-3].node), NULL, (yyvsp[0].node), (yylsp[-7]));
    }
#line 10365 "parser.tab.c"
    break;

  case 503: /* BasicForStatement: FOR '(' ForInit ';' ';' ForUpdate ')' Statement  */
#line 3588 "parser.y"
                                                      {
        (yyval.node) = make_for_stmt((yyvsp[-5].node), NULL, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-7]));
    }
#line 10373 "parser.tab.c"
    break;

  case 504: /* BasicForStatement: FOR '(' ForInit ';' ';' ')' Statement  */
#line 3591 "parser.y"
                                            {
        (yyval.node) = make_for_stmt((yyvsp[-4].node), NULL, NULL, (yyvsp[0].node), (yylsp[-6]));
    }
#line 10381 "parser.tab.c"
    break;

  case 505: /* BasicForStatement: FOR '(' ';' Expression ';' ForUpdate ')' Statement  */
#line 3594 "parser.y"
                                                         {
        (yyval.node) = make_for_stmt(NULL, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-7]));
    }
#line 10389 "parser.tab.c"
    break;

  case 506: /* BasicForStatement: FOR '(' ';' Expression ';' ')' Statement  */
#line 3597 "parser.y"
                                               {
        (yyval.node) = make_for_stmt(NULL, (yyvsp[-3].node), NULL, (yyvsp[0].node), (yylsp[-6]));
    }
#line 10397 "parser.tab.c"
    break;

  case 507: /* BasicForStatement: FOR '(' ';' ';' ForUpdate ')' Statement  */
#line 3600 "parser.y"
                                              {
        (yyval.node) = make_for_stmt(NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-6]));
    }
#line 10405 "parser.tab.c"
    break;

  case 508: /* BasicForStatement: FOR '(' ';' ';' ')' Statement  */
#line 3603 "parser.y"
                                    {
        (yyval.node) = make_for_stmt(NULL, NULL, NULL, (yyvsp[0].node), (yylsp[-5]));
    }
#line 10413 "parser.tab.c"
    break;

  case 509: /* BasicForStatementNoShortIf: FOR '(' ForInit ';' Expression ';' ForUpdate ')' StatementNoShortIf  */
#line 3610 "parser.y"
                                                                        {
        (yyval.node) = make_for_stmt((yyvsp[-6].node), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10421 "parser.tab.c"
    break;

  case 510: /* BasicForStatementNoShortIf: FOR '(' ForInit ';' Expression ';' ')' StatementNoShortIf  */
#line 3613 "parser.y"
                                                                {
        (yyval.node) = make_for_stmt((yyvsp[-5].node), (yyvsp[-3].node), NULL, (yyvsp[0].node), (yylsp[-7]));
    }
#line 10429 "parser.tab.c"
    break;

  case 511: /* BasicForStatementNoShortIf: FOR '(' ForInit ';' ';' ForUpdate ')' StatementNoShortIf  */
#line 3616 "parser.y"
                                                               {
        (yyval.node) = make_for_stmt((yyvsp[-5].node), NULL, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-7]));
    }
#line 10437 "parser.tab.c"
    break;

  case 512: /* BasicForStatementNoShortIf: FOR '(' ForInit ';' ';' ')' StatementNoShortIf  */
#line 3619 "parser.y"
                                                     {
        (yyval.node) = make_for_stmt((yyvsp[-4].node), NULL, NULL, (yyvsp[0].node), (yylsp[-6]));
    }
#line 10445 "parser.tab.c"
    break;

  case 513: /* BasicForStatementNoShortIf: FOR '(' ';' Expression ';' ForUpdate ')' StatementNoShortIf  */
#line 3622 "parser.y"
                                                                  {
        (yyval.node) = make_for_stmt(NULL, (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-7]));
    }
#line 10453 "parser.tab.c"
    break;

  case 514: /* BasicForStatementNoShortIf: FOR '(' ';' Expression ';' ')' StatementNoShortIf  */
#line 3625 "parser.y"
                                                        {
        (yyval.node) = make_for_stmt(NULL, (yyvsp[-3].node), NULL, (yyvsp[0].node), (yylsp[-6]));
    }
#line 10461 "parser.tab.c"
    break;

  case 515: /* BasicForStatementNoShortIf: FOR '(' ';' ';' ForUpdate ')' StatementNoShortIf  */
#line 3628 "parser.y"
                                                       {
        (yyval.node) = make_for_stmt(NULL, NULL, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-6]));
    }
#line 10469 "parser.tab.c"
    break;

  case 516: /* BasicForStatementNoShortIf: FOR '(' ';' ';' ')' StatementNoShortIf  */
#line 3631 "parser.y"
                                             {
        (yyval.node) = make_for_stmt(NULL, NULL, NULL, (yyvsp[0].node), (yylsp[-5]));
    }
#line 10477 "parser.tab.c"
    break;

  case 517: /* ForInit: StatementExpressionList  */
#line 3638 "parser.y"
                            { (yyval.node) = relabel_list((yyvsp[0].node), AST_FOR_INIT_LIST); }
#line 10483 "parser.tab.c"
    break;

  case 518: /* ForInit: LocalVariableDeclaration  */
#line 3639 "parser.y"
                               { (yyval.node) = (yyvsp[0].node); }
#line 10489 "parser.tab.c"
    break;

  case 519: /* ForUpdate: StatementExpressionList  */
#line 3644 "parser.y"
                            { (yyval.node) = relabel_list((yyvsp[0].node), AST_FOR_UPDATE_LIST); }
#line 10495 "parser.tab.c"
    break;

  case 520: /* StatementExpressionList: StatementExpression  */
#line 3649 "parser.y"
                        {
        (yyval.node) = make_list_node(AST_EXPRESSION_LIST, (yyloc));
        if ((yyvsp[0].node)) ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 10504 "parser.tab.c"
    break;

  case 521: /* StatementExpressionList: StatementExpressionList ',' StatementExpression  */
#line 3653 "parser.y"
                                                      {
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-2].node);
    }
#line 10515 "parser.tab.c"
    break;

  case 522: /* EnhancedForStatement: FOR '(' VariableModifiers LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' Statement  */
#line 3667 "parser.y"
                                                                                                  {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-9]));
    }
#line 10524 "parser.tab.c"
    break;

  case 523: /* EnhancedForStatement: FOR '(' Annotations LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' Statement  */
#line 3671 "parser.y"
                                                                                              {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-9]));
    }
#line 10533 "parser.tab.c"
    break;

  case 524: /* EnhancedForStatement: FOR '(' LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' Statement  */
#line 3675 "parser.y"
                                                                                  {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10542 "parser.tab.c"
    break;

  case 525: /* EnhancedForStatement: FOR '(' VariableModifiers LocalVariableType TYPE_IDENTIFIER ':' Expression ')' Statement  */
#line 3679 "parser.y"
                                                                                               {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10551 "parser.tab.c"
    break;

  case 526: /* EnhancedForStatement: FOR '(' Annotations LocalVariableType TYPE_IDENTIFIER ':' Expression ')' Statement  */
#line 3683 "parser.y"
                                                                                         {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10560 "parser.tab.c"
    break;

  case 527: /* EnhancedForStatement: FOR '(' LocalVariableType TYPE_IDENTIFIER ':' Expression ')' Statement  */
#line 3687 "parser.y"
                                                                             {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-7]));
    }
#line 10569 "parser.tab.c"
    break;

  case 528: /* EnhancedForStatement: FOR '(' VariableModifiers LocalVariableType IdentifierComplement Dims ':' Expression ')' Statement  */
#line 3691 "parser.y"
                                                                                                         {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-9]));
    }
#line 10578 "parser.tab.c"
    break;

  case 529: /* EnhancedForStatement: FOR '(' Annotations LocalVariableType IdentifierComplement Dims ':' Expression ')' Statement  */
#line 3695 "parser.y"
                                                                                                   {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-9]));
    }
#line 10587 "parser.tab.c"
    break;

  case 530: /* EnhancedForStatement: FOR '(' LocalVariableType IdentifierComplement Dims ':' Expression ')' Statement  */
#line 3699 "parser.y"
                                                                                       {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10596 "parser.tab.c"
    break;

  case 531: /* EnhancedForStatement: FOR '(' VariableModifiers LocalVariableType IdentifierComplement ':' Expression ')' Statement  */
#line 3703 "parser.y"
                                                                                                    {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10605 "parser.tab.c"
    break;

  case 532: /* EnhancedForStatement: FOR '(' Annotations LocalVariableType IdentifierComplement ':' Expression ')' Statement  */
#line 3707 "parser.y"
                                                                                              {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10614 "parser.tab.c"
    break;

  case 533: /* EnhancedForStatement: FOR '(' LocalVariableType IdentifierComplement ':' Expression ')' Statement  */
#line 3711 "parser.y"
                                                                                  {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-7]));
    }
#line 10623 "parser.tab.c"
    break;

  case 534: /* EnhancedForStatementNoShortIf: FOR '(' VariableModifiers LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' StatementNoShortIf  */
#line 3722 "parser.y"
                                                                                                           {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-9]));
    }
#line 10632 "parser.tab.c"
    break;

  case 535: /* EnhancedForStatementNoShortIf: FOR '(' Annotations LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' StatementNoShortIf  */
#line 3726 "parser.y"
                                                                                                       {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-9]));
    }
#line 10641 "parser.tab.c"
    break;

  case 536: /* EnhancedForStatementNoShortIf: FOR '(' LocalVariableType TYPE_IDENTIFIER Dims ':' Expression ')' StatementNoShortIf  */
#line 3730 "parser.y"
                                                                                           {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10650 "parser.tab.c"
    break;

  case 537: /* EnhancedForStatementNoShortIf: FOR '(' VariableModifiers LocalVariableType TYPE_IDENTIFIER ':' Expression ')' StatementNoShortIf  */
#line 3734 "parser.y"
                                                                                                        {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10659 "parser.tab.c"
    break;

  case 538: /* EnhancedForStatementNoShortIf: FOR '(' Annotations LocalVariableType TYPE_IDENTIFIER ':' Expression ')' StatementNoShortIf  */
#line 3738 "parser.y"
                                                                                                  {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10668 "parser.tab.c"
    break;

  case 539: /* EnhancedForStatementNoShortIf: FOR '(' LocalVariableType TYPE_IDENTIFIER ':' Expression ')' StatementNoShortIf  */
#line 3742 "parser.y"
                                                                                      {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-7]));
    }
#line 10677 "parser.tab.c"
    break;

  case 540: /* EnhancedForStatementNoShortIf: FOR '(' VariableModifiers LocalVariableType IdentifierComplement Dims ':' Expression ')' StatementNoShortIf  */
#line 3746 "parser.y"
                                                                                                                  {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-9]));
    }
#line 10686 "parser.tab.c"
    break;

  case 541: /* EnhancedForStatementNoShortIf: FOR '(' Annotations LocalVariableType IdentifierComplement Dims ':' Expression ')' StatementNoShortIf  */
#line 3750 "parser.y"
                                                                                                            {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-9]));
    }
#line 10695 "parser.tab.c"
    break;

  case 542: /* EnhancedForStatementNoShortIf: FOR '(' LocalVariableType IdentifierComplement Dims ':' Expression ')' StatementNoShortIf  */
#line 3754 "parser.y"
                                                                                                {
        AstNode *param = make_parameter_with_dims((yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10704 "parser.tab.c"
    break;

  case 543: /* EnhancedForStatementNoShortIf: FOR '(' VariableModifiers LocalVariableType IdentifierComplement ':' Expression ')' StatementNoShortIf  */
#line 3758 "parser.y"
                                                                                                             {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10713 "parser.tab.c"
    break;

  case 544: /* EnhancedForStatementNoShortIf: FOR '(' Annotations LocalVariableType IdentifierComplement ':' Expression ')' StatementNoShortIf  */
#line 3762 "parser.y"
                                                                                                       {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-8]));
    }
#line 10722 "parser.tab.c"
    break;

  case 545: /* EnhancedForStatementNoShortIf: FOR '(' LocalVariableType IdentifierComplement ':' Expression ')' StatementNoShortIf  */
#line 3766 "parser.y"
                                                                                           {
        AstNode *param = make_parameter_with_dims((yyvsp[-5].node), (yyvsp[-4].node), NULL, (yyloc));
        (yyval.node) = make_foreach_stmt(param, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-7]));
    }
#line 10731 "parser.tab.c"
    break;

  case 546: /* BreakStatement: BREAK TYPE_IDENTIFIER ';'  */
#line 3774 "parser.y"
                              {
        AstNode *node = make_unary_stmt(AST_BREAK, (yyvsp[-1].node), (yylsp[-2]));
        (yyval.node) = node;
    }
#line 10740 "parser.tab.c"
    break;

  case 547: /* BreakStatement: BREAK IdentifierComplement ';'  */
#line 3778 "parser.y"
                                     {
        AstNode *node = make_unary_stmt(AST_BREAK, (yyvsp[-1].node), (yylsp[-2]));
        (yyval.node) = node;
    }
#line 10749 "parser.tab.c"
    break;

  case 548: /* BreakStatement: BREAK ';'  */
#line 3782 "parser.y"
                {
        (yyval.node) = make_simple_stmt(AST_BREAK, (yylsp[-1]));
    }
#line 10757 "parser.tab.c"
    break;

  case 549: /* YieldStatement: YIELD Expression ';'  */
#line 3789 "parser.y"
                         {
        (yyval.node) = make_unary_stmt(AST_YIELD, (yyvsp[-1].node), (yylsp[-2]));
    }
#line 10765 "parser.tab.c"
    break;

  case 550: /* ContinueStatement: CONTINUE ';'  */
#line 3796 "parser.y"
                 {
        (yyval.node) = make_simple_stmt(AST_CONTINUE, (yylsp[-1]));
    }
#line 10773 "parser.tab.c"
    break;

  case 551: /* ContinueStatement: CONTINUE TYPE_IDENTIFIER ';'  */
#line 3799 "parser.y"
                                   {
        AstNode *node = make_unary_stmt(AST_CONTINUE, (yyvsp[-1].node), (yylsp[-2]));
        (yyval.node) = node;
    }
#line 10782 "parser.tab.c"
    break;

  case 552: /* ContinueStatement: CONTINUE IdentifierComplement ';'  */
#line 3803 "parser.y"
                                        {
        AstNode *node = make_unary_stmt(AST_CONTINUE, (yyvsp[-1].node), (yylsp[-2]));
        (yyval.node) = node;
    }
#line 10791 "parser.tab.c"
    break;

  case 553: /* ReturnStatement: RETURN Expression ';'  */
#line 3811 "parser.y"
                          {
        (yyval.node) = make_unary_stmt(AST_RETURN, (yyvsp[-1].node), (yylsp[-2]));
    }
#line 10799 "parser.tab.c"
    break;

  case 554: /* ReturnStatement: RETURN ';'  */
#line 3814 "parser.y"
                 {
        (yyval.node) = make_simple_stmt(AST_RETURN, (yylsp[-1]));
    }
#line 10807 "parser.tab.c"
    break;

  case 555: /* ThrowStatement: THROW Expression ';'  */
#line 3821 "parser.y"
                         {
        (yyval.node) = make_unary_stmt(AST_THROW, (yyvsp[-1].node), (yylsp[-2]));
    }
#line 10815 "parser.tab.c"
    break;

  case 556: /* SynchronizedStatement: SYNCHRONIZED '(' Expression ')' Block  */
#line 3828 "parser.y"
                                          {
        (yyval.node) = make_binary_stmt(AST_SYNCHRONIZED, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-4]));
    }
#line 10823 "parser.tab.c"
    break;

  case 557: /* TryStatement: TRY Block Catches  */
#line 3835 "parser.y"
                      {
        (yyval.node) = make_try_stmt((yyvsp[-1].node), (yyvsp[0].node), NULL, (yylsp[-2]));
    }
#line 10831 "parser.tab.c"
    break;

  case 558: /* TryStatement: TRY Block Catches Finally  */
#line 3838 "parser.y"
                                {
        (yyval.node) = make_try_stmt((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-3]));
    }
#line 10839 "parser.tab.c"
    break;

  case 559: /* TryStatement: TRY Block Finally  */
#line 3841 "parser.y"
                        {
        (yyval.node) = make_try_stmt((yyvsp[-1].node), NULL, (yyvsp[0].node), (yylsp[-2]));
    }
#line 10847 "parser.tab.c"
    break;

  case 560: /* TryStatement: TryWithResourcesStatement  */
#line 3844 "parser.y"
                                {
        (yyval.node) = (yyvsp[0].node);
    }
#line 10855 "parser.tab.c"
    break;

  case 561: /* Catches: CatchClause  */
#line 3851 "parser.y"
                {
        (yyval.node) = make_list_node(AST_STATEMENT_LIST, (yyloc));
        if ((yyvsp[0].node)) ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 10864 "parser.tab.c"
    break;

  case 562: /* Catches: Catches CatchClause  */
#line 3855 "parser.y"
                          {
        if ((yyvsp[0].node)) ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 10873 "parser.tab.c"
    break;

  case 563: /* CatchClause: CATCH '(' CatchFormalParameter ')' Block  */
#line 3863 "parser.y"
                                             {
        AstNode *node = AST_BRANCH_AT(AST_CATCH, (yylsp[-4]), 0);
        if ((yyvsp[-2].node)) ast_add_child(node, (yyvsp[-2].node));
        if ((yyvsp[0].node)) ast_add_child(node, (yyvsp[0].node));
        (yyval.node) = node;
    }
#line 10884 "parser.tab.c"
    break;

  case 564: /* CatchFormalParameter: VariableModifiers CatchType TYPE_IDENTIFIER Dims  */
#line 3876 "parser.y"
                                                     {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 10892 "parser.tab.c"
    break;

  case 565: /* CatchFormalParameter: Annotations CatchType TYPE_IDENTIFIER Dims  */
#line 3879 "parser.y"
                                                 {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 10900 "parser.tab.c"
    break;

  case 566: /* CatchFormalParameter: CatchType TYPE_IDENTIFIER Dims  */
#line 3882 "parser.y"
                                     {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 10908 "parser.tab.c"
    break;

  case 567: /* CatchFormalParameter: VariableModifiers CatchType TYPE_IDENTIFIER  */
#line 3885 "parser.y"
                                                  {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 10916 "parser.tab.c"
    break;

  case 568: /* CatchFormalParameter: Annotations CatchType TYPE_IDENTIFIER  */
#line 3888 "parser.y"
                                            {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 10924 "parser.tab.c"
    break;

  case 569: /* CatchFormalParameter: CatchType TYPE_IDENTIFIER  */
#line 3891 "parser.y"
                                {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 10932 "parser.tab.c"
    break;

  case 570: /* CatchFormalParameter: VariableModifiers CatchType IdentifierComplement Dims  */
#line 3894 "parser.y"
                                                            {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 10940 "parser.tab.c"
    break;

  case 571: /* CatchFormalParameter: Annotations CatchType IdentifierComplement Dims  */
#line 3897 "parser.y"
                                                      {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 10948 "parser.tab.c"
    break;

  case 572: /* CatchFormalParameter: CatchType IdentifierComplement Dims  */
#line 3900 "parser.y"
                                          {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 10956 "parser.tab.c"
    break;

  case 573: /* CatchFormalParameter: VariableModifiers CatchType IdentifierComplement  */
#line 3903 "parser.y"
                                                       {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 10964 "parser.tab.c"
    break;

  case 574: /* CatchFormalParameter: Annotations CatchType IdentifierComplement  */
#line 3906 "parser.y"
                                                 {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 10972 "parser.tab.c"
    break;

  case 575: /* CatchFormalParameter: CatchType IdentifierComplement  */
#line 3909 "parser.y"
                                     {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 10980 "parser.tab.c"
    break;

  case 576: /* CatchType: UnannClassOrInterfaceType  */
#line 3916 "parser.y"
                              {
        AstNode *list = make_list_node(AST_EXCEPTION_TYPE_LIST, (yyloc));
        ast_add_child(list, make_type_node((yyvsp[0].node), (yyloc)));
        (yyval.node) = list;
    }
#line 10990 "parser.tab.c"
    break;

  case 577: /* CatchType: CatchType '|' UnannClassOrInterfaceType  */
#line 3921 "parser.y"
                                              {
        if ((yyvsp[-2].node)) {
            ast_add_child((yyvsp[-2].node), make_type_node((yyvsp[0].node), (yylsp[0])));
        }
        (yyval.node) = (yyvsp[-2].node);
    }
#line 11001 "parser.tab.c"
    break;

  case 578: /* Finally: FINALLY Block  */
#line 3931 "parser.y"
                  {
        AstNode *node = AST_BRANCH_AT(AST_FINALLY, (yylsp[-1]), 0);
        if ((yyvsp[0].node)) ast_add_child(node, (yyvsp[0].node));
        (yyval.node) = node;
    }
#line 11011 "parser.tab.c"
    break;

  case 579: /* TryWithResourcesStatement: TRY ResourceSpecification Block Catches Finally  */
#line 3940 "parser.y"
                                                    {
        AstNode *node = make_try_stmt((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-4]));
        if ((yyvsp[-3].node)) {
            ast_prepend_child(node, (yyvsp[-3].node));
        }
        (yyval.node) = node;
    }
#line 11023 "parser.tab.c"
    break;

  case 580: /* TryWithResourcesStatement: TRY ResourceSpecification Block Catches  */
#line 3947 "parser.y"
                                              {
        AstNode *node = make_try_stmt((yyvsp[-1].node), (yyvsp[0].node), NULL, (yylsp[-3]));
        if ((yyvsp[-2].node)) {
            ast_prepend_child(node, (yyvsp[-2].node));
        }
        (yyval.node) = node;
    }
#line 11035 "parser.tab.c"
    break;

  case 581: /* TryWithResourcesStatement: TRY ResourceSpecification Block Finally  */
#line 3954 "parser.y"
                                              {
        AstNode *node = make_try_stmt((yyvsp[-1].node), NULL, (yyvsp[0].node), (yylsp[-3]));
        if ((yyvsp[-2].node)) {
            ast_prepend_child(node, (yyvsp[-2].node));
        }
        (yyval.node) = node;
    }
#line 11047 "parser.tab.c"
    break;

  case 582: /* TryWithResourcesStatement: TRY ResourceSpecification Block  */
#line 3961 "parser.y"
                                      {
        AstNode *node = make_try_stmt((yyvsp[0].node), NULL, NULL, (yylsp[-2]));
        if ((yyvsp[-1].node)) {
            ast_prepend_child(node, (yyvsp[-1].node));
        }
        (yyval.node) = node;
    }
#line 11059 "parser.tab.c"
    break;

  case 583: /* ResourceSpecification: '(' ResourceList ')'  */
#line 3972 "parser.y"
                         {
        (yyval.node) = AST_BRANCH_AT(AST_RESOURCE_SPEC, (yyloc), 1, (yyvsp[-1].node));
    }
#line 11067 "parser.tab.c"
    break;

  case 584: /* ResourceSpecification: '(' ResourceList ';' ')'  */
#line 3975 "parser.y"
                             {
        (yyval.node) = AST_BRANCH_AT(AST_RESOURCE_SPEC, (yyloc), 1, (yyvsp[-2].node));
    }
#line 11075 "parser.tab.c"
    break;

  case 585: /* ResourceList: Resource  */
#line 3983 "parser.y"
             {
        (yyval.node) = make_list_node(AST_RESOURCE_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 11084 "parser.tab.c"
    break;

  case 586: /* ResourceList: ResourceList ';' Resource  */
#line 3987 "parser.y"
                              {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 11093 "parser.tab.c"
    break;

  case 587: /* Resource: VariableModifiers LocalVariableType TYPE_IDENTIFIER '=' Expression  */
#line 3996 "parser.y"
                                                                       {
        AstNode *decl = make_resource_decl((yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[0].node), (yyloc));
        (yyval.node) = AST_BRANCH_AT(AST_RESOURCE, (yyloc), 1, decl);
    }
#line 11102 "parser.tab.c"
    break;

  case 588: /* Resource: Annotations LocalVariableType TYPE_IDENTIFIER '=' Expression  */
#line 4000 "parser.y"
                                                                 {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-4].node), (yylsp[-4]));
        AstNode *decl = make_resource_decl(mods, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[0].node), (yyloc));
        (yyval.node) = AST_BRANCH_AT(AST_RESOURCE, (yyloc), 1, decl);
    }
#line 11112 "parser.tab.c"
    break;

  case 589: /* Resource: LocalVariableType TYPE_IDENTIFIER '=' Expression  */
#line 4005 "parser.y"
                                                     {
        AstNode *decl = make_resource_decl(NULL, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[0].node), (yyloc));
        (yyval.node) = AST_BRANCH_AT(AST_RESOURCE, (yyloc), 1, decl);
    }
#line 11121 "parser.tab.c"
    break;

  case 590: /* Resource: VariableModifiers LocalVariableType IdentifierComplement '=' Expression  */
#line 4009 "parser.y"
                                                                            {
        AstNode *decl = make_resource_decl((yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[0].node), (yyloc));
        (yyval.node) = AST_BRANCH_AT(AST_RESOURCE, (yyloc), 1, decl);
    }
#line 11130 "parser.tab.c"
    break;

  case 591: /* Resource: Annotations LocalVariableType IdentifierComplement '=' Expression  */
#line 4013 "parser.y"
                                                                      {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-4].node), (yylsp[-4]));
        AstNode *decl = make_resource_decl(mods, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[0].node), (yyloc));
        (yyval.node) = AST_BRANCH_AT(AST_RESOURCE, (yyloc), 1, decl);
    }
#line 11140 "parser.tab.c"
    break;

  case 592: /* Resource: LocalVariableType IdentifierComplement '=' Expression  */
#line 4018 "parser.y"
                                                          {
        AstNode *decl = make_resource_decl(NULL, (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[0].node), (yyloc));
        (yyval.node) = AST_BRANCH_AT(AST_RESOURCE, (yyloc), 1, decl);
    }
#line 11149 "parser.tab.c"
    break;

  case 593: /* Resource: VariableAccess  */
#line 4022 "parser.y"
                   {
        (yyval.node) = AST_BRANCH_AT(AST_RESOURCE, (yyloc), 1, (yyvsp[0].node));
    }
#line 11157 "parser.tab.c"
    break;

  case 594: /* VariableAccess: CommonName  */
#line 4029 "parser.y"
               { (yyval.node) = (yyvsp[0].node); }
#line 11163 "parser.tab.c"
    break;

  case 595: /* VariableAccess: FieldAccess  */
#line 4030 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 11169 "parser.tab.c"
    break;

  case 596: /* Pattern: TypePattern  */
#line 4035 "parser.y"
                { (yyval.node) = (yyvsp[0].node); }
#line 11175 "parser.tab.c"
    break;

  case 597: /* TypePattern: LocalVariableDeclaration_TypePattern  */
#line 4044 "parser.y"
                                         { (yyval.node) = (yyvsp[0].node); }
#line 11181 "parser.tab.c"
    break;

  case 598: /* LocalVariableDeclaration_TypePattern: VariableModifiers LocalVariableType VariableDeclarator  */
#line 4049 "parser.y"
                                                           {
        (yyval.node) = make_type_pattern_node((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 11189 "parser.tab.c"
    break;

  case 599: /* LocalVariableDeclaration_TypePattern: Annotations LocalVariableType VariableDeclarator  */
#line 4052 "parser.y"
                                                       {
        AstNode *mods = make_modifiers_from_annotations((yyvsp[-2].node), (yylsp[-2]));
        (yyval.node) = make_type_pattern_node(mods, (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 11198 "parser.tab.c"
    break;

  case 600: /* LocalVariableDeclaration_TypePattern: LocalVariableType VariableDeclarator  */
#line 4056 "parser.y"
                                           {
        (yyval.node) = make_type_pattern_node(NULL, (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 11206 "parser.tab.c"
    break;

  case 601: /* Primary: PrimaryNoNewArray  */
#line 4065 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 11212 "parser.tab.c"
    break;

  case 602: /* Primary: ArrayCreationExpression  */
#line 4066 "parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 11218 "parser.tab.c"
    break;

  case 603: /* PrimaryNoNewArray: Literal  */
#line 4071 "parser.y"
            { (yyval.node) = (yyvsp[0].node); }
#line 11224 "parser.tab.c"
    break;

  case 604: /* PrimaryNoNewArray: ClassLiteral  */
#line 4072 "parser.y"
                   { (yyval.node) = (yyvsp[0].node); }
#line 11230 "parser.tab.c"
    break;

  case 605: /* PrimaryNoNewArray: THIS  */
#line 4073 "parser.y"
           {
        (yyval.node) = make_this_expr_node(NULL, (yylsp[0]));
    }
#line 11238 "parser.tab.c"
    break;

  case 606: /* PrimaryNoNewArray: CommonName '.' THIS  */
#line 4076 "parser.y"
                          {
        (yyval.node) = make_this_expr_node((yyvsp[-2].node), (yylsp[0]));
    }
#line 11246 "parser.tab.c"
    break;

  case 607: /* PrimaryNoNewArray: '(' Expression ')'  */
#line 4080 "parser.y"
                         { (yyval.node) = (yyvsp[-1].node); }
#line 11252 "parser.tab.c"
    break;

  case 608: /* PrimaryNoNewArray: ClassInstanceCreationExpression  */
#line 4081 "parser.y"
                                      { (yyval.node) = (yyvsp[0].node); }
#line 11258 "parser.tab.c"
    break;

  case 609: /* PrimaryNoNewArray: FieldAccess  */
#line 4082 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 11264 "parser.tab.c"
    break;

  case 610: /* PrimaryNoNewArray: ArrayAccess  */
#line 4083 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 11270 "parser.tab.c"
    break;

  case 611: /* PrimaryNoNewArray: MethodInvocation  */
#line 4084 "parser.y"
                       { (yyval.node) = (yyvsp[0].node); }
#line 11276 "parser.tab.c"
    break;

  case 612: /* PrimaryNoNewArray: MethodReference  */
#line 4085 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 11282 "parser.tab.c"
    break;

  case 613: /* ClassLiteral: CommonName DimsNoAnnotations '.' CLASS  */
#line 4091 "parser.y"
                                           {
        attach_dims((yyvsp[-3].node), (yyvsp[-2].node));
        (yyval.node) = make_class_literal_node((yyvsp[-3].node), (yylsp[0]));
    }
#line 11291 "parser.tab.c"
    break;

  case 614: /* ClassLiteral: NumericType DimsNoAnnotations '.' CLASS  */
#line 4095 "parser.y"
                                              {
        attach_dims((yyvsp[-3].node), (yyvsp[-2].node));
        (yyval.node) = make_class_literal_node((yyvsp[-3].node), (yylsp[0]));
    }
#line 11300 "parser.tab.c"
    break;

  case 615: /* ClassLiteral: BOOLEAN DimsNoAnnotations '.' CLASS  */
#line 4099 "parser.y"
                                          {
        AstNode *bool_node = make_keyword_leaf("boolean", (yylsp[-3]));
        attach_dims(bool_node, (yyvsp[-2].node));
        (yyval.node) = make_class_literal_node(bool_node, (yylsp[0]));
    }
#line 11310 "parser.tab.c"
    break;

  case 616: /* ClassLiteral: CommonName '.' CLASS  */
#line 4106 "parser.y"
                           {
        (yyval.node) = make_class_literal_node((yyvsp[-2].node), (yylsp[0]));
    }
#line 11318 "parser.tab.c"
    break;

  case 617: /* ClassLiteral: NumericType '.' CLASS  */
#line 4109 "parser.y"
                            {
        (yyval.node) = make_class_literal_node((yyvsp[-2].node), (yylsp[0]));
    }
#line 11326 "parser.tab.c"
    break;

  case 618: /* ClassLiteral: BOOLEAN '.' CLASS  */
#line 4112 "parser.y"
                        {
        AstNode *bool_node = make_keyword_leaf("boolean", (yylsp[-2]));
        (yyval.node) = make_class_literal_node(bool_node, (yylsp[0]));
    }
#line 11335 "parser.tab.c"
    break;

  case 619: /* ClassLiteral: VOID '.' CLASS  */
#line 4117 "parser.y"
                     {
        AstNode *void_node = make_keyword_leaf("void", (yylsp[-2]));
        (yyval.node) = make_class_literal_node(void_node, (yylsp[0]));
    }
#line 11344 "parser.tab.c"
    break;

  case 620: /* DimsNoAnnotations: LBRACK ']'  */
#line 4125 "parser.y"
               {
        AstNode *list = make_list_node(AST_DIM_LIST, (yyloc));
        ast_add_child(list, make_dim_node((yylsp[-1])));
        (yyval.node) = list;
    }
#line 11354 "parser.tab.c"
    break;

  case 621: /* DimsNoAnnotations: DimsNoAnnotations LBRACK ']'  */
#line 4130 "parser.y"
                                   {
        ast_add_child((yyvsp[-2].node), make_dim_node((yylsp[-1])));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 11363 "parser.tab.c"
    break;

  case 622: /* ClassInstanceCreationExpression: UnqualifiedClassInstanceCreationExpression  */
#line 4138 "parser.y"
                                               { (yyval.node) = (yyvsp[0].node); }
#line 11369 "parser.tab.c"
    break;

  case 623: /* ClassInstanceCreationExpression: CommonName '.' UnqualifiedClassInstanceCreationExpression  */
#line 4139 "parser.y"
                                                                {
        ast_prepend_child((yyvsp[0].node), (yyvsp[-2].node));
        (yyvsp[0].node)->scope = (yyvsp[-2].node);
        (yyval.node) = (yyvsp[0].node);
    }
#line 11379 "parser.tab.c"
    break;

  case 624: /* ClassInstanceCreationExpression: Primary '.' UnqualifiedClassInstanceCreationExpression  */
#line 4144 "parser.y"
                                                             {
        ast_prepend_child((yyvsp[0].node), (yyvsp[-2].node));
        (yyvsp[0].node)->scope = (yyvsp[-2].node);
        (yyval.node) = (yyvsp[0].node);
    }
#line 11389 "parser.tab.c"
    break;

  case 625: /* UnqualifiedClassInstanceCreationExpression: NEW TypeArguments ClassOrInterfaceTypeToInstantiate '(' ArgumentList ')' ClassBody  */
#line 4153 "parser.y"
                                                                                       {
        AstNode *type = (yyvsp[-4].node);
        if ((yyvsp[-5].node)) {
            ast_add_child(type, (yyvsp[-5].node));
        }
        (yyval.node) = make_new_class_core(type, (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-6]));
    }
#line 11401 "parser.tab.c"
    break;

  case 626: /* UnqualifiedClassInstanceCreationExpression: NEW TypeArguments ClassOrInterfaceTypeToInstantiate '(' ArgumentList ')'  */
#line 4160 "parser.y"
                                                                               {
        AstNode *type = (yyvsp[-3].node);
        if ((yyvsp[-4].node)) {
            ast_add_child(type, (yyvsp[-4].node));
        }
        (yyval.node) = make_new_class_core(type, (yyvsp[-1].node), NULL, (yylsp[-5]));
    }
#line 11413 "parser.tab.c"
    break;

  case 627: /* UnqualifiedClassInstanceCreationExpression: NEW TypeArguments ClassOrInterfaceTypeToInstantiate '(' ')' ClassBody  */
#line 4167 "parser.y"
                                                                            {
        AstNode *type = (yyvsp[-3].node);
        if ((yyvsp[-4].node)) {
            ast_add_child(type, (yyvsp[-4].node));
        }
        (yyval.node) = make_new_class_core(type, make_list_node(AST_ARGUMENT_LIST, (yyloc)), (yyvsp[0].node), (yylsp[-5]));
    }
#line 11425 "parser.tab.c"
    break;

  case 628: /* UnqualifiedClassInstanceCreationExpression: NEW TypeArguments ClassOrInterfaceTypeToInstantiate '(' ')'  */
#line 4174 "parser.y"
                                                                  {
        AstNode *type = (yyvsp[-2].node);
        if ((yyvsp[-3].node)) {
            ast_add_child(type, (yyvsp[-3].node));
        }
        (yyval.node) = make_new_class_core(type, make_list_node(AST_ARGUMENT_LIST, (yyloc)), NULL, (yylsp[-4]));
    }
#line 11437 "parser.tab.c"
    break;

  case 629: /* UnqualifiedClassInstanceCreationExpression: NEW ClassOrInterfaceTypeToInstantiate '(' ArgumentList ')' ClassBody  */
#line 4181 "parser.y"
                                                                           {
        (yyval.node) = make_new_class_core((yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-5]));
    }
#line 11445 "parser.tab.c"
    break;

  case 630: /* UnqualifiedClassInstanceCreationExpression: NEW ClassOrInterfaceTypeToInstantiate '(' ArgumentList ')'  */
#line 4184 "parser.y"
                                                                 {
        (yyval.node) = make_new_class_core((yyvsp[-3].node), (yyvsp[-1].node), NULL, (yylsp[-4]));
    }
#line 11453 "parser.tab.c"
    break;

  case 631: /* UnqualifiedClassInstanceCreationExpression: NEW ClassOrInterfaceTypeToInstantiate '(' ')' ClassBody  */
#line 4187 "parser.y"
                                                              {
        (yyval.node) = make_new_class_core((yyvsp[-3].node), make_list_node(AST_ARGUMENT_LIST, (yyloc)), (yyvsp[0].node), (yylsp[-4]));
    }
#line 11461 "parser.tab.c"
    break;

  case 632: /* UnqualifiedClassInstanceCreationExpression: NEW ClassOrInterfaceTypeToInstantiate '(' ')'  */
#line 4190 "parser.y"
                                                    {
        (yyval.node) = make_new_class_core((yyvsp[-2].node), make_list_node(AST_ARGUMENT_LIST, (yyloc)), NULL, (yylsp[-3]));
    }
#line 11469 "parser.tab.c"
    break;

  case 633: /* ClassOrInterfaceTypeToInstantiate: AnnotationIdentifiers TypeArgumentsOrDiamond  */
#line 4197 "parser.y"
                                                 {
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-1].node);
    }
#line 11480 "parser.tab.c"
    break;

  case 634: /* ClassOrInterfaceTypeToInstantiate: AnnotationIdentifiers  */
#line 4203 "parser.y"
                            {
        (yyval.node) = (yyvsp[0].node);
    }
#line 11488 "parser.tab.c"
    break;

  case 635: /* AnnotationIdentifiers: Annotations IDENTIFIER_AnnotationIdentifiers  */
#line 4210 "parser.y"
                                                 {
        ast_prepend_child((yyvsp[0].node), (yyvsp[-1].node));
        (yyval.node) = (yyvsp[0].node);
    }
#line 11497 "parser.tab.c"
    break;

  case 636: /* AnnotationIdentifiers: IDENTIFIER_AnnotationIdentifiers  */
#line 4214 "parser.y"
                                       { (yyval.node) = (yyvsp[0].node); }
#line 11503 "parser.tab.c"
    break;

  case 637: /* AnnotationIdentifiers: AnnotationIdentifiers '.' Annotations IDENTIFIER_AnnotationIdentifiers  */
#line 4215 "parser.y"
                                                                             {
        ast_prepend_child((yyvsp[0].node), (yyvsp[-1].node));
        (yyval.node) = make_field_access_node((yyvsp[-3].node), (yyvsp[0].node), (yylsp[-2]));
    }
#line 11512 "parser.tab.c"
    break;

  case 638: /* AnnotationIdentifiers: AnnotationIdentifiers '.' IDENTIFIER_AnnotationIdentifiers  */
#line 4219 "parser.y"
                                                                 {
        (yyval.node) = make_field_access_node((yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]));
    }
#line 11520 "parser.tab.c"
    break;

  case 639: /* TypeArgumentsOrDiamond: TypeArguments  */
#line 4227 "parser.y"
                  {
        (yyval.node) = (yyvsp[0].node);
    }
#line 11528 "parser.tab.c"
    break;

  case 640: /* TypeArgumentsOrDiamond: DIAMOND  */
#line 4230 "parser.y"
              {
        AstNode *list = make_list_node(AST_TYPE_ARGUMENT_LIST, (yylsp[0]));
        ast_set_text(list, "<>");
        (yyval.node) = list;
    }
#line 11538 "parser.tab.c"
    break;

  case 641: /* FieldAccess: Primary '.' TYPE_IDENTIFIER  */
#line 4239 "parser.y"
                                {
        (yyval.node) = make_field_access_node((yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]));
    }
#line 11546 "parser.tab.c"
    break;

  case 642: /* FieldAccess: SUPER '.' TYPE_IDENTIFIER  */
#line 4242 "parser.y"
                                {
        (yyval.node) = make_field_access_node(make_keyword_leaf("super", (yylsp[-2])), (yyvsp[0].node), (yylsp[-1]));
    }
#line 11554 "parser.tab.c"
    break;

  case 643: /* FieldAccess: CommonName '.' SUPER '.' TYPE_IDENTIFIER  */
#line 4246 "parser.y"
                                               {
        AstNode *super_node = make_field_access_node((yyvsp[-4].node), make_keyword_leaf("super", (yylsp[-2])), (yylsp[-3]));
        (yyval.node) = make_field_access_node(super_node, (yyvsp[0].node), (yylsp[-1]));
    }
#line 11563 "parser.tab.c"
    break;

  case 644: /* FieldAccess: Primary '.' IdentifierComplement  */
#line 4250 "parser.y"
                                       {
        (yyval.node) = make_field_access_node((yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]));
    }
#line 11571 "parser.tab.c"
    break;

  case 645: /* FieldAccess: SUPER '.' IdentifierComplement  */
#line 4253 "parser.y"
                                     {
        (yyval.node) = make_field_access_node(make_keyword_leaf("super", (yylsp[-2])), (yyvsp[0].node), (yylsp[-1]));
    }
#line 11579 "parser.tab.c"
    break;

  case 646: /* FieldAccess: CommonName '.' SUPER '.' IdentifierComplement  */
#line 4257 "parser.y"
                                                    {
        AstNode *super_node = make_field_access_node((yyvsp[-4].node), make_keyword_leaf("super", (yylsp[-2])), (yylsp[-3]));
        (yyval.node) = make_field_access_node(super_node, (yyvsp[0].node), (yylsp[-1]));
    }
#line 11588 "parser.tab.c"
    break;

  case 647: /* ArrayAccess: CommonName LBRACK_ArrayAccess Expression ']'  */
#line 4265 "parser.y"
                                                 {
        (yyval.node) = make_array_access_node((yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-2]));
    }
#line 11596 "parser.tab.c"
    break;

  case 648: /* ArrayAccess: PrimaryNoNewArray LBRACK_ArrayAccess Expression ']'  */
#line 4269 "parser.y"
                                                          {
        (yyval.node) = make_array_access_node((yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-2]));
    }
#line 11604 "parser.tab.c"
    break;

  case 649: /* ArrayAccess: NEW PrimitiveType Dims ArrayInitializer LBRACK_ArrayAccess Expression ']'  */
#line 4272 "parser.y"
                                                                                {
        AstNode *type = (yyvsp[-5].node);
        attach_dims(type, (yyvsp[-4].node));
        AstNode *base = make_array_creation_node(type, NULL, NULL, (yyvsp[-3].node), (yylsp[-6]));
        (yyval.node) = make_array_access_node(base, (yyvsp[-1].node), (yylsp[-2]));
    }
#line 11615 "parser.tab.c"
    break;

  case 650: /* ArrayAccess: NEW UnannPrimitiveType Dims ArrayInitializer LBRACK_ArrayAccess Expression ']'  */
#line 4278 "parser.y"
                                                                                     {
        AstNode *type = (yyvsp[-5].node);
        attach_dims(type, (yyvsp[-4].node));
        AstNode *base = make_array_creation_node(type, NULL, NULL, (yyvsp[-3].node), (yylsp[-6]));
        (yyval.node) = make_array_access_node(base, (yyvsp[-1].node), (yylsp[-2]));
    }
#line 11626 "parser.tab.c"
    break;

  case 651: /* ArrayAccess: NEW ClassOrInterfaceType Dims ArrayInitializer LBRACK_ArrayAccess Expression ']'  */
#line 4284 "parser.y"
                                                                                       {
        AstNode *type = (yyvsp[-5].node);
        attach_dims(type, (yyvsp[-4].node));
        AstNode *base = make_array_creation_node(type, NULL, NULL, (yyvsp[-3].node), (yylsp[-6]));
        (yyval.node) = make_array_access_node(base, (yyvsp[-1].node), (yylsp[-2]));
    }
#line 11637 "parser.tab.c"
    break;

  case 652: /* MethodInvocation: IDENTIFIER_MethodDeclarator '(' ArgumentList ')'  */
#line 4323 "parser.y"
                                                     {
        (yyval.node) = make_method_invocation_node(NULL, NULL, (yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-3]));
    }
#line 11645 "parser.tab.c"
    break;

  case 653: /* MethodInvocation: IDENTIFIER_MethodDeclarator '(' ')'  */
#line 4326 "parser.y"
                                          {
        (yyval.node) = make_method_invocation_node(NULL, NULL, (yyvsp[-2].node), NULL, (yylsp[-2]));
    }
#line 11653 "parser.tab.c"
    break;

  case 654: /* MethodInvocation: CommonName '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ArgumentList ')'  */
#line 4333 "parser.y"
                                                                                    {
        (yyval.node) = make_method_invocation_node((yyvsp[-6].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-5]));
    }
#line 11661 "parser.tab.c"
    break;

  case 655: /* MethodInvocation: CommonName '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ')'  */
#line 4336 "parser.y"
                                                                       {
        (yyval.node) = make_method_invocation_node((yyvsp[-5].node), (yyvsp[-3].node), (yyvsp[-2].node), NULL, (yylsp[-4]));
    }
#line 11669 "parser.tab.c"
    break;

  case 656: /* MethodInvocation: CommonName '.' IDENTIFIER_MethodDeclarator '(' ArgumentList ')'  */
#line 4339 "parser.y"
                                                                      {
        (yyval.node) = make_method_invocation_node((yyvsp[-5].node), NULL, (yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-4]));
    }
#line 11677 "parser.tab.c"
    break;

  case 657: /* MethodInvocation: CommonName '.' IDENTIFIER_MethodDeclarator '(' ')'  */
#line 4342 "parser.y"
                                                         {
        (yyval.node) = make_method_invocation_node((yyvsp[-4].node), NULL, (yyvsp[-2].node), NULL, (yylsp[-3]));
    }
#line 11685 "parser.tab.c"
    break;

  case 658: /* MethodInvocation: Primary '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ArgumentList ')'  */
#line 4345 "parser.y"
                                                                                 {
        (yyval.node) = make_method_invocation_node((yyvsp[-6].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-5]));
    }
#line 11693 "parser.tab.c"
    break;

  case 659: /* MethodInvocation: Primary '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ')'  */
#line 4348 "parser.y"
                                                                    {
        (yyval.node) = make_method_invocation_node((yyvsp[-5].node), (yyvsp[-3].node), (yyvsp[-2].node), NULL, (yylsp[-4]));
    }
#line 11701 "parser.tab.c"
    break;

  case 660: /* MethodInvocation: Primary '.' IDENTIFIER_MethodDeclarator '(' ArgumentList ')'  */
#line 4351 "parser.y"
                                                                   {
        (yyval.node) = make_method_invocation_node((yyvsp[-5].node), NULL, (yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-4]));
    }
#line 11709 "parser.tab.c"
    break;

  case 661: /* MethodInvocation: Primary '.' IDENTIFIER_MethodDeclarator '(' ')'  */
#line 4354 "parser.y"
                                                      {
        (yyval.node) = make_method_invocation_node((yyvsp[-4].node), NULL, (yyvsp[-2].node), NULL, (yylsp[-3]));
    }
#line 11717 "parser.tab.c"
    break;

  case 662: /* MethodInvocation: SUPER '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ArgumentList ')'  */
#line 4357 "parser.y"
                                                                               {
        (yyval.node) = make_method_invocation_node(make_keyword_leaf("super", (yylsp[-6])), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-5]));
    }
#line 11725 "parser.tab.c"
    break;

  case 663: /* MethodInvocation: SUPER '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ')'  */
#line 4360 "parser.y"
                                                                  {
        (yyval.node) = make_method_invocation_node(make_keyword_leaf("super", (yylsp[-5])), (yyvsp[-3].node), (yyvsp[-2].node), NULL, (yylsp[-4]));
    }
#line 11733 "parser.tab.c"
    break;

  case 664: /* MethodInvocation: SUPER '.' IDENTIFIER_MethodDeclarator '(' ArgumentList ')'  */
#line 4363 "parser.y"
                                                                 {
        (yyval.node) = make_method_invocation_node(make_keyword_leaf("super", (yylsp[-5])), NULL, (yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-4]));
    }
#line 11741 "parser.tab.c"
    break;

  case 665: /* MethodInvocation: SUPER '.' IDENTIFIER_MethodDeclarator '(' ')'  */
#line 4366 "parser.y"
                                                    {
        (yyval.node) = make_method_invocation_node(make_keyword_leaf("super", (yylsp[-4])), NULL, (yyvsp[-2].node), NULL, (yylsp[-3]));
    }
#line 11749 "parser.tab.c"
    break;

  case 666: /* MethodInvocation: CommonName '.' SUPER '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ArgumentList ')'  */
#line 4373 "parser.y"
                                                                                              {
        AstNode *super_node = make_field_access_node((yyvsp[-8].node), make_keyword_leaf("super", (yylsp[-6])), (yylsp[-7]));
        (yyval.node) = make_method_invocation_node(super_node, (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-5]));
    }
#line 11758 "parser.tab.c"
    break;

  case 667: /* MethodInvocation: CommonName '.' SUPER '.' TypeArguments IDENTIFIER_MethodDeclarator '(' ')'  */
#line 4377 "parser.y"
                                                                                 {
        AstNode *super_node = make_field_access_node((yyvsp[-7].node), make_keyword_leaf("super", (yylsp[-5])), (yylsp[-6]));
        (yyval.node) = make_method_invocation_node(super_node, (yyvsp[-3].node), (yyvsp[-2].node), NULL, (yylsp[-4]));
    }
#line 11767 "parser.tab.c"
    break;

  case 668: /* MethodInvocation: CommonName '.' SUPER '.' IDENTIFIER_MethodDeclarator '(' ArgumentList ')'  */
#line 4381 "parser.y"
                                                                                {
        AstNode *super_node = make_field_access_node((yyvsp[-7].node), make_keyword_leaf("super", (yylsp[-5])), (yylsp[-6]));
        (yyval.node) = make_method_invocation_node(super_node, NULL, (yyvsp[-3].node), (yyvsp[-1].node), (yylsp[-4]));
    }
#line 11776 "parser.tab.c"
    break;

  case 669: /* MethodInvocation: CommonName '.' SUPER '.' IDENTIFIER_MethodDeclarator '(' ')'  */
#line 4385 "parser.y"
                                                                   {
        AstNode *super_node = make_field_access_node((yyvsp[-6].node), make_keyword_leaf("super", (yylsp[-4])), (yylsp[-5]));
        (yyval.node) = make_method_invocation_node(super_node, NULL, (yyvsp[-2].node), NULL, (yylsp[-3]));
    }
#line 11785 "parser.tab.c"
    break;

  case 670: /* ArgumentList: Expression  */
#line 4393 "parser.y"
                {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        if ((yyvsp[0].node)) {
            ast_add_child((yyval.node), (yyvsp[0].node));
        }
    }
#line 11796 "parser.tab.c"
    break;

  case 671: /* ArgumentList: ArgumentList ',' Expression  */
#line 4399 "parser.y"
                                  {
        if ((yyvsp[0].node)) {
            ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        }
        (yyval.node) = (yyvsp[-2].node);
    }
#line 11807 "parser.tab.c"
    break;

  case 672: /* MethodReference: Primary DOUBLE_COLON TypeArguments TYPE_IDENTIFIER  */
#line 4411 "parser.y"
                                                       {
        (yyval.node) = make_method_reference_node((yyvsp[-3].node), (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-2]));
    }
#line 11815 "parser.tab.c"
    break;

  case 673: /* MethodReference: Primary DOUBLE_COLON TYPE_IDENTIFIER  */
#line 4414 "parser.y"
                                           {
        (yyval.node) = make_method_reference_node((yyvsp[-2].node), NULL, (yyvsp[0].node), (yylsp[-1]));
    }
#line 11823 "parser.tab.c"
    break;

  case 674: /* MethodReference: ReferenceType DOUBLE_COLON TypeArguments TYPE_IDENTIFIER  */
#line 4417 "parser.y"
                                                               {
        (yyval.node) = make_method_reference_node((yyvsp[-3].node), (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-2]));
    }
#line 11831 "parser.tab.c"
    break;

  case 675: /* MethodReference: ReferenceType DOUBLE_COLON TYPE_IDENTIFIER  */
#line 4420 "parser.y"
                                                 {
        (yyval.node) = make_method_reference_node((yyvsp[-2].node), NULL, (yyvsp[0].node), (yylsp[-1]));
    }
#line 11839 "parser.tab.c"
    break;

  case 676: /* MethodReference: SUPER DOUBLE_COLON TypeArguments TYPE_IDENTIFIER  */
#line 4423 "parser.y"
                                                       {
        (yyval.node) = make_method_reference_node(make_keyword_leaf("super", (yylsp[-3])), (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-2]));
    }
#line 11847 "parser.tab.c"
    break;

  case 677: /* MethodReference: SUPER DOUBLE_COLON TYPE_IDENTIFIER  */
#line 4426 "parser.y"
                                         {
        (yyval.node) = make_method_reference_node(make_keyword_leaf("super", (yylsp[-2])), NULL, (yyvsp[0].node), (yylsp[-1]));
    }
#line 11855 "parser.tab.c"
    break;

  case 678: /* MethodReference: CommonName '.' SUPER DOUBLE_COLON TypeArguments TYPE_IDENTIFIER  */
#line 4431 "parser.y"
                                                                      {
        AstNode *super_node = make_field_access_node((yyvsp[-5].node), make_keyword_leaf("super", (yylsp[-3])), (yylsp[-4]));
        (yyval.node) = make_method_reference_node(super_node, (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-2]));
    }
#line 11864 "parser.tab.c"
    break;

  case 679: /* MethodReference: CommonName '.' SUPER DOUBLE_COLON TYPE_IDENTIFIER  */
#line 4435 "parser.y"
                                                        {
        AstNode *super_node = make_field_access_node((yyvsp[-4].node), make_keyword_leaf("super", (yylsp[-2])), (yylsp[-3]));
        (yyval.node) = make_method_reference_node(super_node, NULL, (yyvsp[0].node), (yylsp[-1]));
    }
#line 11873 "parser.tab.c"
    break;

  case 680: /* MethodReference: Primary DOUBLE_COLON TypeArguments IdentifierComplement  */
#line 4441 "parser.y"
                                                              {
        (yyval.node) = make_method_reference_node((yyvsp[-3].node), (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-2]));
    }
#line 11881 "parser.tab.c"
    break;

  case 681: /* MethodReference: Primary DOUBLE_COLON IdentifierComplement  */
#line 4444 "parser.y"
                                                {
        (yyval.node) = make_method_reference_node((yyvsp[-2].node), NULL, (yyvsp[0].node), (yylsp[-1]));
    }
#line 11889 "parser.tab.c"
    break;

  case 682: /* MethodReference: ReferenceType DOUBLE_COLON TypeArguments IdentifierComplement  */
#line 4447 "parser.y"
                                                                    {
        (yyval.node) = make_method_reference_node((yyvsp[-3].node), (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-2]));
    }
#line 11897 "parser.tab.c"
    break;

  case 683: /* MethodReference: ReferenceType DOUBLE_COLON IdentifierComplement  */
#line 4450 "parser.y"
                                                      {
        (yyval.node) = make_method_reference_node((yyvsp[-2].node), NULL, (yyvsp[0].node), (yylsp[-1]));
    }
#line 11905 "parser.tab.c"
    break;

  case 684: /* MethodReference: SUPER DOUBLE_COLON TypeArguments IdentifierComplement  */
#line 4453 "parser.y"
                                                            {
        (yyval.node) = make_method_reference_node(make_keyword_leaf("super", (yylsp[-3])), (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-2]));
    }
#line 11913 "parser.tab.c"
    break;

  case 685: /* MethodReference: SUPER DOUBLE_COLON IdentifierComplement  */
#line 4456 "parser.y"
                                              {
        (yyval.node) = make_method_reference_node(make_keyword_leaf("super", (yylsp[-2])), NULL, (yyvsp[0].node), (yylsp[-1]));
    }
#line 11921 "parser.tab.c"
    break;

  case 686: /* MethodReference: CommonName '.' SUPER DOUBLE_COLON TypeArguments IdentifierComplement  */
#line 4461 "parser.y"
                                                                           {
        AstNode *super_node = make_field_access_node((yyvsp[-5].node), make_keyword_leaf("super", (yylsp[-3])), (yylsp[-4]));
        (yyval.node) = make_method_reference_node(super_node, (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-2]));
    }
#line 11930 "parser.tab.c"
    break;

  case 687: /* MethodReference: CommonName '.' SUPER DOUBLE_COLON IdentifierComplement  */
#line 4465 "parser.y"
                                                             {
        AstNode *super_node = make_field_access_node((yyvsp[-4].node), make_keyword_leaf("super", (yylsp[-2])), (yylsp[-3]));
        (yyval.node) = make_method_reference_node(super_node, NULL, (yyvsp[0].node), (yylsp[-1]));
    }
#line 11939 "parser.tab.c"
    break;

  case 688: /* MethodReference: ReferenceType DOUBLE_COLON TypeArguments NEW  */
#line 4473 "parser.y"
                                                   {
        (yyval.node) = make_method_reference_node((yyvsp[-3].node), (yyvsp[-1].node), make_keyword_leaf("new", (yylsp[0])), (yylsp[-2]));
    }
#line 11947 "parser.tab.c"
    break;

  case 689: /* MethodReference: ReferenceType DOUBLE_COLON NEW  */
#line 4476 "parser.y"
                                     {
        (yyval.node) = make_method_reference_node((yyvsp[-2].node), NULL, make_keyword_leaf("new", (yylsp[0])), (yylsp[-1]));
    }
#line 11955 "parser.tab.c"
    break;

  case 690: /* ArrayCreationExpression: NEW PrimitiveType DimExprs Dims  */
#line 4483 "parser.y"
                                    {
        attach_dims((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = make_array_creation_node((yyvsp[-2].node), (yyvsp[-1].node), NULL, NULL, (yylsp[-3]));
    }
#line 11964 "parser.tab.c"
    break;

  case 691: /* ArrayCreationExpression: NEW PrimitiveType DimExprs  */
#line 4487 "parser.y"
                                 {
        (yyval.node) = make_array_creation_node((yyvsp[-1].node), (yyvsp[0].node), NULL, NULL, (yylsp[-2]));
    }
#line 11972 "parser.tab.c"
    break;

  case 692: /* ArrayCreationExpression: NEW UnannPrimitiveType DimExprs Dims  */
#line 4490 "parser.y"
                                           {
        attach_dims((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = make_array_creation_node((yyvsp[-2].node), (yyvsp[-1].node), NULL, NULL, (yylsp[-3]));
    }
#line 11981 "parser.tab.c"
    break;

  case 693: /* ArrayCreationExpression: NEW UnannPrimitiveType DimExprs  */
#line 4494 "parser.y"
                                      {
        (yyval.node) = make_array_creation_node((yyvsp[-1].node), (yyvsp[0].node), NULL, NULL, (yylsp[-2]));
    }
#line 11989 "parser.tab.c"
    break;

  case 694: /* ArrayCreationExpression: NEW ClassOrInterfaceType DimExprs Dims  */
#line 4497 "parser.y"
                                             {
        attach_dims((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = make_array_creation_node((yyvsp[-2].node), (yyvsp[-1].node), NULL, NULL, (yylsp[-3]));
    }
#line 11998 "parser.tab.c"
    break;

  case 695: /* ArrayCreationExpression: NEW ClassOrInterfaceType DimExprs  */
#line 4501 "parser.y"
                                        {
        (yyval.node) = make_array_creation_node((yyvsp[-1].node), (yyvsp[0].node), NULL, NULL, (yylsp[-2]));
    }
#line 12006 "parser.tab.c"
    break;

  case 696: /* ArrayCreationExpression: NEW PrimitiveType Dims ArrayInitializer  */
#line 4504 "parser.y"
                                              {
        attach_dims((yyvsp[-2].node), (yyvsp[-1].node));
        (yyval.node) = make_array_creation_node((yyvsp[-2].node), NULL, NULL, (yyvsp[0].node), (yylsp[-3]));
    }
#line 12015 "parser.tab.c"
    break;

  case 697: /* ArrayCreationExpression: NEW UnannPrimitiveType Dims ArrayInitializer  */
#line 4508 "parser.y"
                                                   {
        attach_dims((yyvsp[-2].node), (yyvsp[-1].node));
        (yyval.node) = make_array_creation_node((yyvsp[-2].node), NULL, NULL, (yyvsp[0].node), (yylsp[-3]));
    }
#line 12024 "parser.tab.c"
    break;

  case 698: /* ArrayCreationExpression: NEW ClassOrInterfaceType Dims ArrayInitializer  */
#line 4512 "parser.y"
                                                     {
        attach_dims((yyvsp[-2].node), (yyvsp[-1].node));
        (yyval.node) = make_array_creation_node((yyvsp[-2].node), NULL, NULL, (yyvsp[0].node), (yylsp[-3]));
    }
#line 12033 "parser.tab.c"
    break;

  case 699: /* DimExprs: DimExpr  */
#line 4520 "parser.y"
             {
        (yyval.node) = make_list_node(AST_DIM_EXPR_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 12042 "parser.tab.c"
    break;

  case 700: /* DimExprs: DimExprs DimExpr  */
#line 4524 "parser.y"
                       {
        ast_add_child((yyvsp[-1].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-1].node);
    }
#line 12051 "parser.tab.c"
    break;

  case 701: /* DimExpr: Annotations_Dims LBRACK_ArrayAccess Expression ']'  */
#line 4532 "parser.y"
                                                       {
        AstNode *dim = make_dim_node((yylsp[-2]));
        if ((yyvsp[-3].node)) {
            ast_add_child(dim, (yyvsp[-3].node));
        }
        ast_add_child(dim, (yyvsp[-1].node));
        (yyval.node) = dim;
    }
#line 12064 "parser.tab.c"
    break;

  case 702: /* DimExpr: LBRACK_ArrayAccess Expression ']'  */
#line 4540 "parser.y"
                                        {
        AstNode *dim = make_dim_node((yylsp[-2]));
        ast_add_child(dim, (yyvsp[-1].node));
        (yyval.node) = dim;
    }
#line 12074 "parser.tab.c"
    break;

  case 703: /* Expression: LambdaExpression  */
#line 4550 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 12080 "parser.tab.c"
    break;

  case 704: /* Expression: AssignmentExpression  */
#line 4551 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 12086 "parser.tab.c"
    break;

  case 705: /* LambdaExpression: LambdaParameters ARROW LambdaBody  */
#line 4558 "parser.y"
                                      {
        (yyval.node) = make_lambda_node((yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]));
    }
#line 12094 "parser.tab.c"
    break;

  case 706: /* LambdaParameters: '(' LambdaParameterList ')'  */
#line 4565 "parser.y"
                                {
        (yyval.node) = (yyvsp[-1].node);
    }
#line 12102 "parser.tab.c"
    break;

  case 707: /* LambdaParameters: '(' ')'  */
#line 4568 "parser.y"
              {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
    }
#line 12110 "parser.tab.c"
    break;

  case 708: /* LambdaParameters: TYPE_IDENTIFIER_IdentifierforLambdaParameterList  */
#line 4571 "parser.y"
                                                       { (yyval.node) = (yyvsp[0].node); }
#line 12116 "parser.tab.c"
    break;

  case 709: /* LambdaParameters: IdentifierComplement_IdentifierforLambdaParameterList  */
#line 4572 "parser.y"
                                                            { (yyval.node) = (yyvsp[0].node); }
#line 12122 "parser.tab.c"
    break;

  case 710: /* LambdaParameterList: LambdaParameterforLambdaParameterList  */
#line 4580 "parser.y"
                                          { (yyval.node) = (yyvsp[0].node); }
#line 12128 "parser.tab.c"
    break;

  case 711: /* LambdaParameterList: IdentifierforLambdaParameterList  */
#line 4581 "parser.y"
                                       { (yyval.node) = (yyvsp[0].node); }
#line 12134 "parser.tab.c"
    break;

  case 712: /* IdentifierforLambdaParameterList: TYPE_IDENTIFIER_IdentifierforLambdaParameterList  */
#line 4586 "parser.y"
                                                     {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 12143 "parser.tab.c"
    break;

  case 713: /* IdentifierforLambdaParameterList: IdentifierComplement_IdentifierforLambdaParameterList  */
#line 4590 "parser.y"
                                                            {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 12152 "parser.tab.c"
    break;

  case 714: /* IdentifierforLambdaParameterList: IdentifierforLambdaParameterList ',' TYPE_IDENTIFIER_IdentifierforLambdaParameterList  */
#line 4594 "parser.y"
                                                                                            {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 12161 "parser.tab.c"
    break;

  case 715: /* IdentifierforLambdaParameterList: IdentifierforLambdaParameterList ',' IdentifierComplement_IdentifierforLambdaParameterList  */
#line 4598 "parser.y"
                                                                                                 {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 12170 "parser.tab.c"
    break;

  case 716: /* IdentifierComplement_IdentifierforLambdaParameterList: VAR_IdentifierComplement_IdentifierforLambdaParameterList  */
#line 4606 "parser.y"
                                                              { (yyval.node) = (yyvsp[0].node); }
#line 12176 "parser.tab.c"
    break;

  case 717: /* IdentifierComplement_IdentifierforLambdaParameterList: YIELD_IdentifierComplement_IdentifierforLambdaParameterList  */
#line 4607 "parser.y"
                                                                  { (yyval.node) = (yyvsp[0].node); }
#line 12182 "parser.tab.c"
    break;

  case 718: /* LambdaParameterforLambdaParameterList: LambdaParameter  */
#line 4612 "parser.y"
                    {
        (yyval.node) = make_list_node(AST_ARGUMENT_LIST, (yyloc));
        ast_add_child((yyval.node), (yyvsp[0].node));
    }
#line 12191 "parser.tab.c"
    break;

  case 719: /* LambdaParameterforLambdaParameterList: LambdaParameterforLambdaParameterList ',' LambdaParameter  */
#line 4616 "parser.y"
                                                                {
        ast_add_child((yyvsp[-2].node), (yyvsp[0].node));
        (yyval.node) = (yyvsp[-2].node);
    }
#line 12200 "parser.tab.c"
    break;

  case 720: /* LambdaParameter: VariableModifiers LambdaParameterType TYPE_IDENTIFIER Dims  */
#line 4624 "parser.y"
                                                               {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 12208 "parser.tab.c"
    break;

  case 721: /* LambdaParameter: Annotations LambdaParameterType TYPE_IDENTIFIER Dims  */
#line 4627 "parser.y"
                                                           {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 12216 "parser.tab.c"
    break;

  case 722: /* LambdaParameter: LambdaParameterType TYPE_IDENTIFIER Dims  */
#line 4630 "parser.y"
                                               {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 12224 "parser.tab.c"
    break;

  case 723: /* LambdaParameter: VariableModifiers LambdaParameterType TYPE_IDENTIFIER  */
#line 4633 "parser.y"
                                                            {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 12232 "parser.tab.c"
    break;

  case 724: /* LambdaParameter: Annotations LambdaParameterType TYPE_IDENTIFIER  */
#line 4636 "parser.y"
                                                      {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 12240 "parser.tab.c"
    break;

  case 725: /* LambdaParameter: LambdaParameterType TYPE_IDENTIFIER  */
#line 4639 "parser.y"
                                          {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 12248 "parser.tab.c"
    break;

  case 726: /* LambdaParameter: VariableModifiers LambdaParameterType IdentifierComplement Dims  */
#line 4642 "parser.y"
                                                                      {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 12256 "parser.tab.c"
    break;

  case 727: /* LambdaParameter: Annotations LambdaParameterType IdentifierComplement Dims  */
#line 4645 "parser.y"
                                                                {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 12264 "parser.tab.c"
    break;

  case 728: /* LambdaParameter: LambdaParameterType IdentifierComplement Dims  */
#line 4648 "parser.y"
                                                    {
        (yyval.node) = make_parameter_with_dims((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yyloc));
    }
#line 12272 "parser.tab.c"
    break;

  case 729: /* LambdaParameter: VariableModifiers LambdaParameterType IdentifierComplement  */
#line 4651 "parser.y"
                                                                 {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 12280 "parser.tab.c"
    break;

  case 730: /* LambdaParameter: Annotations LambdaParameterType IdentifierComplement  */
#line 4654 "parser.y"
                                                           {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 12288 "parser.tab.c"
    break;

  case 731: /* LambdaParameter: LambdaParameterType IdentifierComplement  */
#line 4657 "parser.y"
                                               {
        (yyval.node) = make_parameter_with_dims((yyvsp[-1].node), (yyvsp[0].node), NULL, (yyloc));
    }
#line 12296 "parser.tab.c"
    break;

  case 732: /* LambdaParameter: VariableArityParameter  */
#line 4660 "parser.y"
                             { (yyval.node) = (yyvsp[0].node); }
#line 12302 "parser.tab.c"
    break;

  case 733: /* LambdaParameterType: UnannType  */
#line 4665 "parser.y"
              { (yyval.node) = (yyvsp[0].node); }
#line 12308 "parser.tab.c"
    break;

  case 734: /* LambdaParameterType: VAR  */
#line 4666 "parser.y"
          { (yyval.node) = make_keyword_leaf("var", (yylsp[0])); }
#line 12314 "parser.tab.c"
    break;

  case 735: /* LambdaBody: Expression  */
#line 4671 "parser.y"
               { (yyval.node) = (yyvsp[0].node); }
#line 12320 "parser.tab.c"
    break;

  case 736: /* LambdaBody: Block  */
#line 4672 "parser.y"
            { (yyval.node) = (yyvsp[0].node); }
#line 12326 "parser.tab.c"
    break;

  case 737: /* AssignmentExpression: ConditionalExpression  */
#line 4677 "parser.y"
                          { (yyval.node) = (yyvsp[0].node); }
#line 12332 "parser.tab.c"
    break;

  case 738: /* AssignmentExpression: Assignment  */
#line 4678 "parser.y"
                 { (yyval.node) = (yyvsp[0].node); }
#line 12338 "parser.tab.c"
    break;

  case 739: /* Assignment: LeftHandSide AssignmentOperator Expression  */
#line 4683 "parser.y"
                                               {
        (yyval.node) = make_assignment_node((yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node), (yylsp[-1]));
    }
#line 12346 "parser.tab.c"
    break;

  case 740: /* LeftHandSide: CommonName  */
#line 4690 "parser.y"
               { (yyval.node) = (yyvsp[0].node); }
#line 12352 "parser.tab.c"
    break;

  case 741: /* LeftHandSide: FieldAccess  */
#line 4691 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 12358 "parser.tab.c"
    break;

  case 742: /* LeftHandSide: ArrayAccess  */
#line 4692 "parser.y"
                  { (yyval.node) = (yyvsp[0].node); }
#line 12364 "parser.tab.c"
    break;

  case 743: /* AssignmentOperator: '='  */
#line 4697 "parser.y"
         { (yyval.node) = make_keyword_leaf("=", (yylsp[0])); }
#line 12370 "parser.tab.c"
    break;

  case 744: /* AssignmentOperator: MUL_ASSIGN  */
#line 4698 "parser.y"
                  { (yyval.node) = make_keyword_leaf("*=", (yylsp[0])); }
#line 12376 "parser.tab.c"
    break;

  case 745: /* AssignmentOperator: DIV_ASSIGN  */
#line 4699 "parser.y"
                  { (yyval.node) = make_keyword_leaf("/=", (yylsp[0])); }
#line 12382 "parser.tab.c"
    break;

  case 746: /* AssignmentOperator: MOD_ASSIGN  */
#line 4700 "parser.y"
                 { (yyval.node) = make_keyword_leaf("%=", (yylsp[0])); }
#line 12388 "parser.tab.c"
    break;

  case 747: /* AssignmentOperator: ADD_ASSIGN  */
#line 4701 "parser.y"
                  { (yyval.node) = make_keyword_leaf("+=", (yylsp[0])); }
#line 12394 "parser.tab.c"
    break;

  case 748: /* AssignmentOperator: SUB_ASSIGN  */
#line 4702 "parser.y"
                  { (yyval.node) = make_keyword_leaf("-=", (yylsp[0])); }
#line 12400 "parser.tab.c"
    break;

  case 749: /* AssignmentOperator: SHL_ASSIGN  */
#line 4703 "parser.y"
                 { (yyval.node) = make_keyword_leaf("<<=", (yylsp[0])); }
#line 12406 "parser.tab.c"
    break;

  case 750: /* AssignmentOperator: SHR_ASSIGN  */
#line 4704 "parser.y"
                  { (yyval.node) = make_keyword_leaf(">>=", (yylsp[0])); }
#line 12412 "parser.tab.c"
    break;

  case 751: /* AssignmentOperator: USHR_ASSIGN  */
#line 4705 "parser.y"
                   { (yyval.node) = make_keyword_leaf(">>>=", (yylsp[0])); }
#line 12418 "parser.tab.c"
    break;

  case 752: /* AssignmentOperator: AND_ASSIGN  */
#line 4706 "parser.y"
                  { (yyval.node) = make_keyword_leaf("&=", (yylsp[0])); }
#line 12424 "parser.tab.c"
    break;

  case 753: /* AssignmentOperator: XOR_ASSIGN  */
#line 4707 "parser.y"
                  { (yyval.node) = make_keyword_leaf("^=", (yylsp[0])); }
#line 12430 "parser.tab.c"
    break;

  case 754: /* AssignmentOperator: OR_ASSIGN  */
#line 4708 "parser.y"
                 { (yyval.node) = make_keyword_leaf("|=", (yylsp[0])); }
#line 12436 "parser.tab.c"
    break;

  case 755: /* ConditionalExpression: ConditionalOrExpression  */
#line 4713 "parser.y"
                                                                {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12444 "parser.tab.c"
    break;

  case 756: /* ConditionalExpression: ConditionalOrExpression '?' Expression ':' ConditionalExpression  */
#line 4716 "parser.y"
                                                                                                        {
        (yyval.node) = make_conditional_expr((yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-3]));
    }
#line 12452 "parser.tab.c"
    break;

  case 757: /* ConditionalExpression: ConditionalOrExpression '?' Expression ':' LambdaExpression  */
#line 4719 "parser.y"
                                                                                                   {
        (yyval.node) = make_conditional_expr((yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), (yylsp[-3]));
    }
#line 12460 "parser.tab.c"
    break;

  case 758: /* ConditionalOrExpression: ConditionalAndExpression  */
#line 4726 "parser.y"
                                                                  {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12468 "parser.tab.c"
    break;

  case 759: /* ConditionalOrExpression: ConditionalOrExpression OR ConditionalAndExpression  */
#line 4730 "parser.y"
    {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "||", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12476 "parser.tab.c"
    break;

  case 760: /* ConditionalAndExpression: InclusiveOrExpression  */
#line 4737 "parser.y"
                                                                {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12484 "parser.tab.c"
    break;

  case 761: /* ConditionalAndExpression: ConditionalAndExpression AND InclusiveOrExpression  */
#line 4740 "parser.y"
                                                                                             {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "&&", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12492 "parser.tab.c"
    break;

  case 762: /* InclusiveOrExpression: ExclusiveOrExpression  */
#line 4747 "parser.y"
                                                             {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12500 "parser.tab.c"
    break;

  case 763: /* InclusiveOrExpression: InclusiveOrExpression '|' ExclusiveOrExpression  */
#line 4750 "parser.y"
                                                                                       {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "|", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12508 "parser.tab.c"
    break;

  case 764: /* ExclusiveOrExpression: AndExpression  */
#line 4757 "parser.y"
                                                     {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12516 "parser.tab.c"
    break;

  case 765: /* ExclusiveOrExpression: ExclusiveOrExpression '^' AndExpression  */
#line 4760 "parser.y"
                                                                               {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "^", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12524 "parser.tab.c"
    break;

  case 766: /* AndExpression: EqualityExpression  */
#line 4767 "parser.y"
                                                  {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12532 "parser.tab.c"
    break;

  case 767: /* AndExpression: AndExpression '&' EqualityExpression  */
#line 4770 "parser.y"
                                                                    {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "&", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12540 "parser.tab.c"
    break;

  case 768: /* EqualityExpression: RelationalExpression  */
#line 4777 "parser.y"
                         { (yyval.node) = (yyvsp[0].node); }
#line 12546 "parser.tab.c"
    break;

  case 769: /* EqualityExpression: EqualityExpression EQ RelationalExpression  */
#line 4778 "parser.y"
                                                                               {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "==", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12554 "parser.tab.c"
    break;

  case 770: /* EqualityExpression: EqualityExpression NE RelationalExpression  */
#line 4781 "parser.y"
                                                                               {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "!=", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12562 "parser.tab.c"
    break;

  case 771: /* SimpleRelationalExpression: ShiftExpression  */
#line 4788 "parser.y"
                                                          {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12570 "parser.tab.c"
    break;

  case 772: /* RelationalExpression: SimpleRelationalExpression  */
#line 4795 "parser.y"
                                                                 {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12578 "parser.tab.c"
    break;

  case 773: /* RelationalExpression: SimpleRelationalExpression INSTANCEOF ReferenceType  */
#line 4798 "parser.y"
                                                                                          {
        (yyval.node) = make_instanceof_expr((yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]));
    }
#line 12586 "parser.tab.c"
    break;

  case 774: /* RelationalExpression: SimpleRelationalExpression INSTANCEOF Pattern  */
#line 4801 "parser.y"
                                                                                    {
        (yyval.node) = make_instanceof_expr((yyvsp[-2].node), (yyvsp[0].node), (yylsp[-1]));
    }
#line 12594 "parser.tab.c"
    break;

  case 775: /* RelationalExpression: SimpleRelationalExpression '<' ShiftExpression  */
#line 4804 "parser.y"
                                                                                     {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "<", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12602 "parser.tab.c"
    break;

  case 776: /* RelationalExpression: SimpleRelationalExpression '>' ShiftExpression  */
#line 4807 "parser.y"
                                                                                     {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), ">", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12610 "parser.tab.c"
    break;

  case 777: /* RelationalExpression: SimpleRelationalExpression LE ShiftExpression  */
#line 4810 "parser.y"
                                                                                    {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "<=", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12618 "parser.tab.c"
    break;

  case 778: /* RelationalExpression: SimpleRelationalExpression GE ShiftExpression  */
#line 4813 "parser.y"
                                                                                    {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), ">=", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12626 "parser.tab.c"
    break;

  case 779: /* ShiftExpression: AdditiveExpression  */
#line 4820 "parser.y"
                                                    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12634 "parser.tab.c"
    break;

  case 780: /* ShiftExpression: ShiftExpression SHL_OP AdditiveExpression  */
#line 4823 "parser.y"
                                                                            {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "<<", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12642 "parser.tab.c"
    break;

  case 781: /* ShiftExpression: ShiftExpression SHR_OP AdditiveExpression  */
#line 4826 "parser.y"
                                                                            {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), ">>", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12650 "parser.tab.c"
    break;

  case 782: /* ShiftExpression: ShiftExpression USHR_OP AdditiveExpression  */
#line 4829 "parser.y"
                                                                            {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), ">>>", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12658 "parser.tab.c"
    break;

  case 783: /* AdditiveExpression: MultiplicativeExpression  */
#line 4836 "parser.y"
                                                              {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12666 "parser.tab.c"
    break;

  case 784: /* AdditiveExpression: AdditiveExpression '+' MultiplicativeExpression  */
#line 4839 "parser.y"
                                                                                     {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "+", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12674 "parser.tab.c"
    break;

  case 785: /* AdditiveExpression: AdditiveExpression '-' MultiplicativeExpression  */
#line 4842 "parser.y"
                                                                                     {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "-", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12682 "parser.tab.c"
    break;

  case 786: /* MultiplicativeExpression: UnaryExpression  */
#line 4849 "parser.y"
                    { (yyval.node) = (yyvsp[0].node); }
#line 12688 "parser.tab.c"
    break;

  case 787: /* MultiplicativeExpression: MultiplicativeExpression '*' UnaryExpression  */
#line 4850 "parser.y"
                                                                                        {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "*", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12696 "parser.tab.c"
    break;

  case 788: /* MultiplicativeExpression: MultiplicativeExpression '/' UnaryExpression  */
#line 4853 "parser.y"
                                                                                        {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "/", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12704 "parser.tab.c"
    break;

  case 789: /* MultiplicativeExpression: MultiplicativeExpression '%' UnaryExpression  */
#line 4856 "parser.y"
                                                                                        {
        (yyval.node) = make_binary_expr((yyvsp[-2].node), "%", (yyvsp[0].node), (yylsp[-1]));
    }
#line 12712 "parser.tab.c"
    break;

  case 790: /* UnaryExpression: PreIncrementExpression  */
#line 4863 "parser.y"
                           { (yyval.node) = (yyvsp[0].node); }
#line 12718 "parser.tab.c"
    break;

  case 791: /* UnaryExpression: PreDecrementExpression  */
#line 4864 "parser.y"
                             { (yyval.node) = (yyvsp[0].node); }
#line 12724 "parser.tab.c"
    break;

  case 792: /* UnaryExpression: '+' UnaryExpression  */
#line 4865 "parser.y"
                          {
        (yyval.node) = make_unary_expr("+", (yyvsp[0].node), (yylsp[-1]), false);
    }
#line 12732 "parser.tab.c"
    break;

  case 793: /* UnaryExpression: '-' UnaryExpression  */
#line 4868 "parser.y"
                          {
        (yyval.node) = make_unary_expr("-", (yyvsp[0].node), (yylsp[-1]), false);
    }
#line 12740 "parser.tab.c"
    break;

  case 794: /* UnaryExpression: UnaryExpressionNotPlusMinus  */
#line 4873 "parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 12746 "parser.tab.c"
    break;

  case 795: /* PreIncrementExpression: PREFIX_INC UnaryExpression  */
#line 4878 "parser.y"
                                                {
        (yyval.node) = make_unary_expr("++", (yyvsp[0].node), (yylsp[-1]), false);
    }
#line 12754 "parser.tab.c"
    break;

  case 796: /* PreDecrementExpression: PREFIX_DEC UnaryExpression  */
#line 4885 "parser.y"
                                                {
        (yyval.node) = make_unary_expr("--", (yyvsp[0].node), (yylsp[-1]), false);
    }
#line 12762 "parser.tab.c"
    break;

  case 797: /* UnaryExpressionNotPlusMinus: PostfixExpression  */
#line 4892 "parser.y"
                      { (yyval.node) = (yyvsp[0].node); }
#line 12768 "parser.tab.c"
    break;

  case 798: /* UnaryExpressionNotPlusMinus: '~' UnaryExpression  */
#line 4893 "parser.y"
                          {
        (yyval.node) = make_unary_expr("~", (yyvsp[0].node), (yylsp[-1]), false);
    }
#line 12776 "parser.tab.c"
    break;

  case 799: /* UnaryExpressionNotPlusMinus: '!' UnaryExpression  */
#line 4896 "parser.y"
                          {
        (yyval.node) = make_unary_expr("!", (yyvsp[0].node), (yylsp[-1]), false);
    }
#line 12784 "parser.tab.c"
    break;

  case 800: /* UnaryExpressionNotPlusMinus: CastExpression  */
#line 4899 "parser.y"
                     { (yyval.node) = (yyvsp[0].node); }
#line 12790 "parser.tab.c"
    break;

  case 801: /* UnaryExpressionNotPlusMinus: SwitchExpression  */
#line 4900 "parser.y"
                       { (yyval.node) = (yyvsp[0].node); }
#line 12796 "parser.tab.c"
    break;

  case 802: /* PostfixExpression: Primary  */
#line 4905 "parser.y"
            { (yyval.node) = (yyvsp[0].node); }
#line 12802 "parser.tab.c"
    break;

  case 803: /* PostfixExpression: CommonName  */
#line 4907 "parser.y"
    {
        (yyval.node) = (yyvsp[0].node);
    }
#line 12810 "parser.tab.c"
    break;

  case 804: /* PostfixExpression: PostIncrementExpression  */
#line 4910 "parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 12816 "parser.tab.c"
    break;

  case 805: /* PostfixExpression: PostDecrementExpression  */
#line 4911 "parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 12822 "parser.tab.c"
    break;

  case 806: /* PostIncrementExpression: PostfixExpression INC_OP  */
#line 4916 "parser.y"
                                          {
        (yyval.node) = make_unary_expr("++", (yyvsp[-1].node), (yylsp[0]), true);
    }
#line 12830 "parser.tab.c"
    break;

  case 807: /* PostDecrementExpression: PostfixExpression DEC_OP  */
#line 4923 "parser.y"
                                          {
        (yyval.node) = make_unary_expr("--", (yyvsp[-1].node), (yylsp[0]), true);
    }
#line 12838 "parser.tab.c"
    break;

  case 808: /* CastExpression: '(' PrimitiveType ')' UnaryExpression  */
#line 4930 "parser.y"
                                          {
        (yyval.node) = make_cast_expr((yyvsp[-2].node), (yyvsp[0].node), (yylsp[-3]));
    }
#line 12846 "parser.tab.c"
    break;

  case 809: /* CastExpression: '(' UnannPrimitiveType ')' UnaryExpression  */
#line 4933 "parser.y"
                                                 {
        (yyval.node) = make_cast_expr(make_type_node((yyvsp[-2].node), (yylsp[-2])), (yyvsp[0].node), (yylsp[-3]));
    }
#line 12854 "parser.tab.c"
    break;

  case 810: /* CastExpression: '(' ReferenceType AdditionalBounds RPAREN_CastExpression UnaryExpressionNotPlusMinus  */
#line 4938 "parser.y"
                                                                                           {
        (yyval.node) = make_cast_expr((yyvsp[-3].node), (yyvsp[0].node), (yylsp[-4]));
    }
#line 12862 "parser.tab.c"
    break;

  case 811: /* CastExpression: '(' ReferenceType AdditionalBounds RPAREN_CastExpression LambdaExpression  */
#line 4941 "parser.y"
                                                                                {
        (yyval.node) = make_cast_expr((yyvsp[-3].node), (yyvsp[0].node), (yylsp[-4]));
    }
#line 12870 "parser.tab.c"
    break;

  case 812: /* SwitchExpression: SWITCH '(' Expression ')' SwitchBlock  */
#line 4948 "parser.y"
                                          {
        AstNode *node = make_unary_stmt(AST_SWITCH_EXPR, (yyvsp[-2].node), (yylsp[-4]));
        if ((yyvsp[0].node)) {
            ast_add_child(node, (yyvsp[0].node));
        }
        (yyval.node) = node;
    }
#line 12882 "parser.tab.c"
    break;


#line 12886 "parser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 4958 "parser.y"


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
