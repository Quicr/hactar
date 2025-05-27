# This docker images can be built with "docker build -t build-hactar-ui ."
# The docker inamge can be run to build the STM32 binary with "docker run -it -v $PWD/.:/src build-hactar-ui"
FROM ubuntu:22.04
LABEL description="Docker image for mgmt"

RUN apt-get update
RUN apt-get install -y build-essential git gcc-arm-none-eabi binutils-arm-none-eabi \
    libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib stlink-tools python3 pip

RUN pip install pyserial

RUN addgroup --gid 1000 compile_user && \
    adduser --uid 1000 --gid 1000 --disabled-password compile_user

USER compile_user

WORKDIR /project/mgmt

ENV target=compile

CMD ["sh", "-c", "make -j $target"]
