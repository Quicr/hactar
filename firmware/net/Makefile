PORT 				= /dev/ttyUSB0
CORE 				= esp8266:esp8266
BOARD				= ${CORE}:nodemcuv2
BUILD_PATH			= ../build/net
CACHE_PATH    		= ${BUILD_PATH}/cache
SHARED_INC_DIR		= ../shared_inc
INO_FILE			= net.ino

all: copy compile upload

copy:
	cp -r ${SHARED_INC_DIR} ./

compile: copy
	arduino-cli compile --build-path ${BUILD_PATH} --build-cache-path ${CACHE_PATH} --fqbn ${BOARD}

program: compile upload

upload:
	arduino-cli upload --port ${PORT} --fqbn ${BOARD} --input-dir ${BUILD_PATH} ${INO_FILE}

monitor:
	arduino-cli monitor --port ${PORT} --config baudrate=115200 --fqbn ${BOARD}

install_board:
	arduino-cli core update-index
	arduino-cli core install ${CORE}

clean:
	rm -rf ${BUILD_PATH}