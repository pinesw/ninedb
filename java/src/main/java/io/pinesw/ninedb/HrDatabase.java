package io.pinesw.ninedb;

public class HrDatabase implements AutoCloseable {
    static {
        NativeLoader.load();
    }

    private static native long hrdb_open(String path, DbConfig config);

    private static native void hrdb_close(long db_handle);

    private static native void hrdb_add(long db_handle, long x0, long y0, long x1, long y1, byte[] value);

    private static native byte[][] hrdb_search(long db_handle, long x0, long y0, long x1, long y1);

    private static native void hrdb_flush(long db_handle);

    private static native void hrdb_compact(long db_handle);

    private long db_handle;

    private HrDatabase(long db_handle) {
        this.db_handle = db_handle;
    }

    public void add(long x0, long y0, long x1, long y1, byte[] value) {
        hrdb_add(db_handle, x0, y0, x1, y1, value);
    }

    public byte[][] search(long x0, long y0, long x1, long y1) {
        return hrdb_search(db_handle, x0, y0, x1, y1);
    }

    @Override
    public void close() {
        hrdb_close(db_handle);
    }

    public void flush() {
        hrdb_flush(db_handle);
    }

    public void compact() {
        hrdb_compact(db_handle);
    }

    public static HrDatabase open(String path, DbConfig config) {
        return new HrDatabase(hrdb_open(path, config));
    }
}
