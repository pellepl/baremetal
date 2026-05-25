# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

### STM device utilities using STM32CubeProgrammer
# If DEVICE is defined, there must be only one device whose id has a substring match.
# If DEVICE_FILTER is defined, only devices whose id has a substring match are operated on.
# If DEVICE_FILE is defined, only devices whose id are listed in given file (one id per line) are operated on.

ifdef STM32_PRG_PATH
PROG ?= $(STM32_PRG_PATH)/STM32_Programmer_CLI
else
STM32_CUBE_PROGRAMMER_CLI := $(lastword $(sort $(wildcard \
	/opt/st/stm32cubeide_*/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_*/tools/bin/STM32_Programmer_CLI \
	/opt/st/stm32cubeprogrammer*/bin/STM32_Programmer_CLI \
	/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI \
)))
ifneq "$(STM32_CUBE_PROGRAMMER_CLI)" ""
PROG ?= $(STM32_CUBE_PROGRAMMER_CLI)
else
PROG ?= STM32_Programmer_CLI
endif
endif

STM32_GDBSERVER_CLI := $(lastword $(sort $(wildcard \
	/opt/st/stm32cubeide_*/plugins/com.st.stm32cube.ide.mcu.externaltools.stlink-gdb-server.linux64_*/tools/bin/ST-LINK_gdbserver \
	/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/ST-LINK_gdbserver \
)))
ifneq "$(STM32_GDBSERVER_CLI)" ""
STM32_GDBSERVER ?= $(STM32_GDBSERVER_CLI)
else
STM32_GDBSERVER ?= ST-LINK_gdbserver
endif

STM32_ARM_GDB_CLI := $(lastword $(sort $(wildcard \
	/opt/st/stm32cubeide_*/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*.linux64_*/tools/bin/arm-none-eabi-gdb \
	/usr/local/STMicroelectronics/STM32Cube/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*.linux64_*/tools/bin/arm-none-eabi-gdb \
)))
ifneq "$(TOOLCHAIN_DIR)" ""
STM32_GDB ?= $(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)gdb
else ifneq "$(STM32_ARM_GDB_CLI)" ""
STM32_GDB ?= $(STM32_ARM_GDB_CLI)
else
STM32_GDB ?= $(CROSS_COMPILE)gdb
endif

ifneq "$(findstring /,$(PROG))" ""
STM32_CUBE_PROGRAMMER_DIR ?= $(abspath $(dir $(PROG))/..)
STM32_GDBSERVER_CP_ARG ?= -cp "$(STM32_CUBE_PROGRAMMER_DIR)"
else
STM32_GDBSERVER_CP_ARG ?=
endif

STM32_CUBE_PORT ?= SWD
STM32_CUBE_FREQ ?= 4000
STM32_CUBE_MODE ?= UR
STM32_CUBE_RESET ?= HWrst
STM32_CUBE_GLOBAL_ARGS ?= -q
STM32_CUBE_CONNECT_ARGS ?= port=$(STM32_CUBE_PORT) freq=$(STM32_CUBE_FREQ) mode=$(STM32_CUBE_MODE) reset=$(STM32_CUBE_RESET)
STM32_CUBE_FLASH_ARGS ?= -v
STM32_CUBE_RESET_TIMEOUT ?= 10
STM32_CUBE_FLASH_TIMEOUT ?= 60
STM32_CUBE_ERASE_TIMEOUT ?= 60
STM32_GDBSERVER_RESET_ARGS ?= -k --halt
STM32_GDBSERVER_ARGS ?= -e
GDB_PORT ?= 3333

define strip_ansi
sed -r 's/\x1b\[[0-9;]*[[:alpha:]]//g'
endef

define list_stlink_devices
$(shell "$(PROG)" -l st-link-only 2> /dev/null | $(strip_ansi) | sed -n 's/.*ST-LINK SN[[:space:]]*:[[:space:]]*//p' | tr -d '\r')
endef

ifndef DEVICE_SINGLE
DEVICE_SERIAL_ARG = sn=$$arg
DEVICE_GDBSERVER_SERIAL_ARG = -i $(STM_DEVICE_FIRST)
else
DEVICE_SERIAL_ARG =
DEVICE_GDBSERVER_SERIAL_ARG =
endif

