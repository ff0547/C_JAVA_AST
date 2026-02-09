/**
 * SyntaxTest.java - Java 15 语法特性测试用例
 * 包含文本块、密封类、模式匹配 instanceof 等 Java 15 特性
 */
public class SyntaxTest {

    // 文本块 (Text Blocks) - Java 15 正式特性
    private static final String JSON_BLOCK = """
        {
            "name": "Java 15 Syntax Test",
            "version": "15",
            "features": [
                "Text Blocks",
                "Sealed Classes",
                "Pattern Matching for instanceof"
            ]
        }
        """;

    // 密封类 (Sealed Classes) - Java 15 预览特性
    public sealed interface Shape permits Circle, Rectangle, Triangle {
    // public sealed interface Shape {
        double area();
    }

    public static final class Circle implements Shape {
        private final double radius;
        
        public Circle(double radius) {
            this.radius = radius;
        }
        
        @Override
        public double area() {
            return Math.PI * radius * radius;
        }
    }

    public static final class Rectangle implements Shape {
        private final double width;
        private final double height;
        
        public Rectangle(double width, double height) {
            this.width = width;
            this.height = height;
        }
        
        @Override
        public double area() {
            return width * height;
        }
    }

    public static non-sealed class Triangle implements Shape {
        private final double base;
        private final double height;
        
        public Triangle(double base, double height) {
            this.base = base;
            this.height = height;
        }
        
        @Override
        public double area() {
            return 0.5 * base * height;
        }
    }

    // 模式匹配 instanceof (Pattern Matching for instanceof) - Java 15 预览特性
    public static String describeObject(Object obj) {
        if (obj instanceof String s) {
            return "字符串长度为: " + s.length();
        } else if (obj instanceof Integer i) {
            return "整数值为: " + i;
        } else if (obj instanceof Circle c) {
            return "圆形，半径: " + c.radius + ", 面积: " + c.area();
        } else {
            return "未知对象: " + obj.getClass().getName();
        }
    }

    public static void main(String[] args) {
        System.out.println("=== Java 15 语法测试 ===");
        
        // 测试文本块
        System.out.println("JSON 文本块:");
        System.out.println(JSON_BLOCK);
        
        // 测试密封类
        Shape circle = new Circle(5.0);
        Shape rectangle = new Rectangle(4.0, 6.0);
        Shape triangle = new Triangle(3.0, 4.0);
        
        System.out.println("圆形面积: " + circle.area());
        System.out.println("矩形面积: " + rectangle.area());
        System.out.println("三角形面积: " + triangle.area());
        
        // 测试模式匹配 instanceof
        System.out.println(describeObject("Hello Java 15"));
        System.out.println(describeObject(42));
        System.out.println(describeObject(circle));
        
        System.out.println("=== 测试完成 ===");
    }
}