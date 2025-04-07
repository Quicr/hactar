# This docker images can be built with "docker build -t build-hactar-ui ."
# The docker inamge can be run to build the STM32 binary with "docker run -it -v $PWD/.:/src build-hactar-ui"
FROM ubuntu:22.04
LABEL description="Build Hactar UI"

RUN apt-get update
RUN apt-get install -y build-essential git
RUN apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib

ARG USER_ID=1000
ARG GROUP_ID=1000

RUN addgroup --gid $GROUP_ID compile_user && \
    adduser --uid $USER_ID --gid $GROUP_ID --disabled-password compile_user

USER compile_user

WORKDIR /src/ui

# instead of copy, run with -v ${PWD}/..:/src
# COPY . /src
CMD make compile -j
#CMD /bin/tcsh