_filter = $(foreach v,$(2),$(if $(findstring $(1),$(v)),$(v),))

# Lists all connected ST-LINK devices.
stm-list: .prereq-devs
	@echo ""
	@echo "Devices: $(STM_DEVICES)"

# Execute $(1) with each entry of $(2) as variable "arg" in parallel, $(3) seconds timeout.
# ex $(call _parallel,some_parallel_task $$arg,apple pear orange,10)
define _parallel
	@rm -rf .parallel.pids
	$(foreach arg,$(2), \
          arg="$(arg)"; \
          timeout $(3)s $(1) & \
          echo "$$!" >> .parallel.pids; \
        ) \
        while read pid; do \
          wait $$pid; \
        done <.parallel.pids
	@rm -rf .parallel.pids
endef

# Resets stm devices.
stm-reset: .prereq-devs
	@echo "Resetting $(STM_DEVICES)"
	$(call _parallel, \
			"$(PROG)" $(STM32_CUBE_GLOBAL_ARGS) \
				-c $(STM32_CUBE_CONNECT_ARGS) $(DEVICE_SERIAL_ARG) \
				-rst \
	  ,$(STM_DEVICES),$(STM32_CUBE_RESET_TIMEOUT))

# Flashes stm devices with the application binary.
stm-flash: ${TARGET_DIR}/$(TARGETNAME).hex .prereq-devs
	@echo "Flashing $< to $(STM_DEVICES)"
	$(call _parallel, \
			"$(PROG)" $(STM32_CUBE_GLOBAL_ARGS) \
				-c $(STM32_CUBE_CONNECT_ARGS) $(DEVICE_SERIAL_ARG) \
				-d "$(realpath $<)" $(STM32_CUBE_FLASH_ARGS) \
				-rst \
	  ,$(STM_DEVICES),$(STM32_CUBE_FLASH_TIMEOUT))

# Flashes stm devices with given file.
stm-flash-file: .prereq-devs
	@echo "Flashing $(FILE) to $(STM_DEVICES)"
	@if [ -z "$(realpath $(FILE))" ]; then \
		echo "*** ERROR: No file specified or file not found, please define FILE to path to file to flash"; \
		exit 1; \
	fi
	$(call _parallel, \
			"$(PROG)" $(STM32_CUBE_GLOBAL_ARGS) \
				-c $(STM32_CUBE_CONNECT_ARGS) $(DEVICE_SERIAL_ARG) \
				-d "$(realpath $(FILE))" $(STM32_CUBE_FLASH_ARGS) \
				-rst \
	  ,$(STM_DEVICES),$(STM32_CUBE_FLASH_TIMEOUT))

# Fully erases stm devices.
stm-erase-all: .prereq-devs
	@echo "Erasing $(STM_DEVICES)"
	$(call _parallel, \
			"$(PROG)" $(STM32_CUBE_GLOBAL_ARGS) -y \
				-c $(STM32_CUBE_CONNECT_ARGS) $(DEVICE_SERIAL_ARG) \
				-e all \
	  ,$(STM_DEVICES),$(STM32_CUBE_ERASE_TIMEOUT))

# Connects first best device to ST-LINK GDB server.
stm-connect: .prereq-devs .prereq-gdbserver
	@echo "Connecting to $(STM_DEVICE_FIRST) on gdb port $(GDB_PORT)"
	"$(STM32_GDBSERVER)" \
		-p $(GDB_PORT) \
		-d \
		--frequency $(STM32_CUBE_FREQ) \
		$(DEVICE_GDBSERVER_SERIAL_ARG) \
		$(STM32_GDBSERVER_CP_ARG) \
		$(STM32_GDBSERVER_RESET_ARGS) \
		$(STM32_GDBSERVER_ARGS)

