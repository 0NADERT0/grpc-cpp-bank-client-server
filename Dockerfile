FROM ubuntu:latest

ENV MY_INSTALL_DIR=/usr/local

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    git \
    pkg-config \
    libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /deps
RUN git clone --recurse-submodules -b v1.71.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc && \
    cd grpc && \
    mkdir -p cmake/build && \
    cd cmake/build && \
    cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_CXX_STANDARD=17 \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../.. && \
    make -j$(nproc) && \
    make install && \
    cd ../../.. && \
    rm -rf grpc


WORKDIR /app
COPY . .

RUN mkdir build && \
    cd build && \
    cmake .. && \
    make

CMD ["./build/server"]