import java.util.*;
import java.util.function.*;
import java.util.stream.*;
import java.io.*;
import java.nio.file.*;
import java.time.*;
import java.sql.*;
import java.net.*;
import java.util.concurrent.*;
import java.util.regex.*;

// 自定义注解
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.METHOD, ElementType.TYPE})
@interface TestAnnotation {
    String value() default "default";
    int priority() default 0;
}

// 自定义异常
class CustomException extends Exception {
    public CustomException(String message) {
        super(message);
    }
}

// 泛型接口
interface Processor<T> {
    void process(T item);
    T getResult();
}

// 枚举类型
enum Status {
    PENDING, PROCESSING, COMPLETED, FAILED
}

// 记录类 (Java 14+)
// record Point(int x, int y) {
//     public double distanceTo(Point other) {
//         return Math.sqrt(Math.pow(x - other.x, 2) + Math.pow(y - other.y, 2));
//     }
// }

// 密封接口 (Java 17+)
sealed interface Shape permits Circle, Rectangle, Triangle {
    double area();
}

// 密封接口的实现
final class Circle implements Shape {
    private final double radius;

    public Circle(double radius) {
        this.radius = radius;
    }

    @Override
    public double area() {
        return Math.PI * radius * radius;
    }
}

final class Rectangle implements Shape {
    private final double width, height;

    public Rectangle(double width, double height) {
        this.width = width;
        this.height = height;
    }

    @Override
    public double area() {
        return width * height;
    }
}

final class Triangle implements Shape {
    private final double base, height;

    public Triangle(double base, double height) {
        this.base = base;
        this.height = height;
    }

    @Override
    public double area() {
        return 0.5 * base * height;
    }
}

// 主测试类
@TestAnnotation(value = "MainTest", priority = 1)
public class ComprehensiveJavaTest<T extends Comparable<T>> implements Processor<T>, AutoCloseable {

    // 静态变量
    private static final int MAX_COUNT = 100;
    private static int instanceCount = 0;

    // 实例变量
    private String name;
    private T value;
    private List<T> items;
    private Status status;

    // 静态初始化块
    static {
        System.out.println("Static initialization block executed");
        instanceCount = 0;
    }

    // 实例初始化块
    {
        System.out.println("Instance initialization block executed");
        instanceCount++;
        items = new ArrayList<>();
    }

    // 构造函数
    public ComprehensiveJavaTest() {
        this("Default");
    }

    public ComprehensiveJavaTest(String name) {
        this.name = name;
        this.status = Status.PENDING;
    }

    // 泛型方法
    public <U extends Number> U processNumber(U number) {
        System.out.println("Processing number: " + number);
        return number;
    }

    // 接口方法实现
    @Override
    public void process(T item) {
        items.add(item);
        System.out.println("Processed item: " + item);
    }

    @Override
    public T getResult() {
        if (!items.isEmpty()) {
            return items.get(items.size() - 1);
        }
        return null;
    }

    // AutoCloseable 接口实现
    @Override
    public void close() {
        System.out.println("Closing resource: " + name);
        items.clear();
    }

    // 异常处理示例
    public void riskyMethod() throws CustomException, IOException {
        try {
            // 可能抛出异常的代码
            if (Math.random() > 0.5) {
                throw new CustomException("Random exception");
            }

            // 使用try-with-resources
            try (FileWriter writer = new FileWriter("test.txt")) {
                writer.write("Test content");
            }

        } catch (IOException e) {
            System.err.println("IO Exception: " + e.getMessage());
            throw e;
        } finally {
            System.out.println("Finally block executed");
        }
    }

    // 流API示例
    public void streamExamples() {
        // 创建列表
        List<String> names = Arrays.asList("Alice", "Bob", "Charlie", "David", "Eve");

        // 流操作
        List<String> result = names.stream()
                .filter(name -> name.length() > 3)
                .map(String::toUpperCase)
                .sorted()
                .collect(Collectors.toList());

        System.out.println("Filtered names: " + result);

        // 并行流
        long count = names.parallelStream()
                .filter(name -> name.contains("a"))
                .count();

        System.out.println("Names containing 'a': " + count);

        // 使用Collectors
        Map<Integer, List<String>> groupedByNameLength = names.stream()
                .collect(Collectors.groupingBy(String::length));

        System.out.println("Grouped by length: " + groupedByNameLength);
    }

