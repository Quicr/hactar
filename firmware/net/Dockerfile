FROM espressif/idf:v5.5
LABEL description="Docker image for net"

RUN apt-get update
RUN apt-get upgrade -y 

RUN apt-get install -y make git curl unzip bash
RUN apt-get install -y pkg-config
RUN apt-get install -y cmake

WORKDIR /project/net

ENV target=compile

CMD ["sh", "-c", "make -j $target WHO=docker"]
