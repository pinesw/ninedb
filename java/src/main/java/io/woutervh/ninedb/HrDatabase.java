package io.woutervh.ninedb;

public class HrDatabase {
    static {
        System.loadLibrary("ninedb");
    }

    private static native long open(String path, DbConfig config);

    public static void main(String[] args) {
        long result = HrDatabase.open("", new DbConfig());
        System.out.println(result);
    }
}
