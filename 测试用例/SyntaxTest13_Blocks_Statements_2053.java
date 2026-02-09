// SyntaxTest.java
import java.io.*;
import java.util.*;
import java.util.function.*;

public class SyntaxTest {
    // 类级别变量
    private static int staticField = 10;
    private int instanceField = 20;
    protected final String constant = "CONSTANT";
    public static final double PI = 3.14159;
    volatile boolean flag = false;
    transient List<String> transientList = new ArrayList<>();

    // 枚举类型
    enum Color { RED, GREEN, BLUE }

    // 注解类型
    @interface MyAnnotation {
        String value() default "";
        int count() default 0;
    }

    // 嵌套类
    class InnerClass {
        void display() {
            System.out.println("Inner class method");
        }
    }

    // 静态嵌套类
    static class StaticNestedClass {
        void display() {
            System.out.println("Static nested class method");
        }
    }

    // 接口
    interface MyInterface {
        void method();
        default void defaultMethod() {
            System.out.println("Default method");
        }
        static void staticMethod() {
            System.out.println("Static method in interface");
        }
    }

    // 函数式接口实现
    Function<String, Integer> stringToInt = Integer::parseInt;

    public static void main(String[] args) {
        // 局部变量声明（包括 var 和 final）
        int x = 5;
        var y = "Hello";
        final int z = x + 2;
        @SuppressWarnings("unused")
        var list = new ArrayList<String>();
        
        // 空语句
        ;
        
        // 表达式语句
        x++;
        --x;
        y.toString();
        System.out.println(x);
        new Object();
        int[] array = new int[10];
        array[0] = 1;
        
        // 标签语句
        outer:
        for (int i = 0; i < 3; i++) {
            inner:
            while (x < 10) {
                if (i == 2) break outer;
                if (x == 5) continue inner;
                x++;
            }
        }
        
        // if 语句（包括各种形式）
        if (x > 0) {
            System.out.println("Positive");
        }
        
        if (x % 2 == 0) {
            System.out.println("Even");
        } else {
            System.out.println("Odd");
        }
        
        // 嵌套 if-else
        if (x > 10) {
            System.out.println("Greater than 10");
        } else if (x > 5) {
            System.out.println("Greater than 5 but less than or equal to 10");
        } else {
            System.out.println("5 or less");
        }
        
        // while 和 do-while
        while (x > 0) {
            x--;
        }
        
        do {
            x++;
        } while (x < 5);
        
        // 基本 for 循环（多种形式）
        for (int i = 0; i < 10; i++) {
            System.out.println(i);
        }
        
        for (int i = 0, j = 10; i < j; i++, j--) {
            System.out.println(i * j);
        }
        
        for (;;) {
            if (x >= 10) break;
            x++;
        }
        
        // 增强 for 循环
        int[] arr = {1, 2, 3};
        for (var num : arr) {
            System.out.println(num);
        }
        
        List<String> stringList = Arrays.asList("a", "b", "c");
        for (String s : stringList) {
            System.out.println(s);
        }
        
        // switch 语句（传统和规则形式）
        switch (x) {
            case 1:
                System.out.println("One");
                break;
            case 2:
            case 3:
                System.out.println("Two or Three");
                break;
            default:
                System.out.println("Other");
        }
        
        // switch 表达式
        String result = switch (y) {
            case "Hello" -> "Greeting";
            case "World" -> {
                System.out.println("World");
                yield "World";
            }
            default -> throw new IllegalArgumentException("Unexpected value: " + y);
        };
        
        // 多case标签的switch规则
        int day = 3;
        String dayType = switch (day) {
            case 1, 2, 3, 4, 5 -> "Weekday";
            case 6, 7 -> "Weekend";
            default -> "Invalid";
        };
        
        // assert 语句
        assert x >= 0 : "x should be non-negative";
        
        // break/continue/return
        for (int i = 0; i < 10; i++) {
            if (i == 5) break;
            if (i % 2 == 0) continue;
            System.out.println(i);
        }
        
        // yield 在 switch 表达式中的使用
        int value = 2;
        int yielded = switch (value) {
            case 1 -> {
                System.out.println("One");
                yield 1;
            }
            case 2 -> {
                System.out.println("Two");
                yield 2;
            }
            default -> {
                System.out.println("Other");
                yield 0;
            }
        };
        
        // synchronized 语句
        Object lock = new Object();
        synchronized (lock) {
            System.out.println("Synchronized block");
        }
        
        // try-catch-finally 和 try-with-resources
        try {
            System.out.println("Try block");
            throw new RuntimeException("Test exception");
        } catch (RuntimeException e) {
            System.out.println("Caught exception: " + e.getMessage());
        } finally {
            System.out.println("Finally block");
        }
        
        // 多重catch
        try {
            int num = Integer.parseInt("abc");
        } catch (NumberFormatException e) {
            System.out.println("Number format exception");
        } catch (Exception e) {
            System.out.println("General exception");
        }
        
        // 多异常捕获
        try {
            Class.forName("NonExistentClass");
        } catch (ClassNotFoundException | NoClassDefFoundError e) {
            System.out.println("Class loading failed");
        }
        
        // try-with-resources
        try (java.io.StringReader reader = new java.io.StringReader("test");
             java.io.StringWriter writer = new java.io.StringWriter()) {
            int c;
            while ((c = reader.read()) != -1) {
                writer.write(c);
            }
        } catch (java.io.IOException e) {
            e.printStackTrace();
        }
        
        // throw 语句
        if (x < 0) {
            throw new IllegalArgumentException("x cannot be negative");
        }
        
        // 返回语句
        if (x == 0) {
            return;
        }
        
        //  lambda 表达式和方法引用
        List<String> names = Arrays.asList("John", "Jane", "Doe");
        names.forEach(name -> System.out.println(name));
        names.forEach(System.out::println);
        
        // 方法引用
        Function<String, Integer> parser = Integer::parseInt;
        Supplier<List<String>> listSupplier = ArrayList::new;
        
        // 数组操作
        int[] numbers = new int[5];
        numbers[0] = 1;
        int length = numbers.length;
        int[][] matrix = new int[3][3];
        matrix[0][0] = 1;
        
        // 实例化数组的简写形式
        int[] quickArray = {1, 2, 3, 4, 5};
        String[] quickStringArray = {"a", "b", "c"};
        
        // 类型转换
        Object obj = "Hello";
        if (obj instanceof String) {
            String str = (String) obj;
            System.out.println(str.length());
        }
        
        // 模式匹配 instanceof (Java 16+)
        if (obj instanceof String s) {
            System.out.println(s.length());
        }
        
        // 三元运算符
        int max = (x > y.length()) ? x : y.length();
        
        // 文本块 (Java 15+)
        String textBlock = """
            This is a text block
            It spans multiple lines
            """;
        
        // // 记录类 (Java 16+)
        // record Point(int x, int y) {}
        // Point p = new Point(10, 20);
        // System.out.println(p.x());
        
        // 密封类和接口 (Java 17+)
        // sealed interface Shape permits Circle, Rectangle {}
        // final class Circle implements Shape {}
        // non-sealed class Rectangle implements Shape {}
        
        // var与匿名类
        var anonymous = new Object() {
            void method() {
                System.out.println("Anonymous class method");
            }
        };
        anonymous.method();
    }
    
