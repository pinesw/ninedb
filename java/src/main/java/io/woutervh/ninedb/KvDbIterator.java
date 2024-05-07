package io.woutervh.ninedb;

public class KvDbIterator implements AutoCloseable {
    static {
        NativeLoader.load();
    }

    private static native void itr_close(long iterator_handle);

    private static native void itr_next(long iterator_handle);

    private static native boolean itr_has_next(long iterator_handle);

    private static native byte[] itr_get_key(long iterator_handle);

    private static native byte[] itr_get_value(long iterator_handle);

    private long iterator_handle;

    KvDbIterator(long iterator_handle) {
        this.iterator_handle = iterator_handle;
    }

    @Override
    public void close() {
        itr_close(iterator_handle);
    }

    public void next() {
        itr_next(iterator_handle);
    }

    public boolean hasNext() {
        return itr_has_next(iterator_handle);
    }

    public byte[] getKey() {
        return itr_get_key(iterator_handle);
    }

    public byte[] getValue() {
        return itr_get_value(iterator_handle);
    }
}
