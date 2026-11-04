# Copyright (c) 2023 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

TOOLCHAIN_GDB ?= $(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)gdb

# Runs the executable
pc-flash: ${TARGET_DIR}/$(TARGETNAME).elf
	@echo "Running $<"
	@$<

# Starts a debug session
pc-debug: ${TARGET_DIR}/$(TARGETNAME).elf
	$(TOOLCHAIN_GDB) \
		-ex "file ${TARGET_DIR}/$(TARGETNAME).elf" \
		-ex "break main" \
		-ex "run" \

