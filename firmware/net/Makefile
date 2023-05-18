PORT 				= /dev/ttyUSB0
CORE 				= esp32:esp32
CHIP				= esp32s3
BOARD				= ${CORE}:${CHIP}
OPTIONS				= :UploadSpeed=115200
BUILD_PATH			= ../build/net
CACHE_PATH    		= ${BUILD_PATH}/cache
SHARED_INC_DIR		= ../shared_inc
INO_FILE			= net.ino

UPLOAD_BAUD 		= 40000
UPLOAD_START_ADDR	= 0x8000

all: compile upload

copy:
	cp -r ${SHARED_INC_DIR} ./

compile: copy
	arduino-cli compile --build-path ${BUILD_PATH} --build-cache-path ${CACHE_PATH} --fqbn ${BOARD}

upload:
	esptool.py --port=${PORT} --baud=${UPLOAD_BAUD} --chip=${CHIP} write_flash ${UPLOAD_START_ADDR} ${BUILD_PATH}/net.ino.bin
# arduino-cli upload --port ${PORT} --fqbn ${BOARD}${OPTIONS} --input-dir ${BUILD_PATH} ${INO_FILE}

program: compile upload

monitor:
	arduino-cli monitor --port ${PORT} --config baudrate=115200 --fqbn ${BOARD}

install_board:
	arduino-cli core update-index
	arduino-cli core install ${CORE}

clean:
	rm -rf ${BUILD_PATH}
