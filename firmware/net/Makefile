BAUD 				= 115200
PORT 				= /dev/ttyUSB0
BUILD_PATH			= ./build
TARGET				= net

BOOTLOADER_ADDR		= 0x00000
PARITIONS_ADDR		= 0x08000
PROG_ADDR			= 0x10000

ESP_TOOL_ARGS 		= --port=${PORT} --baud=${BAUD} write_flash \
	${BOOTLOADER_ADDR} ${BUILD_PATH}/bootloader/bootloader.bin \
	${PARITIONS_ADDR} ${BUILD_PATH}/partition_table/partition-table.bin \
	${PROG_ADDR} ${BUILD_PATH}/${TARGET}.bin

all: compile upload

compile:
	idf.py build

upload:
	python ../tools/command_sender.py ${PORT} ${BAUD} net_upload
	if [ ${OS} == Windows_NT ] ; then \
		python -m esptool ${ESP_TOOL_ARGS} ; \
	else \
		esptool.py ${ESP_TOOL_ARGS} ; \
	fi \

program: compile upload

monitor:
	python ../tools/command_sender.py ${PORT} ${BAUD} debug
	screen ${PORT} ${BAUD}

clean:
	rm -rf ${BUILD_PATH}
