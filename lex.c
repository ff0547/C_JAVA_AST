// #include "new.h" // 引入头文�?
#include "shared.h" // 引入头文�?
// #include "new.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"  // Bison生成的头文件
#include <stdbool.h>     // 支持bool类型
#include "java_ast.h"

extern int Index[MAX];
extern char* Token[MAX];
extern int count;
extern int TokenLine[MAX];
extern YYLTYPE yylloc;

FILE *yyin;

// 新增全局变量（存储转换后�?Token 序列�?
int *token_list = NULL;
int token_index = 0;
int max_len = 0;

// 全局文件名变�?
static const char *input_filename = NULL;

// 设置输入文件�?
void set_input_file(const char *filename) {
    input_filename = filename;
}

static AstNode *make_token_node(int token, const char *lexeme, int line) {
    if (!lexeme) {
        return NULL;
    }

    AstKind kind;
    switch (token) {
        case TYPE_IDENTIFIER:
        case TYPE_IDENTIFIER_EnumDeclaration:
        case TYPE_IDENTIFIER_MethodDeclarator:
        case TYPE_IDENTIFIER_AnnotationIdentifiers:
        case TYPE_IDENTIFIER_IdentifierforLambdaParameterList:
        case VAR_IdentifierComplement_IdentifierforLambdaParameterList:
        case YIELD_IdentifierComplement_IdentifierforLambdaParameterList:
        case VAR_MethodDeclarator:
        case YIELD_MethodDeclarator:
        case VAR_AnnotationIdentifiers:
        case YIELD_AnnotationIdentifiers:
        case VAR_IdentifierComplement:
        case YIELD_IdentifierComplement:
        case SEALED_IdentifierComplement:
        case NON_SEALED_IdentifierComplement:
        case UnqualifiedMethodIdentifier:
            kind = AST_IDENTIFIER;
            break;
        case INTEGERLITERAL:
            kind = AST_INT_LITERAL;
            break;
        case FLOATINGPOINTLITERAL:
            kind = AST_FLOAT_LITERAL;
            break;
        case CHARACTERLITERAL:
            kind = AST_CHAR_LITERAL;
            break;
        case STRINGLITERAL:
            kind = AST_STRING_LITERAL;
            break;
        case TextBlock:
            kind = AST_TEXT_BLOCK;
            break;
        case TRUE:
        case FALSE:
            kind = AST_BOOL_LITERAL;
            break;
        case MY_NULL:
            kind = AST_NULL_LITERAL;
            break;
        case NUMBER:
            kind = AST_LITERAL;
            break;
        default:
            return NULL;
    }

    return ast_leaf(kind, lexeme, line, 0);
}

// lex.c 开头添�?
// void init_input(struct input_t *in, FILE *f) {
//     // 通过指针初始化结构体
//     in->file = f;
//     in->cur = in->tok = in->lim = in->buf;
//     in->eof = false;
//     in->line = 1;
// }
void freeTokens();
void printTokens();

typedef void* yyscanner;

// 定义集合A中的token类型
#define IS_IN_SET_TI_VAR_YIELD_IdentifierComplement_IdentifierforLambdaParameterList(token) ( \
    (token) == 44 || /* , */ \
    (token) == TYPE_IDENTIFIER || /* 269 */ \
    (token) == VAR_IdentifierComplement || /* 280 */ \
    (token) == YIELD_IdentifierComplement || /* 281 */ \
    (token) == TYPE_IDENTIFIER_IdentifierforLambdaParameterList || /* 260 */ \
    (token) == VAR_IdentifierComplement_IdentifierforLambdaParameterList || /* 261 */ \
    (token) == YIELD_IdentifierComplement_IdentifierforLambdaParameterList || /* 262 */ \
    (token) == SEALED_IdentifierComplement || /* 383 */ \
    (token) == NON_SEALED_IdentifierComplement /* 384 */ \
)

// 定义不能跟在CastExpression中的括号后面的操作符合集
#define IS_NOT_FOLLOW_IN_CASTEXPRESSION(token) ( \
    (token) == 37 || /* % */ \
    (token) == 38 || /* & */ \
    (token) == 41 || /* ) 左括号不�?*/ \
    (token) == 42 || /* * */ \
    (token) == 43 || /* + */ \
    (token) == 44 || /* , */ \
    (token) == 47 || /* / */ \
    (token) == 58 || /* : */ \
    (token) == 59 || /* ; */ \
    (token) == 60 || /* < */ \
    (token) == 61 || /* = */ \
    (token) == 62 || /* > */ \
    (token) == 63 || /* ? */ \
    (token) == 94 || /* ^ */ \
    (token) == 123 || /* { */ \
    (token) == 124 || /* | */ \
    (token) == ARROW || /* -> */ \
    (token) == ADD_ASSIGN || /* += */ \
    (token) == SUB_ASSIGN || /* -= */ \
    (token) == MUL_ASSIGN || /* *= */ \
    (token) == DIV_ASSIGN || /* /= */ \
    (token) == AND_ASSIGN || /* &= */ \
    (token) == OR_ASSIGN || /* |= */ \
    (token) == XOR_ASSIGN || /* ^= */ \
    (token) == MOD_ASSIGN || /* %= */ \
    (token) == SHL_ASSIGN || /* <<= */ \
    (token) == SHR_ASSIGN || /* >>= */ \
    (token) == USHR_ASSIGN || /* >>>= */ \
    (token) == USHR_OP || /* >>> */ \
    (token) == SHR_OP || /* >> */ \
    (token) == SHL_OP || /* << */ \
    (token) == GE || /* >= */ \
    (token) == LE || /* <= */ \
    (token) == NE || /* != */ \
    (token) == EQ || /* == */ \
    (token) == AND || /* && */ \
    (token) == OR  /* || */ \
)

#define IS_IN_PARENTHESES_OF_CASTEXPRESSION(token) ( \
    (token) == 38 || /* & */ \
    (token) == 40 || /* ( */ \
    (token) == 41 || /* ) */ \
    (token) == 46 || /* . */ \
    (token) == 60 || /* < */ \
    (token) == 62 || /* > */ \
    (token) == 64 || /* @ */ \
    (token) == 91 || /* [ */ \
    (token) == 93 || /* ] */ \
    (token) == TYPE_IDENTIFIER_IdentifierforLambdaParameterList || \
    (token) == TYPE_IDENTIFIER_AnnotationIdentifiers || \
    (token) == TYPE_IDENTIFIER_MethodDeclarator || \
    (token) == TYPE_IDENTIFIER || \
    (token) == DOT_CommonName || \
    (token) == DOT || \
    (token) == VAR_IdentifierComplement || \
    (token) == LANGLE || \
    (token) == BYTE || \
    (token) == SHORT || \
    (token) == INT || \
    (token) == LONG || \
    (token) == CHAR || \
    (token) == FLOAT || \
    (token) == DOUBLE || \
    (token) == AMPERSAND || \
    (token) == BOOLEAN \
)

#define IS_NOT_FRONT_OF_CASTEXPRESSION(token) ( \
    (token) == 46 || /* . */ \
    (token) == ARROW || /* -> */ \
    (token) == 91 || /* [ */ \
    (token) == NEW || \
    (token) == 60 || /* < */ \
    (token) == 63 || /* ? */ \
    (token) == CASE || \
    (token) == 64 || /* @ */ \
    (token) == DEFAULT || \
    (token) == DEFAULT_SwitchLabel || \
    (token) == LANGLE || \
    (token) == LBRACK || \
    (token) == DOT || \
    (token) == BYTE || \
    (token) == SHORT || \
    (token) == INT || \
    (token) == LONG || \
    (token) == CHAR || \
    (token) == FLOAT || \
    (token) == DOUBLE || \
    (token) == BOOLEAN || \
    (token) == IF || \
    (token) == WHILE || \
    (token) == FOR || \
    (token) == TRY || \
    (token) == SWITCH || \
    (token) == ELLIPSIS || \
    (token) == DOUBLE_COLON \
)

#define IN_ALL_MODIFIERS(token) ( \
    (token) == PUBLIC || \
    (token) == PROTECTED || \
    (token) == PRIVATE || \
    (token) == ABSTRACT || \
    (token) == STATIC || \
    (token) == FINAL || \
    (token) == STRICTFP || \
    (token) == TRANSIENT || \
    (token) == VOLATILE || \
    (token) == SYNCHRONIZED || \
    (token) == NATIVE || \
    (token) == DEFAULT || \
    (token) == SEALED || \
    (token) == NON_SEALED \
)

#define START_OF_UNANNTYPE(token) ( \
    (token) == BYTE || \
    (token) == SHORT || \
    (token) == INT || \
    (token) == LONG || \
    (token) == CHAR || \
    (token) == FLOAT || \
    (token) == DOUBLE || \
    (token) == BOOLEAN || \
    (token) == TYPE_IDENTIFIER \
)

// 新增 TokenStream 结构�?
typedef struct {
    int *index;      // Token 编号数组
    char **tokens;   // Token 内容数组
    int count;       // Token 总数
    int current_pos; // 当前位置
} TokenStream;

// 初始化函�?
TokenStream* create_stream(int *index, char **tokens, int count) {
    TokenStream *ts = malloc(sizeof(TokenStream));
    ts->index = index;
    ts->tokens = tokens;
    ts->count = count;
    ts->current_pos = 0;
    return ts;
}

// 销毁函�?
void destroy_stream(TokenStream *ts) {
    free(ts);
}

// 检查从当前位置开始，后面是否�?")" �?"->"
bool find_arrow_after_rparen(TokenStream* ts, int pos) {
    int depth = 0;
    for (int i = pos + 1; i < ts->count; i++) {
        if (ts->index[i] == 40) depth++; // 遇到 '(' 增加嵌套深度
        else if (ts->index[i] == 41) {   // 遇到 ')'
            if (depth == 0) {
                return (i + 1 < ts->count && ts->index[i + 1] == ARROW);
            }
            depth--;
        }
    }
    return false;
}

// 自动�?：检查TYPE_IDENTIFIER_IdentifierforLambdaParameterList、VAR_IdentifierComplement_IdentifierforLambdaParameterList、YIELD_IdentifierComplement_IdentifierforLambdaParameterList�?
// 即检查从指定位置开始的token序列是否符合Lambda参数条件
bool is_lambda_parameter(TokenStream *ts, int start_pos) {
    int pos = start_pos;
    int token;
    
    // 向后跳过所有属于集合A的token
    while (pos < ts->count) {
        token = ts->index[pos];
        if (!IS_IN_SET_TI_VAR_YIELD_IdentifierComplement_IdentifierforLambdaParameterList(token)) {
            break;
        }
        pos++;
    }
    
    // 检查是否到达token流末�?
    if (pos >= ts->count) {
        return false;
    }
    
    // 检查第一个不属于A的token
    token = ts->index[pos];
    
    // 情况1: 遇到')'且后面是ARROW
    if (token == 41) { // ')'
        if (pos + 1 < ts->count && ts->index[pos + 1] == ARROW) {
            return true;
        }
    }
    
    return false;
}

