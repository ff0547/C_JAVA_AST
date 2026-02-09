// SyntaxTest.java
import java.util.*;
import java.util.function.*;
import java.lang.annotation.*;

@Retention(RetentionPolicy.RUNTIME)
@interface MyAnnotation {
    String value() default "";
}

public class SyntaxTest<T extends Comparable<T>> {
    // ========== 字面量测试 ==========
    private int intLiteral = 42;
    private long longLiteral = 1234567890L;
    private float floatLiteral = 3.14f;
    private double doubleLiteral = 2.71828;
    private boolean boolLiteral = true;
    private char charLiteral = 'A';
    private String stringLiteral = "Hello, World!";
    private Object nullLiteral = null;
    
    // ========== 类字面量测试 ==========
    private Class<?> classLiteral1 = String.class;
    private Class<?> classLiteral2 = int[].class;
    private Class<?> classLiteral3 = int[][].class;
    private Class<?> classLiteral4 = boolean.class;
    private Class<?> classLiteral5 = void.class;
    private Class<?> classLiteral6 = SyntaxTest.class;
    
    // ========== this 关键字测试 ==========
    private int value = 100;
    
    public SyntaxTest() {
        this(50); // 调用另一个构造函数
    }
    
    public SyntaxTest(int value) {
        this.value = value;
    }
    
    public void testThis() {
        System.out.println("this.value: " + this.value);
        System.out.println("this.toString(): " + this.toString());
    }
    
    // ========== TypeName.this 测试 ==========
    class InnerClass {
        public SyntaxTest getOuter() {
            return SyntaxTest.this;
        }
        
        public void testInnerThis() {
            System.out.println("InnerClass.this: " + InnerClass.this.toString());
            System.out.println("SyntaxTest.this: " + SyntaxTest.this.toString());
        }
    }
    
