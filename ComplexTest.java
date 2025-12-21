import java.util.ArrayList;
import java.util.List;

enum Color {
    RED, GREEN, BLUE
}

interface Named {
    String name();
}

@Deprecated
public class ComplexTest<T extends Number & Comparable<T>> implements Runnable, Named {

    private static int counter = 0;

    private T value;

    // 构造器
    public ComplexTest(T value) {
        this.value = value;
    }

    public static void main(String[] args) {
        // 泛型与构造调用
        ComplexTest<Integer> ct = new ComplexTest<>(42);

        // 局部变量与集合
        List<String> list = new ArrayList<>();
        list.add("hello");
        list.add("world");

        // Lambda 表达式 & forEach
        list.forEach(s -> {
            int len = s.length();
            if (len > 0) {
                System.out.println(s + ":" + len);
            } else {
                System.out.println("empty");
            }
        });

        // for 循环与静态字段
        for (int i = 0; i < 3; i++) {
            counter += i;
        }

        // try-catch
        try {
            int r = ct.compute(10, 20);
            System.out.println("result = " + r);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }

        // 使用内部类
        Inner inner = new Inner(5);
        System.out.println("inner.inc() = " + inner.inc());

        // 匿名内部类
        Runnable r = new Runnable() {
            @Override
            public void run() {
                System.out.println("anonymous run, counter = " + counter);
            }
        };
        r.run();

        // 调用 run()
        ct.run();
    }

    @Override
    public void run() {
        // 内部匿名类访问外部实例字段
        new Thread(new Runnable() {
            @Override
            public void run() {
                System.out.println("inner thread, value = " + value);
            }
        }).start();
    }

    @Override
    public String name() {
        return "ComplexTest";
    }

    public int compute(int a, int b) {
        int result;

        // switch 语句
        switch (a) {
            case 0:
                result = b;
                break;
            case 1:
                result = a + b;
                break;
            default:
                result = a - b;
        }

        // while 循环
        while (result < 0) {
            result++;
        }

        if (result == 0) {
            throw new IllegalArgumentException("result should not be zero");
        }

        return result;
    }

    // 静态内部类
    static class Inner {
        int x;

        Inner(int x) {
            this.x = x;
        }

        int inc() {
            return ++x;
        }
    }
}
