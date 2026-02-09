// SyntaxTest.java - Java编译器语法测试样本
package com.example;

import java.util.*;
import java.io.*;

/**
 * 主类：包含多种Java语法特性
 */
public class SyntaxTest {
    // 1. 基本数据类型与变量
    private int number = 42;
    private final String message = "Hello, Compiler!";
    private static double PI = 3.14159;
    
    // 2. 集合与泛型
    private List<String> names = new ArrayList<>();
    
    // 3. 枚举类型
    enum Status { ACTIVE, INACTIVE, PENDING }
    
    // 4. 内部类
    class InnerClass {
        void display() {
            System.out.println("Inner class method");
        }
    }

    // 5. 方法重载
    public int calculate(int a, int b) {
        return a + b;
    }
    
    public double calculate(double a, double b) {
        return a * b;
    }

    // 6. 异常处理
    public void readFile() throws IOException {
        try (BufferedReader br = new BufferedReader(new FileReader("test.txt"))) {
            String line;
            while ((line = br.readLine()) != null) {
                System.out.println(line);
            }
        } catch (FileNotFoundException e) {
            System.err.println("File not found: " + e.getMessage());
        }
    }

    // 7. 控制流语句
    public void controlFlowDemo() {
        int i = 10;
        if (i > 5) {
            for (int j = 0; j < i; j++) {
                switch (j % 3) {
                    case 0:
                        System.out.println(j + ": Zero");
                        break;
                    case 1:
                        System.out.println(j + ": One");
                        break;
                    default:
                        System.out.println(j + ": Two");
                }
            }
        }
    }

    // 8. 接口与Lambda
    interface MathOperation {
        int execute(int a, int b);
    }

    public void lambdaDemo() {
        MathOperation add = (a, b) -> a + b;
        System.out.println("Lambda result: " + add.execute(5, 3));
    }

    // 9. 注解
    @Deprecated
    public void oldMethod() {
        System.out.println("Deprecated method");
    }

    // 10. 静态初始化块
    static {
        System.out.println("Static initialization block executed");
    }

    // 11. 主方法
    public static void main(String[] args) {
        System.out.println("--- SyntaxTest Running ---");
        
        // 创建实例
        SyntaxTest test = new SyntaxTest();
        
        // 集合操作
        test.names.add("Alice");
        test.names.add("Bob");
        System.out.println("Names: " + test.names);
        
        // 枚举使用
        Status status = Status.ACTIVE;
        System.out.println("Status: " + status);
        
        // 内部类使用
        InnerClass inner = test.new InnerClass();
        inner.display();
        
        // 方法重载调用
        System.out.println("Int calculation: " + test.calculate(3, 4));
        System.out.println("Double calculation: " + test.calculate(3.0, 4.5));
        
        // 控制流演示
        test.controlFlowDemo();
        
        // Lambda调用
        test.lambdaDemo();
        
        // 异常处理调用
        try {
            test.readFile();
        } catch (IOException e) {
            System.err.println("IO error: " + e.getMessage());
        }
        
        System.out.println("--- Execution Complete ---");
    }
}