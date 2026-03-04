.PHONY: all clean

CTL_BIN := link/ctl/target/release/ctl
MGMT_BIN := link/mgmt/target/thumbv6m-none-eabi/release/mgmt
UI_BIN := firmware/ui/build/ui.bin
NET_BIN := firmware/net/build/net.bin

all: $(CTL_BIN) $(MGMT_BIN) $(UI_BIN) $(NET_BIN)

$(CTL_BIN):
	cd link/ctl && cargo build --release

$(MGMT_BIN):
	cd link/mgmt && cargo build --release

$(UI_BIN):
	$(MAKE) -C firmware/ui compile

$(NET_BIN):
	$(MAKE) -C firmware/net compile

clean:
	cd link/ctl && cargo clean
	cd link/mgmt && cargo clean
	$(MAKE) -C firmware/ui clean
	$(MAKE) -C firmware/net clean
