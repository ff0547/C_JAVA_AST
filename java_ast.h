#ifndef JAVA_AST_H
#define JAVA_AST_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

// AST 节点类型枚举。
typedef enum AstKind {
    AST_COMPILATION_UNIT,   // 编译单元 (CompilationUnit).
    AST_PACKAGE_DECL,       // package 声明 (PackageDeclaration).
    AST_IMPORT_DECL,        // import 声明 (ImportDeclaration).
    AST_MODULE_DECL,        // module 声明 (ModuleDeclaration).
    AST_REQUIRES_DIRECTIVE, // requires 指令 (RequiresDirective).
    AST_EXPORTS_DIRECTIVE,  // exports 指令 (ExportsDirective).
    AST_OPENS_DIRECTIVE,    // opens 指令 (OpensDirective).
    AST_USES_DIRECTIVE,     // uses 指令 (UsesDirective).
    AST_PROVIDES_DIRECTIVE, // provides 指令 (ProvidesDirective).
    AST_CLASS_DECL,         // 类声明 (ClassDeclaration).
    AST_INTERFACE_DECL,     // 接口声明 (InterfaceDeclaration).
    AST_ANNOTATION_DECL,    // 注解类型声明 (AnnotationTypeDeclaration).
    AST_ENUM_DECL,          // 枚举声明 (EnumDeclaration).
    AST_RECORD_DECL,        // record 声明 (RecordDeclaration).
    AST_FIELD_DECL,         // 字段声明 (FieldDeclaration).
    AST_LOCAL_VAR_DECL,     // 本地变量声明 (LocalVariableDeclaration).
    AST_METHOD_DECL,        // 方法声明 (MethodDeclaration).
    AST_CONSTRUCTOR_DECL,   // 构造器声明 (ConstructorDeclaration).
    AST_PARAMETER,          // 参数 (Parameter).
    AST_TYPE,               // 类型 (Type).
    AST_TYPE_PARAMETER,     // 泛型形参 (TypeParameter).
    AST_TYPE_PARAMETER_LIST,// 泛型形参列表 (TypeParameterList).
    AST_TYPE_ARGUMENT,      // 泛型实参 (TypeArgument).
    AST_TYPE_ARGUMENT_TYPE, // 泛型实参：类型 (TypeArgumentType).
    AST_TYPE_ARGUMENT_WILDCARD, // 泛型实参：通配符 (TypeArgumentWildcard).
    AST_TYPE_ARGUMENT_LIST, // 泛型实参列表 (TypeArgumentList).
    AST_TYPE_BOUND_LIST,    // 泛型边界列表 (TypeBoundList).
    AST_TYPE_BOUND,         // 泛型边界 (TypeBound).
    AST_ADDITIONAL_BOUNDS,  // 额外边界列表 (AdditionalBounds).
    AST_WILDCARD,           // 通配符 (Wildcard).
    AST_WILDCARD_BOUND,     // 通配符边界 (WildcardBound).
    AST_TYPE_PATTERN,       // 类型模式 (TypePattern).
    AST_EXTENDS,            // extends 子句 (ExtendsClause).
    AST_IMPLEMENTS,         // implements 子句 (ImplementsClause).
    AST_PERMITS,            // permits 子句 (PermitsClause).
    AST_MODIFIER_LIST,      // 修饰符列表 (ModifierList).
    AST_ANNOTATION,         // 注解 (Annotation).
    AST_DIM,                // 维度 (Dim).
    AST_DIM_LIST,           // 维度列表 (DimList).
    AST_BLOCK,              // 代码块 (Block).
    AST_ANNOTATION_LIST,    // 注解列表 (AnnotationList).
    AST_STATEMENT_LIST,     // 语句列表 (StatementList).
    AST_STATIC_INIT,        // 静态初始化块 (StaticInitializer).
    AST_INSTANCE_INIT,      // 实例初始化块 (InstanceInitializer).
    AST_IF,                 // if 语句 (IfStatement).
    AST_ELSE_CLAUSE,        // else 子句 (ElseClause).
    AST_SWITCH,             // switch 语句 (SwitchStatement).
    AST_SWITCH_EXPR,        // switch 表达式 (SwitchExpression).
    AST_SWITCH_LABEL,       // switch 标签 (SwitchLabel).
    AST_SWITCH_GROUP,       // switch 语句组 (SwitchGroup).
    AST_SWITCH_RULE,        // switch 规则 (SwitchRule).
    AST_SWITCH_RULE_LIST,   // switch 规则列表 (SwitchRuleList).
    AST_SWITCH_LABEL_LIST,  // switch 标签列表 (SwitchLabelList).
    AST_FOR,                // for 语句 (ForStatement).
    AST_FOR_INIT,           // for 初始化部分 (ForInit).
    AST_FOR_COND,           // for 条件部分 (ForCondition).
    AST_FOR_UPDATE,         // for 更新部分 (ForUpdate).
    AST_FOR_INIT_LIST,      // for 初始化表达式列表 (ForInitList).
    AST_FOR_UPDATE_LIST,    // for 更新表达式列表 (ForUpdateList).
    AST_VAR_DECL_LIST,      // 变量声明列表 (VarDeclList).
    AST_FOR_EACH,           // 增强 for 语句 (ForEachStatement).
    AST_WHILE,              // while 语句 (WhileStatement).
    AST_DO_WHILE,           // do-while 语句 (DoWhileStatement).
    AST_TRY,                // try 语句 (TryStatement).
    AST_CATCH,              // catch 子句 (CatchClause).
    AST_FINALLY,            // finally 子句 (FinallyClause).
    AST_SYNCHRONIZED,       // synchronized 语句 (SynchronizedStatement).
    AST_ASSERT,             // assert 语句 (AssertStatement).
    AST_RETURN,             // return 语句 (ReturnStatement).
    AST_BREAK,              // break 语句 (BreakStatement).
    AST_CONTINUE,           // continue 语句 (ContinueStatement).
    AST_THROW,              // throw 语句 (ThrowStatement).
    AST_YIELD,              // yield 语句 (YieldStatement).
    AST_LABELED_STATEMENT,  // 标签语句 (LabeledStatement).
    AST_EXPRESSION,         // 表达式 (Expression).
    AST_ASSIGN,             // 赋值表达式 (Assignment).
    AST_DEFAULT_VALUE,      // 默认值 (DefaultValue).
    AST_BINARY_EXPR,        // 二元表达式 (BinaryExpression).
    AST_INSTANCEOF,         // instanceof 表达式 (InstanceofExpression).
    AST_UNARY_EXPR,         // 一元表达式 (UnaryExpression).
    AST_CAST,               // 类型转换表达式 (CastExpression).
    AST_CONDITIONAL_EXPR,   // 条件（三元）表达式 (ConditionalExpression).
    AST_LITERAL,            // 字面量 (Literal).
    AST_INT_LITERAL,        // 整数字面量 (IntLiteral).
    AST_FLOAT_LITERAL,      // 浮点字面量 (FloatLiteral).
    AST_STRING_LITERAL,     // 字符串字面量 (StringLiteral).
    AST_TEXT_BLOCK,         // 文本块字面量 (TextBlock).
    AST_CHAR_LITERAL,       // 字符字面量 (CharLiteral).
    AST_BOOL_LITERAL,       // 布尔字面量 (BoolLiteral).
    AST_NULL_LITERAL,       // null 字面量 (NullLiteral).
    AST_CLASS_LITERAL,      // class 字面量 (ClassLiteral).
    AST_THIS_EXPR,          // this 表达式 (ThisExpression).
    AST_ENUM_CONST,         // 枚举常量 (EnumConstant).
    AST_IDENTIFIER,         // 标识符 (Identifier).
    AST_RESOURCE_SPEC,      // 资源声明规范 (ResourceSpec).
    AST_RESOURCE_LIST,      // 资源列表 (ResourceList).
    AST_RESOURCE_DECL,      // 资源声明 (ResourceDeclaration).
    AST_RESOURCE,           // 资源项 (Resource).
    AST_MEMBER_ACCESS,      // 成员访问 (MemberAccess).
    AST_METHOD_REFERENCE,   // 方法引用 (MethodReference).
    AST_ARRAY_ACCESS,       // 数组访问 (ArrayAccess).
    AST_ARRAY_INIT,         // 数组初始化器 (ArrayInitializer).
    AST_METHOD_INVOCATION,  // 方法调用 (MethodInvocation).
    AST_EXPLICIT_CTOR_INVOCATION, // 显式构造器调用 (ExplicitConstructorInvocation).
    AST_ARGUMENT_LIST,      // 实参列表 (ArgumentList).
    AST_EXPRESSION_LIST,    // 表达式列表 (ExpressionList).
    AST_DIM_EXPR_LIST,      // 维度表达式列表 (DimExprList).
    AST_EXCEPTION_TYPE_LIST,// 异常类型列表 (ExceptionTypeList).
    AST_NEW_CLASS,          // 对象创建 (ObjectCreation).
    AST_ARRAY_CREATION,     // 数组创建 (ArrayCreation).
    AST_LAMBDA,             // lambda 表达式 (LambdaExpression).
    AST_VARIABLE_DECL,      // 变量声明 (VariableDeclaration).
    AST_EMPTY,              // 空节点 (Empty).
    AST_IMPORT_LIST,        // import 列表 (ImportList).
    AST_TYPE_DECL_LIST      // 类型声明列表。
} AstKind;