// 自动�?：跳过泛型参数列�?
int skip_generic_arguments(TokenStream *ts, int start_pos) {
    int pos = start_pos;
    int depth = 1;// 初始深度�?（已经有一�?<'�?
    
    int i=1;
    while (pos < ts->count && depth > 0) {
        int token = ts->index[pos];
        
        if (token == LANGLE || token == '<') {  // 遇到嵌套�?<'
            depth++;
        } 
        else if (token == '>') {  // 遇到'>'
            depth--;
        }
        // 检查是否遇到可能的结束标记
        else if (token == ';' ) {
            // 这些token通常不会出现在泛型参数列表中，提前结�?
            // printf("调试：因�?提前结束\n");
            break;
        }
        else if ( token != '('     && token != ')'     && token != '{'     && token != '}'     &&
                  token != ','     && token != ']'     && token != '['     && token != '='     &&
                  token != '?'     && token != EXTENDS && token != SUPER   && token != '@'     &&
                  token != TYPE_IDENTIFIER && token != '.' && token != DOT && token != AT_Dims &&
                  token != BOOLEAN && token != BYTE    && token != SHORT   && token != INT     &&
                  token != LONG    && token != CHAR    && token != FLOAT   && token != DOUBLE  &&
                  token != VAR_IdentifierComplement    && token != YIELD_IdentifierComplement   ) {
            // 这些token通常不会出现在泛型参数列表中，提前结�?
            // printf("调试：提前结束\n");
            break;
        }
        pos++;
    }
    
    // 返回结束位置（匹配的'>'之后的位置）
    return pos;
}

// 自动�?：跳过数组维度声明（Dims自动机）
int skip_dims(TokenStream *ts, int start_pos) {
    int pos = start_pos;
    int state = 0; // 0:初始状�? 1:情况1, 2:情况2, 3:情况3, 4:情况4, 5:情况5
    int depth = 0; // 括号嵌套深度
    
    while (pos < ts->count) {
        int token = ts->index[pos];
        
        switch (state) {
            case 0: // 初始状�?
                if (token == '@' || token == AT_Dims) {
                    state = 1; // 进入情况1
                    pos++;
                } else if (token == '[') {
                    state = 2; // 进入情况2
                    pos++;
                } else {
                    return start_pos; // 不符合入口条件，返回起始位置
                }
                break;
                
            case 1: // 情况1：已遇到@或AT_Dims
                if (token == TYPE_IDENTIFIER_MethodDeclarator ||
                    token == VAR_IdentifierComplement ||
                    token == TYPE_IDENTIFIER ||
                    token == YIELD_IdentifierComplement) {
                    state = 4; // 进入情况4
                    pos++;
                } else {
                    return start_pos; // 不符合条件，返回起始位置
                }
                break;
                
            case 2: // 情况2：已遇到'['
                if (token == ']') {
                    pos++; // 跳过']'
                    
                    // 检查下一个token
                    if (pos >= ts->count) return pos; // 到达末尾
                    
                    int next_token = ts->index[pos];
                    if (next_token == '@' || next_token == AT_Dims) {
                        state = 1; // 进入情况1
                    } else if (next_token == '[') {
                        state = 2; // 返回情况2
                    } else {
                        return pos; // 退出自动机，返回当前位�?
                    }
                } else {
                    return start_pos; // 不符合条件，返回起始位置
                }
                break;
                
            case 3: // 情况3：已遇到DOT_CommonName�?.'
                if (token == TYPE_IDENTIFIER_MethodDeclarator ||
                    token == VAR_IdentifierComplement ||
                    token == TYPE_IDENTIFIER ||
                    token == YIELD_IdentifierComplement) {
                    state = 4; // 进入情况4
                    pos++;
                } else {
                    return start_pos; // 不符合条件，返回起始位置
                }
                break;
                
            case 4: // 情况4：已遇到标识�?
                if (token == DOT_CommonName || token == '.') {
                    state = 3; // 进入情况3
                    pos++;
                } else if (token == '[') {
                    state = 2; // 进入情况2
                    pos++;
                } else if (token == '@' || token == AT_Dims) {
                    state = 1; // 进入情况1
                    pos++;
                } else if (token == '(') {
                    state = 5; // 进入情况5
                    depth = 1; // 初始化括号深�?
                    pos++;
                } else {
                    return start_pos; // 不符合条件，返回起始位置
                }
                break;
                
            case 5: // 情况5：已遇到'('
                if (token == '(') {
                    depth++; // 增加嵌套深度
                } else if (token == ')') {
                    depth--; // 减少嵌套深度
                    if (depth == 0) {
                        // 找到匹配�?)'
                        pos++; // 跳过')'
                        
                        // 检查下一个token
                        if (pos >= ts->count) return start_pos; // 到达末尾
                        
                        int next_token = ts->index[pos];
                        if (next_token == '@' || next_token == AT_Dims) {
                            state = 1; // 进入情况1
                            pos++;
                        } else if (next_token == '[') {
                            state = 2; // 进入情况2
                            pos++;
                        } else {
                            return start_pos; // 不符合条件，返回起始位置
                        }
                    }
                }
                pos++;
                break;
        }
    }
    
    return start_pos; // 如果到达token流末尾，返回起始位置
}

// 自动�?：向前跳过嵌套泛型参数列�?
int skip_nested_generic(TokenStream *ts, int start_pos) {
    int pos = start_pos;
    int depth = 1;  // 初始深度�?（已经有一�?>'�?
    int max_steps = 65536;  // 最大步数限�?
    int steps = 0;
    
    while (pos >= 0 && depth > 0 && steps < max_steps) {
        int token = ts->index[pos];
        
        if (token == '>') {
            depth++;
        } else if (token == '<') {
            depth--;
        }
        
        // 检查token是否在允许的集合�?
        bool valid_token = 
            token == ',' || token == '?' || token == EXTENDS || token == SUPER ||
            token == '@' || token == '(' || token == TYPE_IDENTIFIER || 
            token == '=' || token == '{' || token == '}' || token == ')' ||
            token == LANGLE || token == '>' || token == '.' || token == DOT ||
            token == '[' || token == ']' || token == AT_Dims || token == BOOLEAN ||
            token == BYTE || token == SHORT || token == INT || token == LONG ||
            token == CHAR || token == FLOAT || token == DOUBLE || token == DOT_CommonName;
        
        if (!valid_token) {
            return -1;  // 遇到不允许的token，退�?
        }
        
        pos--;
        steps++;
    }
    
    // 检查是否成功匹�?
    if (depth == 0) {
        return pos;  // 返回匹配�?<'位置
    }
    return -1;  // 匹配失败
}

// 辅助函数：检查token是否在泛型参数列表中允许出现
static bool is_valid_in_generic(int token) {
    switch (token) {
        case 44:  // ','
        case 63:  // '?'
        case EXTENDS: // 290
        case SUPER:   // 291
        case 64:   // '@'
        case 40:   // '('
        case 61:   // '='
        case 123:  // '{'
        case 125:  // '}'
        case 41:   // ')'
        case LANGLE: // 271
        case 46:   // '.' 
        case DOT:   // 278
        case 91:   // '['
        case 93:   // ']'
        case AT_Dims: // 272
        case BOOLEAN: // 313
        case BYTE:    // 283
        case SHORT:   // 285
        case INT:     // 284
        case LONG:    // 286
        case CHAR:    // 287
        case FLOAT:   // 288
        case DOUBLE:  // 289
        case 62:   // '>' 
        case 60:   // '<'
        case 38:   // '&'
        // 类型标识符集�?
        case TYPE_IDENTIFIER: // 269
        case TYPE_IDENTIFIER_EnumDeclaration: // 270
        case TYPE_IDENTIFIER_MethodDeclarator: // 263
        case TYPE_IDENTIFIER_AnnotationIdentifiers: // 266
        case VAR_AnnotationIdentifiers: // 267
        case YIELD_AnnotationIdentifiers: // 268
        case VAR_MethodDeclarator: // 264
        case YIELD_MethodDeclarator: // 265
        case VAR_IdentifierComplement: // 280
        case YIELD_IdentifierComplement: // 281
            return true;
        default:
            return false;
    }
}

// 自动�?：检查从start_pos开始的泛型参数列表是否合法
static bool check_generic_arguments(TokenStream *ts, int start_pos) {
    int depth = 1;
    int pos = start_pos + 1; // 从下一个token开�?
    while (pos < ts->count && depth > 0) {
        int token = ts->index[pos];
        if (token == 60 || token == LANGLE) { // 60�?<'�?71是LANGLE
            depth++;
        } else if (token == 62) { // '>'
            depth--;
            if (depth == 0) {
                break; // 匹配到最外层�?>'，结�?
            }
        }

        // 如果当前depth>0（表示还在参数列表内部），则检查token是否合法
        if (depth > 0 && !is_valid_in_generic(token)) {
            return false;
        }
        pos++;
    }
    return depth == 0; // 如果depth�?，说明匹配成�?
}

// 辅助函数：跳过注�?
static int skip_annotation(TokenStream *ts, int pos) {
    if (pos + 2 >= ts->count) return pos;   // @ Identifier 后面得有东西跟着
    if (ts->index[pos] != 64 && ts->index[pos] != AT_AnnotationTypeDeclaration) 
        return pos;

    // 跳过 '@'
    pos++;
    
    // 检查类型标识符
    int token = ts->index[pos];
    if (token != TYPE_IDENTIFIER && token != TYPE_IDENTIFIER_MethodDeclarator)
        return pos;
    
    // 跳过类型标识�?
    pos++;

    token = ts->index[pos];
    int state = 0;
    
    while (pos < ts->count) {
        token = ts->index[pos];
        
        switch (state) {
            case 0: // 初始状�?
                if (token == 40) { // '('
                    state = 2;
                } 
                else if (token == 46 || token == DOT_CommonName) { // '.' 
                    state = 3;
                } 
                else {
                    return pos; // 其他情况直接返回 @ Identifier 后面1个的token位置
                }
                break;
                
            case 2: // 状�?：遇�?('
            {
                int depth = 1;
                pos++; // 跳过 '('
                // 处理括号内容
                while (pos < ts->count && depth > 0) {
                    if (ts->index[pos] == 40) depth++;
                    else if (ts->index[pos] == 41) depth--;
                    pos++;
                }
                return pos; // 括号处理完成后返�?
            }
                
            case 3: // 状�?：遇�?'.' �?DOT_CommonName
                pos++; // 跳过点号
                if (pos >= ts->count) return pos;
                
                token = ts->index[pos];
                if (token == TYPE_IDENTIFIER || token == TYPE_IDENTIFIER_MethodDeclarator) {
                    pos++; // 跳过标识�?
                    if (pos >= ts->count) return pos;
                    
                    token = ts->index[pos];
                    if (token == 40) { // '('
                        state = 2; // 进入状�?处理括号
                    } 
                    else if (token == 46 || token == DOT_CommonName) { // '.' 
                        state = 3; // 保持状�?处理点号
                    } 
                    else {
                        return pos; // 其他情况返回
                    }
                } 
                else {
                    return pos; // 不是标识符则返回
                }
                break;
        }
        
        // 移动到下一个token
        if (state != 0) pos++;
    }
    
    return pos;
}

