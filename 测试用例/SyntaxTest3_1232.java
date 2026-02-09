// SyntaxTest.java - JDK 15 语法测试样本 (不含Sealed Classes和Record)
package com.example;

import java.util.*;
import java.util.stream.*;

import com.example.SyntaxTest.Point;

import java.io.*;
import java.text.*;
import java.time.*;
import java.time.format.*;

/**
 * Java 15 语法特性测试 (不含Sealed Classes和Record)
 */
public final class SyntaxTest {
    // 1. 文本块 (JDK 15)
    private static final String JSON_BLOCK = """
        {
            "name": "Java 15",
            "features": [
                "Text Blocks",
                "Pattern Matching",
                "Switch Expressions"
            ]
        }""";

    // 多行文本块
    String textBlock = """
        这是一个文本块示例，
        可以跨越多行，
        无需使用换行符或连接符。
        """;

    // 文本块内使用格式化
    String name = "Java";
    String version = "17";
    String message = """
        Hello, %s!
        Version: %s
        """.formatted(name, version);

    // 模拟 XML 数据
    String xml = """
        <root>
            <element>内容</element>
        </root>
        """;

    // 使用缩进方法调整文本块
    String text = """
            {
                "key": "value"
            }
        """.indent(2); // 整体缩进 2 空格

    // 检查文本块是否包含特定内容
    String code = """
        public void test() {
            System.out.println("Hello");
        }
        """;
    
    // 2. 传统类替代记录类
    public static final class Point {
        private final int x;
        private final int y;
        
        public Point(int x, int y) {
            this.x = x;
            this.y = y;
        }
        
        public int x() { return x; }
        public int y() { return y; }
        
        public double distanceFromOrigin() {
            return Math.sqrt(x * x + y * y);
        }
        
        @Override
        public String toString() {
            return "Point[x=" + x + ", y=" + y + "]";
        }
    }
    
    // 3. 传统接口和实现替代密封接口
    public interface Shape {
        double area();
    }
    
    public static class Circle implements Shape {
        private final double radius;
        
        public Circle(double radius) {
            this.radius = radius;
        }
        
        public double radius() { return radius; }
        
        @Override
        public double area() {
            return Math.PI * radius * radius;
        }
    }
    
    public static class Rectangle implements Shape {
        private final double width;
        private final double height;
        
        public Rectangle(double width, double height) {
            this.width = width;
            this.height = height;
        }
        
        public double width() { return width; }
        public double height() { return height; }
        
        @Override
        public double area() {
            return width * height;
        }
    }
    
    // 4. 模式匹配 instanceof (JDK 15 预览)
    private static String getShapeInfo(Object shape) {
        if (shape instanceof Circle c) {
            return "Circle with radius: " + c.radius();
        } else if (shape instanceof Rectangle r) {
            return "Rectangle area: " + r.area();
        } else if (shape instanceof Point p) {
            return "Point at (" + p.x() + ", " + p.y() + ")";
        }
        return "Unknown shape";
    }
    
    // 5. switch表达式 (JDK 14)
    private static String getDayType(Day day) {
        return switch (day) {
            case MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY -> "Weekday";
            case SATURDAY, SUNDAY -> "Weekend";
        };
    }
    
    // 6. 多行注释
    /*
     * 这是一个传统的多行注释
     * 用于测试注释语法
     */
    
    // 7. 新的日期API
    private static void printCurrentDate() {
        var now = LocalDateTime.now();
        var formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");
        System.out.println("Current time: " + formatter.format(now));
    }
    
    // 8. var类型推断
    private static void varDemo() {
        var list = new ArrayList<String>();
        list.add("Java");
        list.add("15");
        
        var stream = list.stream()
            .map(String::toUpperCase)
            .collect(Collectors.joining(" "));
        
        System.out.println("Var demo: " + stream);
    }
    
    // 9. yield在switch中的使用
    private static int calculate(String operation, int a, int b) {
        return switch (operation) {
            case "+" -> a + b;
            case "-" -> a - b;
            case "*" -> a * b;
            case "/" -> {
                if (b == 0) throw new ArithmeticException("Division by zero");
                yield a / b;
            }
            default -> throw new IllegalArgumentException("Unknown operation");
        };
    }
    
    // 10. 接口私有方法
    public interface Logger {
        default void log(String message) {
            logMessage(format(message));
        }
        
        private String format(String message) {
            return "[" + new Date() + "] " + message;
        }
        
        private void logMessage(String formatted) {
            System.out.println(formatted);
        }
    }
    
    // 11. 增强的NullPointerException
    private static void triggerNPE() {
        String[] words = null;
        try {
            // 故意触发NPE
            System.out.println(words[0].toUpperCase());
        } catch (NullPointerException e) {
            System.out.println("NPE caught with enhanced message:");
            e.printStackTrace();
        }
    }
    
    // 12. 枚举类
    enum Day {
        MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY
    }
    
    // 13. 嵌套类
    static class NestedClass {
        void printMessage() {
            System.out.println("Message from nested class");
        }
    }
    
    // 14. 泛型方法
    public static <T> T firstElement(List<T> list) {
        if (list.isEmpty()) {
            throw new NoSuchElementException("List is empty");
        }
        return list.get(0);
    }
    
    // 15. try-with-resources
    private static void writeToFile() throws IOException {
        try (var writer = new FileWriter("output.txt")) {
            writer.write("Java 15 syntax test completed");
            System.out.println("File written successfully");
        }
    }
    
    // 16. 主方法
    public static void main(String[] args) throws IOException {
        System.out.println("=== JDK 15 Syntax Test (without Records/Sealed Classes) ===");
        
        // 文本块输出
        System.out.println("JSON Block:\n" + JSON_BLOCK);
        
        // Point类使用
        Point p = new Point(3, 4);
        System.out.println("\nPoint demo: " + p + ", Distance: " + p.distanceFromOrigin());
        
        // Shape实现演示
        Shape circle = new Circle(5.0);
        Shape rect = new Rectangle(4, 6);
        System.out.println("\nShape implementations demo:");
        System.out.println("Circle area: " + circle.area());
        System.out.println("Rectangle area: " + rect.area());
        
        // 模式匹配instanceof
        System.out.println("\nPattern matching demo:");
        System.out.println(getShapeInfo(circle));
        System.out.println(getShapeInfo(rect));
        System.out.println(getShapeInfo(p));
        
        // switch表达式
        System.out.println("\nSwitch expression demo:");
        System.out.println("FRIDAY is a " + getDayType(Day.FRIDAY));
        System.out.println("SUNDAY is a " + getDayType(Day.SUNDAY));
        
        // var类型推断
        varDemo();
        
        // yield在switch中的使用
        System.out.println("\nYield demo:");
        System.out.println("10 / 2 = " + calculate("/", 10, 2));
        
        // 接口私有方法
        Logger logger = new Logger() {};
        logger.log("Testing private interface method");
        
        // 日期API
        printCurrentDate();
        
        // 增强的NPE
        System.out.println("\nTesting enhanced NullPointerException:");
        triggerNPE();
        
        // 嵌套类
        NestedClass nested = new NestedClass();
        nested.printMessage();
        
        // 泛型方法
        List<String> names = List.of("Alice", "Bob", "Charlie");
        System.out.println("\nGeneric method demo: " + firstElement(names));
        
        // try-with-resources
        writeToFile();
        
        System.out.println("\n=== Test Completed ===");
    }
}
