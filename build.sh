mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_QJS_LIBC=OFF -DPLATFORM=retroppc -DRETRO68_ROOT=$RETRO68_TOOLCHAIN_PATH -DCMAKE_TOOLCHAIN_FILE=$RETRO68_INSTALL_PATH/cmake/retroppc.toolchain.cmake.in &&
make -j$(nproc)