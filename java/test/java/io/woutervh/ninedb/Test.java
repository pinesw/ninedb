package io.woutervh.ninedb;

public class Test {
    public static void TestKvDb() {
        DbConfig config = new DbConfig();
        config.deleteIfExists = true;
        config.maxNodeEntries = 3;
        config.reduce = (byte[][] keys) -> {
            return keys[0];
        };

        try (KvDatabase db = KvDatabase.open("data/test-kvdb", config)) {
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

            try (KvDbIterator it = db.begin()) {
                while (it.hasNext()) {
                    byte[] key = it.getKey();
                    byte[] value = it.getValue();
                    System.out.println(new String(key) + " -> " + new String(value));
                    it.next();
                }
            }
        }
    }

    public static void TestHrDb() {
        DbConfig config = new DbConfig();
        config.deleteIfExists = true;
        config.maxNodeEntries = 3;

        try (HrDatabase db = HrDatabase.open("data/test-hrdb", config)) {
            db.add(0l, 0l, 0l, 0l, "value_1".getBytes());
            db.add(0l, 0l, 1l, 1l, "value_2".getBytes());
            db.add(0l, 0l, 10l, 10l, "value_3".getBytes());
            db.add(0l, 0l, 100l, 100l, "value_4".getBytes());
            db.add(1l, 1l, 0l, 0l, "value_5".getBytes());
            db.add(1l, 1l, 1l, 1l, "value_6".getBytes());
            db.add(1l, 1l, 10l, 10l, "value_7".getBytes());
            db.add(1l, 1l, 100l, 100l, "value_8".getBytes());
            db.add(10l, 10l, 0l, 0l, "value_9".getBytes());
            db.add(10l, 10l, 1l, 1l, "value_10".getBytes());
            db.flush();

            byte[][] results = db.search(0l, 0l, 100l, 100l);
            for (byte[] result : results) {
                System.out.println(new String(result));
            }
        }
    }

    public static void main(String[] args) {
        System.out.println("Testing HrDatabase");
        TestHrDb();
        
        System.out.println("Testing KvDatabase");
        TestKvDb();
    }
}