// AST 节点，包含可选文本与子节点列表。
typedef struct AstNode {
    AstKind kind;               // 节点类型。
    char *text;                 // 文本载荷（可选）。
    struct AstNode *scope;      // 点号左侧的表达式（receiver），可选。
    struct AstNode **children;  // 子节点数组。
    size_t child_count;         // 子节点数量。
    size_t child_capacity;      // 子节点容量。
} AstNode;

#ifdef __cplusplus
extern "C" {
#endif

// 创建节点（可选文本，行列参数仅用于接口占位）。
AstNode *ast_node_create(AstKind kind, const char *text, int line, int column);
// 创建叶子节点。
AstNode *ast_leaf(AstKind kind, const char *text, int line, int column);
// 创建分支节点并追加子节点。
AstNode *ast_branch(AstKind kind, int line, int column, size_t child_count, ...);

// 设置节点文本（深拷贝）。
void ast_set_text(AstNode *node, const char *text);
// 追加子节点。
void ast_add_child(AstNode *parent, AstNode *child);
// 追加多个子节点。
void ast_add_children(AstNode *parent, AstNode **children, size_t count);
// 预留子节点容量。
void ast_reserve_children(AstNode *parent, size_t capacity);

// 获取节点类型名称。
const char *ast_kind_name(AstKind kind);
// 打印 AST。
void ast_print(const AstNode *node, FILE *out, int indent);
// 释放 AST。
void ast_free(AstNode *node);

#ifdef __cplusplus
}
#endif

#endif /* JAVA_AST_H */
