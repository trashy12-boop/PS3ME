/* GPL-2.0-only. Exercises actual CLDC bytecodes and the moving collector. */
public final class VmBaseline {
    private static int checks;
    private static Object retained;

    private static void check(boolean condition, String name) {
        if (!condition) throw new RuntimeException("FAILED: " + name);
        ++checks;
    }

    private interface Value { int value(); }
    private static final class Node implements Value {
        Node next;
        final int number;
        Node(int number, Node next) { this.number = number; this.next = next; }
        public int value() { return number; }
    }

    private static final class Worker extends Thread {
        boolean done;
        int result;
        public void run() {
            synchronized (this) {
                result = 12345;
                done = true;
                notifyAll();
            }
        }
    }

    private static void arithmetic() {
        int x = 2147483647;
        check(x + 1 == -2147483648, "int wraparound");
        int minus = -17;
        check(minus / 3 == -5 && minus % 3 == -2, "signed division");
        check((minus >>> 28) == 15, "unsigned shift");
        long wide = 0x123456789abcdef0L;
        check((wide >>> 32) == 0x12345678L, "long shift");
        check(wide / 0x100000000L == 0x12345678L, "long division");
        double d = 1.25;
        check(d * 4.0 == 5.0 && d + 0.5 == 1.75, "double arithmetic");
        float f = 0.5f;
        check(f * 3 == 1.5f, "float arithmetic");
    }

    private static void objectsAndGc() {
        Node head = null;
        for (int i = 0; i < 800; ++i) head = new Node(i, head);
        retained = head;
        Object[] roots = new Object[] { head, "survives-collection", new int[31] };
        for (int cycle = 0; cycle < 80; ++cycle) {
            byte[][] churn = new byte[12][];
            for (int i = 0; i < churn.length; ++i) churn[i] = new byte[4096];
            System.gc();
        }
        check(retained == roots[0], "static and array roots after GC");
        int sum = 0;
        int count = 0;
        for (Node n = (Node) retained; n != null; n = n.next) {
            Value polymorphic = n;
            sum += polymorphic.value();
            ++count;
        }
        check(count == 800 && sum == 319600, "linked objects and interface calls after GC");
        check("survives-collection".equals(roots[1]), "string reference after GC");
        long[] longs = new long[] { -1L, 0x1020304050607080L };
        check(longs[0] == -1L && longs[1] == 0x1020304050607080L, "long array");
        int[] src = new int[] { 2, 3, 5, 7, 11 };
        int[] dst = new int[6];
        System.arraycopy(src, 1, dst, 2, 3);
        check(dst[1] == 0 && dst[2] == 3 && dst[4] == 7, "native arraycopy");
        check(new StringBuffer("PS3").append('M').append('E').toString().equals("PS3ME"),
              "string allocation and native methods");
    }

    private static void exceptions() {
        boolean caught = false;
        try {
            Object empty = null;
            empty.hashCode();
        } catch (NullPointerException expected) { caught = true; }
        check(caught, "null reference exception");
        caught = false;
        try {
            int[] a = new int[2];
            a[2] = 10;
        } catch (ArrayIndexOutOfBoundsException expected) { caught = true; }
        check(caught, "array bounds exception");
        int finallyValue = 0;
        try { throw new IllegalArgumentException(); }
        catch (IllegalArgumentException expected) { finallyValue = 3; }
        finally { finallyValue += 4; }
        check(finallyValue == 7, "exception unwinding and finally");
    }

    private static void threads() throws InterruptedException {
        Worker worker = new Worker();
        synchronized (worker) {
            worker.start();
            long deadline = System.currentTimeMillis() + 10000;
            while (!worker.done && System.currentTimeMillis() < deadline) worker.wait(100);
            check(worker.done && worker.result == 12345, "thread scheduling and monitors");
        }
        long before = System.currentTimeMillis();
        Thread.sleep(20);
        check(System.currentTimeMillis() >= before + 15, "sleep and clock");
    }

    public static void main(String[] args) throws Exception {
        arithmetic();
        objectsAndGc();
        exceptions();
        threads();
        System.out.println("PS3ME_VM_BASELINE_OK checks=" + checks);
    }
}