// 辅助函数：跳过未修饰的类/接口类型 UnannClassOrInterfaceType
// 跳过未修饰的�?接口类型 - 完整实现
static int skip_UnannClassOrInterfaceType(TokenStream *ts, int start_pos) {
    int pos = start_pos;
    int state = 0; // 0: 初始状�? 1: 处理泛型, 2: 处理点号, 3: 处理注解
    int depth = 0; // 用于处理嵌套泛型
    
    while (pos < ts->count) {
        int token = ts->index[pos];
        
        switch (state) {
            case 0: // 初始状态：期望类型标识�?
                if (token == TYPE_IDENTIFIER || token == TYPE_IDENTIFIER_MethodDeclarator) {
                    pos++; // 跳过类型标识�?
                    if (pos >= ts->count) return pos;
                    
                    token = ts->index[pos];
                    if (token == LANGLE || token == 60) { // '<'
                        state = 1; // 进入状�?处理泛型
                        depth = 1; // 初始化深�?
                        pos++; // 跳过 '<'
                    } 
                    else if (token == 46 || token == DOT_CommonName || token == DOT) { // '.' 
                        state = 2; // 进入状�?处理点号
                        pos++; // 跳过点号
                    } 
                    else {
                        return pos; // 其他情况直接返回当前位置
                    }
                } 
                else {
                    return pos; // 不是类型标识符则返回
                }
                break;
                
            case 1: // 状�?：处理泛型参�?
                if (token == LANGLE || token == 60) { // '<'
                    depth++; // 嵌套深度增加
                } 
                else if (token == 62) { // '>'
                    depth--; // 嵌套深度减少
                    if (depth == 0) {
                        // 找到匹配�?'>'
                        pos++; // 跳过 '>'
                        return pos; // 返回下一个位�?
                    }
                }
                else if (token == 59){ // ';'
                    return pos;
                }
                pos++; // 移动到下一个token
                break;
                
            case 2: // 状�?：处理点号后
                if (token == TYPE_IDENTIFIER || token == TYPE_IDENTIFIER_MethodDeclarator) {
                    state = 0; // 返回状�?处理类型标识�?
                    // 注意：这里不移动位置，下个循环会处理这个标识�?
                } 
                else if (token == 64 || token == AT_AnnotationTypeDeclaration) { // '@'
                    state = 3; // 进入状�?处理注解
                    pos = skip_annotation(ts, pos); // 跳过连续注解
                } 
                else {
                    return pos; // 其他情况返回
                }
                break;
                
            case 3: // 状�?：处理注解后
                if (token == 64 || token == AT_AnnotationTypeDeclaration) { // '@'
                    pos = skip_annotation(ts, pos); // 跳过连续注解
                } 
                else if (token == TYPE_IDENTIFIER) {
                    // 检查下一个token
                    int next_pos = pos + 1;
                    if (next_pos < ts->count) {
                        int next_token = ts->index[next_pos];
                        if (next_token == LANGLE || next_token == 60) { // '<'
                            state = 1; // 进入状�?处理泛型
                            depth = 1; // 初始化深�?
                            pos = next_pos + 1; // 跳过标识符和'<'
                        }
                        else if (token == 46 || token == DOT_CommonName || token == DOT) { // '.' 
                            state = 2; // 进入状�?处理点号
                            pos++; // 跳过点号
                        }
                        else {
                            return next_pos; // 其他情况返回下一个位�?
                        }
                    } 
                    else {
                        return pos; // 没有下一个token
                    }
                } 
                else {
                    return pos; // 其他情况返回
                }
                break;
        }
    }
    
    return pos; // 返回最终位�?
}

// 反向跳过注解或括�?
static int reverse_skip_annotation(TokenStream *ts, int start_pos) {
    int pos = start_pos;
    if (pos < 0) return pos; // 越界检�?
    
    int state = 0;
    int depth = 0;
    
    while (pos >= 0) {
        int token = ts->index[pos];
        
        switch (state) {
            case 0: // 初始状�?
                if (token == 41) { // ')'
                    state = 1; // 进入状�?
                    depth = 1;
                    pos--;
                } else if (token == TYPE_IDENTIFIER || token == TYPE_IDENTIFIER_MethodDeclarator) {
                    state = 2; // 进入状�?
                    pos--;
                } else {
                    return pos; // 直接返回当前位置
                }
                break;
                
            case 1: // 状�?：处�?)'
                if (token == 41) { // ')'
                    depth++;
                } else if (token == 40) { // '('
                    depth--;
                    if (depth == 0) {
                        // 找到匹配�?('，向前检查一个token
                        pos--;
                        if (pos >= 0) {
                            token = ts->index[pos];
                            if (token == TYPE_IDENTIFIER || token == TYPE_IDENTIFIER_MethodDeclarator) {
                                state = 2; // 进入状�?
                                pos--;
                            } else {
                                return pos; // 返回当前位置
                            }
                        } else {
                            return pos; // 越界返回
                        }
                    }
                }
                pos--;
                break;
                
            case 2: // 状�?：处理TYPE_IDENTIFIER或TYPE_IDENTIFIER_MethodDeclarator
                if (token == DOT_CommonName || token == 46) { // '.' or DOT_CommonName
                    state = 3; // 进入状�?
                    pos--;
                } else if (token == 64 || token == AT_AnnotationTypeDeclaration) { // '@'
                    state = 4; // 进入状�?
                    pos--;
                } else {
                    return pos; // 返回当前位置
                }
                break;
                
            case 3: // 状�?：处�?.'或DOT_CommonName
                if (token == TYPE_IDENTIFIER || token == TYPE_IDENTIFIER_MethodDeclarator) {
                    state = 2; // 回到状�?
                    pos--;
                } else {
                    return pos; // 返回当前位置
                }
                break;
                
            case 4: // 状�?：处�?@'
                // 向前检查一个token并返�?
                pos--;
                return pos;
        }
    }
    
    return pos; // 返回最终位�?
}


