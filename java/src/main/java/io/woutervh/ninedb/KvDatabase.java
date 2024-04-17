package io.woutervh.ninedb;

public class KvDatabase implements AutoCloseable {
    static {
        System.loadLibrary("ninedb");
    }

    private static native long kvdb_open(String path, DbConfig config);
    private static native void kvdb_close(long db_handle);
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

    private KvDatabase(long db_handle) {
        this.db_handle = db_handle;
    }

    public void add(byte[] key, byte[] value) {
        kvdb_add(db_handle, key, value);
    }

    public byte[] get(byte[] key) {
        return kvdb_get(db_handle, key);
    }

    @Override
    public void close() {
        kvdb_close(db_handle);
    }

    public static KvDatabase open(String path, DbConfig config) {
        long db_handle = kvdb_open(path, config);
        return new KvDatabase(db_handle);
    }

    public static void main(String[] args) throws Exception {
        try (KvDatabase db = KvDatabase.open("data/test-hrdb", new DbConfig())) {
            db.add("key".getBytes(), "value".getBytes());
            System.out.println(new String(db.get("key".getBytes())));
        }
    }
}
