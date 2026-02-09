// 请注意，以下带着 “// re2c” 的部分不是注释！请勿乱删！

// re2c $INPUT -o $OUTPUT -i
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <ctype.h>
#include "java_ast.h"  // 必须在 parser.tab.h 之前，因为 YYSTYPE 使用了 AstNode
#include "parser.tab.h"
#include "shared.h"  // 包含共享头文件

// #define MAX 1500

int Index[MAX];             // 对应yytokentype后面的数字
char*Token[MAX];            // 将Index对应内容放入其中
// static int count=0;
int count=0;                // 扫描到的token数量
int TokenLine[MAX];         // 存储每个token的行号

static bool in_generic = false;      // 标记是否在泛型参数列表中
static int prev_token_type = -1;     // 储存前一个token
static int generic_depth = 0;        // 记录尖括号（'<''>'）深度
static int line = 1;                 // 添加行号跟踪，初始行号为1
static bool in_switch_expr = false;  // 标记是否在switch表达式中
static int switch_expr_depth = 0;    // 记录switch表达式内大括号的嵌套深度

// 打印所有token信息的函数
void printTokens() {
    printf("Total number of tokens: %d\n", count);
    printf("Index\tToken\n");
    for (int i = 0; i < count; i++) {
        printf("%d\t%s\n", Index[i], Token[i]);
    }
}


// 释放 Token 数组中的内存（假设使用 malloc 分配）
void freeTokens() {
    for (int i = 0; i < count; i++) {
        if (Token[i] != NULL) {
            free(Token[i]);  // 使用 free 释放内存
            Token[i] = NULL;  // 将指针置为 nullptr 避免悬挂指针
        }
    }
}

void store_token(int index,const char*value){
    if (count >= MAX) {
        fprintf(stderr, "Too many tokens, increase MAX\n");
        return;
    }

    Index[count] = index;
    // 为 Token[count] 分配空间，并存储解析到的值
    Token[count] = malloc(strlen(value) + 1);
    if (!Token[count]) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    strcpy(Token[count], value);

    TokenLine[count] = line; // 存储行号

    count++;

    prev_token_type = index; // 记录当前 token 类型
    // fprintf(stdout,"Memory %d\n",index);
}


void handle_id(const unsigned char *id_start, size_t id_length) {
    if (count >= MAX) {
        fprintf(stderr, "Too many tokens, increase MAX\n");
        return;
    }
    
    // 为新 token 分配空间
    Token[count] = (char*)malloc(id_length + 1);
    if (!Token[count]) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    // 复制 token 并添加 null 终止符
    memcpy(Token[count], id_start, id_length);
    Token[count][id_length] = '\0';
    Index[count]=TYPE_IDENTIFIER;

    prev_token_type = 269; // 记录当前 token 类型为 TYPE_IDENTIFIER （269）

    // 可以选择性地更新 Index 数组或其他处理
    // Index[count] = ...

    // 存储行号
    TokenLine[count] = line;

    count++; // 增加计数器
}



/*!max:re2c*/
static const size_t SIZE = 64 * 1024;
/*
struct input_t {
    //unsigned char buf[SIZE + YYMAXFILL];
    unsigned char buf[64 * 1024 + YYMAXFILL];  // 输入缓冲区
    unsigned char *lim;  // 指向缓冲区有效数据的结束位置
    unsigned char *cur;  // 当前扫描位置（词法分析的"光标"）
    unsigned char *mar;  // 匹配回溯标记（用于re2c的回溯机制）
    unsigned char *tok;  // 当前token的起始位置
    // 指针关系 buf ≤ tok ≤ mar ≤ cur ≤ lim

    bool eof;
    FILE * file;

    /*
    input_t(FILE *f)
        : buf()
        , lim(buf + SIZE)
        , cur(lim)
        , mar(lim)
        , tok(lim)
        , eof(false)
        , file(f)
    {}
    bool fill(size_t need)
    {
        if (eof) {
            return false;
        }
        const size_t free = tok - buf;
        if (free < need) {
            return false;
        }
        memmove(buf, tok, lim - tok);
        lim -= free;
        cur -= free;
        mar -= free;
        tok -= free;
        lim += fread(lim, 1, free, file);
        if (lim < buf + SIZE) {
            eof = true;
            memset(lim, 0, YYMAXFILL);
            lim += YYMAXFILL;
        }
        return true;
    }

};
*/