    // 日期时间API示例
    public void dateTimeExamples() {
        // 当前日期时间
        LocalDateTime now = LocalDateTime.now();
        System.out.println("Current date/time: " + now);

        // 特定日期
        LocalDate date = LocalDate.of(2023, Month.DECEMBER, 25);
        System.out.println("Christmas: " + date);

        // 日期计算
        LocalDate nextWeek = LocalDate.now().plusWeeks(1);
        System.out.println("Next week: " + nextWeek);

        // 持续时间
        Duration duration = Duration.between(now, now.plusHours(2));
        System.out.println("Duration: " + duration.toMinutes() + " minutes");
    }

    // 文件操作示例
    public void fileOperations() throws IOException {
        // 创建临时文件
        Path tempFile = Files.createTempFile("test", ".txt");

        // 写入文件
        Files.write(tempFile, Arrays.asList("Line 1", "Line 2", "Line 3"));

        // 读取文件
        List<String> lines = Files.readAllLines(tempFile);
        System.out.println("File content: " + lines);

        // 删除文件
        Files.deleteIfExists(tempFile);
    }

    // 多线程示例
    public void threadingExamples() throws InterruptedException, ExecutionException {
        // 使用ExecutorService
        ExecutorService executor = Executors.newFixedThreadPool(3);

        // 提交任务
        List<Future<String>> futures = new ArrayList<>();
        for (int i = 0; i < 5; i++) {
            final int taskId = i;
            futures.add(executor.submit(() -> {
                Thread.sleep(1000);
                return "Task " + taskId + " completed by " + Thread.currentThread().getName();
            }));
        }

        // 获取结果
        for (Future<String> future : futures) {
            System.out.println(future.get());
        }

        // 关闭ExecutorService
        executor.shutdown();
        executor.awaitTermination(5, TimeUnit.SECONDS);

        // 使用CompletableFuture
        CompletableFuture.supplyAsync(() -> {
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        throw new IllegalStateException(e);
                    }
                    return "Async result";
                }).thenApply(String::toUpperCase)
                .thenAccept(System.out::println)
                .join();
    }

    // 正则表达式示例
    public void regexExamples() {
        String text = "The quick brown fox jumps over the lazy dog. Email: test@example.com";

        // 匹配模式
        Pattern pattern = Pattern.compile("\\b\\w{5}\\b"); // 5字母单词
        Matcher matcher = pattern.matcher(text);

        while (matcher.find()) {
            System.out.println("Found: " + matcher.group());
        }

        // 替换
        String replaced = text.replaceAll("fox", "cat");
        System.out.println("Replaced: " + replaced);

        // 提取电子邮件
        Pattern emailPattern = Pattern.compile("\\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Z|a-z]{2,}\\b");
        Matcher emailMatcher = emailPattern.matcher(text);
        if (emailMatcher.find()) {
            System.out.println("Email found: " + emailMatcher.group());
        }
    }

    // 反射示例
    public void reflectionExamples() throws Exception {
        Class<?> clazz = this.getClass();

        // 获取类信息
        System.out.println("Class name: " + clazz.getName());
        System.out.println("Annotations: " + Arrays.toString(clazz.getAnnotations()));

        // 获取方法信息
        for (Method method : clazz.getDeclaredMethods()) {
            System.out.println("Method: " + method.getName() +
                    ", Return type: " + method.getReturnType());
        }

        // 创建实例
        ComprehensiveJavaTest<?> instance = (ComprehensiveJavaTest<?>) clazz.getConstructor().newInstance();
        System.out.println("Instance created: " + instance.name);
    }

    // 记录类使用示例
    public void recordExample() {
        Point p1 = new Point(0, 0);
        Point p2 = new Point(3, 4);

        System.out.println("Point 1: " + p1);
        System.out.println("Point 2: " + p2);
        System.out.println("Distance: " + p1.distanceTo(p2));

        // 记录类的解构
        int x = p1.x();
        int y = p1.y();
        System.out.println("Coordinates: (" + x + ", " + y + ")");
    }

    // 密封类使用示例
    public void sealedClassExample() {
        Shape circle = new Circle(5.0);
        Shape rectangle = new Rectangle(4.0, 6.0);
        Shape triangle = new Triangle(3.0, 4.0);

        List<Shape> shapes = Arrays.asList(circle, rectangle, triangle);

        for (Shape shape : shapes) {
            System.out.println("Shape area: " + shape.area());

            // 使用模式匹配 (Java 16+)
            if (shape instanceof Circle c) {
                System.out.println("It's a circle with radius: " + c.area() / Math.PI);
            } else if (shape instanceof Rectangle r) {
                System.out.println("It's a rectangle");
            } else if (shape instanceof Triangle t) {
                System.out.println("It's a triangle");
            }
        }
    }

    // 可选值示例
    public void optionalExample() {
        Optional<String> optional = Optional.ofNullable(System.getProperty("user.name"));

        // 多种方式处理Optional
        String username = optional.orElse("Unknown");
        System.out.println("Username: " + username);

        optional.ifPresent(name -> System.out.println("Hello, " + name));

        Optional<String> upperCaseName = optional.map(String::toUpperCase);
        upperCaseName.ifPresent(System.out::println);
    }

    // 函数式编程示例
    public void functionalProgrammingExamples() {
        // 函数接口
        Function<String, Integer> stringToInt = Integer::parseInt;
        Predicate<String> isLong = s -> s.length() > 5;
        Consumer<String> printer = System.out::println;
        Supplier<String> generator = () -> "Generated: " + Math.random();

        // 使用方法
        int number = stringToInt.apply("42");
        boolean longEnough = isLong.test("Hello World");
        printer.accept("Testing consumer");
        String value = generator.get();

        System.out.println("Number: " + number);
        System.out.println("Long enough: " + longEnough);
        System.out.println("Generated value: " + value);

        // 函数组合
        Function<String, String> toUpper = String::toUpperCase;
        Function<String, String> addExclamation = s -> s + "!";
        Function<String, String> composed = toUpper.andThen(addExclamation);

        String result = composed.apply("hello");
        System.out.println("Composed result: " + result);

        // 使用BiFunction
        BiFunction<Integer, Integer, Integer> adder = Integer::sum;
        int sum = adder.apply(5, 7);
        System.out.println("Sum: " + sum);
    }

    // 主方法
    public static void main(String[] args) {
        try (ComprehensiveJavaTest<String> test = new ComprehensiveJavaTest<>("TestInstance")) {
            // 基本操作
            test.process("Item1");
            test.process("Item2");
            System.out.println("Last result: " + test.getResult());

            // 调用各种示例方法
            test.streamExamples();
            test.dateTimeExamples();
            test.optionalExample();
            test.functionalProgrammingExamples();
            test.recordExample();
            test.sealedClassExample();

            // 异常处理
            try {
                test.riskyMethod();
            } catch (Exception e) {
                System.err.println("Caught exception: " + e.getMessage());
            }

            // 多线程
            try {
                test.threadingExamples();
            } catch (Exception e) {
                System.err.println("Threading error: " + e.getMessage());
            }

            // 正则表达式
            test.regexExamples();

            // 反射
            try {
                test.reflectionExamples();
            } catch (Exception e) {
                System.err.println("Reflection error: " + e.getMessage());
            }

            // 文件操作
            try {
                test.fileOperations();
            } catch (IOException e) {
                System.err.println("File operation error: " + e.getMessage());
            }

        } catch (Exception e) {
            System.err.println("Unexpected error: " + e.getMessage());
            e.printStackTrace();
        }

        System.out.println("Test completed. Instances created: " + instanceCount);
    }

    // 重写toString方法
    @Override
    public String toString() {
        return "ComprehensiveJavaTest{" +
                "name='" + name + '\'' +
                ", value=" + value +
                ", items=" + items +
                ", status=" + status +
                '}';
    }

    // 重写equals和hashCode方法
    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        ComprehensiveJavaTest<?> that = (ComprehensiveJavaTest<?>) o;
        return Objects.equals(name, that.name) &&
                Objects.equals(value, that.value) &&
                Objects.equals(items, that.items) &&
                status == that.status;
    }

    @Override
    public int hashCode() {
        return Objects.hash(name, value, items, status);
    }
}