FROM debian:bookworm

ENV DEBIAN_FRONTEND=noninteractive
ENV PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig
ENV PKG_CONFIG_SYSROOT_DIR=/
RUN dpkg --add-architecture arm64 \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        make \
        git \
        g++-aarch64-linux-gnu \
        protobuf-compiler \
        protobuf-compiler-grpc \
        libsdl2-dev:arm64 \
        libgrpc++-dev:arm64 \
        libprotobuf-dev:arm64 \
        libc-ares-dev:arm64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
CMD ["bash", "-c", "cmake -S apps/nexus-ui -B build/docker/arm64/ui -DCMAKE_BUILD_TYPE=Release -DNEXUS_UI_FBDEV=ON -DNEXUS_PROTOC_EXECUTABLE=/usr/bin/protoc -DNEXUS_GRPC_CPP_PLUGIN_EXECUTABLE=/usr/bin/grpc_cpp_plugin -DCMAKE_TOOLCHAIN_FILE=$PWD/apps/nexus-ui/cmake/aarch64-linux-gnu-toolchain.cmake -DSDL2_DIR=/usr/lib/aarch64-linux-gnu/cmake/SDL2 -DProtobuf_DIR=/usr/lib/aarch64-linux-gnu/cmake/protobuf -DgRPC_DIR=/usr/lib/aarch64-linux-gnu/cmake/grpc && cmake --build build/docker/arm64/ui --parallel ${BUILD_JOBS:-2} && mkdir -p build/docker/arm64/nexus-r36s-bundle/lib && install -m 0755 build/docker/arm64/ui/nexus-ui build/docker/arm64/nexus-r36s-bundle/nexus-ui"]
