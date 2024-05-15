Generate header files using the following command:

```bash
javac -h java/src/main/cpp \
  -cp java/src/main/java \
  java/src/main/java/io/woutervh/ninedb/KvDatabase.java \
  java/src/main/java/io/woutervh/ninedb/KvDbIterator.java \
  java/src/main/java/io/woutervh/ninedb/HrDatabase.java
```
