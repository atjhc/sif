FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies and Clang-18 from LLVM's official repo
RUN apt-get update && apt-get install -y \
    wget make \
    software-properties-common && \
    wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 18 && \
    rm llvm.sh && \
    apt-get install -y \
    libc++-18-dev \
    libc++abi-18-dev \
    lldb-18 \
    && apt-get clean

# Set Clang-18 as default
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100 \
    update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-18 100

# Set up a non-root user (optional for security)
# RUN useradd -ms /bin/bash developer
# USER developer