# Starts a debug session - stm-connect must have been issued first.
stm-debug: ${TARGET_DIR}/$(TARGETNAME).hex .prereq-devs .prereq-gdb
	$(STM32_GDB) \
		-ex "file ${TARGET_DIR}/$(TARGETNAME).elf" \
		-ex "target remote localhost:$(GDB_PORT)" \
		-ex "break HardFault_Handler" \
		$(EXTRA_DEBUG_COMMANDS)

# Starts a bare debug session - stm-connect must have been issued first.
stm-debug-bare: .prereq-devs .prereq-gdb
	$(STM32_GDB) \
		-ex "target remote localhost:$(GDB_PORT)" \
		$(EXTRA_CONNECT_COMMANDS)

rightparen:=)

.prereq-devs: .prereq-prog
ifndef DEVICE_SINGLE
  ifdef DEVICE_FILE
	$(eval STM_DEVICES := $(shell \
	   ids="$$("$(PROG)" -l st-link-only 2> /dev/null | $(strip_ansi) | sed -n 's/.*ST-LINK SN[[:space:]]*:[[:space:]]*//p' | tr -d '\r')"; \
           file_ids="$$(cat $(DEVICE_FILE))"; \
           for fid in $$file_ids; do \
               case $$ids in *$$fid* $(rightparen) echo "$$fid";; \
               esac; \
           done \
        ))
	@if [ $(words $(STM_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No devices selected using device id file \"$(DEVICE_FILE)\""; \
		exit 1; \
	fi
  else ifdef DEVICE
	$(eval STM_DEVICES := $(call _filter,$(DEVICE),$(list_stlink_devices)))
	@if [ $(words $(STM_DEVICES)) -gt 1 ]; then \
		echo "*** ERROR: Ambiguous device id \"$(DEVICE)\", found multiple devices: $(STM_DEVICES)"; \
		exit 1; \
	fi
	@if [ $(words $(STM_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No device selected using device id \"$(DEVICE)\""; \
		exit 1; \
	fi
	$(eval STM_DEVICE_FIRST := $(word 1, $(STM_DEVICES)))
  else ifdef DEVICE_FILTER
	$(eval STM_DEVICES := $(call _filter,$(DEVICE_FILTER),$(list_stlink_devices)))
	@if [ $(words $(STM_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No devices selected using device filter \"$(DEVICE_FILTER)\""; \
		exit 1; \
	fi
  else
	$(eval STM_DEVICES := $(list_stlink_devices))
	@if [ $(words $(STM_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No devices connected"; \
		exit 1; \
	fi
  endif
else # DEVICE_SINGLE
	$(eval STM_DEVICES := DEVICE_SINGLE)
endif # DEVICE_SINGLE
	$(eval STM_DEVICE_FIRST ?= $(word 1, $(STM_DEVICES)))

# private, checks we have STM32CubeProgrammer
.prereq-prog: has_prog=$(shell if [ -x "$(PROG)" ] || command -v "$(PROG)" > /dev/null 2>&1; then echo 0; else echo 1; fi)
.prereq-prog:
	@if [ $(has_prog) -ne 0 ]; then \
		echo "*** ERROR: Could not find $(PROG). Make sure PROG or STM32_PRG_PATH is set correctly."; \
		exit 1; \
	fi

.prereq-gdbserver: has_gdbserver=$(shell if [ -x "$(STM32_GDBSERVER)" ] || command -v "$(STM32_GDBSERVER)" > /dev/null 2>&1; then echo 0; else echo 1; fi)
.prereq-gdbserver:
	@if [ $(has_gdbserver) -ne 0 ]; then \
		echo "*** ERROR: Could not find $(STM32_GDBSERVER). Make sure STM32_GDBSERVER is set correctly."; \
		exit 1; \
	fi

.prereq-gdb: has_gdb=$(shell if [ -x "$(STM32_GDB)" ] || command -v "$(STM32_GDB)" > /dev/null 2>&1; then echo 0; else echo 1; fi)
.prereq-gdb:
	@if [ $(has_gdb) -ne 0 ]; then \
		echo "*** ERROR: Could not find $(STM32_GDB). Make sure STM32_GDB, TOOLCHAIN_DIR, or CROSS_COMPILE is set correctly."; \
		exit 1; \
	fi
