package io.woutervh.ninedb;

public class HrDatabase {
    static {
        System.loadLibrary("ninedb");
    }

    private static native long open(String path, DbConfig config);
}
