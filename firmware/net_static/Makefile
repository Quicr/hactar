BAUD 				= 115200
PORT 				= /dev/ttyUSB0
BUILD_PATH			= ./build
TARGET				= net

BOOTLOADER_ADDR		= 0x00000
PARITIONS_ADDR		= 0x08000
PROG_ADDR		= 0x10000

ESP_TOOL_ARGS 		= --trace --no-stub --port=${PORT} --baud=${BAUD} write_flash --no-compress \
	${BOOTLOADER_ADDR} ${BUILD_PATH}/bootloader/bootloader.bin \
	${PARITIONS_ADDR} ${BUILD_PATH}/partition_table/partition-table.bin \
	${PROG_ADDR} ${BUILD_PATH}/${TARGET}.bin

all: compile

compile:
	idf.py build

upload: compile
	python3 ../flasher/flasher.py  --baud=${BAUD} --chip="net" \
								 -bin=${BUILD_PATH} --port=/dev/ttyUSB0

upload_esptool:
	python3 ../tools/command_sender.py ${PORT} ${BAUD} net_upload
	if [ ${OS} == Windows_NT ] ; then \
		python3 -m esptool ${ESP_TOOL_ARGS} ; \
	else \
		esptool.py ${ESP_TOOL_ARGS} ; \
	fi \

program: compile upload

monitor:
	python3 ../tools/command_sender.py ${PORT} ${BAUD} net_debug
	idf.py monitor

py_monitor:
	python3 ../tools/monitor.py ${PORT} ${BAUD}

clean:
	rm -rf ${BUILD_PATH}

fullclean:
	idf.py fullclean