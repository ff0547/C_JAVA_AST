// SyntaxTest.java
import java.lang.annotation.*;
import java.util.*;

// 测试注解定义
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.TYPE, ElementType.FIELD, ElementType.METHOD, ElementType.PARAMETER})
@interface TestAnnotation {
    String value() default "test";
}

// 1. 普通类声明
public class SyntaxTest<T extends Number> extends ArrayList<T> implements Serializable, Cloneable {
    
    // 字段声明测试
    // 各种修饰符组合
    public static final int CONSTANT = 100;
    @TestAnnotation protected volatile transient String field1;
    private static final long serialVersionUID = 1L;
    transient volatile int[] arrayField;
    @TestAnnotation("custom") private final List<String> list = new ArrayList<>();
    
    // 实例初始化块
    {
        field1 = "initialized";
        arrayField = new int[10];
    }
    
    // 静态初始化块
    static {
        System.out.println("Class initialized");
    }
    
    // 构造函数测试
    public SyntaxTest() {
        super();
    }
    
    @TestAnnotation
    protected SyntaxTest(T element) {
        this.add(element);
    }
    
    // 方法声明测试
    public <E extends Comparable<E>> E genericMethod(E param, List<? super E> list) throws IllegalArgumentException, NullPointerException {
        list.add(param);
        return param;
    }
    
    // 可变参数方法
    public static final void varargsMethod(String... args) {
        for (String arg : args) {
            System.out.println(arg);
        }
    }
    
    // 同步方法
    public synchronized void synchronizedMethod() {
        // 方法体
    }
    
    // 本地方法
    public native void nativeMethod();
    
    // 抽象方法（需要在抽象类中）
    // abstract void abstractMethod();
    
    // 接收器参数测试
    public void methodWithReceiver(@TestAnnotation SyntaxTest this) {
        System.out.println("Receiver method");
    }
    
    // 内部类测试
    private class InnerClass {
        private int innerField;
        
        public InnerClass(int value) {
            this.innerField = value;
        }
    }
    
    // 静态嵌套类
    public static class NestedClass {
        public void nestedMethod() {
            System.out.println("Nested method");
        }
    }
    
    // 空声明测试
    ;
}

// 2. 枚举声明
enum TestEnum implements Serializable {
    
    // 枚举常量测试
    FIRST(1) {
        @Override
        public void customMethod() {
            System.out.println("First constant");
        }
    },
    @TestAnnotation SECOND(2),
    THIRD(3) {
        public void customMethod() {
            System.out.println("Third constant");
        }
    };
    
    // 枚举字段
    private final int value;
    
    // 枚举构造函数
    private TestEnum(int value) {
        this.value = value;
    }
    
    // 枚举方法
    public int getValue() {
        return value;
    }
    
    public void customMethod() {
        System.out.println("Default implementation");
    }
    
    // 枚举体声明
    private static final int ENUM_CONSTANT = 100;
    
    public static void enumStaticMethod() {
        System.out.println("Static method in enum");
    }
}

// 3. 接口声明测试
interface TestInterface<T> {
    // 接口常量
    String INTERFACE_CONSTANT = "constant";
    
    // 接口方法
    void interfaceMethod(T param);
    
    // 默认方法
    default void defaultMethod() {
        System.out.println("Default method");
    }
    
    // 静态方法
    static void staticInterfaceMethod() {
        System.out.println("Static interface method");
    }
}

// 4. 注解声明测试
@interface CustomAnnotation {
    String value();
    int number() default 42;
    Class<?> type() default Object.class;
}

// 5. 复杂类型参数测试
class TypeParameterTest<
    T extends Number & Serializable,
    U extends Comparable<U> & Cloneable,
    V extends List<T> & RandomAccess> {
    
    // 通配符测试
    public void wildcardTest(List<? extends Number> list1, List<? super Integer> list2) {
        // 方法体
    }
    
    // 嵌套类型参数
    public <S extends Map<U, V>> S complexTypeParameter(S map) {
        return map;
    }
}

// 6. 数组类型测试
class ArrayTest {
    private int[][][] multiArray;
    private String[] stringArray;
    
    public ArrayTest() {
        multiArray = new int[10][][];
        stringArray = new String[]{"one", "two", "three"};
    }
    
    // 可变参数与数组
    public void acceptArrays(int[] intArray, String... varargs) {
        System.out.println(Arrays.toString(intArray));
        System.out.println(Arrays.toString(varargs));
    }
}

// 7. 异常处理测试
class ExceptionTest {
    public void testExceptions() throws IOException, RuntimeException {
        try {
            throw new IOException("Test exception");
        } catch (IOException | SecurityException e) {
            System.out.println("Caught exception: " + e.getMessage());
        } finally {
            System.out.println("Finally block");
        }
    }
}

// 8. 匿名类和lambda测试
class AnonymousTest {
    private Runnable anonymousField = new Runnable() {
        @Override
        public void run() {
            System.out.println("Anonymous class");
        }
    };
    
    public void testLambda() {
        List<String> list = Arrays.asList("a", "b", "c");
        list.forEach(s -> System.out.println(s));
        
        Thread thread = new Thread(() -> {
            for (int i = 0; i < 10; i++) {
                System.out.println(i);
            }
        });
    }
}

// 9. 严格浮点测试
strictfp class StrictFPTest {
    public strictfp double calculate(double a, double b) {
        return a * b;
    }
}

// 10. 泛型构造函数测试
class GenericConstructor {
    public <T> GenericConstructor(T parameter) {
        System.out.println("Generic constructor with: " + parameter);
    }
}

// 11. 显式构造函数调用测试
class ExplicitConstructorCall {
    private int value;
    
    public ExplicitConstructorCall() {
        this(42); // 调用另一个构造函数
    }
    
    public ExplicitConstructorCall(int value) {
        this.value = value;
    }
}

class ChildClass extends ExplicitConstructorCall {
    public ChildClass() {
        super(); // 显式调用父类构造函数
    }
    
    public ChildClass(int value) {
        super(value);
    }
}