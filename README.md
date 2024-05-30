```bash
conan install . --output-folder=build --build=missing
cmake . -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug
```