int base_yylex(TokenStream *ts, int *lookahead, int *error) {
    *error=0; // 初始化错误码
    if (ts->current_pos >= ts->count) {
        *error = -1; // EOF
        return -1;
    }
    if (ts->index[ts->current_pos] == -1) {
        *error = -2; // 非法token
        return -1;
    }
    // 获取当前token
    int current = ts->index[ts->current_pos];
    // 获取�?个token
    int prev_token = (ts->current_pos > 0) ? ts->index[ts->current_pos - 1] : -1;
    // 获取�?个token（用于检查CATCH�?
    int prev_prev_token = (ts->current_pos > 1) ? ts->index[ts->current_pos - 2] : -1;
    // 获取�?�?�?个token
    int next_token1 = (ts->current_pos < ts->count - 1) ? ts->index[ts->current_pos + 1] : -1;
    int next_token2 = (ts->current_pos < ts->count - 2) ? ts->index[ts->current_pos + 2] : -1;
    int next_token3 = (ts->current_pos < ts->count - 3) ? ts->index[ts->current_pos + 2] : -1;
    
    if (*error) return -1;

    // 获取下一个token（不移动指针�?
    if (ts->current_pos < ts->count) {
        *lookahead = ts->index[ts->current_pos+1];
    } else {
        *lookahead = -1;
    }
        // --- 合成 diamond 操作符 <> ---
    // 典型场景：new ArrayList<>() / new HashMap<>() / new X<>() { ... }
    // token 约定：'<' == 60, '>' == 62, '(' == 40
    if ((current == 60 || current == LANGLE) && next_token1 == 62 && next_token2 == 40) {
        // lookahead 应该指向 '('
        *lookahead = next_token2;
        ts->current_pos += 2;   // 跳过 '<' 和 '>'
        return DIAMOND;
    }

    // 使用switch处理不同类型的token
    switch(current) {
        case TYPE_IDENTIFIER:  // 值为269
            // 1. 处理枚举声明标识�?
            if (prev_token == ENUM &&  // ENUM值为318
                (next_token1 == 123 ||  // '{' 的ASCII�?
                 next_token1 == IMPLEMENTS)) {  // IMPLEMENTS值为308
                ts->index[ts->current_pos] = TYPE_IDENTIFIER_EnumDeclaration;
            }
            // 2. 处理简单情况的Lambda参数标识�?
            else if (
                // 情况1: 向后检查两个token: ')' ARROW
                (next_token1 == 41 && next_token2 == ARROW) ||  // 41 = ')', ARROW=356
                // 情况2: 向后检查一个token: ARROW
                (next_token1 == ARROW)
            ) {
                if(prev_token != VAR
                    && prev_token != ']' 
                    && prev_token != '>' 
                    && prev_token != TYPE_IDENTIFIER 
                    && prev_token != VAR_IdentifierComplement 
                    && prev_token != YIELD_IdentifierComplement 
                    && prev_token != BOOLEAN 
                    && prev_token != BYTE 
                    && prev_token != SHORT 
                    && prev_token != INT 
                    && prev_token != LONG 
                    && prev_token != CHAR 
                    && prev_token != FLOAT 
                    && prev_token != DOUBLE
                    && prev_token != CASE){
                        if(prev_token == 44){  // ','
                            int co = ts->current_pos - 2;
                            int to = ts->index[co]; // 前第2个token 
                            while(to == 44 
                                || to == 260
                                || to == 261
                                || to == 262
                                || to == 269
                                || to == 280
                                || to == 281){
                                    co--;
                                    to = ts->index[co];
                            }
                            if(to == 40){ // '('
                                ts->index[ts->current_pos] = TYPE_IDENTIFIER_IdentifierforLambdaParameterList;
                            }
                        }
                        else{
                            ts->index[ts->current_pos] = TYPE_IDENTIFIER_IdentifierforLambdaParameterList;
                        }
                }
            }
            // 3. 处理方法声明标识�?
            // else if (next_token1 == 40) {  // 40 = '('
            //     ts->index[ts->current_pos] = TYPE_IDENTIFIER_MethodDeclarator;
            // }
            // 4. 处理复杂情况的Lambda参数标识�?
            else if (prev_token == 40 || prev_token == 44) {  // '(' or ','
                if (next_token1 == 44 && is_lambda_parameter(ts, ts->current_pos + 2)) { //下一个token�?',' 则进入自动机
                    ts->index[ts->current_pos] = TYPE_IDENTIFIER_IdentifierforLambdaParameterList;
                }
            }
            // 4. 处理注解标识�?TYPE_IDENTIFIER_AnnotationIdentifiers (规则4.1)
            else {
                int current_pos = ts->current_pos;
                int prev_token_val = (current_pos > 0) ? ts->index[current_pos - 1] : -1;
                int check_pos = current_pos - 1;
                bool is_annotation_1 = false;
                bool is_annotation_2 = false;

                // 检查前一个token是否�?.'，或者前一个token�?)'或TYPE_IDENTIFIER
                if (prev_token_val == 46 || prev_token_val == DOT_CommonName|| prev_token_val == 41 || prev_token_val == TYPE_IDENTIFIER) {
                    // 反复执行reverse_skip_annotation直到得到�?)'或TYPE_IDENTIFIER的token
                    while (check_pos >= 0) {
                        int token_type = ts->index[check_pos];
                        if (token_type == 41 || token_type == TYPE_IDENTIFIER) {
                            check_pos = reverse_skip_annotation(ts, check_pos);
                        } else {
                            break;
                        }
                    }
                    if (check_pos > 0 && (ts->index[check_pos] == 46 || ts->index[check_pos] == DOT_CommonName)) {
                        int token_after_skip = ts->index[check_pos-1];
                        if (token_after_skip == TYPE_IDENTIFIER_AnnotationIdentifiers) {
                            is_annotation_1 = true;
                            is_annotation_2 = true;
                        }
                    }
                    //printf("check1\n");
                }

                if (!is_annotation_1) {
                    // 从当前TYPE_IDENTIFIER开始向前检�?
                    check_pos = current_pos - 1;
                    while (check_pos >= 0) {
                        int token_type = ts->index[check_pos];
                        if (token_type == 41 || token_type == TYPE_IDENTIFIER) {
                            check_pos = reverse_skip_annotation(ts, check_pos);
                        } else {
                            break;
                        }
                    }
                    if (check_pos >= 0) {
                        int token_after_skip = ts->index[check_pos];
                        if (token_after_skip == NEW) {
                            is_annotation_1 = true; // 标记为注解标识符
                            //printf("check2\n");
                        } else if (token_after_skip == 62) { // '>'
                            // 找到匹配�?<'
                            int angle_depth = 1;
                            int angle_pos = check_pos - 1;
                            while (angle_pos >= 0 && angle_depth > 0) {
                                if (ts->index[angle_pos] == 62) angle_depth++;
                                else if (ts->index[angle_pos] == 60 || ts->index[angle_pos] == LANGLE) angle_depth--;
                                angle_pos--;
                            }
                            if (angle_depth == 0) {
                                // 找到匹配�?<'，检查前一个token
                                if (angle_pos >= 0) {
                                    int token_before_angle = ts->index[angle_pos];
                                    if (token_before_angle == NEW) {
                                        is_annotation_1 = true;
                                    }
                                }
                            }
                        }
                    }
                }

                if (is_annotation_1 && !is_annotation_2) {
                    // 进入自动机状�?
                    int state = 0;
                    int pos = current_pos;
                    while (state >= 0 && pos < ts->count) {
                        int token = ts->index[pos];
                        switch (state) {
                            case 0: // 状�?
                                //printf("check 状�?\n");
                                if (token == TYPE_IDENTIFIER) {
                                    // 向后检查一个token
                                    if (pos + 1 < ts->count) {
                                        int next_token = ts->index[pos + 1];
                                        if (next_token == 46 || next_token == DOT_CommonName) state = 1;
                                        else if (next_token == 40) state = 2;
                                        else if (next_token == DIAMOND) state = 3;
                                        else if (next_token == 60 || next_token == LANGLE) state = 4;
                                        else state = -1;
                                    } else state = -1;
                                    pos++;
                                } else state = -1;
                                break;
                            case 1: // 状�?: 遇到'.'
                                //printf("check 状�?\n");
                                if (token == 46 || token == DOT_CommonName) {
                                    if (pos + 1 < ts->count) {
                                        int next_token = ts->index[pos + 1];
                                        if (next_token == 64) state = 5;
                                        else if (next_token == TYPE_IDENTIFIER) state = 0;
                                        else state = -1;
                                    } else state = -1;
                                    pos++;
                                } else state = -1;
                                break;
                            case 2: // 状�?: 遇到'('
                                //printf("check 状�?\n");
                                if (token == 40) {
                                    is_annotation_2 = true;
                                    state = -1; 
                                } else state = -1;
                                break;
                            case 3: // 状�?: 遇到DIAMOND
                                //printf("check 状�?\n");
                                if (token == DIAMOND) {
                                    if (pos + 1 < ts->count) {
                                        int next_token = ts->index[pos + 1];
                                        if (next_token == 40) state = 2;
                                        else state = -1;
                                    } else state = -1;
                                    pos++;
                                } else state = -1;
                                break;
                            case 4: // 状�?: 遇到'<'或LANGLE
                                //printf("check 状�?\n");
                                if (token == 60 || token == LANGLE) {
                                    int depth = 1;
                                    pos++;
                                    while (pos < ts->count && depth > 0) {
                                        int t = ts->index[pos];
                                        if (t == 60 || t == LANGLE) depth++;
                                        else if (t == 62) depth--;
                                        // 检查是否在允许的token集合�?
                                        if (t != 44 && t != 63 && t != EXTENDS && t != SUPER && t != 64 &&
                                            t != 40 && t != 61 && t != 123 && t != 125 && t != 41 &&
                                            t != LANGLE && t != 62 && t != 46 && t != DOT_CommonName&& t != DOT &&
                                            t != 91 && t != 93 && t != AT_Dims && t != BOOLEAN &&
                                            t != BYTE && t != SHORT && t != INT && t != LONG &&
                                            t != CHAR && t != FLOAT && t != DOUBLE &&
                                            t != TYPE_IDENTIFIER) {
                                            state = -1;
                                            break;
                                        }
                                        pos++;
                                    }
                                    if (depth == 0) {
                                        if (pos < ts->count) {
                                            int next_token = ts->index[pos];
                                            if (next_token == 40) state = 2;
                                            else state = -1;
                                        } else state = -1;
                                    } else state = -1;
                                } else state = -1;
                                break;
                            case 5: // 状�?: 遇到'@'
                                //printf("check 状�?\n");
                                if (token == 64) {
                                    pos = skip_annotation(ts, pos); // 跳过注解
                                    if (pos < ts->count) {
                                        int next_token = ts->index[pos];
                                        if (next_token == TYPE_IDENTIFIER) state = 0;
                                        else state = -1;
                                    } else state = -1;
                                } else state = -1;
                                break;
                        }
                    }
                    if (is_annotation_1 && is_annotation_2) {
                        ts->index[ts->current_pos] = TYPE_IDENTIFIER_AnnotationIdentifiers;
                    } else {
                        // 只有在不是注解标识符时才检查是否为方法声明标识�?
                        if (next_token1 == 40) {  // 40 = '('
                            ts->index[ts->current_pos] = TYPE_IDENTIFIER_MethodDeclarator;
                        }
                    }
                } else if(is_annotation_1 && is_annotation_2){
                    ts->index[ts->current_pos] = TYPE_IDENTIFIER_AnnotationIdentifiers;
                }
            }
            if(ts->index[ts->current_pos] != TYPE_IDENTIFIER_AnnotationIdentifiers){
                if (next_token1 == 40) {  // 40 = '('
                    ts->index[ts->current_pos] = TYPE_IDENTIFIER_MethodDeclarator;
                }
            }
            break;
            
        case VAR_IdentifierComplement:  // 值为280
            // 1. 处理简单情况的Lambda参数标识�?
            if (
                // 情况1: 向后检查两个token: ')' ARROW
                (next_token1 == 41 && next_token2 == ARROW) ||  // 41 = ')', ARROW=356
                // 情况2: 向后检查一个token: ARROW
                (next_token1 == ARROW)
            ) {
                if(prev_token != VAR
                    && prev_token != ']' 
                    && prev_token != '>' 
                    && prev_token != TYPE_IDENTIFIER 
                    && prev_token != VAR_IdentifierComplement 
                    && prev_token != YIELD_IdentifierComplement
                    && prev_token != YIELD
                    && prev_token != BOOLEAN 
                    && prev_token != BYTE 
                    && prev_token != SHORT 
                    && prev_token != INT 
                    && prev_token != LONG 
                    && prev_token != CHAR 
                    && prev_token != FLOAT 
                    && prev_token != DOUBLE){
                        ts->index[ts->current_pos] = VAR_IdentifierComplement_IdentifierforLambdaParameterList;
                    }
            }
            // 2. 处理方法声明标识�?
            else if (next_token1 == 40) {  // 40 = '('
                ts->index[ts->current_pos] = VAR_MethodDeclarator;
            }
            // 3. 处理复杂情况的Lambda参数标识�?
            else if (prev_token == 40 || prev_token == 44) {  // '(' or ','
                if (next_token1 == 44 && is_lambda_parameter(ts, ts->current_pos + 2)) { //下一个token�?',' 则进入自动机
                    ts->index[ts->current_pos] = VAR_IdentifierComplement_IdentifierforLambdaParameterList;
                }
            }
            // 4. 处理注解标识�?(规则4.2)
            else if (next_token1 == DIAMOND && next_token2 == 40) {  // DIAMOND '('
                ts->index[ts->current_pos] = VAR_AnnotationIdentifiers;
            }
            else if (next_token1 == '<') {  // 泛型参数列表
                int end_pos = skip_generic_arguments(ts, ts->current_pos + 2);
                if (end_pos < ts->count && ts->index[end_pos] == 40) {  // 检查下一个token�?('
                    ts->index[ts->current_pos] = VAR_AnnotationIdentifiers;
                }
            }
            // 5. 处理VAR情况（规�?.1�?
            else if (next_token1 == TYPE_IDENTIFIER || 
                next_token1 == VAR_IdentifierComplement || 
                next_token1 == YIELD_IdentifierComplement) {
                
                // 2. 检查向前一个token
                if (prev_token == FINAL || 
                    prev_token == TYPE_IDENTIFIER || 
                    prev_token == '{' || 
                    prev_token == '(' || 
                    prev_token == ')' || 
                    prev_token == ';' || 
                    prev_token == ',' ||
                    prev_token == -1) {
                    
                    // 3. 特殊检查：当向前是'('时，确保不是CATCH
                    if (prev_token == '(' && prev_prev_token == CATCH) { // 这里没写错�?
                        // 不符合条件，保持为VAR_IdentifierComplement
                    } 
                    else {
                        // 4. 检查向后两个token不为") {"
                        if (!(next_token2 == 41 && next_token3 == 123)) {
                            // 5. 进入Dims自动�?
                            int after_dims_pos = skip_dims(ts, ts->current_pos + 1);
                            
                            // 检查跳过Dims后的两个token
                            int after_dims_token1 = (after_dims_pos < ts->count) ? 
                                ts->index[after_dims_pos] : -1;
                            int after_dims_token2 = (after_dims_pos + 1 < ts->count) ? 
                                ts->index[after_dims_pos + 1] : -1;
                            
                            if (!(after_dims_token1 == 41 && after_dims_token2 == 123)) {
                                // 所有条件满足，转换为VAR
                                ts->index[ts->current_pos] = VAR;
                            }
                        }
                    }
                }
            }    
            break;

        case YIELD_IdentifierComplement:  // 值为281
            // 1. 处理简单情况的Lambda参数标识�?
            if (
                // 情况1: 向后检查两个token: ')' ARROW
                (next_token1 == 41 && next_token2 == ARROW) ||  // 41 = ')', ARROW=356
                
                // 情况2: 向后检查一个token: ARROW
                (next_token1 == ARROW)
            ) {
                if(prev_token != VAR //应该是VAR，这里为了测试先替一下，待修�?
                    && prev_token != ']' 
                    && prev_token != '>' 
                    && prev_token != TYPE_IDENTIFIER 
                    && prev_token != VAR_IdentifierComplement 
                    && prev_token != YIELD_IdentifierComplement
                    && prev_token != YIELD 
                    && prev_token != BOOLEAN 
                    && prev_token != BYTE 
                    && prev_token != SHORT 
                    && prev_token != INT 
                    && prev_token != LONG 
                    && prev_token != CHAR 
                    && prev_token != FLOAT 
                    && prev_token != DOUBLE){
                        ts->index[ts->current_pos] = YIELD_IdentifierComplement_IdentifierforLambdaParameterList;
                    }
            }
            // 2. 处理方法声明标识�?
            else if (next_token1 == 40) {  // 40 = '('
                ts->index[ts->current_pos] = YIELD_MethodDeclarator;
            }
            // 3. 处理复杂情况的Lambda参数标识�?
            else if (prev_token == 40 || prev_token == 44) {  // '(' or ','
                if (next_token1 == 44 && is_lambda_parameter(ts, ts->current_pos + 2)) { //下一个token�?',' 则进入自动机
                    ts->index[ts->current_pos] = YIELD_IdentifierComplement_IdentifierforLambdaParameterList;
                }
            }
            // 4. 处理注解标识�?(规则4.3)
            else if (next_token1 == DIAMOND && next_token2 == 40) {  // DIAMOND '('
                ts->index[ts->current_pos] = YIELD_AnnotationIdentifiers;
            }
            else if (next_token1 == '<') {  // 泛型参数列表
                int end_pos = skip_generic_arguments(ts, ts->current_pos + 2);
                if (end_pos < ts->count && ts->index[end_pos] == 40) {  // 检查下一个token�?('
                    ts->index[ts->current_pos] = YIELD_AnnotationIdentifiers;
                }
            }
            break;

        // 规则6：DEFAULT_SwitchLabel处理
        case DEFAULT:  // 值为320
            // 检查下一个token是否为ARROW(356)�?:'(58)
            if (next_token1 == ARROW || next_token1 == 58) {  // 58 = ':'
                ts->index[ts->current_pos] = DEFAULT_SwitchLabel;  // 277
            }
            break;

        // 规则11
        case 46:  // '.'
            //规则11.1 检查DOT 
            if (
                // 向前检查：前一个token是TYPE_IDENTIFIER(269)�?>'(62)
                (prev_token == TYPE_IDENTIFIER || prev_token == 62)
            ) {
                if(next_token1 == 64){  // 向后检查：下一个token�?@'(64)
                    // 条件满足，转换为DOT(278)
                    ts->index[ts->current_pos] = DOT; // 值为278
                }
                if(next_token1 == TYPE_IDENTIFIER){  // 向后检查：下两个token是TYPE_IDENTIFIER LANGLE或者TYPE_IDENTIFIER '<'
                    if(next_token2 == LANGLE || next_token2 == '<'){
                        // 条件满足，转换为DOT(278)
                        ts->index[ts->current_pos] = DOT; // 值为278

                    }
                }
            }
            //规则11.2 检查DOT_CommonName
            if (
                // 向前检查：前一个token是TYPE_IDENTIFIER(269)、VAR_IdentifierComplement(280)�?
                (prev_token == TYPE_IDENTIFIER || 
                 prev_token == VAR_IdentifierComplement || 
                 prev_token == YIELD_IdentifierComplement || 
                 prev_token == TYPE_IDENTIFIER_MethodDeclarator) &&
                // 向后检查：下一个token是TYPE_IDENTIFIER(269)、VAR_IdentifierComplement(280)�?
                (next_token1 == TYPE_IDENTIFIER || 
                 next_token1 == VAR_IdentifierComplement || 
                 next_token1 == YIELD_IdentifierComplement)
            ) {
                // 检查排除条件：前两个token不能�?'.' �?DOUBLE_COLON 后跟标识�?
                if (ts->current_pos > 1) {
                    int prev_prev_token = ts->index[ts->current_pos - 2];
                    
                    // 检查是否是需要排除的组合
                    if ((prev_prev_token == '.' || prev_prev_token == DOUBLE_COLON) &&
                        (prev_token == TYPE_IDENTIFIER || 
                         prev_token == VAR_IdentifierComplement || 
                         prev_token == YIELD_IdentifierComplement)) {
                        // 排除条件满足，保持为'.'
                    } 
                    // 检查是否是 '>' 后跟标识符的情况
                    else if (prev_prev_token == '>' &&
                             (prev_token == TYPE_IDENTIFIER || 
                              prev_token == VAR_IdentifierComplement || 
                              prev_token == YIELD_IdentifierComplement)) {
                        // 进入嵌套泛型回溯
                        int match_pos = skip_nested_generic(ts, ts->current_pos - 3);
                        if (match_pos >= 0) {
                            // 成功匹配嵌套泛型，保持为DOT_CommonName
                            ts->index[ts->current_pos] = DOT_CommonName;
                        } 
                    }
                    // 检查是否是 MethodInvocation 中的情况
                    //（即 '.' 后面为一�?TYPE_IDENTIFIER_MethodDeclarator ，但检�?'.' �?TYPE_IDENTIFIER 还没有变�?TYPE_IDENTIFIER_MethodDeclarator
                    else if(next_token2 == 40) {  // '(' 
                        // 排除条件满足，保持为'.'
                    }
                    else {
                        // 其他情况，保持为DOT_CommonName
                        ts->index[ts->current_pos] = DOT_CommonName;
                    }
                }
                else{ ts->index[ts->current_pos] = DOT_CommonName; }
            }
            break;

        case 60:  // '<'
            // 调用check_generic_arguments函数检查后续的泛型参数列表是否合法
            if (check_generic_arguments(ts, ts->current_pos)) {
                ts->index[ts->current_pos] = LANGLE; // 转换�?71
            }
            break;

        case 64:  // '@'
            // 检查后续三个token是否存在
            if (ts->current_pos + 3 < ts->count) {
                int token1 = ts->index[ts->current_pos + 1];
                int token2 = ts->index[ts->current_pos + 2];
                int token3 = ts->index[ts->current_pos + 3];
                
                // 检查是否符合规则：INTERFACE TYPE_IDENTIFIER '{'
                if (token1 == INTERFACE &&      // INTERFACE = 319
                    token2 == TYPE_IDENTIFIER && // TYPE_IDENTIFIER = 269
                    token3 == 123) {             // '{' = 123
                    ts->index[ts->current_pos] = AT_AnnotationTypeDeclaration; // 274
                    break;
                }
            }

            // 规则12.1：AT_Dims处理（只有在不是AT_AnnotationTypeDeclaration时执行）
            if (ts->current_pos + 1 < ts->count) {
                int next_token = ts->index[ts->current_pos + 1];
                if (next_token == TYPE_IDENTIFIER || next_token == TYPE_IDENTIFIER_MethodDeclarator) {
                    int pos = ts->current_pos + 2;  // 从下下个token开�?
                    int state = 0;  // 0: 初始状�? 1: 状�?, 2: 状�?, 3: 状�?
                    bool is_at_dims = false;  // 最终判断结�?
                    int depth = 0;  // 括号嵌套深度计数�?
                    
                    // 自动机主循环
                    while (pos < ts->count) {
                        int token = ts->index[pos];
                        
                        switch (state) {
                            case 0:  // 初始状�?
                                if (token == 91) {  // '['
                                    state = 1;  // 进入状�?
                                } 
                                else if (token == 40) {  // '('
                                    state = 2;  // 进入状�?
                                    depth = 1;  // 初始化括号深�?
                                    pos++;
                                } 
                                else if (token == 46 || token == DOT_CommonName) {  // '.' �?DOT_CommonName
                                    state = 3;  // 进入状�?
                                    pos++;
                                }
                                else if( (token == 64 || 
                                         token == 272)  && 
                                        (ts->index[pos+1] == TYPE_IDENTIFIER || 
                                         ts->index[pos+1] == TYPE_IDENTIFIER_MethodDeclarator) ){  //下一个仍为Annotation
                                            pos+=2;   //继续检查下一个Annotation
                                }
                                else {
                                    goto end_at_dims;  // 不符合条件，退�?
                                }
                                break;
                            
                            case 1:  // 状�?：遇�?['
                                is_at_dims = true;  // 满足条件
                                goto end_at_dims;  // 结束检�?
                            
                            case 2:  // 状�?：遇�?('，需要匹配括�?
                                // NormalAnnotation/SingleElementAnnotation中的括号()副循�?
                                while (pos < ts->count){
                                    int token2 = ts->index[pos];
                                    if (token2 == 40) {  // '('
                                        depth++;
                                    }
                                    else if (token2 == 41) {  // ')'
                                        depth--;
                                        if (depth == 0) {
                                            // 括号匹配完成
                                            pos++;
                                            break;  // 检查下一个token
                                        }
                                    }
                                    else if(token2 == 59 || token2 == 91 || token2 == 93 ){
                                        break;
                                    }
                                    pos++;
                                }
                                if(depth == 0){
                                    if(ts->index[pos] == 91){
                                        is_at_dims = true;  // 满足条件
                                    }
                                    else if(ts->index[pos] == 64 || ts->index[pos] == 272){ //下一个可能为Annotation
                                        state = 0;
                                        continue;  //继续检查下一个Annotation
                                    }
                                    goto end_at_dims;  // 结束检�?
                                }
                                else{
                                    is_at_dims = false;
                                    goto end_at_dims;  // 不符合条件，退�?
                                }
                                break;
                            
                            case 3:  // 状�?：遇�?.'或DOT_CommonName
                                if (token == TYPE_IDENTIFIER || token == TYPE_IDENTIFIER_MethodDeclarator) {
                                    state = 0;  // 回到初始状�?
                                    pos++;
                                    continue;  // 检查下一个token
                                } 
                                else {
                                    goto end_at_dims;  // 不符合条件，退�?
                                }
                                break;
                        }
                    }
                    
                    end_at_dims:
                    // 如果满足条件，则转换为AT_Dims
                    if (is_at_dims) {
                        ts->index[ts->current_pos] = AT_Dims;  // 272
                    }
                }
            }

            // 3. 检查是否是修饰符注�?(AT_Modifier) - 只有当不是AT_AnnotationTypeDeclaration或AT_Dims时才执行
            if (ts->index[ts->current_pos] == 64 &&  // 当前token仍是'@'（未被前两个规则修改�?
                ts->current_pos + 1 < ts->count) {
                
                int next_token = ts->index[ts->current_pos + 1];
                if (next_token == TYPE_IDENTIFIER || next_token == TYPE_IDENTIFIER_MethodDeclarator) {
                    
                    // 检查前一个token是否在修饰符�?
                    if (prev_token != -1 && IN_ALL_MODIFIERS(prev_token)) {
                        if(prev_token == DEFAULT && 
                            (prev_prev_token == 41 || prev_prev_token == 93)){
                                break;
                        }
                        ts->index[ts->current_pos] = AT_Modifier;
                        break;
                    }
                    
                    int state = 0;
                    int pos = ts->current_pos;
                    int depth = 0;
                    int angle_depth = 0;
                    bool is_at_modifier = false;
                    
                    // 自动机主循环
                    while (pos < ts->count && !is_at_modifier) {
                        int token = ts->index[pos];
                        
                        switch (state) {
                            case 0: // 初始状�?
                                if (token == 64) { // '@'
                                    pos++;
                                    if (pos >= ts->count) break;
                                    token = ts->index[pos];
                                    if (token == TYPE_IDENTIFIER || token == TYPE_IDENTIFIER_MethodDeclarator) {
                                        state = 1;
                                        pos++;
                                    } else {
                                        goto end_at_modifier;
                                    }
                                }
                                break;
                                
                            case 1: // 已处理标识符
                                if (token == 40) { // '('
                                    depth = 1;
                                    pos++;
                                    // 跳过括号内容
                                    while (pos < ts->count && depth > 0) {
                                        if (ts->index[pos] == 40) depth++;
                                        else if (ts->index[pos] == 41 || ts->index[pos] == RPAREN_CastExpression) depth--;
                                        pos++;
                                    }
                                    state = 2;
                                } 
                                else if (token == 46 || token == DOT_CommonName) { // '.' 
                                    pos++;
                                    state = 3;
                                } 
                                else if (token == 64 || token == AT_AnnotationTypeDeclaration) { // '@'
                                    pos++;
                                    state = 4;
                                } 
                                else if (START_OF_UNANNTYPE(token)) {
                                    state = 5;
                                } 
                                else if (token == LANGLE || token == 60) { // '<'
                                    pos++;
                                    state = 6;
                                } 
                                else if (IN_ALL_MODIFIERS(token) || token == ENUM || token == CLASS || 
                                         token == INTERFACE || token == VOID) {
                                    is_at_modifier = true;
                                    goto end_at_modifier;
                                } 
                                else {
                                    goto end_at_modifier;
                                }
                                break;
                                
                            case 2: // 状�?：处理完括号�?
                                if (pos < ts->count) {
                                    token = ts->index[pos];
                                    if (IN_ALL_MODIFIERS(token) || token == ENUM || token == CLASS || 
                                        token == INTERFACE || token == VOID) {
                                        is_at_modifier = true;
                                        goto end_at_modifier;
                                    } 
                                    else if (token == 64 || token == AT_AnnotationTypeDeclaration) {
                                        pos++;
                                        state = 4;
                                    } 
                                    else {
                                        goto end_at_modifier;
                                    }
                                }
                                break;
                                
                            case 3: // 状�?：处理点号后
                                if (pos < ts->count) {
                                    token = ts->index[pos];
                                    if (token == TYPE_IDENTIFIER || token == TYPE_IDENTIFIER_MethodDeclarator) {
                                        state = 1;
                                        pos++;
                                    } else {
                                        goto end_at_modifier;
                                    }
                                }
                                break;
                                
                            case 4: // 状�?：处理注�?
                                if (token == INTERFACE) {
                                    // 检查后续两个token：TYPE_IDENTIFIER '{'
                                    if (pos + 2 < ts->count) {
                                        int next1 = ts->index[pos + 1];
                                        int next2 = ts->index[pos + 2];
                                        if (next1 == TYPE_IDENTIFIER && next2 == 123) {
                                            is_at_modifier = true;
                                            goto end_at_modifier;
                                        }
                                    }
                                    goto end_at_modifier;
                                } 
                                else if (token == TYPE_IDENTIFIER || token == TYPE_IDENTIFIER_MethodDeclarator) { // 处理多个注解的情�?
                                    state = 1;
                                    pos++;
                                } 
                                else {
                                    goto end_at_modifier;
                                }
                                break;
                                
                            case 5: // 状�?：处理基本类型或�?接口类型
                                if (token == BYTE || token == SHORT || token == INT || 
                                    token == LONG || token == CHAR || token == FLOAT || 
                                    token == DOUBLE || token == BOOLEAN) {
                                    
                                    // 跳过数组维度
                                    while (pos < ts->count && 
                                          (ts->index[pos] == 91 || ts->index[pos] == 93)) {
                                        pos++;
                                    }
                                    
                                    if (pos >= ts->count) goto end_at_modifier;
                                    token = ts->index[pos];
                                    
                                    // 检查标识符
                                    if (token != TYPE_IDENTIFIER && 
                                        token != TYPE_IDENTIFIER_MethodDeclarator && 
                                        token != VAR_IdentifierComplement) {
                                        goto end_at_modifier;
                                    }
                                    else{
                                        pos++; // 进入identifier下一个token
                                    }

                                    // 跳过可能的数组维度和注解
                                    while (pos < ts->count) {
                                        token = ts->index[pos];
                                        if (token == 91 || token == 93) { // '[' or ']'
                                            pos++;
                                        } else if (token == 64 || token == AT_AnnotationTypeDeclaration) { // '@'
                                            pos = skip_annotation(ts, pos);
                                        } else {
                                            break;
                                        }
                                    }
                                    
                                    if (pos >= ts->count) goto end_at_modifier;
                                    token = ts->index[pos];
                                    
                                    if (token == 61) { // '='
                                        is_at_modifier = true;
                                        goto end_at_modifier;
                                    } 
                                    else if (token == 40) { // '('
                                        // 跳过括号内容
                                        depth = 1;
                                        pos++;
                                        while (pos < ts->count && depth > 0) {
                                            if (ts->index[pos] == 40) depth++;
                                            else if (ts->index[pos] == 41) depth--;
                                            pos++;
                                        }
                                        
                                        if (pos >= ts->count) goto end_at_modifier;
                                        token = ts->index[pos];
                                        
                                        if (token == 64 || token == 91 || token == 59 || 
                                            token == DEFAULT || token == THROWS || token == 123) {
                                            is_at_modifier = true;
                                            goto end_at_modifier;
                                        }
                                    }
                                } 
                                else if (token == TYPE_IDENTIFIER) {
                                    pos = skip_UnannClassOrInterfaceType(ts, pos);
                                    
                                    if (pos >= ts->count) goto end_at_modifier;
                                    
                                    // 跳过可能的数组维�?
                                    while (pos < ts->count && 
                                          (ts->index[pos] == 91 || ts->index[pos] == 93)) {
                                        pos++;
                                    }
                                    
                                    if (pos >= ts->count) goto end_at_modifier;
                                    token = ts->index[pos];
                                    
                                    // 检查标识符
                                    if (token != TYPE_IDENTIFIER && 
                                        token != TYPE_IDENTIFIER_MethodDeclarator && 
                                        token != VAR_IdentifierComplement) {
                                        goto end_at_modifier;
                                    }
                                    else{
                                        pos++; // 进入identifier下一个token
                                    }
                                    
                                    // 跳过可能的数组维度和注解
                                    while (pos < ts->count) {
                                        token = ts->index[pos];
                                        if (token == 91 || token == 93) { // '[' or ']'
                                            pos++;
                                        } else if (token == 64 || token == AT_AnnotationTypeDeclaration) { // '@'
                                            pos = skip_annotation(ts, pos);
                                        } else {
                                            break;
                                        }
                                    }
                                    
                                    if (pos >= ts->count) goto end_at_modifier;
                                    token = ts->index[pos];
                                    
                                    if (token == 61) { // '='
                                        is_at_modifier = true;
                                        goto end_at_modifier;
                                    } 
                                    else if (token == 40) { // '('
                                        // 跳过括号内容
                                        depth = 1;
                                        pos++;
                                        while (pos < ts->count && depth > 0) {
                                            if (ts->index[pos] == 40) depth++;
                                            else if (ts->index[pos] == 41) depth--;
                                            pos++;
                                        }
                                        
                                        if (pos >= ts->count) goto end_at_modifier;
                                        token = ts->index[pos];
                                        
                                        if (token == 64 || token == 91 || token == 59 || 
                                            token == DEFAULT || token == THROWS || token == 123) {
                                            is_at_modifier = true;
                                            goto end_at_modifier;
                                        }
                                    }
                                }
                                goto end_at_modifier;
                                
                            case 6: // 状�?：处理泛�?
                                // 跳过泛型内容
                                angle_depth = 1;
                                while (pos < ts->count && angle_depth > 0) {
                                    token = ts->index[pos];
                                    if (token == LANGLE || token == 60) angle_depth++;
                                    else if (token == 62) angle_depth--;
                                    pos++;
                                }
                                
                                // 跳过可能的注�?
                                while (pos < ts->count && 
                                      (ts->index[pos] == 64 || ts->index[pos] == AT_AnnotationTypeDeclaration)) {
                                    pos = skip_annotation(ts, pos);
                                }
                                
                                if (pos >= ts->count) goto end_at_modifier;
                                token = ts->index[pos];
                                
                                if (token == TYPE_IDENTIFIER_MethodDeclarator || 
                                    START_OF_UNANNTYPE(token)) {
                                    state = 7;
                                } 
                                else if (token == VOID) {
                                    is_at_modifier = true;
                                    goto end_at_modifier;
                                } 
                                else if (token == LANGLE || token == 60) {
                                    state = 6;
                                    pos++;
                                } 
                                else {
                                    goto end_at_modifier;
                                }
                                break;
                                
                            case 7: // 状�?：处理泛型后的内�?
                                if(token == TYPE_IDENTIFIER_MethodDeclarator || token == TYPE_IDENTIFIER){
                                    pos++;
                                    if (pos >= ts->count) goto end_at_modifier;
                                    token = ts->index[pos];

                                    if (token == 40) { // '('
                                        // 跳过括号内容
                                        depth = 1;
                                        pos++;
                                        while (pos < ts->count && depth > 0) {
                                            if (ts->index[pos] == 40) depth++;
                                            else if (ts->index[pos] == 41) depth--;
                                            pos++;
                                        }
                                        
                                        if (pos >= ts->count) goto end_at_modifier;
                                        token = ts->index[pos];
                                        
                                        if (token == THROWS || token == 123) {
                                            is_at_modifier = true;
                                            goto end_at_modifier;
                                        }
                                    } 
                                    else if (token == LANGLE || token == 60 || 
                                            token == 46 || token == DOT_CommonName || token == DOT) {
                                        state = 5;
                                        pos--;  // 从identifier的位置进入状�?
                                    }
                                    else{
                                        goto end_at_modifier;
                                    } 
                                }
                                else if (START_OF_UNANNTYPE(token)) {
                                    state = 5;
                                }
                                else{
                                    goto end_at_modifier;
                                }                               
                        }
                        
                        // 移动到下一个token
                        // if (state != 0) pos++;
                    }
                    
                end_at_modifier:
                    if (is_at_modifier) {
                        ts->index[ts->current_pos] = AT_Modifier;
                    }
                }
            }

            break;
            

        case 91:  // '['
            // 规则14：LBRACK_ArrayAccess处理
            // 检查向�?个token不是']'，且向前1个token是特定类�?
            if (ts->current_pos > 0 && ts->current_pos + 1 < ts->count) {
                int next_token = ts->index[ts->current_pos + 1];
                int prev_token = ts->index[ts->current_pos - 1];
                
                if (next_token != 93) { //不是']'
                    ts->index[ts->current_pos] = LBRACK_ArrayAccess; // 276
                    break;  // 已转换，跳过后续处理
                }
            }

            // 检查下一个token是否�?]'
            if (ts->current_pos + 1 < ts->count && ts->index[ts->current_pos + 1] == 93) {
                int pos = ts->current_pos + 2;  // �?]'后的位置开�?
                int state = 0;  // 0: 初始状�? 2: 状�?
                bool is_lbrack = false;  // 最终判断结�?

                while (pos < ts->count) {
                    int token = ts->index[pos];
            
                    switch (state) {
                        case 0:  // 初始状�?
                            if (token == 91 || token == LBRACK) {  // '[' �?LBRACK
                                // 检查下一个token是否�?]'
                                if (pos + 1 < ts->count && ts->index[pos + 1] == 93) {
                                    pos += 2;  // 跳过这对"[]"
                                } else {
                                    goto end_loop;  // 不符合条件，退�?
                                }
                            } else if (token == 46) {  // '.'
                                state = 2;
                                pos++;  // 移动到下一个token检�?
                            } else {
                                goto end_loop;  // 其他token，退�?
                            }
                            break;
                    
                        case 2:  // 状�?
                            if (token == CLASS) {  // CLASS(282)
                                is_lbrack = true;  // 满足所有条�?
                            }
                            goto end_loop;  // 无论是否匹配，都结束检�?
                    }
                }
        
                end_loop:
                // 如果满足状�?且遇到CLASS，则转换为LBRACK
                if (is_lbrack) {
                    ts->index[ts->current_pos] = LBRACK;  // 275
                }
            }
            break;

        case 38:  // '&'
            // 检查前后token条件
            bool is_ampersand_candidate = false;
            bool prev_ok = (prev_token == TYPE_IDENTIFIER || 
                        prev_token == '>' || 
                        prev_token == ']' || 
                        prev_token == VAR_IdentifierComplement);
            
            bool next_ok = (next_token1 == TYPE_IDENTIFIER || 
                        next_token1 == '@' || 
                        next_token1 == VAR_IdentifierComplement);
            
            if (prev_ok && next_ok) {

                // printf("check1\n");

                if(prev_token == '>' || 
                   prev_token == ']' ||
                   next_token1 == '@' ){
                        ts->index[ts->current_pos] = AMPERSAND; // 258
                        break;
                }

                // 自动�?：向前扫�?
                int auto1_result = 0; // 0: 保持�?, 1: 转为AMPERSAND, 2: 进入自动�?
                int pos = ts->current_pos - 1; // �?&'前一个token开�?
                int paren_count = 0; // 括号计数�?
                
                while (pos >= 0) {
                    int token = ts->index[pos];
                    
                    if (token == EXTENDS) { // 290
                        if (pos > 0 && 
                            (ts->index[pos - 1] == TYPE_IDENTIFIER ||
                            ts->index[pos - 1] == 63) ) {
                            auto1_result = 1; // 转为AMPERSAND
                            break;
                        }
                    } 
                    else if (token == ';') { // 59
                            auto1_result = 0; // 保持�?
                            break;
                    } 
                    else if (token == '(') { // 40
                        if (paren_count <= 0) {
                            auto1_result = 2; // 进入自动�?
                            break;
                        }
                        paren_count--;
                    } 
                    else if (token == ')') { // 41
                        paren_count++;
                    }
                    
                    pos--;
                }
                
                // 自动�?：如果需�?
                if (auto1_result == 2) {
                    int pos2 = ts->current_pos + 1; // �?&'后一个token开�?
                    int paren_count2 = 0; // 括号计数�?
                    
                    while (pos2 < ts->count) {
                        int token = ts->index[pos2];
                        
                        // 检查是否遇到终止token
                        if (token == LANGLE || token == DOT || 
                            token == '.' || token == '>' || token == '<') {
                            auto1_result = 1; // 转为AMPERSAND
                            break;
                        }
                        else if (token == ')') { // 处理右括�?41
                            if (paren_count2 <= 0) {
                                if (pos2 + 1 < ts->count && 
                                        (ts->index[pos2 + 1] == 40  ||
                                        ts->index[pos2 + 1] == 126 ||
                                        ts->index[pos2 + 1] == 33 ) ) {   // "(" "~" "!""
                                    auto1_result = 1; // 转为AMPERSAND
                                }else if (pos2 + 1 < ts->count && 
                                    ts->index[pos2 + 1] != 45 &&   // &往后推演不应该出现数值取反（45），但CastExpression中可以有
                                    !IS_NOT_FOLLOW_IN_CASTEXPRESSION(ts->index[pos2 + 1])) {
                                    auto1_result = 1; // 转为AMPERSAND
                                } else {
                                    auto1_result = 0; // 保持�?
                                }
                                break;
                            }
                            paren_count2--;
                        } 
                        else if (token == '(') { // 40
                            paren_count2++;
                        }
                        else if (token != TYPE_IDENTIFIER && 
                            token != VAR_IdentifierComplement &&
                            token != DOT_CommonName) {   // 检查允许继续的token
                            break;
                        }

                        pos2++;
                    }
                }
                
                // 根据自动机结果转换token
                if (auto1_result == 1) {
                    ts->index[ts->current_pos] = AMPERSAND; // 258
                }
            }
            break;

        case 41:  // ')'
            // 规则9：RPAREN_CastExpression处理
            if (ts->current_pos + 1 < ts->count) {
                int next_token = ts->index[ts->current_pos + 1];
                
                // 检查是否在IS_NOT_FOLLOW_IN_CASTEXPRESSION�?
                if (IS_NOT_FOLLOW_IN_CASTEXPRESSION(next_token)) {;
                    // 保持�?)'
                } 
                else {
                    int prev_token = (ts->current_pos > 0) ? ts->index[ts->current_pos - 1] : -1;
                    int next_token = (ts->current_pos + 1 < ts->count) ? ts->index[ts->current_pos + 1] : -1;
                    
                    // 检查前置条�?
                    bool prev_ok = (prev_token == TYPE_IDENTIFIER || 
                                prev_token == '>' || 
                                prev_token == ']');
                    
                    bool next_ok = (next_token == '~' || next_token == '!' || 
                                next_token == INC_OP || next_token == DEC_OP || 
                                next_token == '-' || next_token == '+' || 
                                next_token == '(' || next_token == SWITCH || 
                                next_token == TYPE_IDENTIFIER || 
                                next_token == VAR_IdentifierComplement || 
                                next_token == THIS || 
                                next_token == INTEGERLITERAL || 
                                next_token == FLOATINGPOINTLITERAL || 
                                next_token == TRUE || next_token == FALSE || 
                                next_token == CHARACTERLITERAL || 
                                next_token == STRINGLITERAL || 
                                next_token == MY_NULL || 
                                next_token == NEW || next_token == SUPER || 
                                next_token == UnqualifiedMethodIdentifier || 
                                next_token == '@');
                    
                    if (prev_ok && next_ok) {
                        int pos = ts->current_pos - 1; // �?)'前一个token开�?
                        int paren_count = 0; // 小括号计数器
                        int angle_count = 0; // 尖括号计数器
                        bool is_cast = false; // 最终判断结�?
                        bool found_ampersand = false; // 是否遇到AMPERSAND
                        bool invalid_token = false; // 是否遇到无效token
                        
                        // 自动�?：向前扫�?
                        while (pos >= 0) {
                            int token = ts->index[pos];
                            
                            // 处理括号计数
                            if (token == 41) { // ')'
                                paren_count++;
                            } 
                            else if (token == 40) { // '('
                                if (paren_count <= 0) {
                                    // 找到匹配�?('，退出自动机1
                                    break;
                                }
                                paren_count--;
                            }
                            
                            // 处理尖括号计�?
                            if (token == '>') {
                                angle_count++;
                            } 
                            else if (token == '<') {
                                angle_count--;
                            }
                            
                            // 只在非嵌套区域检查token
                            if (paren_count == 0 && angle_count == 0) {
                                if (token == AMPERSAND) { // 258
                                    found_ampersand = true;
                                    break;
                                }
                                
                                if (!IS_IN_PARENTHESES_OF_CASTEXPRESSION(token)) {
                                    invalid_token = true;
                                    break;
                                }
                            }
                            
                            pos--;
                        }
                        
                        // 根据自动�?结果处理
                        if (found_ampersand) {
                            is_cast = true;
                        } 
                        else if (!invalid_token && pos >= 0) {
                            // 自动�?结束，检查匹�?('前的token
                            int front_token = (pos > 0) ? ts->index[pos - 1] : -1;
                            int front_token_1 = (pos > 1) ? ts->index[pos - 2] : -1;
                            
                            if (front_token != -1 && !IS_NOT_FRONT_OF_CASTEXPRESSION(front_token)) {
                                if(front_token == TYPE_IDENTIFIER || front_token == TYPE_IDENTIFIER_MethodDeclarator ){
                                    if(front_token_1 != DOT && front_token_1 != DOT_CommonName &&
                                        front_token_1 != 64 && front_token_1 != AT_Modifier &&
                                        front_token_1 != AT_AnnotationTypeDeclaration && front_token_1 != AT_Dims){
                                            is_cast = true;
                                        }

                                }
                                else{
                                    is_cast = true;
                                }
                            }
                        }
                        
                        // 如果还不是确定的转换，进入自动机2
                        if (!is_cast && !invalid_token) {
                            int pos2 = ts->current_pos + 1; // �?)'后一个token开�?
                            int paren_count2 = 0; // 小括号计数器
                            
                            // 自动�?：向后扫�?
                            while (pos2 < ts->count) {
                                int token = ts->index[pos2];
                                
                                // 检查终止token
                                if (token == LANGLE || token == DOT || token == '.' || 
                                    token == '>' || token == '<') {
                                    is_cast = true;
                                    break;
                                }
                                
                                // 处理括号计数
                                if (token == 40) { // '('
                                    paren_count2++;
                                } 
                                else if (token == 41) { // ')'
                                    if (paren_count2 <= 0) {
                                        // 检查下一个token是否�?{'
                                        if (pos2 + 1 < ts->count && ts->index[pos2 + 1] != 123) {
                                            is_cast = true;
                                        }
                                        break;
                                    }
                                    paren_count2--;
                                }
                                
                                // 检查允许继续的token
                                if (!(token == TYPE_IDENTIFIER || 
                                    token == YIELD_IdentifierComplement || 
                                    token == VAR_IdentifierComplement || 
                                    token == DOT_CommonName)) {
                                    break;
                                }
                                
                                pos2++;
                            }
                        }
                        
                        // 如果确定为强制类型转换，转换token
                        if (is_cast) {
                            ts->index[ts->current_pos] = RPAREN_CastExpression; // 259
                        }
                    }
                }
            }
            break;

        case SEALED:  // 值为381
        {
            int pos = ts->current_pos + 1; // 从下一个token开始检�?
            bool is_sealed_identifier = false;
            
            while (pos < ts->count) {
                int next_token = ts->index[pos];
                
                if (next_token == CLASS || next_token == INTERFACE) {
                    // 遇到CLASS或INTERFACE，保持为SEALED
                    break;
                } else if (IN_ALL_MODIFIERS(next_token)) {
                    // 遇到修饰符，继续检查下一个token
                    pos++;
                } else if (next_token == 64 || next_token == AT_AnnotationTypeDeclaration) {
                    // 遇到注解，跳过注�?
                    pos = skip_annotation(ts, pos);
                } else {
                    // 遇到其他token，转换为SEALED_IdentifierComplement
                    is_sealed_identifier = true;
                    break;
                }
            }
            
            // 如果到达token流末尾，也转换为SEALED_IdentifierComplement
            if (pos >= ts->count) {
                is_sealed_identifier = true;
            }
            
            if (is_sealed_identifier) {
                ts->index[ts->current_pos] = SEALED_IdentifierComplement; // 转换�?83
            }
            break;
        }

        case NON_SEALED:  // 值为382
        {
            int pos = ts->current_pos + 1; // 从下一个token开始检�?
            bool is_sealed_identifier = false;
            
            while (pos < ts->count) {
                int next_token = ts->index[pos];
                
                if (next_token == CLASS || next_token == INTERFACE) {
                    // 遇到CLASS或INTERFACE，保持为NON_SEALED
                    break;
                } else if (IN_ALL_MODIFIERS(next_token)) {
                    // 遇到修饰符，继续检查下一个token
                    pos++;
                } else if (next_token == 64 || next_token == AT_AnnotationTypeDeclaration) {
                    // 遇到注解，跳过注�?
                    pos = skip_annotation(ts, pos);
                } else {
                    // 遇到其他token，转换为NON_SEALED_IdentifierComplement
                    is_sealed_identifier = true;
                    break;
                }
            }
            
            // 如果到达token流末尾，也转换为NON_SEALED_IdentifierComplement
            if (pos >= ts->count) {
                is_sealed_identifier = true;
            }
            
            if (is_sealed_identifier) {
                ts->index[ts->current_pos] = NON_SEALED_IdentifierComplement; // 转换�?83
            }
            break;
        }

        case 59:  // ';'
            break;

        // case 44:  // ','
        //     // 检查是否是尾随逗号 TRAILING_COMMA
        //     if (next_token1 == 125) {   // '}'
        //         ts->index[ts->current_pos] = TRAILING_COMMA; // 转换�?79
        //     }
        //     break;

        case INC_OP: //++(后缀形式)
            if(next_token1 == 59 || next_token1 == 93 || next_token1 == 125 || next_token1 == 41 || next_token1 == 44 ||                    // 向后检查是';'�?]'�?}'�?)'�?,'
               next_token1 == 61 || next_token1 == EQ || next_token1 == GE || next_token1 == LE || next_token1 == 60 || next_token1 == 62){ // 向后检查是'='�?=='�?>='�?<='�?<'�?>'
                break;
            }
            if(prev_token == 59 || prev_token == 61 ||                   // 向前检查为分号';'或者等�?='
                 prev_token == PREFIX_INC || prev_token == PREFIX_DEC || // 向前检查为'++'(前缀形式)或�?--'(前缀形式)
                 next_token1 == TYPE_IDENTIFIER){                        // 向后检查为TYPE_IDENTIFIER
                ts->index[ts->current_pos] = PREFIX_INC;                 // ++(前缀形式)
            }
            break;

        case DEC_OP: //--(后缀形式)
            if(next_token1 == 59 || next_token1 == 93 || next_token1 == 125 || next_token1 == 41 || next_token1 == 44 ||                    // 向后检查是';'�?]'�?}'�?)'�?,'
               next_token1 == 61 || next_token1 == EQ || next_token1 == GE || next_token1 == LE || next_token1 == 60 || next_token1 == 62){ // 向后检查是'='�?=='�?>='�?<='�?<'�?>'
                break;
            }
            if(prev_token == 59 || prev_token == 61 ||                   // 向前检查为分号';'或者等�?='
                 prev_token == PREFIX_INC || prev_token == PREFIX_DEC || // 向前检查为'++'(前缀形式)或�?--'(前缀形式)
                 next_token1 == TYPE_IDENTIFIER){                        // 向后检查为TYPE_IDENTIFIER
                ts->index[ts->current_pos] = PREFIX_DEC;                 //--(前缀形式)
            }
            break;

        // 这里可以添加其他token类型的处�?
        default:
            break;
    }

 
    int token=ts->index[ts->current_pos];
    ts->current_pos++;
    return token;
}