// 缓冲区初始状态
void init_input(struct input_t* in, FILE* f) {
  // 指针关系 buf ≤ tok ≤ mar ≤ cur ≤ lim
  in->lim = in->buf + SIZE;  // 初始时lim指向缓冲区末尾
  in->mar = in->lim;         // 回溯指针初始化
  in->cur = in->lim;         // 当前扫描位置初始化
  in->tok = in->lim;         // token起始位置初始化
  in->eof = false;           // 文件未结束
  in->file = f;              // 关联输入文件
}


bool fill(struct input_t* in, size_t need)
{
    if (in->eof) {
        return false;
    }

    // 计算缓冲区前部的空闲空间
    const size_t free = in->tok - in->buf;
    if (free < need) {                 // 检查是否有足够空间满足需求
        return false;
    }

    // 移动未处理数据到缓冲区前部
    memmove(in->buf, in->tok, in->lim - in->tok);

    // 更新指针位置
    in->lim -= free;
    in->cur -= free;
    in->mar -= free;
    in->tok -= free;

    // 从文件读取新数据
    in->lim += fread(in->lim, 1, free, in->file);

    // 最后一次填充时检测到EOF，添加 YYMAXFILL 个零作为安全边界，确保所有token都能完整处理
    if (in->lim < in->buf + SIZE) {
        in->eof = true;
        memset(in->lim, 0, YYMAXFILL);     // 填充零值
        in->lim += YYMAXFILL;              // 扩展边界
    }

    return true;
}


/*!re2c re2c:define:YYCTYPE = "unsigned char"; */

// 检查数字是否溢出
static bool adddgt(unsigned long *u, unsigned long d, int base)
{
    // 检查添加新数字后是否会导致整数溢出
    if (*u > (ULONG_MAX - d) / base) {   // 等效于 *u * base + d > ULONG_MAX
        return false;
    }
    *u = (*u) * base + d;
    return true;
}

// 检查是否是二进制有效数字
static bool lex_bin(const unsigned char *s, const unsigned char *e, unsigned long *u)
{
    // *s : 输入字符串起始位置（指向二进制字面量）
    // *e : 输入字符串结束位置
    // *u : 输出解析结果（无符号长整型），下同
    for (*u = 0, s += 2; s < e; ++s) {
        if (*s != '0' && *s != '1') {  // 跳过"0b"前缀后(s += 2)，二进制字符只允许 1 或 0
            return false; // Invalid character for binary literals
        }
        if (!adddgt(u, *s - 0x30u, 2)) {
            return false; // Overflow or other error
        }
    }
    return true;
}

// 八进制
static bool lex_oct(const unsigned char *s, const unsigned char *e, unsigned long *u)
{
    for (*u = 0, ++s; s < e; ++s) {
        if (!adddgt(u, *s - 0x30u, 8)) {
            return false;
        }
    }
    return true;
}

// 十进制
static bool lex_dec(const unsigned char *s, const unsigned char *e, unsigned long *u)
{
    for (*u = 0; s < e; ++s) {
        if (!adddgt(u, *s - 0x30u, 10)) {
            return false;
        }
    }
    return true;
}

// 十六进制
static bool lex_hex(const unsigned char *s, const unsigned char *e, unsigned long *u)
{
    for (*u = 0, s += 2; s < e;) {
    /*!re2c
        re2c:yyfill:enable = 0;      // 禁用缓冲区填充
        re2c:define:YYCURSOR = s;

        // 数字字符 (0-9)
        *     { if (!adddgt(u, s[-1] - 0x30u, 16))      return false; continue; }

        // 小写十六进制字母 (a-f)
        [a-f] { if (!adddgt(u, s[-1] - 0x61u + 10, 16)) return false; continue; }

        // 大写十六进制字母 (A-F)
        [A-F] { if (!adddgt(u, s[-1] - 0x41u + 10, 16)) return false; continue; }
    */
    }
    return true;
}


// 修改后的 lex_str 函数
// 处理字符串字面量和字符字面量
// 追加原始字面量片段（不做反转义）
static bool append_raw_bytes(char *buffer, size_t *buf_idx,
                             const unsigned char *start, const unsigned char *end) {
    size_t len = (size_t)(end - start);
    if (*buf_idx + len >= MAX) return false;
    memcpy(buffer + *buf_idx, start, len);
    *buf_idx += len;
    return true;
}

