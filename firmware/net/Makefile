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

all: compile upload

idf-clean:
	idf.py fullclean

compile:
	idf.py build

upload:
	python ../flasher/flasher.py  --baud=${BAUD} --chip="net" \
								 --net_build_path=${BUILD_PATH}

upload_esptool:
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

py_monitor:
	python ../tools/monitor.py ${PORT} ${BAUD}

clean:
	rm -rf ${BUILD_PATH}
