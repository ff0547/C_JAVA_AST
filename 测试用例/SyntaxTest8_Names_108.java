// SyntaxTest.java
package test.pkg; // PackageName: test.pkg

import java.util.List; // PackageOrTypeName: java.util, TypeIdentifier: List

public class SyntaxTest {
    // TypeName examples
    private String field1; // TypeIdentifier: String
    private java.lang.String field2; // PackageOrTypeName: java.lang, TypeIdentifier: String
    private test.pkg.SyntaxTest.NestedClass field3; // PackageOrTypeName: test.pkg, TypeIdentifier: SyntaxTest, TypeIdentifier: NestedClass

    // ExpressionName examples
    public void method() {
        int localVar = 42; // Identifier: localVar
        this.field1 = "value"; // AmbiguousName: this, Identifier: field1
        SyntaxTest.staticMethod(); // TypeIdentifier: SyntaxTest, Identifier: staticMethod
        test.pkg.SyntaxTest.staticMethod(); // PackageOrTypeName: test.pkg, TypeIdentifier: SyntaxTest, Identifier: staticMethod
    }

    // MethodName examples
    public void instanceMethod() {} // UnqualifiedMethodIdentifier: instanceMethod
    public static void staticMethod() {} // UnqualifiedMethodIdentifier: staticMethod

    // Nested class for TypeName testing
    static class NestedClass {
        public void nestedMethod() {
            SyntaxTest.this.field1 = "nested"; // AmbiguousName: SyntaxTest, AmbiguousName: this, Identifier: field1
        }
    }

    // ModuleName examples (in comments as they require module-info.java)
    // module test.module { } // ModuleName: test.module
    // requires java.base; // ModuleName: java.base
}