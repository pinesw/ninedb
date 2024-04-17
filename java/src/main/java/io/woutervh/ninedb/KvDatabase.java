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

    public KeyValuePair at(long index) {
        return kvdb_at(db_handle, index);
    }

    @Override
    public void close() {
        kvdb_close(db_handle);
    }

    public void flush() {
        kvdb_flush(db_handle);
    }

    public void compact() {
        kvdb_compact(db_handle);
    }

    public static KvDatabase open(String path, DbConfig config) {
        long db_handle = kvdb_open(path, config);
        return new KvDatabase(db_handle);
    }

    public static void main(String[] args) throws Exception {
        DbConfig config = new DbConfig();
        config.deleteIfExists = true;
        config.maxNodeEntries = 3;
        config.reduce = (byte[][] keys) -> {
            return keys[0];
        };

        try (KvDatabase db = KvDatabase.open("data/test-hrdb", config)) {
            db.add("key_1".getBytes(), "value_1".getBytes());
            db.add("key_2".getBytes(), "value_2".getBytes());
            db.add("key_3".getBytes(), "value_3".getBytes());
            db.add("key_4".getBytes(), "value_4".getBytes());
            db.add("key_5".getBytes(), "value_5".getBytes());
            db.add("key_6".getBytes(), "value_6".getBytes());
            db.add("key_7".getBytes(), "value_7".getBytes());
            db.add("key_8".getBytes(), "value_8".getBytes());
            db.add("key_9".getBytes(), "value_9".getBytes());
            db.add("key_10".getBytes(), "value_10".getBytes());
            db.flush();
        }
    }
}
