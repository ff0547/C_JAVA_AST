public class Smoke_All {
    private int f = 1;

    public Smoke_All() {
        this.f = 2;
    }

    public int add(int x) {
        return x + f;
    }

    public static int twice(int x) {
        return x * 2;
    }

    public static void main(String[] args) {
        int x = 1;
        x = x + 2;

        Smoke_All o = new Smoke_All();
        int y = o.add(x);

        int[] a = new int[3];
        a[0] = y;
        a[1] = twice(y);

        System.out.println(a[0] + a[1]);
    }
}
