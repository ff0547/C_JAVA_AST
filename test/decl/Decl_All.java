public class Decl_All {

    private int f = 10;
    private static int SF = 100;

    static class Inner {
        int x;
        Inner(int x) { this.x = x; }
        int get() { return x; }
    }

    public Decl_All() {
        this.f = 11;
    }

    public int add(int x) {
        return x + f + SF;
    }

    public int add(int x, int y) {
        return x + y + f;
    }

    public static void main(String[] args) {
        Decl_All o = new Decl_All();

        // 1) local decl + shadowing
        int f = 1; // shadows field name, access field via this-like pattern: o.f
        {
            int f2 = f + o.f;
            System.out.println(f2);
        }

        // 2) multi-var declaration
        int a = 1, b = 2, c = 3;

        // 3) for-init variable scope
        for (int i = 0; i < 2; i++) {
            int t = i + a;
            System.out.println(t);
        }

        // 4) overloaded methods
        int r1 = o.add(a);
        int r2 = o.add(a, b);

        // 5) inner class usage
        Inner in = new Inner(r1 + r2);

        // 6) array decl + access
        int[] arr = new int[3];
        arr[0] = in.get();
        arr[1] = r2;
        arr[2] = arr[0] + arr[1];

        System.out.println(arr[2] + c);
    }
}
