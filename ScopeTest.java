class ScopeTest {

    static class Inner {
        void m(int x) {}
        int f;
    }

    static class Outer {
        Inner inner = new Inner();
        Inner make() { return new Inner(); }
    }

    void test() {
        Outer o = new Outer();

        // 1) MethodInvocation（有 / 无 scope）
        foo(1, 2);              // scope == NULL
        o.make();               // scope == o

        // 2) 链式方法调用（scope 递归）
        o.make().m(10);         // m.scope = make(), make.scope = o

        // 3) ClassInstanceCreation
        new Inner();            // scope == NULL
        o.new Inner();          // scope == o

        // 4) MethodReference
        Runnable r = o::make;   // scope == o

        // 5) FieldAccess
        int x = o.inner.f;      // f.scope = o.inner
    }

    static void foo(int a, int b) {}
}
