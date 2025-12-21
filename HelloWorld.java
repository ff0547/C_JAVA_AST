public class HelloWorld {
    private String message;

    public HelloWorld(String msg) {
        this.message = msg;
    }

    public void sayHello() {
        System.out.println(message);
    }

    public static void main(String[] args) {
        HelloWorld hello = new HelloWorld("Hello, World!");
        hello.sayHello();
    }
}
