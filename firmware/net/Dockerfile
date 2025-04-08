FROM espressif/idf:v5.4
LABEL description="Docker image for net"

RUN apt-get update && apt-get install -y make git curl unzip cmake bash pkg-config

WORKDIR /project/net

ENV target=compile

CMD make $target