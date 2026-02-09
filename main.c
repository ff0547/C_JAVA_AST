#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared.h"
#include "java_ast.h"
#include "parser.tab.h"
extern int yyparse(void);
extern AstNode *root_ast;

// 语法错误回调：输出错误位置与信息。
// void yyerror(const char *s) {
//     fprintf(stderr, "Parse error: %s\n", s);
// }
void yyerror(const char *s) {
    extern YYLTYPE yylloc;
    fprintf(stderr, ">>> 语法错误发生于 第 %d 行, 第 %d 列: %s <<<\n", 
            yylloc.first_line, yylloc.first_column, s);
}
int main(int argc, char *argv[]) {
    // 打开日志文件进行调试
    FILE *log = fopen("parser_log.txt", "w");
    if (log) {
        fprintf(log, "=== Java Parser 开始运行 ===\n");
        fprintf(log, "参数数量: %d\n", argc);
        fflush(log);
    }
    
    printf("Program started with %d arguments\n", argc);
    fflush(stdout);
    if (argc < 2) {
        if (log) {
            fprintf(log, "错误: 缺少输入文件参数\n");
            fclose(log);
        }
        fprintf(stderr, "Usage: %s <java_file>\n", argv[0]);
        return 1;
    }

    printf("Input file: %s\n", argv[1]);
    fflush(stdout);
    if (log) {
        fprintf(log, "输入文件: %s\n", argv[1]);
        fflush(log);
    }

    // 设置输入文件供词法/语法分析读取。
    set_input_file(argv[1]);
    printf("Input file set\n");
    fflush(stdout);
    if (log) {
        fprintf(log, "已设置输入文件\n");
        fflush(log);
    }

    // 初始化并执行词法分析
    printf("Starting lexical analysis for: %s\n", argv[1]);
    fflush(stdout);
    if (log) {
        fprintf(log, "开始词法分析...\n");
        fflush(log);
    }
    
    if (init_token_list() == 0) {
        if (log) {
            fprintf(log, "错误: 词法分析初始化失败\n");
            fclose(log);
        }
        fprintf(stderr, "Failed to initialize token list\n");
        return 1;
    }

    printf("Lexical analysis completed. Tokens found: %d\n", count);
    fflush(stdout);
    if (log) {
        fprintf(log, "词法分析完成，共找到 %d 个 token\n", count);
        fflush(log);
    }
    
    if (count > 0) {
        printTokens();
        if (log) {
            fprintf(log, "Token 列表已打印\n");
            fflush(log);
        }
    } else {
        printf("No tokens found!\n");
        fflush(stdout);
        if (log) {
            fprintf(log, "警告: 没有找到任何 token\n");
            fflush(log);
        }
    }

    // 语法分析与 AST 构建。
    printf("\nStarting syntax analysis and AST construction...\n");
    fflush(stdout);
    if (log) {
        fprintf(log, "\n开始语法分析和 AST 构建...\n");
        fflush(log);
    }
    
    int parse_result = yyparse();
    printf("Parse result: %d\n", parse_result);
    fflush(stdout);
    if (log) {
        fprintf(log, "语法分析结果: %d\n", parse_result);
        fflush(log);
    }

    if (parse_result == 0 && root_ast) {
        printf("Parse successful! AST constructed.\n\n");
        printf("AST Structure:\n");
        
        if (log) {
            fprintf(log, "解析成功！AST 已构建\n");
            fprintf(log, "\n=== AST 结构 ===\n");
            ast_print(root_ast, log, 0);
            fprintf(log, "\n=== AST 结构结束 ===\n");
            fflush(log);
        }
        
        ast_print(root_ast, stdout, 0);

        // 清理 AST。
        ast_free(root_ast);
        root_ast = NULL;
        
        if (log) {
            fprintf(log, "AST 已释放\n");
            fflush(log);
        }
    } else {
        printf("Parse failed!\n");
        if (log) {
            fprintf(log, "解析失败！\n");
            if (!root_ast) {
                fprintf(log, "警告: root_ast 为 NULL\n");
            }
            fflush(log);
        }
    }

    // 清理资源。
    freeTokens();
    printf("Program finished\n");
    
    if (log) {
        fprintf(log, "\n=== Java Parser 运行结束 ===\n");
        fclose(log);
    }

    return parse_result;
}