static bool lex_str(struct input_t *in, unsigned char q) {
    char buffer[MAX];
    size_t buf_idx = 0;

    // 进入时：in->cur 应该已经在“引号后第一个字符”
    for (;;) {
        in->tok = in->cur;
        const unsigned char *tok0 = in->cur; // 记录本次匹配起点（用于转义解析）

        /*!re2c
            re2c:yyfill:enable = 1;
            re2c:define:YYCURSOR = in->cur;
            re2c:define:YYMARKER = in->mar;
            re2c:define:YYLIMIT = in->lim;
            re2c:define:YYFILL = "if (!fill(in, @@)) return false;";
            re2c:define:YYFILL:naked = 1;

            // 文件结束/非法
            "\x00" { return false; }

            // 字符串/字符字面量里不允许裸换行
            "\n" {
                fprintf(stderr, "Error at line %d: Newline in string literal\n", line);
                return false;
            }

            // 结束引号：注意这里直接按字符判断 q
            "'" {
                if (q == '\'') goto done;
                // 如果 q 不是单引号，说明这是普通字符，落到普通处理：
                if (buf_idx + 1 >= MAX) return false;
                buffer[buf_idx++] = '\'';
                continue;
            }
            "\"" {
                if (q == '"') goto done;
                if (buf_idx + 1 >= MAX) return false;
                buffer[buf_idx++] = '"';
                continue;
            }

            // 反斜杠开始的转义（先识别常见的）
            "\\'"  { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                     else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '\''; }
                     continue; }
            "\\\"" { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                     else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '"'; }
                     continue; }
            "\\\\" { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                     else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '\\'; }
                     continue; }
            "\\n"  { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                     else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '\n'; }
                     continue; }
            "\\t"  { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                     else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '\t'; }
                     continue; }
            "\\b"  { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                     else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '\b'; }
                     continue; }
            "\\r"  { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                     else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '\r'; }
                     continue; }
            "\\f"  { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                     else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '\f'; }
                     continue; }
            "\\v"  { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                     else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '\v'; }
                     continue; }
            "\\s"  { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                     else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = ' '; }
                     continue; }

            // 八进制：\0 ~ \377（用你已有的 lex_oct）
            "\\" [0-7]                   { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                                           else { unsigned long u; if (!lex_oct(in->tok, in->cur, &u)) return false;
                                                  if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = (char)u; }
                                           continue; }
            "\\" [0-7][0-7]              { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                                           else { unsigned long u; if (!lex_oct(in->tok, in->cur, &u)) return false;
                                                  if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = (char)u; }
                                           continue; }
            "\\" [0-3][0-7][0-7]         { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                                           else { unsigned long u; if (!lex_oct(in->tok, in->cur, &u)) return false;
                                                  if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = (char)u; }
                                           continue; }

            // 十六进制：\x...
            "\\x" [0-9a-fA-F]+           { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                                           else { unsigned long u; if (!lex_hex(in->tok, in->cur, &u)) return false;
                                                  if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = (char)u; }
                                           continue; }

            // Unicode：先做“安全处理”，避免 (char) 截断导致乱码/破坏
            // 如果你后续要做 UTF-8 编码输出，可以在这里把 u 转成 UTF-8 序列追加到 buffer。
            "\\u" [0-9a-fA-F]{4}         { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                                           else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '?'; }
                                           continue; }
            "\\U" [0-9a-fA-F]{8}         { if (q == '"') { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; }
                                           else { if (buf_idx + 1 >= MAX) return false; buffer[buf_idx++] = '?'; }
                                           continue; }

            // 普通字符：不是反斜杠、不是换行、不是 0
            [^\\\n\x00]                  { 
                                           if (buf_idx + 1 >= MAX) return false;
                                           buffer[buf_idx++] = (char)in->tok[0];
                                           continue;
                                         }
        */
    }

done:
    buffer[buf_idx] = '\0';

    if (q == '\'') {
        // 允许你保持旧逻辑：只取第一个字符
        char char_value[2] = { buffer[0], '\0' };
        store_token(CHARACTERLITERAL, char_value);
    } else {
        store_token(STRINGLITERAL, buffer);
    }
    return true;
}


