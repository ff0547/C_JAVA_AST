public class Expr_All {

    static int f(int x) { return x + 1; }
    static boolean g(int x) { return (x & 1) == 0; }

    public static void main(String[] args) {
        int a = 1, b = 2, c = 3;

        // 1) precedence: * > +, parentheses
        int x1 = a + b * c;        // a + (b*c)
        int x2 = (a + b) * c;      // (a+b) * c

        // 2) unary and binary mix
        int x3 = -a + 5;
        int x4 = ~b ^ c;           // (~b) ^ c

        // 3) shifts vs plus
        int x5 = a << b + c;       // a << (b+c)

        // 4) relational + equality
        boolean r1 = a + b > c;
        boolean r2 = (a ^ b) != c;

        // 5) logical &&, ||, ! with grouping
        boolean p = (a < b && b < c) || !(a == 0);

        // 6) assignment in condition (common tricky AST shape)
        int t = 0, y;
        while ((y = f(t)) < 3 && g(y)) {
            t = y;
        }

        // 7) chained assignment and compound assignment
        int u, v, w;
        u = v = w = 7;
        u += 2;
        v *= 3;
        w &= 6;

        // 8) member access + method invocation chaining
        String s = "abc";
        int len = s.trim().substring(0, 2).length();

        System.out.println(x1 + x2 + x3 + x4 + x5);
        System.out.println(r1);
        System.out.println(r2);
        System.out.println(p);
        System.out.println(t);
        System.out.println(u + v + w + len);
    }
}
