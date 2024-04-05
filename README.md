## Build

Based on https://docs.conan.io/en/latest/getting_started.html

```shell
mkdir build
cd build
conan install -s build_type=Debug .. --profile ../profile_vc

cmake .. -G "Visual Studio 16"
cmake --build . --config Debug
```
