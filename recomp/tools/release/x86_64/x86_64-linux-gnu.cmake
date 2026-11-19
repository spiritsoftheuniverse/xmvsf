# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
#
# Linux x86_64 from an arm64 Ubuntu with the amd64 multiarch -dev packages (cps2recomp-build-x86_64).
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER x86_64-linux-gnu-g++)
set(CMAKE_LIBRARY_ARCHITECTURE x86_64-linux-gnu)
set(PKG_CONFIG_EXECUTABLE /usr/bin/pkg-config CACHE FILEPATH "")
set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")
