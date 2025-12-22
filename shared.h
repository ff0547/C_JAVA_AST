// shared.h
#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdbool.h>

// Forward declaration for AST
typedef struct AstNode AstNode;

#define MAX 3500  // MAX 是可以容纳的最大token数

// #define SIZE (64 * 1024)

// 定义默认YYMAXFILL值（re2c会覆盖它）（好像不会。）
// #ifndef YYMAXFILL
// #define YYMAXFILL 8
// #endif

// 声明但不定义变量
extern int Index[MAX];
extern char* Token[MAX];
extern int TokenLine[MAX];
extern int count;
// extern int current_token_line;   

// 结构体声明
struct input_t {
    // unsigned char buf[64 * 1024 + YYMAXFILL];
    unsigned char buf[64 * 1024 + 13];
    unsigned char *lim;
    unsigned char *cur;
    unsigned char *mar;
    unsigned char *tok;
    bool eof;
    FILE * file;
};
// struct input_t;

// TokenStream 结构（用于上下文敏感转换）
// typedef struct {
//     int *index;
//     char **tokens;
//     int count;
//     int current_pos;
// } TokenStream;

// TokenStream* create_stream(int *index, char **tokens, int count);
// void destroy_stream(TokenStream *ts);

// 函数声明
void store_token(int index, const char* value);
void handle_id(const unsigned char *id_start, size_t id_length);
void printTokens(void);
void freeTokens(void);
bool lex(struct input_t *in);
void init_input(struct input_t* in, FILE* f);
bool fill(struct input_t* in, size_t need);
extern int init_token_list(void);
void set_input_file(const char *filename);

#endif