    // ========== 类实例创建表达式测试 ==========
    public void testClassInstanceCreation() {
        // 基本实例创建
        Object obj1 = new Object();
        String str1 = new String("Test");
        
        // 带泛型的实例创建
        List<String> list1 = new ArrayList<>();
        List<String> list2 = new ArrayList<String>();
        Map<String, Integer> map = new HashMap<String, Integer>();
        
        // 内部类实例创建
        InnerClass inner = new InnerClass();
        SyntaxTest.InnerClass inner2 = this.new InnerClass();
        
        // 匿名类
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                System.out.println("Anonymous class");
            }
        };
        
        // 带注解的实例创建
        @MyAnnotation("test")
        String annotatedStr = new String("Annotated");
    }
    
    // ========== 数组创建表达式测试 ==========
    public void testArrayCreation() {
        // 基本类型数组
        int[] intArray1 = new int[10];
        int[] intArray2 = new int[]{1, 2, 3, 4, 5};
        int[] intArray3 = {1, 2, 3, 4, 5};
        
        // 多维数组
        int[][] multiDimArray1 = new int[5][10];
        int[][] multiDimArray2 = new int[5][];
        int[][] multiDimArray3 = {{1, 2}, {3, 4}, {5, 6}};
        
        // 对象数组
        String[] strArray1 = new String[5];
        String[] strArray2 = new String[]{"a", "b", "c"};
        String[] strArray3 = {"a", "b", "c"};
        
        // 带注解的数组创建
        @MyAnnotation("array")
        int[] annotatedArray = new int[10];
    }
    
    // ========== 字段访问测试 ==========
    public static int staticField = 100;
    public int instanceField = 200;
    
    public void testFieldAccess() {
        // 访问实例字段
        int value1 = this.instanceField;
        int value2 = instanceField;
        
        // 访问静态字段
        int value3 = SyntaxTest.staticField;
        int value4 = staticField;
        
        // 访问父类字段
        String str = super.toString();
        
        // 通过表达式访问字段
        SyntaxTest obj = new SyntaxTest();
        int value5 = obj.instanceField;
    }
    
    // ========== 方法调用测试 ==========
    public void testMethodInvocation() {
        // 简单方法调用
        this.testFieldAccess();
        testFieldAccess();
        
        // 静态方法调用
        Math.max(10, 20);
        
        // 带泛型参数的方法调用
        List<String> list = new ArrayList<>();
        list.add("test");
        list.<String>add("generic");
        
        // 方法链式调用
        StringBuilder sb = new StringBuilder();
        sb.append("Hello").append(" ").append("World");
        
        // super方法调用
        super.toString();
    }
    
    // ========== 方法引用测试 ==========
    public void testMethodReference() {
        // 静态方法引用
        Function<String, Integer> parser = Integer::parseInt;
        
        // 实例方法引用
        Consumer<String> printer = System.out::println;
        
        // 特定对象的实例方法引用
        String str = "Hello";
        Supplier<Integer> lengthSupplier = str::length;
        
        // 构造方法引用
        Supplier<List<String>> listSupplier = ArrayList::new;
        Function<Integer, int[]> arrayCreator = int[]::new;
        
        // 超类方法引用
        class SubClass extends SyntaxTest {
            void testSuperRef() {
                Function<String, String> func = super::toString;
            }
        }
    }
    
    // ========== Lambda表达式测试 ==========
    public void testLambda() {
        // 基本Lambda表达式
        Function<String, Integer> func1 = (String s) -> s.length();
        
        // 类型推断
        Function<String, Integer> func2 = s -> s.length();
        
        // 多参数Lambda
        BiFunction<Integer, Integer, Integer> adder = (a, b) -> a + b;
        
        // 代码块Lambda
        Function<String, Integer> func3 = s -> {
            int length = s.length();
            return length * 2;
        };
        
        // 方法引用与Lambda混合
        Function<String, Integer> func4 = String::length;
        
        // 作为参数传递
        List<String> list = Arrays.asList("a", "bb", "ccc");
        list.stream()
            .filter(s -> s.length() > 1)
            .map(String::toUpperCase)
            .forEach(System.out::println);
    }
    
    // ========== 赋值表达式测试 ==========
    public void testAssignment() {
        // 简单赋值
        int x = 10;
        String s = "Hello";
        
        // 复合赋值
        x += 5;
        x -= 3;
        x *= 2;
        x /= 4;
        x %= 3;
        
        // 位运算复合赋值
        int y = 0b1010;
        y &= 0b1100;
        y |= 0b1010;
        y ^= 0b1001;
        y <<= 2;
        y >>= 1;
        y >>>= 1;
        
        // 数组元素赋值
        int[] arr = new int[5];
        arr[0] = 10;
        arr[1] += 5;
        
        // 字段赋值
        this.instanceField = 300;
        SyntaxTest.staticField = 200;
    }
    
    // ========== 条件表达式测试 ==========
    public void testConditional() {
        int x = 10;
        int y = 20;
        
        // 条件与
        boolean b1 = x > 0 && y > 0;
        
        // 条件或
        boolean b2 = x > 0 || y > 0;
        
        // 三元运算符
        int max = x > y ? x : y;
        String result = x > 5 ? "Greater" : "Less or equal";
        
        // 嵌套三元运算符
        String grade = x > 90 ? "A" : x > 80 ? "B" : x > 70 ? "C" : "F";
        
        // 三元运算符与Lambda表达式
        Supplier<Integer> supplier = x > 5 ? () -> x : () -> y;
    }
    
    // ========== instanceof表达式测试 ==========
    public void testInstanceof() {
        Object obj = "test";
        
        // 基本instanceof
        boolean isString = obj instanceof String;
        
        // 接口instanceof
        boolean isSerializable = obj instanceof java.io.Serializable;
        
        // 泛型instanceof (擦除后)
        List<String> list = new ArrayList<>();
        boolean isList = list instanceof List;
        
        // 数组instanceof
        int[] arr = new int[10];
        boolean isArray = arr instanceof int[];
    }
    
    // ========== switch表达式测试 ==========
    public void testSwitchExpression() {
        int day = 3;
        
        // 传统switch语句
        switch (day) {
            case 1:
                System.out.println("Monday");
                break;
            case 2:
                System.out.println("Tuesday");
                break;
            default:
                System.out.println("Other day");
        }
        
        // switch表达式 (Java 14+)
        String dayType = switch (day) {
            case 1, 2, 3, 4, 5 -> "Weekday";
            case 6, 7 -> "Weekend";
            default -> {
                System.out.println("Invalid day");
                yield "Unknown";
            }
        };
        
        // 带返回值的switch表达式
        int daysInMonth = switch (day) {
            case 1, 3, 5, 7, 8, 10, 12 -> 31;
            case 4, 6, 9, 11 -> 30;
            case 2 -> 28;
            default -> throw new IllegalArgumentException("Invalid month: " + day);
        };
    }
    
    // ========== 类型转换表达式测试 ==========
    public void testCast() {
        Object obj = "test";
        
        // 基本类型转换
        double d = 3.14;
        int i = (int) d;
        
        // 引用类型转换
        String s = (String) obj;
        
        // 泛型类型转换 (擦除后)
        List<?> list = new ArrayList<String>();
        List<String> strList = (List<String>) list;
        
        // Lambda表达式转换
        Function<String, Integer> func = (Function<String, Integer>) s -> s.length();
    }
    
    // ========== 一元表达式测试 ==========
    public void testUnary() {
        int x = 10;
        
        // 前缀递增/递减
        ++x;
        --x;
        
        // 后缀递增/递减
        x++;
        x--;
        
        // 一元加/减
        int y = +x;
        int z = -x;
        
        // 逻辑非
        boolean b = true;
        boolean notB = !b;
        
        // 按位取反
        int bits = ~x;
        
        // 类型转换表达式
        double d = (double) x;
    }
    
    // ========== 二元表达式测试 ==========
    public void testBinary() {
        int a = 10, b = 5;
        
        // 算术运算
        int sum = a + b;
        int difference = a - b;
        int product = a * b;
        int quotient = a / b;
        int remainder = a % b;
        
        // 关系运算
        boolean isEqual = a == b;
        boolean notEqual = a != b;
        boolean greater = a > b;
        boolean less = a < b;
        boolean greaterOrEqual = a >= b;
        boolean lessOrEqual = a <= b;
        
        // 逻辑运算
        boolean and = (a > 0) && (b > 0);
        boolean or = (a > 0) || (b > 0);
        
        // 位运算
        int bitwiseAnd = a & b;
        int bitwiseOr = a | b;
        int bitwiseXor = a ^ b;
        int leftShift = a << 2;
        int rightShift = a >> 2;
        int unsignedRightShift = a >>> 2;
        
        // 复杂表达式
        int complex = (a + b) * (a - b) / (a % 3);
    }
    
    // ========== 复杂表达式测试 ==========
    public void testComplexExpressions() {
        // 嵌套方法调用
        int length = "test".substring(0, 2).toUpperCase().length();
        
        // 混合运算
        int result = (10 + 5) * 2 / (3 % 2) + (1 << 3);
        
        // 条件表达式与赋值混合
        int x = 10;
        int y = x > 5 ? x * 2 : x / 2;
        
        // 数组访问与方法调用混合
        String[] arr = {"a", "bb", "ccc"};
        int len = arr[1].toUpperCase().length();
        
        // Lambda表达式与方法引用混合
        Function<String, Integer> func = s -> Optional.ofNullable(s)
                                                     .map(String::length)
                                                     .orElse(0);
    }
    
    // ========== 主方法 ==========
    public static void main(String[] args) {
        SyntaxTest test = new SyntaxTest();
        
        // 运行所有测试方法
        test.testThis();
        
        InnerClass inner = test.new InnerClass();
        inner.testInnerThis();
        
        test.testClassInstanceCreation();
        test.testArrayCreation();
        test.testFieldAccess();
        test.testMethodInvocation();
        test.testMethodReference();
        test.testLambda();
        test.testAssignment();
        test.testConditional();
        test.testInstanceof();
        test.testSwitchExpression();
        test.testCast();
        test.testUnary();
        test.testBinary();
        test.testComplexExpressions();
        
        System.out.println("All tests completed successfully!");
    }
    
    // ========== 辅助方法 ==========
    @Override
    public String toString() {
        return "SyntaxTest{value=" + value + "}";
    }
    
    // 泛型方法测试
    public <U extends Comparable<U>> U genericMethod(U param) {
        return param;
    }
    
    // 可变参数测试
    public void varargsMethod(String... args) {
        for (String arg : args) {
            System.out.println(arg);
        }
    }
    
    // 注解测试
    @MyAnnotation("testMethod")
    public void annotatedMethod() {
        System.out.println("Annotated method");
    }
    
    // synchronized方法测试
    public synchronized void synchronizedMethod() {
        System.out.println("Synchronized method");
    }
}