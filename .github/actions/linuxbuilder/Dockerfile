FROM ubuntu:18.04

RUN apt-get update && apt-get install --no-install-recommends -y \
  build-essential \
  cmake \
  libpcap-dev \
  libpng-dev \
  libsdl2-dev \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

COPY build.sh /
