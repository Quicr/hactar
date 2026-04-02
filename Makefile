.PHONY: all clean flash flash-mgmt flash-ui flash-net run-ctl FORCE

CTL_BIN := link/ctl/target/release/ctl
MGMT_BIN := link/mgmt/target/thumbv6m-none-eabi/release/mgmt.bin
UI_BIN := firmware/ui/build/ui.bin
NET_ELF := firmware/net/build/net.elf
NET_PARTITION := firmware/net/build/partition_table/partition-table.bin

# Source file dependencies for Rust crates
CTL_SRCS := $(shell find link/ctl/src -name '*.rs') link/ctl/Cargo.toml
MGMT_SRCS := $(shell find link/mgmt/src -name '*.rs') link/mgmt/Cargo.toml
LINK_SRCS := $(shell find link/link/src -name '*.rs') link/link/Cargo.toml

all: $(CTL_BIN) $(MGMT_BIN) $(UI_BIN) $(NET_ELF)

$(CTL_BIN): $(CTL_SRCS) $(LINK_SRCS)
	cd link/ctl && cargo build --release

$(MGMT_BIN): $(MGMT_SRCS) $(LINK_SRCS)
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
