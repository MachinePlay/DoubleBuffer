# syntax=docker/dockerfile:1
FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai
RUN /bin/sh -c 'apt update && apt install build-essential bison flex vim make git gcc-10 g++-10 git  psmisc libncurses5-dev zlib1g-dev python3 autoconf automake libtool curl  unzip cmake -y\
&& update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 90 --slave /usr/bin/g++ g++ /usr/bin/g++-10 --slave /usr/bin/gcov gcov /usr/bin/gcov-10'
WORKDIR /app
COPY . .
CMD ["gcc -v"]