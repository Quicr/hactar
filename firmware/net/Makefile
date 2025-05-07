SHELL := /bin/bash
.ONESHELL:

BAUD 				= 115200
PORT 				= /dev/ttyUSB0
BUILD_PATH			= ./build
TARGET				= net

BOOTLOADER_ADDR		= 0x00000
PARITIONS_ADDR		= 0x08000
PROG_ADDR			= 0x10000

DOCKER_IMAGE 		= build-hactar-net
mft					= compile

ESP_TOOL_ARGS 		= --trace --no-stub --port=${PORT} --baud=${BAUD} write_flash --no-compress \
	${BOOTLOADER_ADDR} ${BUILD_PATH}/bootloader/bootloader.bin \
	${PARITIONS_ADDR} ${BUILD_PATH}/partition_table/partition-table.bin \
	${PROG_ADDR} ${BUILD_PATH}/${TARGET}.bin

all: compile

debug:
	id
	groups
	docker info

compile:
	idf.py -B $(BUILD_PATH) build

docker: docker-build
	@docker run --rm -e target=${mft} --privileged \
		-v ${PWD}/..:/project \
		-v /dev:/dev \
		-it ${DOCKER_IMAGE}

docker-build:
	@if ! docker image inspect ${DOCKER_IMAGE} > /dev/null 2>&1; then \
		docker build -t ${DOCKER_IMAGE} .; \
	fi

init_upload: compile
	openocd -f board/esp32s3-ftdi.cfg -c "program_esp ${BUILD_PATH}/bootloader/bootloader.bin ${BOOTLOADER_ADDR} verify exit"
	openocd -f board/esp32s3-ftdi.cfg -c "program_esp ${BUILD_PATH}/partition_table/partition-table.bin ${PARITIONS_ADDR} verify exit"

upload: compile
	openocd -f board/esp32s3-ftdi.cfg -c "program_esp ./build/net.bin 0x10000 verify exit"
	python3 ../tools/command_sender.py ${CONSOLE_PORT} 115200 debug

upload_py: compile
	python3 ../flasher/flasher.py  --baud=${BAUD} --chip="net" -bin=${BUILD_PATH}

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

format:
	find core -iname "*.cc" | xargs clang-format -i
	find core -iname "*.hh" | xargs clang-format -i
	find components -path components/quicr -prune -or -path components/nlohmann_json -prune -or -iname "*.cc" -print | xargs clang-format -i
	find components -path components/quicr -prune -or -path components/nlohmann_json -prune -or -iname "*.h*" -print | xargs clang-format -i
	find ../shared -iname "*.c" -or -iname "*.cc" | xargs clang-format -i
	find ../shared_inc -iname "*.h" -or -iname "*.hh" | xargs clang-format -i

clean:
	rm -rf ${BUILD_PATH}

fullclean:
	idf.py fullclean

docker-clean:
	-docker image rm ${DOCKER_IMAGE}