/*
int main()
{
    const char *fname = "example.java";
    FILE *f;

    // // prepare input file
    // f = fopen(fname, "w");
    // if (f == NULL) {
    //     perror("Error opening file");
    //     return 1;
    // }

    // fprintf(f,
    //     "@interface MyAnnotation { String value(); }\n"
    //     "String @DimsAnnotation [] array;\n"
    //     "Object @Dims [][] multiArray;\n"
    //     "public void process(@ParamAnnotation String input) {}\n"
    //     "public void localExample() { @LocalAnnotation int count = 0; }\n"
    //     "class Box<@TypeParam T> {}\n"
    //     "enum Color { @Annotation RED, GREEN, BLUE }\n"
    //     "@PackageAnnotation package com.example;\n"
    //     "public void read() throws @ExceptionAnnotation IOException {}\n"
    //     "stop\n"
    //     "@Annotation public void method() {}\n"
    //     "public @Annotation class MyClass {}\n"
    //     "protected @Annotation static final class InnerClass {}\n"
    //     "public @Annotation void myMethod() {}\n"
    //     "synchronized @Annotation final int calculate() { return 0; }\n"
    //     "private @Annotation String name;\n"
    //     "volatile @Annotation boolean flag;\n"
    //     "public static @Annotation final int CONSTANT = 42;\n"
    //     "@Annotation abstract @AnotherAnnotation void process();\n"
    //     "@Annotation public MyClass() {}\n"
    //     "@Annotation(value = 1) public class TestClass {}\n"  // 
    //     "@Annotation(max = 100) private int value;\n"  // 
    //     "@OuterAnnotation(@InnerAnnotation) public class NestedExample {}\n"  // 
    //     "@Annotation public class GenericClass<T> {}\n"
    //     "@Complex(name = 1, values = {1, 2, 3}) \n"  // 
    //     "public class ComplexAnnotationExample {}\n"
    //     "@Annotation public class ComplexGeneric<@TypeParam T extends @BoundAnnotation Serializable> {}\n"
    //     "public @ReturnAnnotation String getName() { return name; }\n"   
    // );
    // fclose(f);

    // f = fopen(fname, "rb");

    f = fopen(fname, "r");
    if (f == NULL) {
        perror("Error opening question.java");
        return 1;
    }

    struct input_t* in = (struct input_t*) malloc (sizeof(struct input_t));
    init_input(in, f);

    // 设置初始行号
    if (!lex(in)) {
        // 错误处理逻辑
        fprintf(stderr, "Lexical analysis failed!\\n");
        fprintf(stderr, "Lexical analysis failed!
        freeTokens(); // 确保释放动态分配的内存
        fclose(f);    // 关闭文件
        return -1;    // 或者其他适当的错误返回�?
    }

    fclose(f);

    // 创建 TokenStream
    TokenStream *ts = create_stream(Index, Token, count);

    // 使用 lex1 �?lex2 进行词法分析
    int error = 0;
    int lookahead = 0;
    printf("Token analysis:\n");
    
    // 使用 lex2 循环处理所�?token
    while (ts->current_pos < ts->count) {
        int token = base_yylex(ts, &lookahead, &error);
        if (error == -1) {
            printf("Reached EOF\n");
            break;
        }
        if (error == -2) {
            printf("Illegal token encountered\n");
            break;
        }
        printf("Current token ID: %d  Token: %s  Lookahead: %d\n", 
               token, Token[ts->current_pos - 1], lookahead);
    }

    // 清理
    destroy_stream(ts);

    // printTokens();
    freeTokens();

    // cleanup
    remove(fname);
    return 0;
}
*/

