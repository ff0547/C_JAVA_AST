import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

public class ControlFlow_All {

    static class DummyRes implements AutoCloseable {
        private final String name;
        DummyRes(String name) { this.name = name; }
        public void close() throws IOException {
            if (name.length() < 0) throw new IOException("impossible");
        }
    }

    static boolean pred(int i, int j) { return (i + j) % 3 != 0; }

    public static void main(String[] args) throws Exception {
        int sum = 0;

        // 1) for + while + do-while + empty stmt + continue/break
        for (int i = 0, j = 7; i < 3 && j > 0; i++, j -= 2) {
            while (j > 0) {
                do {
                    sum += i * j;
                    if ((sum & 1) == 0) { j--; continue; }
                    else { j -= 2; }
                } while (j > 0 && j % 4 != 0);

                if (j == 3) break;
                ; // EmptyStatement
                j--;
            }
        }

        // 2) label + nested for + labeled break/continue
        outer:
        for (int a = 0; a < 4; a++) {
            inner:
            for (int b = 0; b < 6; b++) {
                if (b == 1) continue inner;
                if (a == 2 && b == 2) break outer;
                if (pred(a, b)) sum += a + b;
                else sum -= 1;
            }
        }

        // 3) switch arrow style
        int a = 1, b = 2;
        switch ((a + b) % 3) {
            case 0 -> { sum += 10; }
            case 1 -> { sum += 20; }
            default -> sum++;
        }

        // 4) traditional switch + fall-through + continue while
        int w = 0;
        while (w < 4) {
            int m = w % 3;
            switch (m) {
                case 0:
                    sum += 100;
                    // fall-through
                case 1:
                    sum += 200;
                    break;
                default:
                    sum += 300;
                    w++;
                    continue;
            }
            w++;
        }

        // 5) for with empty init/update, plus for(;;)
        int z = 2;
        for (; z > 0; ) {
            z--;
        }
        for (;;) { break; }

        // 6) try-with-resources inside loop + while((ch=read())!=-1) + break
        for (int i = 0; i < 2; i++) {
            try (DummyRes r = new DummyRes("r" + i);
                 InputStream in = new ByteArrayInputStream("hi".getBytes(StandardCharsets.UTF_8))) {
                int ch;
                while ((ch = in.read()) != -1) {
                    sum += ch;
                    if (ch == 'i') break;
                }
            } catch (IOException e) {
                sum -= 1000;
            } finally {
                sum += 1;
            }
        }

        System.out.println(sum);
    }
}
