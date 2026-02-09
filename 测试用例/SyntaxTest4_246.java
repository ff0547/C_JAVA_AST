/**
 * SyntaxTest.java
 * 测试 Java 15 TextBlock 语法特性
 * 需要 Java 15 或更高版本运行
 */
public class SyntaxTest {
    
    public static void main(String[] args) {
        System.out.println("=== Java 15 TextBlock 语法测试 ===\n");
        
        // 1. 基本文本块
        String basicTextBlock = """
                这是一个基本的文本块，
                它可以跨越多行，
                而不需要使用换行符或连接符。
                """;
        System.out.println("1. 基本文本块:");
        System.out.println(basicTextBlock);
        
        // 2. 保留格式的文本块
        String formattedText = """
                {
                    "name": "张三",
                    "age": 25,
                    "hobbies": ["阅读", "编程", "音乐"]
                }
                """;
        System.out.println("2. JSON格式文本块:");
        System.out.println(formattedText);
        
        // 3. 文本块与字符串方法
        String textWithMethods = """
                这是一段文本
                它将被用于测试字符串方法
                """;
        System.out.println("3. 文本块字符串方法测试:");
        System.out.println("长度: " + textWithMethods.length());
        System.out.println("行数: " + textWithMethods.lines().count());
        System.out.println("包含'文本': " + textWithMethods.contains("文本"));
        
        // 4. 文本块中的转义字符
        String textWithEscapes = """
                文本块中可以使用转义字符:
                双引号: \"
                单引号: \'
                反斜杠: \\
                制表符: \t<- 这里有一个制表符
                """;
        System.out.println("4. 转义字符测试:");
        System.out.println(textWithEscapes);
        
        // 5. 文本块格式化
        String name = "李四";
        int score = 95;
        String formattedBlock = """
                学生信息:
                姓名: %s
                分数: %d
                等级: %s
                """.formatted(name, score, score >= 90 ? "优秀" : "良好");
        System.out.println("5. 格式化文本块:");
        System.out.println(formattedBlock);
        
        // 6. 文本块与空白处理
        String textWithSpaces = """
                这一行末尾有空格    \s
                这一行末尾没有空格
                """;
        System.out.println("6. 空白处理测试:");
        System.out.println(textWithSpaces.replace(" ", ".")); // 用点代替空格以便观察
        
        System.out.println("=== 测试完成 ===");
    }
}