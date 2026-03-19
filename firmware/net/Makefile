SHELL := /bin/bash
.ONESHELL:

BAUD 				= 115200
port 				=
BUILD_PATH			= ./build
TARGET				= net

BOOTLOADER_ADDR		= 0x00000
PARITIONS_ADDR		= 0x08000
PROG_ADDR			= 0x10000

DOCKER_IMAGE 		= build-hactar-net
mft					= compile

CHECK_FILE 			= $(BUILD_PATH)/$(TARGET)_who.txt
WHO 				= local

ESP_TOOL_ARGS 		= --trace --no-stub --port=${port} --baud=${BAUD} write_flash --no-compress \
	${BOOTLOADER_ADDR} ${BUILD_PATH}/bootloader/bootloader.bin \
	${PARITIONS_ADDR} ${BUILD_PATH}/partition_table/partition-table.bin \
	${PROG_ADDR} ${BUILD_PATH}/${TARGET}.bin

.PHONY: compile upload upload_debugger upload_esptool docker monitor

all: compile

compile: check_who_compiled
	idf.py -B $(BUILD_PATH) build -DCMAKE_POLICY_VERSION_MINIMUM=3.5

upload: compile
	python3 ../../software/hactar-cli/main.py flash --port=${port} --chip="net" -bin=${BUILD_PATH}

upload_debugger: compile
	openocd -f board/esp32s3-ftdi.cfg -c "program_esp ${BUILD_PATH}/bootloader/bootloader.bin ${BOOTLOADER_ADDR} verify exit"
	openocd -f board/esp32s3-ftdi.cfg -c "program_esp ${BUILD_PATH}/partition_table/partition-table.bin ${PARITIONS_ADDR} verify exit"
	openocd -f board/esp32s3-ftdi.cfg -c "program_esp ./build/net.bin 0x10000 verify exit"

upload_esptool:
	python3 ../../software/hactar-cli/main.py flash --port=${port} --chip="net" --use_external_flasher=true
	esptool.py ${ESP_TOOL_ARGS}

docker:
	@if ! docker image inspect ${DOCKER_IMAGE} > /dev/null 2>&1; then \
		docker build -t ${DOCKER_IMAGE} .; \
	fi

	@if [ "$(shell uname -s)" = "Darwin" ]; then \
		docker run --rm -e target=${mft} --privileged \
			-v ${PWD}/..:/project \
			-v /dev:/dev \
			-it ${DOCKER_IMAGE}; \
	else \
		docker run --user $(shell id -u):$(shell id -g) --rm -e target=${mft} --privileged \
			-v ${PWD}/..:/project \
			-v /dev:/dev \
			-it ${DOCKER_IMAGE}; \
	fi

burn_jtag_efuse:
	python3 ../../software/hactar-cli/main.py flash --port=${port} --chip="net" --use_external_flasher=true
	idf.py efuse-burn DIS_USB_JTAG

monitor:
	python3 ../../software/hactar-cli/main.py monitor --port=${port}

idf_monitor:
	python3 ../../software/hactar-cli/main.py flash --port=${port} --chip="net" --use_external_flasher=true
	idf.py monitor

format:
	find core -iname "*.cc" | xargs clang-format -i
	find core -iname "*.hh" | xargs clang-format -i
	find components -path components/quicr -prune -or -path components/nlohmann_json -prune -or -iname "*.cc" -print | xargs clang-format -i
	find components -path components/quicr -prune -or -path components/nlohmann_json -prune -or -iname "*.h*" -print | xargs clang-format -i
	find ../shared -iname "*.c" -or -iname "*.cc" | xargs clang-format -i
	find ../shared_inc -iname "*.h" -or -iname "*.hh" | xargs clang-format -i

check_who_compiled:
	@if [ -f "${CHECK_FILE}" ]; then \
		who=$$(head  -n 1 ${CHECK_FILE});  \
		if [ "$$who" != ${WHO} ]; then  \
			rm -rf ${BUILD_PATH}; \
			mkdir -p ${BUILD_PATH};   \
			touch ${CHECK_FILE};   \
			echo  ${WHO} > ${CHECK_FILE};   \
		fi    \
	else    \
		rm -rf ${BUILD_PATH}; \
		mkdir -p ${BUILD_PATH};   \
		touch ${CHECK_FILE};   \
		echo  ${WHO} > ${CHECK_FILE};   \
	fi

clean:
	rm -rf ${BUILD_PATH}

fullclean:
	idf.py fullclean

docker-clean:
	-docker image rm ${DOCKER_IMAGE}
