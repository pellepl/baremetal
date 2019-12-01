## MSP device utilities
# If DEVICE is defined, there must be only one device whose id has a substring match.
# If DEVICE_FILTER is defined, only devices whose id has a substring match are operated on.
# If DEVICE_FILE is defined, only devices whose id are listed in given file (one id per line) are operated on.

TI_VID_PID := "2047:0013"

MSPDEBUG ?= mspdebug
MSPDEBUG_DRIVER = tilib

define unique_number
$(shell printf "%d" 0x$$(echo "$(1)" | md5sum | cut -c 1-4) | cut -c 1-4)
endef

define list_ti_devices
$(shell lsusb -d $(TI_VID_PID) -v | grep iSerial | awk '{print $$3}')
endef

_filter = $(foreach v,$(2),$(if $(findstring $(1),$(v)),$(v),))

# Lists all connected ST-LINK devices
msp-list: .prereq-devs
	@echo
	@echo Devices: $(MSP_DEVICES)

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
          arg=$(arg); \
          timeout $(3)s $(1) & \
          echo $$! >> .parallel.pids; \
        ) \
        while read pid; do \
          wait $$pid; \
        done <.parallel.pids
	@rm -rf .parallel.pids
endef

# Resets devices.
msp-reset: .prereq-devs
	@echo "Resetting $(MSP_DEVICES)"
	$(call _parallel, \
			$(MSPDEBUG) $(MSPDEBUG_DRIVER) -s $$arg "reset" \
			,$(MSP_DEVICES),10)

# Flashes devices with the application binary.
msp-flash: ${TARGET_DIR}/$(TARGETNAME).hex .prereq-devs
	@echo "Flashing $< to $(MSP_DEVICES)"
	$(call _parallel, \
			$(MSPDEBUG) $(MSPDEBUG_DRIVER) -s $$arg "prog $(realpath $<)" \
		  ,$(MSP_DEVICES),60)

# Flashes stm devices with given file.
msp-flash-file: .prereq-devs
	@echo "Flashing $(FILE) to $(MSP_DEVICES)"
	@if [ -z "$(realpath $(FILE))" ]; then \
		echo "*** ERROR: No file specified or file not found, please define FILE to path to file to flash"; \
		exit 1; \
	fi
	$(call _parallel, \
			$(MSPDEBUG) $(MSPDEBUG_DRIVER) -s $$arg "prog $(realpath $(FILE)))" \
		  ,$(MSP_DEVICES),60)

# Fully erases stm devices.
msp-erase-all: .prereq-devs
	@echo "Erasing $(MSP_DEVICES)"
	$(call _parallel, \
			$(MSPDEBUG) $(MSPDEBUG_DRIVER) -s $$arg "erase" \
		  ,$(MSP_DEVICES),60)

# Connects first best device to MSPDEBUG
msp-connect: .prereq-devs
	@echo "Connecting to $(MSP_DEVICE_FIRST)"
	$(MSPDEBUG) $(MSPDEBUG_DRIVER) -s $(MSP_DEVICE_FIRST) "gdb $(GDB_PORT)"

# Starts a debug session - msp-connect must have been issued first.
msp-debug: ${TARGET_DIR}/$(TARGETNAME).hex .prereq-devs
	$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)gdb \
		-ex "file ${TARGET_DIR}/$(TARGETNAME).elf" \
		-ex "target remote localhost:$(GDB_PORT)" 

# Starts a bare debug session - nrf-connect must have been issued first.
msp-debug-bare: .prereq-devs
	$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)gdb \
		-ex "target remote localhost:$(GDB_PORT)" 

rightparen:=)

.prereq-devs: .prereq-prog
ifdef DEVICE_FILE
	$(eval MSP_DEVICES := $(shell \
	   ids="$$(lsusb -d $(TI_VID_PID) -v | grep iSerial | awk '{print $$3}')"; \
           file_ids="$$(cat $(DEVICE_FILE))"; \
           for fid in "$$file_ids"; do \
               case $$ids in *$$fid* $(rightparen) echo "$$fid";; \
               esac; \
           done \
        ))
	@if [ $(words $(MSP_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No devices selected using device id file \"$(DEVICE_FILE)\""; \
		exit 1; \
	fi
else ifdef DEVICE
	$(eval MSP_DEVICES := $(call _filter,$(DEVICE),$(list_ti_devices)))
	@if [ $(words $(MSP_DEVICES)) -gt 1 ]; then \
		echo "*** ERROR: Ambiguous device id \"$(DEVICE)\", found multiple devices: $(MSP_DEVICES)"; \
		exit 1; \
	fi
	@if [ $(words $(MSP_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No device selected using device id \"$(DEVICE)\""; \
		exit 1; \
	fi
	$(eval MSP_DEVICE_FIRST := $(word 1, $(MSP_DEVICES)))
  ifndef GDB_PORT
	$(eval gdb_port_nbr := $(call unique_number,$(strip $(MSP_DEVICE_FIRST))))
	$(eval GDB_PORT := 3$(gdb_port_nbr))
	$(info Auto GDB_PORT: $(GDB_PORT))
  endif
else ifdef DEVICE_FILTER
	$(eval MSP_DEVICES := $(call _filter,$(DEVICE_FILTER),$(list_ti_devices)))
	@if [ $(words $(MSP_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No devices selected using device filter \"$(DEVICE_FILTER)\""; \
		exit 1; \
	fi
else
	$(eval MSP_DEVICES := $(list_ti_devices))
	@if [ $(words $(MSP_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No devices connected"; \
		exit 1; \
	fi
endif
	$(eval MSP_DEVICE_FIRST ?= $(word 1, $(MSP_DEVICES)))
	$(eval GDB_PORT ?= 3333)

# private, checks we have binaries
.prereq-prog: has_mspdebug=$(shell which $(MSPDEBUG) > /dev/null 2>&1; echo $$?)
.prereq-prog:
	@if [ $(has_mspdebug) -ne 0 ]; then \
		echo "ERROR: Could not find $(MSPDEBUG). Make sure MSPDEBUG variable is set correctly."; \
		exit 1; \
	fi; \
