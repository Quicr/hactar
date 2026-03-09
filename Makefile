.PHONY: all clean flash flash-mgmt flash-ui flash-net run-ctl FORCE

CTL_BIN := link/ctl/target/release/ctl
MGMT_BIN := link/mgmt/target/thumbv6m-none-eabi/release/mgmt.bin
UI_BIN := firmware/ui/build/ui.bin
NET_ELF := firmware/net/build/net.elf
NET_PARTITION := firmware/net/build/partition_table/partition-table.bin

all: $(CTL_BIN) $(MGMT_BIN) $(UI_BIN) $(NET_ELF)

$(CTL_BIN):
	cd link/ctl && cargo build --release

$(MGMT_BIN):
	cd link/mgmt && cargo objcopy --release -- -O binary target/thumbv6m-none-eabi/release/mgmt.bin

$(UI_BIN): FORCE
	$(MAKE) -C firmware/ui compile

FORCE:

$(NET_ELF): FORCE
	$(MAKE) -C firmware/net compile

flash: flash-mgmt flash-ui flash-net

flash-mgmt: $(CTL_BIN) $(MGMT_BIN)
	$(CTL_BIN) mgmt flash $(MGMT_BIN)

flash-ui: $(CTL_BIN) $(UI_BIN)
	$(CTL_BIN) ui flash $(UI_BIN)

flash-net: $(CTL_BIN) $(NET_ELF)
	$(CTL_BIN) net flash --partition-table $(NET_PARTITION) $(NET_ELF)

run-ctl: $(CTL_BIN)
	$(CTL_BIN)

clean:
	cd link/ctl && cargo clean
	cd link/mgmt && cargo clean
	$(MAKE) -C firmware/ui clean
	$(MAKE) -C firmware/net clean
