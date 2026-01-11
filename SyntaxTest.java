// 文件: SyntaxTest.java
package com.example.parser.test;

import java.util.*;
import java.io.Serializable;

public class SyntaxTest<T extends Serializable> extends BaseClass implements TestInterface {

    private static final int MAX_COUNT = 100;
    private transient String instanceVar = "default";
    protected volatile List<T> genericList = new ArrayList<>();
    public static String[][][] multiDimArray = new String[2][3][4];

    enum Status {
        ACTIVE(1), INACTIVE(0), PENDING(2),;

        private int code;
        Status(int code) { this.code = code; }
        public int getCode() { return code; }
    }

    static {
        System.out.println("Static initializer executed");
        multiDimArray[0][0][0] = "test";
    }

    {
        instanceVar = "initialized";
    }

    public SyntaxTest() {}

    @SuppressWarnings("unchecked")
    public SyntaxTest(String param) {
        this.instanceVar = param;
    }

    public synchronized List<T> complexMethod(List<? super Number> input)
            throws IllegalArgumentException, ArithmeticException {

        int localVar = switch(input.size()) {
            case 0 -> 0;
            case 1 -> 1;
            default -> {
                int result = input.size() * 2;
                yield result;
            }
        };

        try (Scanner sc = new Scanner(System.in)) {
            if (localVar > MAX_COUNT) {
                throw new IllegalArgumentException("Value too large");
            }

            for (int i = 0; i < localVar; i++) {
                while (i % 3 == 0) {
                    do {
                        System.out.println("Iteration: " + i);
                    } while (false);
                    break;
                }
            }
        } catch (Exception e) {
            System.err.println("Error occurred: " + e.getMessage());
        } finally {
            System.out.println("Cleanup completed");
        }

        Runnable r = () -> System.out.println("Lambda executed");
        new Thread(r).start();

        genericList.forEach(System.out::println);

        return Collections.emptyList();
    }

    static class NestedStaticClass {
        void display() {
            System.out.println("Static nested class");
        }
    }

    class InnerClass {
        void accessOuter() {
            System.out.println("Accessing outer: " + instanceVar);
        }
    }

    @Override
    public void interfaceMethod() {
        class LocalClass {
            void show() {
                System.out.println("Local class method");
            }
        }
        new LocalClass().show();
    }

    public static void main(String... args) {
        SyntaxTest<String> obj = new SyntaxTest<>("testValue");
        var list = List.of(1, 2.5, 3f);

        TestInterface anonymous = new TestInterface() {
            @Override
            public void interfaceMethod() {
                System.out.println("Anonymous class implementation");
            }
        };

        obj.complexMethod(list);
        anonymous.interfaceMethod();

        NestedStaticClass staticObj = new NestedStaticClass();
        InnerClass innerObj = obj.new InnerClass();

        staticObj.display();
        innerObj.accessOuter();

        assert obj.instanceVar.equals("testValue") : "Assertion failed";
    }
}

class BaseClass {
    protected String baseField = "base";
}

interface TestInterface {
    void interfaceMethod();

    default void defaultMethod() {
        System.out.println("Interface default method");
    }
}

// 替代记录类的传统类
class Point {
    private final int x;
    private final int y;

    public Point(int x, int y) {
        if (x < 0 || y < 0)
            throw new IllegalArgumentException("Negative values not allowed");
        this.x = x;
        this.y = y;
    }

    public int x() { return x; }
    public int y() { return y; }

    @Override
    public String toString() {
        return "Point[" + x + ", " + y + "]";
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Point point = (Point) o;
        return x == point.x && y == point.y;
    }

    @Override
    public int hashCode() {
        return Objects.hash(x, y);
    }
}