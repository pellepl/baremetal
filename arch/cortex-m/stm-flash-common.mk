# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

### STM device utilities
# If DEVICE is defined, there must be only one device whose id has a substring match.
# If DEVICE_FILTER is defined, only devices whose id has a substring match are operated on.
# If DEVICE_FILE is defined, only devices whose id are listed in given file (one id per line) are operated on.


ifeq "$(OPENOCD_DEBUGGER)" "stlink-v2"
OPENOCD_VID_PID ?= "0483:3748"
OPENOCD_INTERFACE_FILE = "interface/stlink-v2.cfg"
else ifeq "$(OPENOCD_DEBUGGER)" "stlink-v2-1"
OPENOCD_VID_PID ?= "0483:374b"
OPENOCD_INTERFACE_FILE = "interface/stlink-v2-1.cfg"
else ifeq "$(OPENOCD_DEBUGGER)" "user"
else
$(error OPENOCD_DEBUGGER is not defined or invalid, please set to "stlink-v2", "stlink-v2-1", or "user")
endif

OPENOCD ?= openocd

ifndef OPENOCD_VID_PID
$(error OPENOCD_VID_PID is not defined. This is expected to match the USB VID PID of the flasher.)
endif
ifndef OPENOCD_INTERFACE_FILE
$(error OPENOCD_INTERFACE_FILE is not defined, e.g. interface/stlink-v2)
endif
ifndef OPENOCD_TARGET_FILE
$(error OPENOCD_TARGET_FILE is not defined, e.g. target/stm32f1x.cfg)
endif

define unique_number
$(shell printf "%d" 0x$$(echo "$(1)" | md5sum | cut -c 1-4) | cut -c 1-4)
endef

# This is one ugly bash thing listing the usb device name,
#   1 making it ascii-hexadecimal ("\xNN\bNN...")
#   2 replacing ascii characters 0xc0 and higher with '?'
#   3 removing all ascii characters >= 0x80
# In order to make it work with fishy st-link v2 dongles and openocd.
# These fishy dongles change name once you've made a successful connection(!)
define list_stlink_devices
$(shell lsusb -d $(OPENOCD_VID_PID) -v 2> /dev/null | grep -oP "iSerial\s*[0-9]\s\K.*" | tr -d '\n' | od -t x1 -w100 | grep -o "\ [0-9a-f][0-9a-f]" | sed -r 's/\s[c-f][0-9a-f]/ 3f/g' | sed -r 's/\s[8-f][0-9a-f]//g' | tr -d '\n' | sed -r 's/\s/\\x/g')
endef

_filter = $(foreach v,$(2),$(if $(findstring $(1),$(v)),$(v),))

# Lists all connected ST-LINK devices
stm-list: .prereq-devs
	@echo ""
	@echo "Devices: $(STM_DEVICES)"

# Execute $(1) with each entry of $(2) as variable "arg" in parallel, $(3) seconds timeout
# ex $(call _parallel,some_parallel_task $$arg,apple pear orange,10)
# will execute
#   some_parallel_task apple
#   some_parallel_task pear
#   some_parallel_task orange
# in parallel each with a timeout for 10 seconds, and await completion
define _parallel
	@rm -rf .parallel.pids
	@$(foreach arg,$(2), \
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
				$(OPENOCD) -f $(OPENOCD_INTERFACE_FILE) \
					-c "hla_serial $$arg" \
					-f $(OPENOCD_TARGET_FILE) \
					-c "gdb_port $(GDB_PORT)" \
					-c "telnet_port $(TELNET_PORT)" \
					-c "tcl_port disabled" \
					-c "init" \
					-c "reset" \
					-c "exit" \
		  ,$(STM_DEVICES),5)

# Flashes stm devices with the application binary.
stm-flash: ${TARGET_DIR}/$(TARGETNAME).hex .prereq-devs
	@echo "Flashing $< to $(STM_DEVICES)"
	$(call _parallel, \
			$(OPENOCD) -f $(OPENOCD_INTERFACE_FILE) \
					-c "hla_serial $$arg" \
					-f $(OPENOCD_TARGET_FILE) \
					-c "gdb_port $(GDB_PORT)" \
					-c "telnet_port $(TELNET_PORT)" \
					-c "tcl_port disabled" \
					-c "init" \
					-c "reset halt" \
					-c "flash write_image erase \"$(realpath $<)\"" \
					-c "reset" \
					-c "exit" \
		  ,$(STM_DEVICES),60)

