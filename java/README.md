## Installation

Central Sonatype page: https://central.sonatype.com/artifact/io.github.pinesw/ninedb

### Maven

Add to the dependencies section of your `pom.xml`:

```xml
<dependency>
    <groupId>io.github.pinesw</groupId>
    <artifactId>ninedb</artifactId>
    <version>VERSION</version>
</dependency>
```

### Gradle

Add to the dependencies section of your `build.gradle`:

```
implementation group: 'io.github.pinesw', name: 'ninedb', version: 'VERSION'
```

## Usage

### Example

```java
import io.pinesw.ninedb.DbConfig;
import io.pinesw.ninedb.KvDatabase;
import io.pinesw.ninedb.KvDbIterator;

public class Main {

    public static void main(String[] args) {
        DbConfig config = new DbConfig();
        
        try (KvDatabase db = KvDatabase.open("data/my-database", config)) {
            db.add("key_1".getBytes(), "value_1".getBytes());
            db.add("key_2".getBytes(), "value_2".getBytes());
            db.add("key_3".getBytes(), "value_3".getBytes());
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
}
```
