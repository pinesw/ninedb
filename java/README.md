```bash
conan install . -of java/build -pr:a profiles/profile_windows-2022_x64_Release.conf

cmake -B java/build \
  -S java \
  -DCMAKE_BUILD_TYPE=Release \
  -DCONAN_PATH="C:\\Users\\Wouter\\workspace\\ninedb/java/build" \
  -DJDK_HOME="C:\\Program Files\\AdoptOpenJDK\\jdk-11.0.9.101-hotspot" \
  -DCMAKE_TOOLCHAIN_FILE="C:\\Users\\Wouter\\workspace\\ninedb/java/build/conan_toolchain.cmake"

cmake --build java/build --config Release
```

```bash
cd java

javac -h src/main/cpp -cp src/main/java src/main/java/io/pinesw/ninedb/KvDatabase.java src/main/java/io/pinesw/ninedb/KvDbIterator.java src/main/java/io/pinesw/ninedb/HrDatabase.java

javac -cp "src/main/java;test/java" test/java/io/pinesw/ninedb/Test.java
java -Djava.library.path=build -cp "src/main/java;test/java" io.pinesw.ninedb.Test
```

```bash
cd java

javap -cp src/main/java -s io.pinesw.ninedb.DbConfig
```
