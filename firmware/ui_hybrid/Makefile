# This is just a convenience Makefile to avoid having to remember
# all the CMake commands and their arguments.

# Set CMAKE_GENERATOR in the environment to select how you build, e.g.:
#   CMAKE_GENERATOR=Ninja

BUILD_DIR=build
TOOLCHAIN_FILE=etc/toolchain.cmake
UI_RS=ui_rs
UI_RS_LIB=${UI-RS}/target/thumbv7em-none-eabihf/debug/libui_rs.a
FIRMWARE=build/ui_hybrid.elf
C_DIR=../ui

.PHONY: clean cclean

all: ${FIRMWARE}

${UI_RS_LIB}: ${UI_RS}/Cargo.toml ${UI_RS}/src/**
	cd ${UI_RS} && cargo build

${BUILD_DIR}: CMakeLists.txt ${C_DIR}/CMakeLists.txt
	cmake -B${BUILD_DIR} -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}

${FIRMWARE}: ${UI_RS_LIB} ${BUILD_DIR}
	cmake --build ${BUILD_DIR}

upload: ${FIRMWARE}
	STM32_Programmer_CLI \
		-c port=swd freq=2400 mode=normal ap=0 speed=Reliable \
		-w ${FIRMWARE} \
		0x08000000 -v -g

clean:
	cd ${UI_RS} && cargo clean
	cmake --build ${BUILD_DIR} --target clean

cclean:
	cd ${UI_RS} && cargo clean
	rm -rf ${BUILD_DIR}
