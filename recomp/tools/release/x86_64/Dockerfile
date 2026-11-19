# Native-speed Linux x86_64 release builds on Apple silicon: an arm64 Ubuntu 24.04 (the same release as
# tools/release/Dockerfile: glibc 2.39, GCC 13) with the x86_64 cross compilers and the amd64 (multiarch) builds of
# the same -dev packages, so the compiler runs natively and only the output is x86_64.
#   docker build -t cps2recomp-build-x86_64 tools/release/x86_64
#   docker run --rm -v "$PWD":/src -w /src cps2recomp-build-x86_64 tools/release/linux.sh
# The release scripts ask `uname -m` which processor to build for and call plain `strip` and `cargo build`: the
# wrappers in /opt/xbuild/bin (first on PATH) answer x86_64 and use the x86_64 tools; CMake takes the toolchain file
# from CMAKE_TOOLCHAIN_FILE (environment: every new build folder, SDL 3's included).
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN dpkg --add-architecture amd64 \
 && sed -i "/^Types: deb/a Architectures: arm64" /etc/apt/sources.list.d/ubuntu.sources \
 && printf "Types: deb\nURIs: http://archive.ubuntu.com/ubuntu/\nSuites: noble noble-updates noble-backports\nComponents: main universe restricted multiverse\nArchitectures: amd64\nSigned-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n\nTypes: deb\nURIs: http://security.ubuntu.com/ubuntu/\nSuites: noble-security\nComponents: main universe restricted multiverse\nArchitectures: amd64\nSigned-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg\n" > /etc/apt/sources.list.d/amd64.sources \
 && apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates cmake ninja-build ccache python3 curl file zip unzip git pkgconf \
    gcc libc6-dev crossbuild-essential-amd64 libwayland-bin \
    zlib1g-dev:amd64 liblzma-dev:amd64 \
    libx11-dev:amd64 libxext-dev:amd64 libxrandr-dev:amd64 libxcursor-dev:amd64 libxi-dev:amd64 libxss-dev:amd64 \
    libxtst-dev:amd64 libxkbcommon-dev:amd64 libwayland-dev:amd64 wayland-protocols libdecor-0-dev:amd64 \
    libegl-dev:amd64 libgl-dev:amd64 libpulse-dev:amd64 libasound2-dev:amd64 libudev-dev:amd64 \
    libdbus-1-dev:amd64 libpipewire-0.3-dev:amd64 \
 && rm -rf /var/lib/apt/lists/*
# (the static C++ and GCC runtimes linked into the programs (-static-libstdc++ -static-libgcc) and GCC's start files:
# the amd64 packages' own, which tools/release/Dockerfile links, in place of the cross packages' builds of the same version)
RUN cd /tmp && apt-get update && apt-get download libstdc++-13-dev:amd64 libgcc-13-dev:amd64 \
 && for d in *.deb; do dpkg -x "$d" /opt/amd64-gcc; done && rm -f *.deb && rm -rf /var/lib/apt/lists/* \
 && N=/opt/amd64-gcc/usr/lib/gcc/x86_64-linux-gnu/13 X=/usr/lib/gcc-cross/x86_64-linux-gnu/13 \
 && for f in "$N"/*.a "$N"/*.o; do [ -e "$X/${f##*/}" ] && ln -sf "$f" "$X/${f##*/}"; done
# (Rust for librashader, as tools/release/Dockerfile has it, plus its x86_64 Linux target)
ENV RUSTUP_HOME=/opt/rustup CARGO_HOME=/opt/cargo
RUN apt-get update && apt-get install -y --no-install-recommends rustup && rm -rf /var/lib/apt/lists/* \
 && rustup toolchain install 1.99.1 --profile minimal --no-self-update \
 && rustup default 1.99.1 \
 && rustup target add x86_64-unknown-linux-gnu
COPY x86_64-linux-gnu.cmake /opt/xbuild/
COPY bin/ /opt/xbuild/bin/
ENV PATH=/opt/xbuild/bin:/opt/cargo/bin:$PATH \
    CMAKE_TOOLCHAIN_FILE=/opt/xbuild/x86_64-linux-gnu.cmake \
    PKG_CONFIG_LIBDIR=/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig \
    CARGO_BUILD_TARGET=x86_64-unknown-linux-gnu \
    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=x86_64-linux-gnu-gcc \
    CC_x86_64_unknown_linux_gnu=x86_64-linux-gnu-gcc CXX_x86_64_unknown_linux_gnu=x86_64-linux-gnu-g++ \
    AR_x86_64_unknown_linux_gnu=x86_64-linux-gnu-ar
