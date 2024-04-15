package io.woutervh.ninedb;

public class KeyValuePair {
    public final byte[] key;
    public final byte[] value;

    public KeyValuePair(byte[] key, byte[] value) {
        this.key = key;
        this.value = value;
    }
}