# Flashes stm devices with given file.
stm-flash-file: .prereq-devs
	@echo "Flashing $(FILE) to $(STM_DEVICES)"
	@if [ -z "$(realpath $(FILE))" ]; then \
		echo "*** ERROR: No file specified or file not found, please define FILE to path to file to flash"; \
		exit 1; \
	fi
	$(call _parallel, \
			$(OPENOCD) -f $(OPENOCD_INTERFACE_FILE) \
					-c "hla_serial $$arg" \
					-f $(OPENOCD_TARGET_FILE) \
					-c "gdb_port $(GDB_PORT)" \
					-c "telnet_port $(TELNET_PORT)" \
					-c "tcl_port disabled" \
					-c "init" \
					-c "reset halt" \
					-c "flash write_image erase \"$(realpath $(FILE))\"" \
					-c "exit" \
		  ,$(STM_DEVICES),60)

# Fully erases stm devices.
stm-erase-all: .prereq-devs
	@echo "Erasing $(STM_DEVICES)"
	$(call _parallel, \
			$(OPENOCD) -f $(OPENOCD_INTERFACE_FILE) \
					-c "hla_serial $$arg" \
					-f $(OPENOCD_TARGET_FILE) \
					-c "gdb_port $(GDB_PORT)" \
					-c "telnet_port $(TELNET_PORT)" \
					-c "tcl_port disabled" \
					-c "init" \
					-c "reset halt" \
					-c "stm32f1x mass_erase 0" \
					-c "exit" \
		  ,$(STM_DEVICES),60)

# Connects first best device to openocd
stm-connect: .prereq-devs
	@echo "Connecting to $(STM_DEVICE_FIRST)"
	$(OPENOCD) -f $(OPENOCD_INTERFACE_FILE) \
	           -c "hla_serial $(STM_DEVICE_FIRST)" \
			   -f $(OPENOCD_TARGET_FILE) \
	           -c "gdb_port $(GDB_PORT)" \
	           -c "telnet_port $(TELNET_PORT)" \
	           -c "tcl_port disabled"

# Starts a debug session - stm-connect must have been issued first.
stm-debug: ${TARGET_DIR}/$(TARGETNAME).hex .prereq-devs
	$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)gdb \
		-ex "file ${TARGET_DIR}/$(TARGETNAME).elf" \
		-ex "target remote localhost:$(GDB_PORT)" \
		-ex "mon speed 10000" \
		-ex "mon flash download=1" \
		-ex "break HardFault_Handler" \

# Starts a bare debug session - stm-connect must have been issued first.
stm-debug-bare: .prereq-devs
	$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)gdb \
		-ex "target remote localhost:$(GDB_PORT)" \
		-ex "mon speed 10000" \
		-ex "mon flash download=1" \

rightparen:=)

.prereq-devs: .prereq-prog
ifdef DEVICE_FILE
	$(eval STM_DEVICES := $(shell \
	   ids="$$(shell lsusb -d $(OPENOCD_VID_PID) -v 2> /dev/null | grep -oP 'iSerial\s*[0-9]\s\K.*' | tr -d '\n' | od -t x1 -w100 | grep -o '\ [0-9a-f][0-9a-f]' | sed -r 's/\s[c-f][0-9a-f]/ 3f/g' | sed -r 's/\s[8-f][0-9a-f]//g' | tr -d '\n' | sed -r 's/\s/\\x/g')"; \
           file_ids="$$(cat $(DEVICE_FILE))"; \
           for fid in "$$file_ids"; do \
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
  ifndef GDB_PORT
	$(eval gdb_port_nbr := $(call unique_number,$(strip $(STM_DEVICE_FIRST))))
	$(eval GDB_PORT := 3$(gdb_port_nbr))
	$(info Auto GDB_PORT: $(GDB_PORT))
  endif
  ifndef TELNET_PORT
	$(eval telnet_port_nbr := $(call unique_number,$(strip $(STM_DEVICE_FIRST))))
	$(eval TELNET_PORT := 4$(telnet_port_nbr))
	$(info Auto TELNET_PORT: $(TELNET_PORT))
  endif
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
	$(eval STM_DEVICE_FIRST ?= $(word 1, $(STM_DEVICES)))
	$(eval GDB_PORT ?= 3333)
	$(eval TELNET_PORT ?= 4444)

# private, checks we have binaries
.prereq-prog: has_openocd=$(shell which $(OPENOCD) > /dev/null 2>&1; echo $$?)
.prereq-prog:
	@if [ $(has_openocd) -ne 0 ]; then \
		echo "*** ERROR: Could not find $(OPENOCD). Make sure OPENOCD variable is set correctly."; \
		exit 1; \
	fi; \
