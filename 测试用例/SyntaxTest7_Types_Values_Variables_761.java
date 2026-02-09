// SyntaxTest.java
import java.util.List;
import java.util.Map;
import java.io.Serializable;

// 定义注解用于测试
@interface MyAnnotation {
    String value() default "";
}

@interface AnotherAnnotation {
    int count() default 0;
}

// 主类，包含所有语法测试
public class SyntaxTest<
    // TypeParameter: {TypeParameterModifier} TypeIdentifier [TypeBound]
    @MyAnnotation T extends Number & Serializable, // TypeBound with AdditionalBound
    @MyAnnotation @AnotherAnnotation U extends Comparable<U> // Multiple annotations
> {
    
    // ===== 之前的语法测试 =====
    
    // 1. PrimitiveType测试
    private int primitiveInt;
    private boolean primitiveBoolean;
    @SuppressWarnings("unused") private byte annotatedByte;
    @SuppressWarnings("unused") private short annotatedShort;
    @SuppressWarnings("unused") private int annotatedInt;
    @SuppressWarnings("unused") private long annotatedLong;
    @SuppressWarnings("unused") private char annotatedChar;
    @SuppressWarnings("unused") private float annotatedFloat;
    @SuppressWarnings("unused") private double annotatedDouble;
    @SuppressWarnings("unused") private boolean annotatedBoolean;
    
    // 2. ReferenceType测试
    // ClassOrInterfaceType
    private String classType;
    private List<Integer> interfaceType;
    
    // TypeVariable
    private T typeVariableField;
    
    // ArrayType
    private int[] intArray;
    private String[] stringArray;
    private T[] typeVariableArray;
    private int[][] multiIntArray;
    
    // 3. ClassOrInterfaceType详细测试
    @MyAnnotation("test")
    private String simpleClassType;
    private java.util.@MyAnnotation List<String> packageClassType;
    private Map.@MyAnnotation Entry<String, Integer> nestedClassType;
    private @MyAnnotation List<Integer> interfaceType2;
    private java.util.@MyAnnotation Collection<String> packageInterfaceType;
    private Map.@MyAnnotation Entry<String, String> nestedInterfaceType;
    
    // ===== 新增语法测试 =====
    
    // 4. Dims测试 - 数组维度
    private int @MyAnnotation [] singleAnnotatedArray;
    private int @MyAnnotation [] @AnotherAnnotation [] multiAnnotatedArray;
    private String @MyAnnotation [] @AnotherAnnotation [] @MyAnnotation [] tripleAnnotatedArray;
    
    // 5. TypeParameter测试
    // 已经在类声明中，添加方法级别的TypeParameter
    public <@MyAnnotation V extends Serializable & Cloneable> void methodWithTypeParameter(V param) {
        // 方法体
    }
    
    // 6. TypeBound测试
    // 已经在类声明中
    
    // 7. TypeArguments测试
    private List<String> simpleTypeArgs;
    private Map<String, Integer> complexTypeArgs;
    private List<Map<String, List<Integer>>> nestedTypeArgs;
    
    // 8. TypeArgument测试
    private List<String> referenceTypeArg;
    private List<?> unboundedWildcard;
    private List<? extends Number> upperBoundedWildcard;
    private List<? super Integer> lowerBoundedWildcard;
    
    // 9. Wildcard测试
    private List<@MyAnnotation ?> annotatedUnboundedWildcard;
    private List<@MyAnnotation ? extends Number> annotatedUpperBoundedWildcard;
    private List<@MyAnnotation ? super String> annotatedLowerBoundedWildcard;
    
    // 复杂组合示例
    private Map<
        @MyAnnotation ? extends Comparable<String>,
        List<@AnotherAnnotation ? super Number>
    > complexWildcardMap;
    
    // 方法参数中的复杂类型
    public void processList(List<@MyAnnotation ? extends Number> list) {
        // 方法体
    }
    
    public <@MyAnnotation W extends Comparable<W>> W processAndReturn(
        Map<@AnotherAnnotation ? super W, List<@MyAnnotation ? extends W>> complexParam
    ) {
        return null;
    }
    
    // 返回复杂类型的方法
    public Map<
        String,
        List<@MyAnnotation ? extends Number>
    > returnComplexType() {
        return null;
    }
    
    // 嵌套泛型与通配符
    public List<Map<String, List<?>>> returnNestedWildcard() {
        return null;
    }
    
    // 静态方法中的类型参数
    public static <@MyAnnotation S extends Serializable & Comparable<S>>
    void staticMethodWithComplexType(S param) {
        // 方法体
    }
    
    // 内部类使用外部类的类型参数
    class InnerClass<K extends T> {
        private K innerField;
        
        public List<@MyAnnotation ? extends K> innerMethod() {
            return null;
        }
    }
    
    // 方法局部变量中的复杂类型
    public void localVariables() {
        Map<
            @MyAnnotation String,
            List<@AnotherAnnotation ? extends Number>
        > localVar = null;
        
        List<@MyAnnotation ? super Integer> @AnotherAnnotation [] localArray = null;
    }
    
    // 确保之前的测试方法仍然存在
    public void methodWithAnnotatedParam(@SuppressWarnings("unused") int param) {
        // Method body
    }

    public @SuppressWarnings("unused") int methodWithAnnotatedReturnType() {
        return 0;
    }

    public void methodWithTypeVariable(T param) {
        // Method body
    }

    public T methodReturningTypeVariable() {
        return null;
    }

    public void methodWithArray(int[] array) {
        // Method body
    }

    public void methodWithMultiArray(int[][] array) {
        // Method body
    }
    
    public void methodWithClassArray(String[] array) {
        // Method body
    }
    
    public void methodWithTypeVariableArray(T[] array) {
        // Method body
    }
    
    public int[] methodReturningPrimitiveArray() {
        return new int[10];
    }
    
    public String[] methodReturningClassArray() {
        return new String[5];
    }
    
    public T[] methodReturningTypeVariableArray() {
        return null;
    }
}