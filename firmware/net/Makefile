PORT 				= COM11
CORE 				= esp32:esp32
CHIP				= esp32s3
BOARD				= ${CORE}:${CHIP}
OPTIONS				= :UploadSpeed=115200
BUILD_PATH			= ../build/net
CACHE_PATH    		= ${BUILD_PATH}/cache
SHARED_INC_DIR		= ../shared_inc
INO_FILE			= net.ino

ARDUINO_DIR			= /home/brett/.arduino15
ARDUINO_DIR			= C:/Users/Brett/AppData/local/Arduino15
ESP_VERSION			= 2.0.9
ESP_VERSION			= 2.0.7
ESP_BOOT_APP_DIR 	= ${ARDUINO_DIR}/packages/esp32/hardware/esp32/${ESP_VERSION}/tools/partitions/boot_app0.bin

UPLOAD_BAUD 		= 30000
# UPLOAD_BAUD 		= 57600
BOOTLOADER_ADDR		= 0x00000
PARITIONS_ADDR		= 0x08000
BOOT_APP_ADDR		= 0x0e000
PROG_ADDR			= 0x10000

ESP_TOOL_ARGS 		= --port=${PORT} --baud=${UPLOAD_BAUD} write_flash \
	${BOOTLOADER_ADDR} ${BUILD_PATH}/net.ino.bootloader.bin \
	${PARITIONS_ADDR} ${BUILD_PATH}/net.ino.partitions.bin \
	${BOOT_APP_ADDR} ${ESP_BOOT_APP_DIR} \
	${PROG_ADDR} ${BUILD_PATH}/net.ino.bin

all: compile upload

copy:
	cp -r ${SHARED_INC_DIR} ./

compile: copy
	arduino-cli compile --warnings all --build-path ${BUILD_PATH} --build-cache-path ${CACHE_PATH} --fqbn ${BOARD}

upload:
	if [ ${OS} == Windows_NT ] ; then \
		python -m esptool ${ESP_TOOL_ARGS} ; \
	else \
		esptool ${ESP_TOOL_ARGS} ; \
	fi \

# esptool --port=${PORT} --baud=${UPLOAD_BAUD} write_flash \
# 	${BOOTLOADER_ADDR} ${BUILD_PATH}/net.ino.bootloader.bin \
# 	${PARITIONS_ADDR} ${BUILD_PATH}/net.ino.partitions.bin \
# 	${BOOT_APP_ADDR} ${ESP_BOOT_APP_DIR} \
# 	${PROG_ADDR} ${BUILD_PATH}/net.ino.bin
# arduino-cli upload -v --port ${PORT} --fqbn ${BOARD}${OPTIONS} --input-dir ${BUILD_PATH} ${INO_FILE}

program: compile upload

monitor:
	arduino-cli monitor --port ${PORT} --config baudrate=115200 --fqbn ${BOARD}

install_board:
	arduino-cli core update-index
	arduino-cli core install ${CORE}

clean:
	rm -rf ${BUILD_PATH}
