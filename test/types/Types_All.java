import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Arrays;

public class Types_All {

    // 1) interface/implements + throws
    static class R implements AutoCloseable {
        public void close() throws java.io.IOException { }
    }

    // 2) very simple generic (remove if your parser doesn't support type args)
    static class Box<T> {
        T v;
        Box(T v) { this.v = v; }
        T get() { return v; }
    }

    public static void main(String[] args) throws Exception {
        // qualified static field access via import
        System.out.println(StandardCharsets.UTF_8);

        // arrays
        int[] a = new int[2];
        a[0] = 1;
        a[1] = 2;

        // simple generic usage (List<String>) + method invocation with args
        List<String> xs = Arrays.asList("a", "bb", "ccc");
        System.out.println(xs.size() + a[0] + a[1]);

        // try-with-resources with AutoCloseable
        try (R r = new R()) {
            System.out.println("ok");
        }

        // generic class usage
        Box<Integer> bi = new Box<>(123);
        System.out.println(bi.get());
    }
}
