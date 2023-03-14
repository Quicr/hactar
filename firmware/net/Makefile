PORT 				= COM3
BOARD				= esp8266:esp8266:nodemcuv2
BUILD_PATH			= ../build/net
CACHE_PATH    		= ${BUILD_PATH}/cache
SHARED_INC_DIR		= ../shared_inc
INO_FILE			= net.ino

all: copy compile upload

copy:
	cp -r ${SHARED_INC_DIR} ./

compile:
	arduino-cli compile --build-path ${BUILD_PATH} --build-cache-path ${CACHE_PATH} --fqbn ${BOARD}

program: compile upload

upload:
	arduino-cli upload --port ${PORT} --fqbn ${BOARD} --input-dir ${BUILD_PATH} ${INO_FILE}

monitor:
	arduino-cli monitor --port COM3 --config baudrate=115200 --fqbn ${BOARD}