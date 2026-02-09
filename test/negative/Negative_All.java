//错误用例测试
public class Negative_All {
    public static void main(String[] args) {
        // 1) missing semicolon
        int x = 1

        // 2) break outside loop/switch
        break;

        // 3) unbalanced parentheses
        if ((x > 0) {
            x++;
        }

        // 4) malformed for
        for (int i = 0 i < 3; i++) {
            x += i;
        }

        // 5) invalid continue (outside loop)
        continue;
    }
}
