# Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com)
# MIT License (see ./LICENSE)

### NRF52 device utilities
# To see all connected devices run target
#   make nrf-list
# If DEVICE is defined, there must be only one device whose id has a substring match.
# If DEVICE_FILTER is defined, only devices whose id has a substring match are operated on.
# If DEVICE_FILE is defined, only devices whose id are listed in given file (one id per line) are operated on.
# Else, all connected devices are operated on.

NRFJPROG ?= nrfjprog

JLINK ?= /opt/SEGGER/JLink/JLinkExe
JLINK_GDBSERVER ?= /opt/SEGGER/JLink/JLinkGDBServerCLExe

ifeq "$(PROC)" "nrf52833"
SOFTDEVICE_PREFIX ?= s140
JLINK_DEVICE ?= nrf52833_xxaa
else ifeq "$(PROC)" "nrf52840"
SOFTDEVICE_PREFIX ?= s140
JLINK_DEVICE ?= nrf52840_xxaa
else ifeq "$(PROC)" "nrf5340app"
JLINK_DEVICE ?= nRF5340_xxAA_APP
NRFJPROG_EXTRAS ?= -f NRF53
else ifeq "$(PROC)" "nrf5340net"
JLINK_DEVICE ?= nRF5340_xxAA_NET
NRFJPROG_EXTRAS ?= -f NRF53 --coprocessor CP_NETWORK
else
SOFTDEVICE_PREFIX ?= s132
JLINK_DEVICE ?= nrf52832_xxaa
endif
#JLINK_DEVICE ?= nrf52810_xxaa
#JLINK_DEVICE ?= nrf52811_xxaa

SOFTDEVICE ?= $(shell find . -name "$(SOFTDEVICE_PREFIX)_nrf52_*_softdevice.hex")

_filter = $(foreach v,$(2),$(if $(findstring $(1),$(v)),$(v),))

# Lists all connected nrf52 devices
nrf-list: .prereq-devs
	@echo
	@echo Devices: $(NRF_DEVICES)

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

# Resets nrf52 devices.
nrf-reset: .prereq-devs
	@echo "Resetting $(NRF_DEVICES)"
	$(call _parallel,$(NRFJPROG) $(NRFJPROG_EXTRAS) -s $$arg -r,$(NRF_DEVICES),5)

# Flashes nrf52 devices with the application binary.
nrf-flash: ${TARGET_DIR}/$(TARGETNAME).hex .prereq-devs
	@echo "Flashing $< to $(NRF_DEVICES)"
	$(call _parallel,$(NRFJPROG) $(NRFJPROG_EXTRAS) -s $$arg --program "$<" --sectorerase && \
          $(NRFJPROG) $(NRFJPROG_EXTRAS) -s $$arg -d,$(NRF_DEVICES),60)

# Flashes nrf52 devices with the soft device.
nrf-flash-softdevice: $(SOFTDEVICE) .prereq-devs
	@echo "Flashing $< to $(NRF_DEVICES)"
	$(call _parallel,$(NRFJPROG) $(NRFJPROG_EXTRAS) -s $$arg --program "$<" --sectorerase,$(NRF_DEVICES),60)

# Flashes nrf52 devices with given file.
nrf-flash-file: .prereq-devs
	@echo "Flashing $(FILE) to $(NRF_DEVICES)"
	@if [ -z "$(realpath $(FILE))" ]; then \
		echo "*** ERROR: No file specified or file not found, please define FILE to path to file to flash"; \
		exit 1; \
	fi
	$(call _parallel,$(NRFJPROG) $(NRFJPROG_EXTRAS) -s $$arg --program "$(realpath $(FILE))" --sectorerase,$(NRF_DEVICES),60)

# Fully erases nrf52 devices.
nrf-erase-all: .prereq-devs
	@echo "Erasing $(NRF_DEVICES)"
	$(call _parallel,$(NRFJPROG) $(NRFJPROG_EXTRAS) -s $$arg --eraseall,$(NRF_DEVICES),60)

# Connects first best device to jlinks gdb server
nrf-connect: .prereq-devs
	@echo "Connecting to $(NRF_DEVICE_FIRST)"
	$(JLINK_GDBSERVER) -device $(JLINK_DEVICE) -if swd -port $(GDB_PORT) -select USB=$(NRF_DEVICE_FIRST)

# Connects first best device to jlinks rtt channel
nrf-rtt: .prereq-devs
	@echo "Connecting to $(NRF_DEVICE_FIRST)"
	$(eval PID := $(shell $(JLINK) -device $(JLINK_DEVICE) -if swd -speed 4000 -selectemubysn $(NRF_DEVICE_FIRST) -autoconnect 1 -rtttelnetport $(RTT_PORT) > /dev/null & echo $$!))
	@echo "JLink started, pid $(PID)"
	@sleep 1
	@echo "Opening trapped netcat on port $(RTT_PORT)"
	@bash -c "trap ' \
                    trap - SIGINT; \
                    echo -e \"\n\n*** CLEANUP: killing JLink process $(PID)\n\"; \
                    kill $(PID); \
                    exit 0;' SIGINT; \
                   nc localhost $(RTT_PORT)"

