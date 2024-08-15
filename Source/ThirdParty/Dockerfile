# get ubuntu 18.04 image
FROM ubuntu:18.04

# install development tools
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    gpg \
    wget \
    ca-certificates \
    libz-dev \
    libc++-dev \
    libc++abi-dev \
    libcurl4-openssl-dev

# we now need to install a newer version of cmake
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
RUN echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null
RUN apt-get update && apt-get install -y cmake

