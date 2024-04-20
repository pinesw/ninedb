package io.woutervh.ninedb;

import java.util.function.Predicate;

public class KvDatabase implements AutoCloseable {
    static {
        System.loadLibrary("ninedb");
    }

    private static native long kvdb_open(String path, DbConfig config);

    private static native void kvdb_close(long db_handle);

    private static native void kvdb_add(long db_handle, byte[] key, byte[] value);

    private static native byte[] kvdb_get(long db_handle, byte[] key);

    private static native KeyValuePair kvdb_at(long db_handle, long index);

    private static native byte[][] kvdb_traverse(long db_handle, Predicate<byte[]> predicate);

    private static native void kvdb_flush(long db_handle);

    private static native void kvdb_compact(long db_handle);

    private static native long kvdb_begin(long db_handle);

    private static native long kvdb_seek_key(long db_handle, byte[] key);

    private static native long kvdb_seek_index(long db_handle, long index);

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

    public byte[][] traverse(Predicate<byte[]> predicate) {
        return kvdb_traverse(db_handle, predicate);
    }

    public KvDbIterator begin() {
        long it_handle = kvdb_begin(db_handle);
        return new KvDbIterator(it_handle);
    }

    public KvDbIterator seekKey(byte[] key) {
        long it_handle = kvdb_seek_key(db_handle, key);
        return new KvDbIterator(it_handle);
    }

    public KvDbIterator seekIndex(long index) {
        long it_handle = kvdb_seek_index(db_handle, index);
        return new KvDbIterator(it_handle);
    }

    public static KvDatabase open(String path, DbConfig config) {
        long db_handle = kvdb_open(path, config);
        return new KvDatabase(db_handle);
    }
}
