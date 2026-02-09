import java.util.Arrays;

public class Corpus_Small {

    static int clamp(int x, int lo, int hi) {
        if (x < lo) return lo;
        if (x > hi) return hi;
        return x;
    }

    static int sumIf(int[] a) {
        int s = 0;
        for (int i = 0; i < a.length; i++) {
            int v = a[i];
            if (v % 2 == 0) s += v;
            else s -= 1;
        }
        return s;
    }

    static String classify(int x) {
        switch (x % 3) {
            case 0 -> { return "A"; }
            case 1 -> { return "B"; }
            default -> { return "C"; }
        }
    }

    public static void main(String[] args) {
        int[] a = new int[5];
        for (int i = 0; i < a.length; i++) {
            a[i] = clamp(i * 3 - 2, -1, 7);
        }

        int s = 0;
        try {
            s = sumIf(a);
            if (s == 0) return;
        } finally {
            // ensure finally exists in AST even with return paths
            System.out.println("done");
        }

        String tag = classify(s);
        String msg = "sum=" + s + ", tag=" + tag;
        System.out.println(Arrays.toString(a));
        System.out.println(msg);
    }
}
