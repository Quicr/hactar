port=

all: upload_serial

upload_serial: check_arg check_idf upload_mgmt_serial upload_ui_serial upload_net_serial

upload_debugger: check_idf upload_mgmt_debugger upload_ui_debugger upload_net_debugger 

upload_mgmt_serial:
	cd mgmt && ${MAKE} upload port=${port}

upload_ui_serial:
	cd ui && ${MAKE} upload

upload_net_serial:
	cd net && ${MAKE} upload

upload_mgmt_debugger:
	cd mgmt && ${MAKE} upload_debugger

upload_ui_debugger:
	cd ui && ${MAKE} upload_debugger

upload_net_debugger:
	cd net && ${MAKE} upload_debugger

check_arg:
	@if [ -z "${port}" ]; then \
		echo "port is required. Usage: make build port=/dev/ttyUSBx"; \
		exit 1; \
	fi

check_idf:
	@command -v idf.py >/dev/null 2>&1 || { \
		echo "ERROR: espressif idf.py not found on your path, please run their export script"; \
		exit 1; \
	}
