#!/bin/bash
TOOL_CHAIN_STANDALONE=/home/faywong/libav_prebuilt/ndk_standalone
./configure --target-os=linux \
--enable-cross-compile \
--sysroot=$TOOL_CHAIN_STANDALONE/sysroot \
--cross-prefix=$TOOL_CHAIN_STANDALONE/bin/arm-linux-androideabi- \
--arch=armv6t2 \
--enable-pic \
--extra-cflags="-fPIC -DANDROID" \
--enable-shared \
--disable-static \
--disable-avdevice \
--disable-asm \
--disable-yasm \
--disable-programs
