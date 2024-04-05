FROM alpine:3.19 AS builder

WORKDIR /project

RUN apk add --no-cache build-base=0.5-r3
RUN apk add --no-cache cmake=3.27.8-r0
RUN apk add --no-cache mingw-w64-gcc=13.2.0-r2
RUN apk add --no-cache python3=3.11.8-r0 py3-pip=23.3.1-r0
RUN apk add --no-cache nodejs=20.11.1-r0 npm=10.2.5-r0
RUN pip install conan==2.2.1 --break-system-packages
RUN wget https://github.com/bfgroup/b2/releases/download/5.1.0/b2-5.1.0.tar.bz2
RUN tar -xf b2-5.1.0.tar.bz2
RUN (cd b2-5.1.0; ./bootstrap.sh)
RUN (cd b2-5.1.0; ./b2 install --prefix=/usr)

COPY profiles profiles
COPY conanfile.txt conanfile.txt
RUN npm i node-api-headers
RUN mkdir -p node_modules/node-api-headers/lib
RUN x86_64-w64-mingw32-dlltool -d node_modules/node-api-headers/def/node_api.def -l node_modules/node-api-headers/lib/node_api.lib

COPY js/CMakeLists.txt js/CMakeLists.txt
COPY js/binding.cpp js/binding.cpp
COPY js/include js/include
COPY src src
COPY test test
COPY build_all.sh build_all.sh
