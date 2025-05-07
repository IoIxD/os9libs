# OpenGL on Mac OS 9

Getting OpenGL going on Mac OS 9 under Retro68, without glut, sucks.

This repo fixes that by:

- Provides the proper stub libraries for use with Retro68 (`lib`)
- Provides the header files from Apple's original OpenGL SDK (`include`)
- Provides an example on how to use OpenGL using glut, using Apple's "agl" functions instead (`examples`)

To build said example:

- set `$RETRO68_TOOLCHAIN_PATH` to the path of your Retro68 toolchain
- set `$RETRO68_INSTALL_PATH` to the cloned Retro68 repo.
- run build.sh to build for both 68k and PowerPC.
