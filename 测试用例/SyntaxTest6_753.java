/**
 * 该文件展示了 Java 中 yield 语句的各种用法
 * 包括作为 switch 表达式中的关键字和作为标识符的用法
 */
public class SyntaxTest {
    
    public static void main(String[] args) {
        // 测试用例 1：基本用法
        testBasicYield();
        
        // 测试用例 2：复杂逻辑处理
        testComplexYield();
        
        // 测试用例 3：传统冒号语法与 yield
        testColonSyntaxYield();
        
        // 测试用例 4：枚举类型与 yield
        testEnumYield();
        
        // 测试用例 5：返回值类型一致性
        testTypeConsistencyYield();
        
        // 测试用例 6：yield 作为标识符
        testYieldAsIdentifier();
        
        // 测试用例 7：更多 yield 作为标识符的示例
        testMoreYieldIdentifiers();
    }
    
    // 测试用例 1：基本用法
    private static void testBasicYield() {
        System.out.println("=== 测试用例 1: 基本用法 ===");
        
        int day = 3;
        String dayType = switch (day) {
            case 1, 2, 3, 4, 5 -> "Weekday";
            case 6, 7 -> "Weekend";
            default -> {
                System.out.println("Invalid day: " + day);
                yield "Unknown"; // 在代码块中使用 yield
            }
        };
        
        System.out.println("Day " + day + " is a " + dayType);
        System.out.println();
    }
    
    // 测试用例 2：复杂逻辑处理
    private static void testComplexYield() {
        System.out.println("=== 测试用例 2: 复杂逻辑处理 ===");
        
        int score = 85;
        String grade = switch (score / 10) {
            case 10, 9 -> "A";
            case 8 -> {
                if (score >= 85) {
                    yield "A-";
                } else {
                    yield "B+";
                }
            }
            case 7 -> "B";
            case 6 -> "C";
            default -> {
                System.out.println("Needs improvement");
                yield "F";
            }
        };
        
        System.out.println("Score: " + score + ", Grade: " + grade);
        System.out.println();
    }
    
    // 测试用例 3：传统冒号语法与 yield
    private static void testColonSyntaxYield() {
        System.out.println("=== 测试用例 3: 传统冒号语法与 yield ===");
        
        String fruit = "Apple";
        int price = switch (fruit) {
            case "Apple":
                System.out.println("Selecting apples");
                yield 5; // 必须显式使用 yield
            case "Orange":
                System.out.println("Selecting oranges");
                yield 4;
            case "Banana":
                System.out.println("Selecting bananas");
                yield 3;
            default:
                System.out.println("Unknown fruit: " + fruit);
                yield 0;
        };
        
        System.out.println(fruit + " costs $" + price);
        System.out.println();
    }
    
    // 测试用例 4：枚举类型与 yield
    private static void testEnumYield() {
        System.out.println("=== 测试用例 4: 枚举类型与 yield ===");
        
        enum Size { SMALL, MEDIUM, LARGE, EXTRA_LARGE }
        
        Size size = Size.LARGE;
        String description = switch (size) {
            case SMALL -> "Small size";
            case MEDIUM -> "Medium size";
            case LARGE -> {
                String desc = "Large size for adults";
                yield desc.toUpperCase(); // 可以执行复杂操作
            }
            case EXTRA_LARGE -> "Extra large size";
        };
        
        System.out.println(description);
        System.out.println();
    }
    
    // 测试用例 5：返回值类型一致性
    private static void testTypeConsistencyYield() {
        System.out.println("=== 测试用例 5: 返回值类型一致性 ===");
        
        int value = 2;
        String result = switch (value) {
            case 1 -> "One"; // 字符串
            case 2 -> {
                int temp = 2 * 2;
                yield "Two squared is " + temp; // 必须返回字符串
            }
            default -> {
                System.out.println("Other value");
                yield "Other"; // 必须返回字符串
            }
        };
        
        System.out.println(result);
        System.out.println();
    }
    
    // 测试用例 6：yield 作为标识符
    private static void testYieldAsIdentifier() {
        System.out.println("=== 测试用例 6: yield 作为标识符 ===");
        
        // yield 可以作为变量名（虽然不推荐）
        int yield = 42;
        System.out.println("yield as variable: " + yield);
        
        // yield 可以作为方法名
        String result = yield();
        System.out.println("yield as method name: " + result);
        
        // yield 可以作为参数名
        printYield(yield);
        
        System.out.println();
    }
    
    // 测试用例 7：更多 yield 作为标识符的示例
    private static void testMoreYieldIdentifiers() {
        System.out.println("=== 测试用例 7: 更多 yield 作为标识符的示例 ===");
        
        // yield 作为类字段
        YieldClass yieldInstance = new YieldClass(100);
        System.out.println("YieldClass field: " + yieldInstance.yield);
        
        // yield 作为局部变量
        double yield = 3.14;
        System.out.println("Local yield variable: " + yield);
        
        // yield 作为数组名
        int[] yieldArray = {1, 2, 3};
        System.out.println("yield array length: " + yieldArray.length);
        
        System.out.println();
    }
    
    // 方法名为 yield（虽然不推荐）
    private static String yield() {
        return "This is from yield() method";
    }
    
    // 参数名为 yield（虽然不推荐）
    private static void printYield(int yield) {
        System.out.println("Parameter named yield: " + yield);
    }
    
    // 内部类使用 yield 作为字段名
    static class YieldClass {
        public int yield;
        
        public YieldClass(int yield) {
            this.yield = yield;
        }
    }
}