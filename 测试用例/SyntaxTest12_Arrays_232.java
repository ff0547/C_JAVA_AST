// SyntaxTest.java
public class SyntaxTest {
    // 测试用例
    int[] empty = {};                          // 空初始化
    //int[] withTrailingComma = {,};             // 仅尾随逗号（无效语法，注释掉）
    int[] singleElement = {1};                 // 单元素无尾随逗号
    int[] singleWithComma = {1,};              // 单元素带尾随逗号
    int[] multipleElements = {1, 2, 3};        // 多元素无尾随逗号
    int[] multipleWithComma = {1, 2, 3,};      // 多元素带尾随逗号

    // 嵌套数组初始化
    int[][] nestedEmpty = {{}};                // 嵌套空数组
    //int[][] nestedWithComma = {{,}};           // 嵌套仅逗号（无效语法，注释掉）
    int[][] nestedArray = {{1, 2}, {3, 4}};    // 嵌套多元素
    int[][] nestedTrailing = {{1,}, {2,},};    // 嵌套带尾随逗号

    // 多维数组
    int[][][] multiDim = {
        {{1, 2}, {3, 4}},
        {{5, 6}, {7, 8}},
    };

    // 混合类型（需保持类型兼容）
    Object[] mixedTypes = {1, "text", 3.14,};  // 对象数组支持混合类型

    // 表达式初始化
    int[] withExpression = {1 + 2, 3 * 4, Math.abs(-5)};

    public static void main(String[] args) {
        System.out.println("Array Initialization Syntax Test");
    }
}