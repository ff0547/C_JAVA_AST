// 包声明测试
package com.example.syntax.test;

// 导入声明测试 - 单类型导入
import java.util.ArrayList;
// 导入声明测试 - 按需类型导入
import java.util.*;
// 导入声明测试 - 单静态导入
import static java.lang.Math.PI;
// 导入声明测试 - 静态按需导入
import static java.lang.Math.*;

// 注解定义
@interface MyAnnotation {
    String value() default "";
}

// 类声明测试
public class SyntaxTest {
    
    // 字段声明
    private int value;
    public static final String CONSTANT = "TEST";
    
    // 构造方法
    public SyntaxTest() {
        this.value = 0;
    }
    
    public SyntaxTest(int value) {
        this.value = value;
    }
    
    // 方法声明
    public int getValue() {
        return value;
    }
    
    public void setValue(int value) {
        this.value = value;
    }
    
    // 静态方法
    public static void staticMethod() {
        System.out.println("Static method called");
    }
    
    // 使用静态导入
    public double calculateCircleArea(double radius) {
        return PI * pow(radius, 2);
    }
    
    // 主方法
    public static void main(String[] args) {
        // 使用导入的类
        ArrayList<String> list = new ArrayList<>();
        list.add("Test");
        
        SyntaxTest test = new SyntaxTest(10);
        System.out.println("Value: " + test.getValue());
        
        staticMethod();
        
        double area = test.calculateCircleArea(5.0);
        System.out.println("Area: " + area);
    }
}

// 接口声明测试
interface MyInterface {
    void interfaceMethod();
    
    // Java 8 默认方法
    default void defaultMethod() {
        System.out.println("Default method implementation");
    }
    
    // 静态方法
    static void staticInterfaceMethod() {
        System.out.println("Static interface method");
    }
}

// 空类型声明测试
;

// 另一个类实现接口
class MyImplementation implements MyInterface {
    @Override
    public void interfaceMethod() {
        System.out.println("Interface method implemented");
    }
    
    // 注解使用测试
    @MyAnnotation("test")
    public void annotatedMethod() {
        System.out.println("Annotated method");
    }
}

// 枚举测试
enum Day {
    MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY
}

// 抽象类测试
abstract class AbstractClass {
    abstract void abstractMethod();
    
    void concreteMethod() {
        System.out.println("Concrete method in abstract class");
    }
}