// 初始�?Token 序列（类�?lib.c �?init_token_list�?
int init_token_list() {
    printf("init_token_list called\n");
    // 检查是否设置了输入文件�?
    if (!input_filename) {
        fprintf(stderr, "Error: No input file specified\n");
        return 0;
    }

    printf("Input filename: %s\n", input_filename);

    // 1. 执行 RE2C 词法分析（填�?Token[]/Index[]�?
    struct input_t in;
    FILE *f = fopen(input_filename, "r");
    if (!f) {
        fprintf(stderr, "Cannot open file: %s\n", input_filename);
        return 0;
    }

    printf("File opened successfully\n");

    init_input(&in, f);
    if (!lex(&in)) return 0;

    printf("Lexed %d tokens\n", count);

    // lex(&in); // 调用 RE2C 词法分析�?

    // 2. 创建 TokenStream 并进行上下文敏感转换
    TokenStream *ts = create_stream(Index, Token, count);
    if (!ts) return 0;

    max_len = count + 100;
    token_list = malloc(max_len * sizeof(int));
    token_index = 0;

    int error, lookahead;
    while (ts->current_pos < ts->count && token_index < max_len) {
        int token = base_yylex(ts, &lookahead, &error);
        if (error) break;
        token_list[token_index++] = token; // 存储转换后的 Token

        // if(TokenLine[token_index-1] == 295){
        //     // printf("token:%d\n",token);
        //     printf("�?%d �?token : %d\n",TokenLine[token_index-1],token);
        // }
        
    }

    destroy_stream(ts);
    fclose(f);
    return 1;
}

// 供解析器调用�?yylex 函数
int yylex(void) {
    static int token_pos = 0;  // current token index

    if (token_list == NULL) {
        init_token_list();
    }

    if (token_pos >= token_index) {
        return 0;  // EOF
    }

    int line = (token_pos < MAX) ? TokenLine[token_pos] : -1;
    int token = token_list[token_pos];
    if (token == -2) {
        return 0;
    }
    const char *lexeme = Token[token_pos];
    yylval.node = NULL;
    yylloc.first_line = (line > 0) ? line : 0;
    yylloc.last_line = yylloc.first_line;
    yylloc.first_column = 1;
    yylloc.last_column = 1;
    AstNode *token_node = make_token_node(token, lexeme, line);
    if (token_node) {
        yylval.node = token_node;
    }

    token_pos++;
    return token;
}