    // 泛型方法
    public <T> T genericMethod(T param) {
        return param;
    }
    
    // 可变参数方法
    public void varargsMethod(String... strings) {
        for (String s : strings) {
            System.out.println(s);
        }
    }
    
    // 重载方法
    public void overloadedMethod(int x) {
        System.out.println("int: " + x);
    }
    
    public void overloadedMethod(String x) {
        System.out.println("String: " + x);
    }
    
    // 递归方法
    public int factorial(int n) {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }
    
    // 静态方法
    public static void staticMethod() {
        System.out.println("Static method");
    }
    
    // 同步方法
    public synchronized void synchronizedMethod() {
        System.out.println("Synchronized method");
    }
    
    // 带有注解的方法
    @MyAnnotation(value = "test", count = 5)
    public void annotatedMethod() {
        System.out.println("Annotated method");
    }
    
    // 带有throws声明的方法
    public void methodWithThrows() throws IOException, IllegalArgumentException {
        throw new IOException("IO error");
    }
    
    // 实例初始化块
    {
        System.out.println("Instance initializer block");
    }
    
    // 静态初始化块
    static {
        System.out.println("Static initializer block");
    }
    
    // 嵌套try-catch
    public void nestedTryCatch() {
        try {
            try {
                throw new IOException("Inner exception");
            } catch (IOException e) {
                System.out.println("Inner catch: " + e.getMessage());
                throw new RuntimeException("Wrapped exception", e);
            }
        } catch (RuntimeException e) {
            System.out.println("Outer catch: " + e.getMessage());
        }
    }
    
    // 带资源的try语句与final变量
    public void tryWithResourcesFinal() {
        final java.io.StringReader reader = new java.io.StringReader("test");
        try (reader) {
            int c = reader.read();
            System.out.println((char) c);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    
    // 带有标记的break和continue
    public void labeledBreakContinue() {
        int[][] matrix = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
        search:
        for (int i = 0; i < matrix.length; i++) {
            for (int j = 0; j < matrix[i].length; j++) {
                if (matrix[i][j] == 5) {
                    System.out.println("Found at " + i + "," + j);
                    break search;
                }
            }
        }
    }
    
    // 断言的使用
    public void assertUsage(int x) {
        assert x >= 0 : "x must be non-negative";
        System.out.println("x is " + x);
    }
    
    // 传统switch与yield
    public int traditionalSwitchWithYield(int value) {
        return switch (value) {
            case 1:
                yield 10;
            case 2:
                yield 20;
            default:
                yield 0;
        };
    }
    
    // 带有资源的try语句和effectively final变量
    public void tryWithResourcesEffectivelyFinal() {
        java.io.StringReader reader = new java.io.StringReader("test");
        try (reader) {
            int c = reader.read();
            System.out.println((char) c);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    
    // 匿名类
    public void anonymousClassExample() {
        Runnable r = new Runnable() {
            @Override
            public void run() {
                System.out.println("Anonymous class running");
            }
        };
        r.run();
    }
    
    // 局部类
    public void localClassExample() {
        class LocalClass {
            void display() {
                System.out.println("Local class method");
            }
        }
        LocalClass lc = new LocalClass();
        lc.display();
    }
}