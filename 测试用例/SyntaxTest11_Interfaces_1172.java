// Test cases for InterfaceDeclaration and related syntax

// NormalInterfaceDeclaration examples
public interface SimpleInterface {
    // Empty interface
}

protected interface ProtectedInterface {
    // Constants
    int CONSTANT = 10;
    String MESSAGE = "Hello";
}

private interface PrivateInterface {
    // Methods
    void method();
    int calculate();
}

abstract interface AbstractInterface {
    // With extends
    void method();
}

static interface StaticInterface {
    // Static interface
    String process();
}

strictfp interface StrictFPInterface {
    // StrictFP interface
    double compute();
}

@interface AnnotationInterface {
    // Annotation type declaration
    String value();
}

// Interface with type parameters
public interface GenericInterface<T> {
    T getValue();
    void setValue(T value);
}

// Interface extending other interfaces
interface ExtendedInterface extends SimpleInterface, GenericInterface<String> {
    String EXTENDED_CONSTANT = "Extended";
    
    // Default method
    default void defaultMethod() {
        System.out.println("Default implementation");
    }
    
    // Static method
    static void staticMethod() {
        System.out.println("Static method");
    }
    
    // Private method
    private void privateMethod() {
        System.out.println("Private method");
    }
}

// Complex example with multiple features
public interface ComplexInterface<T extends Number, R> extends GenericInterface<T>, ExtendedInterface {
    // Constants with different modifiers
    public static final int PUBLIC_CONST = 1;
    static final int STATIC_CONST = 2;
    final int FINAL_CONST = 3;
    int IMPLICIT_CONST = 4;
    
    // Methods with different modifiers
    void normalMethod();
    abstract void abstractMethod();
    default void defaultMethod() {
        privateHelper();
    }
    static void staticMethod() {
        System.out.println("Another static method");
    }
    private void privateHelper() {
        System.out.println("Private helper");
    }
    private static void privateStaticHelper() {
        System.out.println("Private static helper");
    }
    
    // Nested types
    class NestedClass {
        private int nestedField;
        
        public NestedClass(int value) {
            this.nestedField = value;
        }
        
        public int getValue() {
            return nestedField;
        }
    }
    
    interface NestedInterface {
        void nestedMethod();
    }
}

// Annotation Type Declaration examples
public @interface CustomAnnotation {
    // Annotation type elements
    String name();
    int version() default 1;
    Class<?> type() default Object.class;
    Thread.State state() default Thread.State.NEW;
    
    // Arrays
    String[] tags() default {};
    int[] values();
    
    // Nested annotations
    NestedAnnotation nested() default @NestedAnnotation;
    
    // Constants
    int MAX_VALUE = 100;
    String DEFAULT_NAME = "default";
}

@interface NestedAnnotation {
    String value() default "";
}

// Marker annotation (no elements)
@interface MarkerAnnotation {
}

// Single element annotation
@interface SingleElementAnnotation {
    String value();
}

// Annotation with array initializer
@interface ArrayAnnotation {
    String[] names();
    int[] numbers() default {1, 2, 3};
}

// Examples of using annotations
@MarkerAnnotation
@SingleElementAnnotation("test")
@CustomAnnotation(
    name = "Example",
    version = 2,
    type = String.class,
    state = Thread.State.RUNNABLE,
    tags = {"tag1", "tag2"},
    values = {1, 2, 3},
    nested = @NestedAnnotation("nested")
)
@ArrayAnnotation(names = {"a", "b", "c"})
public class AnnotationUsage {
    @MarkerAnnotation
    private String field;
    
    @SingleElementAnnotation("method")
    public void annotatedMethod(@CustomAnnotation(name = "param") String param) {
        @NestedAnnotation("local") String localVar = a;
        //@NestedAnnotation("local") String localVar = "test";
    }
}

// Interface with various member types
interface ComprehensiveInterface {
    // Constant declarations
    int CONSTANT1 = 1;
    public static final int CONSTANT2 = 2;
    String MESSAGE = "Hello World";
    
    // Method declarations
    void method1();
    public abstract void method2();
    default void method3() {
        System.out.println("Default method");
    }
    static void method4() {
        System.out.println("Static method");
    }
    private void method5() {
        System.out.println("Private method");
    }
    private static void method6() {
        System.out.println("Private static method");
    }
    
    // Nested class
    class NestedClass {
        private int value;
        
        public NestedClass(int value) {
            this.value = value;
        }
        
        public int getValue() {
            return value;
        }
    }
    
    // Nested interface
    interface NestedInterface {
        void nestedMethod();
    }
    
    // Nested annotation
    @interface NestedAnnotation {
        String value();
    }
    
    // Semicolon as member declaration
    ;
}

// Examples of different annotations
@Deprecated
@SuppressWarnings("unchecked")
@CustomAnnotation(name = "Test", values = {1})
public class VariousAnnotations {
    @Override
    @MarkerAnnotation
    public String toString() {
        return "VariousAnnotations";
    }
    
    @SafeVarargs
    public final <T> void safeMethod(T... args) {
        // Method implementation
    }
}

// Interface with type parameters and bounds
interface BoundedGenericInterface<T extends Number & Comparable<T>, U extends Cloneable> {
    T processNumber(U input);
}

// Example of an annotation with all element value types
@interface AllValueTypes {
    // Primitive types
    int integer();
    double decimal();
    boolean flag();
    char character();
    
    // Class and enum
    Class<?> clazz();
    Thread.State state();
    
    // String
    String text();
    
    // Annotation
    NestedAnnotation annotation();
    
    // Array
    int[] numbers();
}

// Test the annotation
@AllValueTypes(
    integer = 42,
    decimal = 3.14,
    flag = true,
    character = 'A',
    clazz = String.class,
    state = Thread.State.BLOCKED,
    text = "example",
    annotation = @NestedAnnotation("test"),
    numbers = {1, 2, 3, 4, 5}
)
class TestClass {
}

// Edge case: interface with no body
interface EmptyInterface {}

// Edge case: annotation with no elements
@interface EmptyAnnotation {}

// Edge case: interface with only constants
interface ConstantOnlyInterface {
    int A = 1;
    String B = "B";
    double C = 3.0;
}

// Edge case: interface with only methods
interface MethodOnlyInterface {
    void method1();
    default void method2() {}
    static void method3() {}
    private void method4() {}
}

// Example of an element value array initializer with trailing comma
@interface TrailingCommaAnnotation {
    int[] values() default {1, 2, 3,};
    String[] names() default {"a", "b", "c",};
}

@TrailingCommaAnnotation(values = {4, 5, 6,}, names = {"d", "e", "f",})
class TrailingCommaTest {
}

// Example of complex nested annotations
@interface ComplexAnnotation {
    SimpleAnnotation simple();
    SimpleAnnotation[] multiples();
}

@interface SimpleAnnotation {
    String value();
}

@ComplexAnnotation(
    simple = @SimpleAnnotation("single"),
    multiples = {
        @SimpleAnnotation("first"),
        @SimpleAnnotation("second"),
        @SimpleAnnotation("third")
    }
)
class ComplexAnnotationTest {
}

// Example of annotation with conditional expression as element value
@interface ConditionalAnnotation {
    String value() default (true) ? "true" : "false";
    int number() default (10 > 5) ? 10 : 5;
}

@ConditionalAnnotation(
    value = (System.currentTimeMillis() > 0) ? "positive" : "negative",
    number = (Integer.MAX_VALUE > 0) ? Integer.MAX_VALUE : 0
)
class ConditionalTest {
}