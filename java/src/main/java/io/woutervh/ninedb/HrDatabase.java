package io.woutervh.ninedb;

public class HrDatabase implements AutoCloseable {
    static {
        System.loadLibrary("ninedb");
    }

    private static native long db_open(String path, DbConfig config);
    private static native void db_close(long db_handle);
    private static native void kvdb_add(long db_handle, byte[] key, byte[] value);
    private static native byte[] kvdb_get(long db_handle, byte[] key);
    private static native KeyValuePair kvdb_at(long db_handle, long index);
    // private static native void kvdb_traverse(long db_handle, );
    private static native void kvdb_flush(long db_handle);
    private static native void kvdb_compact(long db_handle);
    // private static native void kvdb_begin(long db_handle);
    // private static native void kvdb_seek_key(long db_handle);
    // private static native void kvdb_seek_index(long db_handle);

    private long db_handle;

    private HrDatabase(long db_handle) {
        this.db_handle = db_handle;
    }

    @Override
    public void close() {
        db_close(db_handle);
    }

    public static HrDatabase open(String path, DbConfig config) {
        long db_handle = db_open(path, config);
        return new HrDatabase(db_handle);
    }

    public static void main(String[] args) throws Exception {
        try (HrDatabase db = HrDatabase.open("data/test-hrdb", new DbConfig())) {
        }
    }
}
