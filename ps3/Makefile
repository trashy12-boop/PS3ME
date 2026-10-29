# SPDX-License-Identifier: GPL-2.0-only
ROOT := $(abspath ..)
PS3DEV ?= $(ROOT)/deps/ps3dev
export PSL1GHT ?= $(PS3DEV)
JDK_DIR ?= $(ROOT)/../build_deps/jdk8u504-b01
HOST_PREVERIFY_RUNNER ?=
PHONEME_SRC ?= $(ROOT)/deps/phoneME
PPUCXX := $(PS3DEV)/ppu/bin/ppu-g++
PPUAR := $(PS3DEV)/ppu/bin/ppu-ar
HAL := $(ROOT)/upstream/ps2/javacall
BUILD := $(ROOT)/build/ps3
JC := $(PHONEME_SRC)/javacall/interface
INC := -Iinclude -I$(HAL) -I$(HAL)/include -I$(JC) -I$(JC)/common -I$(JC)/midp
CPPFLAGS := $(INC) -DPS3ME_PPU -I$(PS3DEV)/ppu/include -I$(PS3DEV)/portlibs/ppu/include/SDL
CXXFLAGS := -MMD -MP -std=gnu++11 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections
BACKENDS := Storage Video Ps3Backend CooperativeTimers RuntimeServices Controls InputRuntime ControlMenu GameSettings InGameMenu
HAL_NAMES := GameStorage LcdDevice Keypad EventQueue SystemClock MemoryManager Logger TimerService
OBJS := $(addprefix $(BUILD)/,$(addsuffix .o,$(BACKENDS))) $(addprefix $(BUILD)/hal_,$(addsuffix .o,$(HAL_NAMES)))
.PHONY: all platform test-host test-vm-pointer-math native-link-check abi-audit vm-preflight clean
all: platform
platform: $(BUILD)/libps3me_platform.a
$(BUILD):
	mkdir -p $@
$(BUILD)/%.o: src/%.cpp | $(BUILD)
	$(PPUCXX) $(CPPFLAGS) $(CXXFLAGS) -I$(PS3DEV)/portlibs/ppu/include -c $< -o $@
$(BUILD)/hal_%.o: $(HAL)/hal/%.cpp | $(BUILD)
	$(PPUCXX) $(CPPFLAGS) $(CXXFLAGS) -I$(PS3DEV)/portlibs/ppu/include -c $< -o $@
$(BUILD)/libps3me_platform.a: $(OBJS)
	$(PPUAR) rcs $@ $^
test-host: | $(BUILD)
	$(CXX) $(INC) -std=c++11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined src/Storage.cpp src/Video.cpp tests/platform_test.cpp -o $(BUILD)/platform_test
	$(BUILD)/platform_test
test-vm-pointer-math: | $(BUILD)
	$(CXX) -std=c++11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -I$(ROOT)/vm/port $(ROOT)/tests/vm_pointer_math.cpp -o $(BUILD)/vm_pointer_math_host
	$(BUILD)/vm_pointer_math_host
	$(PPUCXX) -std=gnu++11 -Wall -Wextra -Werror -I$(ROOT)/vm/port -c $(ROOT)/tests/vm_pointer_math.cpp -o $(BUILD)/vm_pointer_math.o
native-link-check: platform
	$(PPUCXX) $(CPPFLAGS) $(CXXFLAGS) tests/native_link_check.cpp -L$(BUILD) -lps3me_platform -L$(PS3DEV)/portlibs/ppu/lib -L$(PS3DEV)/ppu/lib -Wl,--gc-sections -lSDL -lm -lgcm_sys -lrsx -lsysutil -lio -laudio -lrt -llv2 -o $(BUILD)/native_link_check.elf
vm-link-check:
	$(MAKE) -B -C $(ROOT)/build/vm_ppu/ps3_ppu/target/product ROMIZING=false JVMWorkSpace=$(PHONEME_SRC)/cldc PS3DEV=$(PS3DEV) JDK_DIR=$(JDK_DIR) PREVERIFY_ORIGINAL=$(PHONEME_SRC)/preverifier/build/linux/preverify HOST_PREVERIFY_RUNNER=$(HOST_PREVERIFY_RUNNER) jvmspi.o NativesTable.o ROMImage.o
	$(PPUCXX) $(CXXFLAGS) -I$(ROOT)/build/vm_ppu/ps3_ppu/dist/include -I$(PS3DEV)/ppu/include tests/vm_native_link.cpp $(addprefix $(ROOT)/build/vm_ppu/ps3_ppu/target/product/,jvmspi.o NativesTable.o ROMImage.o) -Wl,--start-group $(ROOT)/build/vm_ppu/ps3_ppu/dist/lib/libcldc_vm.a $(ROOT)/build/vm_ppu/ps3_ppu/dist/lib/libcldc_vmx.a -lm -lrt -llv2 -Wl,--end-group -o $(BUILD)/vm_native_link.elf
abi-audit:
	python3 $(ROOT)/tools/audit_vm_abi.py --compiler $(PPUCXX) --phoneme $(PHONEME_SRC) --output $(ROOT)/build/vm_abi_audit.json
vm-preflight:
	python3 $(ROOT)/tools/audit_vm_abi.py --compiler $(PPUCXX) --phoneme $(PHONEME_SRC) --output $(ROOT)/build/vm_abi_audit.json --require-compatible
clean:
	rm -rf $(ROOT)/build/ps3

include javacall.mk

# Recompile adapters when their interfaces/configuration change.
-include $(wildcard $(BUILD)/*.d)
