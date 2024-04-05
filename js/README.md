# How to

## Conan + cmake-js

### Prerequisites

- Conan
- CMake
- C++ compiler
- cmake-js

### Conan

Run conan install:

```bash
cd build/
conan install -s build_type=Release ..
```

### cmake-js

```bash
cmake-js configure # only needed once, add -D for debug
cmake-js build # add -D for debug
```

### One-liners

Complete reinstall/rebuild:

```bash
rm -rf build/; mkdir build/; cd build/; conan install -s build_type=Release ..; cd ..; cmake-js build
```

## Conan + node-gyp

*Not working: probably due to debug DLLs being used in the build bin.*

### Conan

Configure generator which will create the gyp file necessary for `node-gyp`:

```bash
conan config install gyp-generator.py -tf generators
```

Run conan install:

```bash
cd build/
conan install -s build_type=Release ..
```

### For default (msvc)

Configure and build using `node-gyp`:

```bash
node-gyp configure
node-gyp build
```

### For cmake

*Currently does not work. Below is an attempt to make it work.*

```bash
# node-gyp configure -- -f cmake
```

```bash
# cd build/Release
# cmake . -G "Visual Studio 16"
# cmake --build . --config Release
```

### Debug Node in Chrome

```bash
node --inspect-brk index.js
```

Then open `chrome://inspect`

### One-liners

Rebuild project:

```bash
rm -rf build; mkdir build; cd build; conan install ..; cd ..; node-gyp configure; node-gyp build;
```

# TODO

- Configure GHA to compile artifacts for every (popular) OS/architecture.