// 处理文本块函数
static bool lex_text_block(struct input_t *in) {
    char buffer[MAX];
    size_t buf_idx = 0;

    // 进入时：lex() 已经匹配并消耗了起始 """，
    // 所以这里的 in->cur 指向文本块内容的第一个字符（可能是 \n 或空格）。
    for (;;) {
        in->tok = in->cur;

        /*!re2c
            re2c:yyfill:enable = 1;
            re2c:define:YYCURSOR = in->cur;
            re2c:define:YYMARKER = in->mar;
            re2c:define:YYLIMIT  = in->lim;
            re2c:define:YYFILL   = "if (!fill(in, @@)) return false;";
            re2c:define:YYFILL:naked = 1;

            // 结束标记：遇到 """ 就结束（这三个引号不进 buffer）
            "\"\"\"" {
                if (buf_idx >= MAX) return false;
                buffer[buf_idx] = '\0';
                store_token(TextBlock, buffer);
                return true;
            }

            // 文件结束：不完整文本块
            "\x00" { return false; }

            // 允许换行：计数行号 + 写入 \n
            "\n" {
                line++;
                if (buf_idx + 1 >= MAX) return false;
                buffer[buf_idx++] = '\n';
                continue;
            }

            // 简单转义：\" \\ \n \t 等（按需补全）
            "\\\"" { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; continue; }
            "\\\\" { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; continue; }
            "\\n"  { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; continue; }
            "\\t"  { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; continue; }
            "\\r"  { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; continue; }

            // Unicode 转义（如果你暂时不想做 UTF-8，就先放 ?，避免 char 截断造成乱码）
            "\\u" [0-9a-fA-F]{4} { if (!append_raw_bytes(buffer, &buf_idx, in->tok, in->cur)) return false; continue; }

            // 其它任意字符：写入当前字符
            [^] {
                if (buf_idx + 1 >= MAX) return false;
                buffer[buf_idx++] = (char)in->tok[0];
                continue;
            }
        */
    }
}

// 修改后的 lex_flt 函数
// 解析标准十进制浮点数和十六进制浮点数
static bool lex_flt(const unsigned char *s, bool is_hex) {
    double d = 0;          // 浮点数结果
    double frac_factor = 1; // 小数部分的权重
    int exp_value = 0;     // 指数值
    double exp_base = is_hex ? 2.0 : 10.0; // 指数基数
    bool exp_negative = false; // 指数是否为负
    char buffer[MAX];       // 用于存储解析到的浮点字面量
    size_t buf_idx = 0;     // 缓冲区索引

    /*!re2c
        re2c:yyfill:enable = 0;
        re2c:define:YYCURSOR = s;
    */

// 整数部分
mant_int:
    /*!re2c
        "."   { buffer[buf_idx++] = '.'; goto mant_frac; }
        [pP]  { if (!is_hex) return false; buffer[buf_idx++] = s[-1]; goto exp_sign; }
        [eE]  { if (is_hex) return false; buffer[buf_idx++] = s[-1]; goto exp_sign; }

        // 新增规则：处理后缀字符
        [fFdD]  { 
            buffer[buf_idx++] = s[-1]; 
            goto sfx; 
        }

        [0-9a-fA-F] { 
            buffer[buf_idx++] = s[-1];
            d = (d * (is_hex ? 16 : 10)) + 
                (s[-1] <= '9' ? s[-1] - '0' : (s[-1] | 32) - 'a' + 10); 
            goto mant_int; 
        }
        * {
            // 输出错误信息，可根据实际情况调整错误信息内容
            fprintf(stderr, "Error at line %d: Unexpected character in mantissa integer part\n", line);
            return false;
        }
    */

// 小数部分
mant_frac:
    /*!re2c
        ""    { goto sfx; }
        [pP]  { if (!is_hex) return false; buffer[buf_idx++] = s[-1]; goto exp_sign; }
        [eE]  { if (is_hex) return false; buffer[buf_idx++] = s[-1]; goto exp_sign; }
        [0-9a-fA-F] { 
            buffer[buf_idx++] = s[-1];
            frac_factor /= (is_hex ? 16.0 : 10.0); 
            d += frac_factor * (s[-1] <= '9' ? s[-1] - '0' : (s[-1] | 32) - 'a' + 10); 
            goto mant_frac; 
        }
    */

// 指数符号
exp_sign:
    /*!re2c
        "+"   { buffer[buf_idx++] = '+'; goto exp; }
        "-"   { buffer[buf_idx++] = '-'; exp_negative = true; goto exp; }
        *     { goto exp; }
    */

// 指数值
exp:
    /*!re2c
        ""    { 
            // 根据指数值调整浮点数
            if (exp_negative) {
                while (exp_value > 0) { 
                    d /= exp_base; 
                    exp_value--; 
                }
            } else {
                while (exp_value > 0) { 
                    d *= exp_base; 
                    exp_value--; 
                }
            }
            goto sfx; 
        }
        [0-9] { 
            buffer[buf_idx++] = s[-1];
            exp_value = (exp_value * 10) + (s[-1] - '0'); 
            goto exp; 
        }
    */

/* 
   处理类型后缀
   f/F → float
   d/D → double
*/
sfx:
    /*!re2c
        ""    { goto end; }
        [fFdD]  { 
            buffer[buf_idx++] = s[-1];
            goto end; 
        }
    */

end:
    buffer[buf_idx] = '\0'; // 终止缓冲区字符串

    // 根据 `is_hex` 标志存储类型
    if (is_hex) {
        store_token(FLOATINGPOINTLITERAL, buffer);
    } else {
        store_token(FLOATINGPOINTLITERAL, buffer);
    }
    return true;
}


