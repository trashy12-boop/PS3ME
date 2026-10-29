# SPDX-License-Identifier: GPL-2.0-only
JC_EXTRA_HAL := TimerService RandomSource OsCore MidletLifecycle
JC_CONTRACTS := $(filter-out javacall_dir.cpp javacall_stubs.cpp,$(notdir $(wildcard $(HAL)/contract/*.cpp)))
JC_OBJS := $(addprefix $(BUILD)/jc_,$(JC_CONTRACTS:.cpp=.o)) $(addprefix $(BUILD)/hal_,$(addsuffix .o,$(JC_EXTRA_HAL))) $(BUILD)/RuntimeServices.o $(BUILD)/CooperativeTimers.o $(BUILD)/JavaCallFiles.o $(BUILD)/RuntimePaths.o $(BUILD)/JavaCallOptional.o
.PHONY: javacall test-timers
javacall: platform $(BUILD)/libps3me_javacall.a
$(BUILD)/jc_%.o: $(HAL)/contract/%.cpp | $(BUILD)
	$(PPUCXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(BUILD)/libps3me_javacall.a: $(JC_OBJS)
	rm -f $@
	$(PPUAR) rcs $@ $^
test-timers: | $(BUILD)
	$(CXX) $(INC) -Iinclude -std=c++11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined src/CooperativeTimers.cpp tests/timers_test.cpp -o $(BUILD)/timers_test
	ASAN_OPTIONS=detect_leaks=0 $(BUILD)/timers_test
JC_PROPERTIES := $(PHONEME_SRC)/javacall/implementation/share/properties
JC_UTILS := $(PHONEME_SRC)/javacall/implementation/share/utils
JC_C_INC := $(CPPFLAGS) -I$(JC_PROPERTIES) -I$(JC_PROPERTIES)/inc -I$(JC_UTILS)/inc
JC_C_OBJS := $(addprefix $(BUILD)/jc_share_,javacall_properties.o javacall_config_db.o javacall_db.o javautil_string.o javautil_unicode.o javautil_printf.o javautil_stdio.o) $(BUILD)/jc_static_properties.o
javacall: $(BUILD)/libps3me_properties.a
$(BUILD)/jc_share_%.o: $(JC_PROPERTIES)/%.c | $(BUILD)
	$(PS3DEV)/ppu/bin/ppu-gcc $(JC_C_INC) -std=gnu99 -O2 -c $< -o $@
$(BUILD)/jc_share_%.o: $(JC_UTILS)/%.c | $(BUILD)
	$(PS3DEV)/ppu/bin/ppu-gcc $(JC_C_INC) -std=gnu99 -O2 -c $< -o $@
$(BUILD)/jc_static_properties.c: $(ROOT)/build/midp_ppu/generated/merged.xml $(ROOT)/tools/generate_static_properties.py
	python3 $(ROOT)/tools/generate_static_properties.py $< $@
$(BUILD)/jc_static_properties.o: $(BUILD)/jc_static_properties.c
	$(PS3DEV)/ppu/bin/ppu-gcc $(JC_C_INC) -std=gnu99 -O2 -c $< -o $@
$(BUILD)/libps3me_properties.a: $(JC_C_OBJS)
	rm -f $@
	$(PPUAR) rcs $@ $^
