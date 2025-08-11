FROM ubuntu:24.04

# Build tools + nice-to-haves for systems work
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential gcc g++ clang make cmake pkg-config \
    gdb strace valgrind \
    libreadline-dev libedit-dev \
    git curl ca-certificates manpages-dev \
  && rm -rf /var/lib/apt/lists/*

# Create a non-root user that matches your host UID/GID at build time
ARG UID=1000
ARG GID=1000
RUN useradd -m -u ${UID} -g ${GID} -s /bin/bash dev

WORKDIR /work
USER dev
ENV HOME=/work