// 查找与')'匹配的'('的索引
int find_matching_paren(int start_index) {
    if (start_index <= 0) {
        return -1;
    }
    int nest_level = 1; // 从 start_index 是 ')'，所以初始嵌套为1
    for (int i = start_index - 1; i >= 0; i--) {
        if (Index[i] == 41) { // ')'
            nest_level++;
        } else if (Index[i] == 40) { // '('
            nest_level--;
            if (nest_level == 0) {
                return i;
            }
        }
    }
    return -1; // 没有找到匹配
}


// static bool lex(struct input_t *in)
bool lex(struct input_t *in)
{
    unsigned long u;
    for (;;) {
        in->tok = in->cur;
        /*!re2c
            re2c:yyfill:enable = 1;            // 启用缓冲区填充机制
            re2c:define:YYCURSOR = in->cur;    // 将 re2c 的内部光标指针映射到自定义结构
            re2c:define:YYMARKER = in->mar;    // 定义回溯标记指针
            re2c:define:YYLIMIT = in->lim;     // 定义缓冲区有效数据结束位置（可安全扫描的边界）
            re2c:define:YYFILL = "if (!fill(in, @@)) return false;";       // 自定义缓冲区填充函数
            re2c:define:YYFILL:naked = 1;      // 指定 YYFILL 为"裸"调用，确保自定义代码被原样插入

            end = "\x00";

            // 在错误规则中添加行号信息
            *   { 
                fprintf(stderr, "Lexical error at line %d: Unexpected character '0x%x'\n", 
                        line, *in->tok); 
                return false; 
            }
            end {
                //fprintf(stderr, "\n");
                return in->lim - in->tok == YYMAXFILL;
            }

            // macros
            macro = ("#" | "%:") ([^\n] | "\\\n")* "\n";
            macro { continue; }

            // whitespaces
            // 多行注释规则
            mcm = "/*" ([^*] | ("*" [^/]))* "*""/";       
            mcm {
                // 计算注释块长度
                size_t comment_length = in->cur - in->tok;
                
                // 只有长度大于4才有实际内容 (/* + */ = 4字符)
                if (comment_length > 4) {
                    const unsigned char *start_ptr = in->tok + 2; // 跳过 /*
                    const unsigned char *end_ptr = in->cur - 2;   // 跳过 */
                    
                    // 遍历注释内容统计换行
                    for (const unsigned char *ptr = start_ptr; ptr < end_ptr; ++ptr) {
                        if (*ptr == '\n') {
                            line++;
                        }
                    }
                }
                continue;
            }

            // 单行注释规则
            scm = "//" [^\n]* "\n";                       
            scm {
                // 单行注释包含一个换行符
                line++; 
                continue;
            }

            // 处理换行符规则
            nl = "\n";
            nl { 
                line++; 
                continue; 
            }

            // 处理空白字符规则（不再包含注释）
            // wsp = ([ \t\v\r] | scm | mcm)+;               // 空白组合
            wsp = [ \t\v\r]+;               // 空白组合
            wsp { 
                continue;                   // 忽略所有空白
            }
            // 文本块规则（三个双引号）
            "\"\"\"" {
                if (!lex_text_block(in)) return false;
                continue;
            }

            // character and string literals
            "L"? ['"] {
    if (!lex_str(in, in->cur[-1])) return false;
    continue;
}

            // 修改后的普通字符串规则
            /*
            "L"? ("\"" [^"]* | "'" [^']*) {
                if (!lex_str(in, in->cur[-1])) return false;
                continue;
            }
            */
            "L"? "''" { return false; }                   // 空字符字面量错误

            

            // integer literals
            oct = "0" [0-7]*;
            dec = [1-9][0-9]*;
            hex = '0x' [0-9a-fA-F]+;
            bin = '0b' [01]+;  // New binary literal rule

            oct { if (!lex_oct(in->tok, in->cur, &u)) return false; goto sfx; }
            dec { if (!lex_dec(in->tok, in->cur, &u)) return false; goto sfx; }
            hex { if (!lex_hex(in->tok, in->cur, &u)) return false; goto sfx; }
            bin { if (!lex_bin(in->tok, in->cur, &u)) return false; goto sfx; }  // New binary parsing


            // 定义基本的标记规则
            DIGIT = [0-9];
            HEX_DIGIT = [0-9a-fA-F];

            // 定义十进制浮点数规则
            FRC = DIGIT* "." DIGIT+ | DIGIT+ ".";
            EXP = 'e' [+-]? DIGIT+;
            FLT = ((FRC EXP? | DIGIT+ EXP) [fFdD]?) | (DIGIT+ [fFdD]);

            // 定义十六进制浮点数规则
            HEX_FRC = "0" [xX] HEX_DIGIT+ "." HEX_DIGIT* | "0" [xX] "." HEX_DIGIT+;
            HEX_EXP = ['p', 'P'] [+-]? DIGIT+;
            HEX_FLT = (HEX_FRC HEX_EXP | "0" [xX] HEX_DIGIT+ HEX_EXP) [fFdD]?;
            FLT {
                // 处理十进制浮点数
                if (lex_flt(in->tok,false)) continue; 
                return false;
            }
            HEX_FLT {
                // 处理十六进制浮点数
                if (lex_flt(in->tok,true)) continue;
                return false;
            }

            // boolean literals
            "false" { store_token(FALSE, "false"); continue;  }
            "true"  { store_token(TRUE, "true"); continue;  }

            // keywords
            "abstract"         { store_token(ABSTRACT, "abstract"); continue; }
            "assert"           { store_token(ASSERT, "assert"); continue; }
            "boolean"          { store_token(BOOLEAN, "boolean"); continue; }
            "break"            { store_token(BREAK, "break"); continue; }
            "byte"             { store_token(BYTE, "byte"); continue; }
            "case"             { store_token(CASE, "case"); continue; }
            "catch"            { store_token(CATCH, "catch"); continue; }
            "char"             { store_token(CHAR, "char"); continue; }
            "class"            { store_token(CLASS, "class"); continue; }
            "continue"         { store_token(CONTINUE, "continue"); continue; }
            "default"          { store_token(DEFAULT, "default"); continue; }
            "do"               { store_token(DO, "do"); continue; }
            "double"           { store_token(DOUBLE, "double"); continue; }
            "else"             { store_token(ELSE, "else"); continue; }
            "enum"             { store_token(ENUM, "enum"); continue; }
            "extends"          { store_token(EXTENDS, "extends"); continue; }
            "final"            { store_token(FINAL, "final"); continue; }
            "finally"          { store_token(FINALLY, "finally"); continue; }
            "float"            { store_token(FLOAT, "float"); continue; }
            "for"              { store_token(FOR, "for"); continue; }
            "if"               { store_token(IF, "if"); continue; }
            "implements"       { store_token(IMPLEMENTS, "implements"); continue; }
            "import"           { store_token(IMPORT, "import"); continue; }
            "instanceof"       { store_token(INSTANCEOF, "instanceof"); continue; }
            "int"              { store_token(INT, "int"); continue; }
            "interface"        { store_token(INTERFACE, "interface"); continue; }
            "long"             { store_token(LONG, "long"); continue; }
            "native"           { store_token(NATIVE, "native"); continue; }
            "new"              { store_token(NEW, "new"); continue; }
            "null"             { store_token(MY_NULL, "null"); continue; }
            "package"          { store_token(PACKAGE, "package"); continue; }
            "private"          { store_token(PRIVATE, "private"); continue; }
            "protected"        { store_token(PROTECTED, "protected"); continue; }
            "public"           { store_token(PUBLIC, "public"); continue; }
            "return"{ 
                if (in_generic) {        // 如果还未退出尖括号就出现return，证明前面所有的增加了depth的'<'全部为小于号
                    generic_depth = 0;  // 退出尖括号
                    in_generic = false;  // 重置判断条件
                    store_token(RETURN, "return"); 
                } else {
                    store_token(RETURN, "return");
                }
                continue;
            }
            "short"            { store_token(SHORT, "short"); continue; }
            "static"           { store_token(STATIC, "static"); continue; }
            "strictfp"         { store_token(STRICTFP, "strictfp"); continue; }
            "super"            { store_token(SUPER, "super"); continue; }
            "switch"           { store_token(SWITCH, "switch"); continue; }
            "synchronized"     { store_token(SYNCHRONIZED, "synchronized"); continue; }
            "this"             { store_token(THIS, "this"); continue; }
            "throw"            { store_token(THROW, "throw"); continue; }
            "throws"           { store_token(THROWS, "throws"); continue; }
            "transient"        { store_token(TRANSIENT, "transient"); continue; }
            "try"              { store_token(TRY, "try"); continue; }
            "void"             { store_token(VOID, "void"); continue; }
            "volatile"         { store_token(VOLATILE, "volatile"); continue; }
            "while"            { store_token(WHILE, "while"); continue; }
            "requires"         { store_token(REQUIRES, "requires"); continue; }
            "to"               { store_token(TO, "to"); continue; }
            "with"             { store_token(WITH, "with"); continue; }
            "open"             { store_token(OPEN, "open"); continue; }
            "opens"            { store_token(OPENS, "opens"); continue; }
            "uses"             { store_token(USES, "uses"); continue; }
            "module"           { store_token(MODULE, "module"); continue; }
            "exports"          { store_token(EXPORTS, "exports"); continue; }
            "provides"         { store_token(PROVIDES, "provides"); continue; }
            "transitive"       { store_token(TRANSITIVE, "transitive"); continue; }

            "var"              { store_token(VAR_IdentifierComplement, "var"); continue; }
            // "yield"            { store_token(YIELD, "yield"); continue; }
            "yield" {
                if (in_switch_expr) {
                    store_token(YIELD, "yield");
                } else {
                    store_token(YIELD_IdentifierComplement, "yield");
                }
                continue;
            }

            "sealed"            { store_token(SEALED, "sealed"); continue; }
            "non-sealed"        { store_token(NON_SEALED, "non-sealed"); continue; }
            "permits"           { store_token(PERMITS, "permits"); continue; }

            // operators and punctuation (including preprocessor)
            ","               { store_token(44, ","); continue; }
            "("               { store_token(40, "("); continue; }
            ")"               { store_token(41, ")"); continue; }

            // "{"               { store_token(123, "{"); continue; }
            "{" {
                // 如果不是在switch表达式中，检查是否进入switch表达式
                if (!in_switch_expr) {
                    // 检查前一个token是否是')'
                    if (count > 0 && Index[count-1] == 41) {
                        int paren_index = find_matching_paren(count-1);
                        if (paren_index > 0) {
                            if (Index[paren_index-1] == SWITCH) {
                                // 这是一个switch表达式的大括号
                                in_switch_expr = true;
                                switch_expr_depth = 1; // 初始化深度为1
                            }
                        }
                    }
                } else {
                    // 已经在switch表达式中，增加深度
                    switch_expr_depth++;
                }
                store_token(123, "{");
                continue;
            }

            // "}"               { store_token(125, "}"); continue; }
            "}" {
                if (in_switch_expr) {
                    switch_expr_depth--;
                    if (switch_expr_depth == 0) {
                        in_switch_expr = false; // 退出switch表达式
                    }
                }
                store_token(125, "}");
                continue;
            }

            "["               { store_token(91, "["); continue; }
            "]"               { store_token(93, "]"); continue; }

            ";" { 
                if (in_generic) {        // 如果还未退出尖括号就出现分号“;”，证明前面所有的增加了depth的'<'全部为小于号
                    generic_depth = 0;  // 退出尖括号
                    in_generic = false;  // 重置判断条件
                    store_token(59, ";"); 
                } else {
                    store_token(59, ";"); 
                }
                continue;
            }
            
            "."               { store_token(46, "."); continue; }
            "..."             { store_token(ELLIPSIS, "..."); continue; }
            "@"               {store_token(64, "@");continue; }
            "::"              { store_token(DOUBLE_COLON, "::"); continue; }
            "="               { store_token(61, "="); continue; }

            // 在 < 规则中
            "<" {
                if (prev_token_type == TYPE_IDENTIFIER || 
                    prev_token_type == CLASS || 
                    prev_token_type == INTERFACE) {
                    in_generic = true;
                    generic_depth++;
                }
                store_token(60, "<");
                continue;
            }

            // 在 > 规则中
            ">" {
                if (in_generic) {
                    generic_depth--;
                    if (generic_depth == 0) {
                        in_generic = false;
                    }
                    store_token(62, ">");
                } else {
                    store_token(62, ">");
                }
                continue;
            }

            "!"               { store_token(33, "!"); continue; }
            "~"               { store_token(126, "~"); continue; }
            "?"               { store_token(63, "?"); continue; }
            ":"               { store_token(58, ":"); continue; }
            "->"              { store_token(ARROW, "->"); continue; }
            "<>"              { store_token(DIAMOND, "<>"); continue; }
            "=="              { store_token(EQ, "=="); continue; }

            ">="{
                if (in_generic) {
                    if (generic_depth > 0) generic_depth--; // 第一个 > 减少深度
                    
                    // 存储第一个 >
                    store_token(62, ">");
                    
                    // 手动回退处理第二个 >
                    in->cur--;
                    
                    if (generic_depth == 0) {
                        in_generic = false;
                    }
                    continue;
                } else {
                    store_token(GE, ">="); continue;
                    continue;
                }
            }
            
            "<="              { store_token(LE, "<="); continue; }
            "!="              { store_token(NE, "!="); continue; }
            "&&"              { store_token(AND, "&&"); continue; }
            "||"              { store_token(OR, "||"); continue; }
            "++"              { store_token(INC_OP, "++"); continue; }
            "--"              { store_token(DEC_OP, "--"); continue; }
            "+"               { store_token(43, "+"); continue; }
            "-"               { store_token(45, "-"); continue; }
            "*"               { store_token(42, "*"); continue; }
            "/"               { store_token(47, "/"); continue; }
            "&"               { store_token(38, "&"); continue; }
            "|"               { store_token(124, "|"); continue; }
            "^"               { store_token(94, "^"); continue; }
            "%"               { store_token(37, "%"); continue; }
            "<<"              { store_token(SHL_OP, "<<"); continue; }

            // 在 >> 规则中
            ">>" {
                if (in_generic) {
                    if (generic_depth > 0) generic_depth--; // 第一个 > 减少深度
                    
                    // 存储第一个 >
                    store_token(62, ">");
                    
                    // 手动回退处理第二个 >
                    in->cur--;

                    if (generic_depth == 0) {
                        in_generic = false;
                    }
                    continue;
                } else {
                    store_token(SHR_OP, ">>");
                    continue;
                }
            }

            ">>>"{
                if (in_generic) {
                    if (generic_depth > 0) generic_depth--; // 第一个 > 减少深度
                    
                    // 存储第一个 >
                    store_token(62, ">");

                    // 手动回退处理第二个 >
                    in->cur-=2;
                    
                    // 如果还有嵌套
                    if (generic_depth == 0) {
                        in_generic = false;
                    }
                    continue;
                } else {
                    store_token(USHR_OP, ">>>");
                    continue;
                }
            }

            "+="           { store_token(ADD_ASSIGN, "+="); continue; }
            "-="           { store_token(SUB_ASSIGN, "-="); continue; }
            "*="           { store_token(MUL_ASSIGN, "*="); continue; }
            "/="           { store_token(DIV_ASSIGN, "/="); continue; }
            "&="           { store_token(AND_ASSIGN, "&="); continue; }
            "|="           { store_token(OR_ASSIGN, "|="); continue; }
            "^="           { store_token(XOR_ASSIGN, "^="); continue; }
            "%="           { store_token(MOD_ASSIGN, "%="); continue; }
            "<<="          { store_token(SHL_ASSIGN, "<<="); continue; }

            ">>="{
                if (in_generic) {
                    if (generic_depth > 0) generic_depth--; // 第一个 > 减少深度
                    
                    // 存储第一个 >
                    store_token(62, ">");

                    // 手动回退处理第二个 >
                    in->cur-=2;
                    
                    // 如果还有嵌套
                    if (generic_depth == 0) {
                        in_generic = false;
                    }
                    continue;
                } else {
                    store_token(SHR_ASSIGN, ">>=");
                    continue;
                }
            }

            ">>>="{
                if (in_generic) {
                    if (generic_depth > 0) generic_depth--; // 第一个 > 减少深度
                    
                    // 存储第一个 >
                    store_token(62, ">");

                    // 手动回退处理第二个 >
                    in->cur-=3;
                    
                    // 如果还有嵌套
                    if (generic_depth == 0) {
                        in_generic = false;
                    }
                    continue;
                } else {
                    store_token(USHR_ASSIGN, ">>>=");
                    continue;
                }
            }


            // identifiers
            id = [a-zA-Z_][a-zA-Z_0-9]*;
            id {  handle_id(in->tok, in->cur - in->tok); continue; }
        */

        // 整数字面量后缀处理
        sfx:
            /*!re2c
                ""          { if (u > INT_MAX) return false; 
                            char buf[32]; 
                            snprintf(buf, sizeof(buf), "%d", (int)(u)); 
                            store_token(INTEGERLITERAL, buf); 
                            continue; }

                'u'         { if (u > UINT_MAX) return false; 
                            char buf[32]; 
                            snprintf(buf, sizeof(buf), "%d", (int)(u)); 
                            store_token(INTEGERLITERAL, buf); 
                            continue; }

                'l' | 'L'   { if (u > LONG_MAX) return false; 
                            char buf[32]; 
                            snprintf(buf, sizeof(buf), "%d", (int)(u)); 
                            store_token(INTEGERLITERAL, buf); 
                            continue; }

                'ul' | 'lu' { char buf[32]; 
                            snprintf(buf, sizeof(buf), "%d", (int)(u)); 
                            store_token(INTEGERLITERAL, buf); 
                            continue; }
            */
    }
}