# Starts a debug session - nrf-connect must have been issued first.
nrf-debug: ${TARGET_DIR}/$(TARGETNAME).hex .prereq-devs
	$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)gdb \
		-ex "file ${TARGET_DIR}/$(TARGETNAME).elf" \
		-ex "target remote localhost:$(GDB_PORT)" \
		-ex "mon speed 10000" \
		-ex "mon flash download=1" \
		-ex "break HardFault_Handler" \

# Starts a bare debug session - nrf-connect must have been issued first.
nrf-debug-bare: .prereq-devs
	$(TOOLCHAIN_DIR)/bin/$(CROSS_COMPILE)gdb \
		-ex "target remote localhost:$(GDB_PORT)" \
		-ex "mon speed 10000" \
		-ex "mon flash download=1" \

rightparen:=)

.prereq-devs: .prereq-prog
ifdef DEVICE_FILE
	$(eval NRF_DEVICES := $(shell \
	   jlink_ids="$$($(NRFJPROG) -i)"; \
           file_ids="$$(cat $(DEVICE_FILE))"; \
           for fid in "$$file_ids"; do \
               case $$jlink_ids in *$$fid* $(rightparen) echo "$$fid";; \
               esac; \
           done \
        ))
	@if [ $(words $(NRF_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No devices selected using device id file \"$(DEVICE_FILE)\""; \
		exit 1; \
	fi
else ifdef DEVICE
	$(eval NRF_DEVICES := $(call _filter,$(DEVICE),$(shell $(NRFJPROG) -i)))
	@if [ $(words $(NRF_DEVICES)) -gt 1 ]; then \
		echo "*** ERROR: Ambiguous device id \"$(DEVICE)\", found multiple devices: $(NRF_DEVICES)"; \
		exit 1; \
	fi
	@if [ $(words $(NRF_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No device selected using device id \"$(DEVICE)\""; \
		exit 1; \
	fi
	$(eval NRF_DEVICE_FIRST := $(word 1, $(NRF_DEVICES)))
  ifndef GDB_PORT
	$(eval gdb_port_nbr := $(shell echo "$(strip $(NRF_DEVICE_FIRST))" | grep -o "[0-9][0-9][0-9][0-9]$$"))
    ifeq "$(PROC)" "nrf5340app"
	  $(eval GDB_PORT := $(shell echo $$(( 3$(gdb_port_nbr)&65534 )) ) )
    else ifeq "$(PROC)" "nrf5340net"
	  $(eval GDB_PORT := $(shell echo $$(( 3$(gdb_port_nbr)|1 )) ) )
    else
	  $(eval GDB_PORT := 3$(gdb_port_nbr) )
    endif
	$(info Auto GDB_PORT: $(GDB_PORT))
  endif
  ifndef RTT_PORT
	$(eval rtt_port_nbr := $(shell echo "$(strip $(NRF_DEVICE_FIRST))" | grep -o "[0-9][0-9][0-9][0-9]$$"))
    ifeq "$(PROC)" "nrf5340app"
	  $(eval RTT_PORT := $(shell echo $$(( 4$(rtt_port_nbr)&65534 )) ) )
    else ifeq "$(PROC)" "nrf5340net"
	  $(eval RTT_PORT := $(shell echo $$(( 4$(rtt_port_nbr)|1 )) ) )
    else
	  $(eval RTT_PORT := 4$(rtt_port_nbr) )
    endif
	$(info Auto RTT_PORT: $(RTT_PORT))
  endif
else ifdef DEVICE_FILTER
	$(eval NRF_DEVICES := $(call _filter,$(DEVICE_FILTER),$(shell $(NRFJPROG) -i)))
	@if [ $(words $(NRF_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No devices selected using device filter \"$(DEVICE_FILTER)\""; \
		exit 1; \
	fi
else
	$(eval NRF_DEVICES := $(shell $(NRFJPROG) -i))
	@if [ $(words $(NRF_DEVICES)) -eq 0 ]; then \
		echo "*** ERROR: No devices connected"; \
		exit 1; \
	fi
endif
	$(eval NRF_DEVICE_FIRST ?= $(word 1, $(NRF_DEVICES)))
	$(eval GDB_PORT ?= 2331)
	$(eval RTT_PORT ?= 5331)

# private, checks we have nrfjprog
.prereq-prog: has_nrfjprog=$(shell which $(NRFJPROG) > /dev/null 2>&1; echo $$?)
.prereq-prog: has_jlinkgdbserver=$(shell which $(JLINK_GDBSERVER) > /dev/null 2>&1; echo $$?)
.prereq-prog: has_jlink=$(shell which $(JLINK) > /dev/null 2>&1; echo $$?)
.prereq-prog:
	@if [ $(has_nrfjprog) -ne 0 ]; then \
		echo "ERROR: Could not find $(NRFJPROG). Make sure NRFJPROG variable is set correctly."; \
		exit 1; \
	fi; \
	if [ $(has_jlinkgdbserver) -ne 0 ]; then \
		echo "ERROR: Could not find $(JLINK_GDBSERVER). Make sure JLINK_GDBSERVER variable is set correctly."; \
		exit 1; \
	fi; \
	if [ $(has_jlink) -ne 0 ]; then \
		echo "WARNING: Could not find $(JLINK). Make sure JLINK variable is set correctly."; \
	fi
