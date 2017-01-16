#!/bin/bash
set -euo pipefail
set -o xtrace

if [[ ! -x .deps/bin/ninja ]]; then
    mkdir -p .deps/bin
    pushd .deps/bin
    curl -fLO https://github.com/ninja-build/ninja/releases/download/v1.7.2/ninja-linux.zip
    unzip ninja-linux.zip
    ninja --version
    rm ninja-linux.zip
    popd
fi

if [[ ! -x .deps/bin/cmake ]]; then
    mkdir -p .deps
    pushd .deps
    curl -fLO https://cmake.org/files/v3.7/cmake-3.7.2-Linux-x86_64.tar.gz
    tar -xf cmake-3.7.2-Linux-x86_64.tar.gz --strip-components=1
    cmake --version
    rm cmake-3.7.2-Linux-x86_64.tar.gz
    popd
fi

libcxx_msan_install_prefix=$(pwd)/.deps/opt/libcxx-msan-3.9.1
if [[ ${ENABLE_MSAN:-0} -eq 1 && ! -d "$libcxx_msan_install_prefix" ]]; then
    work_dir=$(mktemp -d -t libcxx_build.XXXXX)
    pushd "$work_dir"

    curl -fLO 'http://releases.llvm.org/3.9.1/llvm-3.9.1.src.tar.xz'
    curl -fLO 'http://releases.llvm.org/3.9.1/libcxx-3.9.1.src.tar.xz'
    curl -fLO 'http://releases.llvm.org/3.9.1/libcxxabi-3.9.1.src.tar.xz'

    mkdir -p src src/projects/libcxx src/projects/libcxxabi
    tar xf llvm-3.9.1.src.tar.xz --strip-components=1 -C src
    tar xf libcxx-3.9.1.src.tar.xz --strip-components=1 -C src/projects/libcxx
    tar xf libcxxabi-3.9.1.src.tar.xz --strip-components=1 -C src/projects/libcxxabi

    mkdir -p build
    pushd build
    env CFLAGS= CXXFLAGS= LDFLAGS= cmake                       \
        ../src                                                 \
        -DCMAKE_C_COMPILER="${C_COMPILER}"                     \
        -DCMAKE_CXX_COMPILER="${COMPILER}"                     \
        -DCMAKE_BUILD_TYPE=Release                             \
        -DCMAKE_INSTALL_PREFIX="${libcxx_msan_install_prefix}" \
        -DLLVM_USE_SANITIZER=MemoryWithOrigins                 \
        -GNinja
    ninja -j2 cxx
    ninja -j2 install-libcxx install-libcxxabi
    popd

    popd
    rm -fr "$work_dir"
fi
