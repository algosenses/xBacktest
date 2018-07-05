CC	:= g++
LD	:= g++

MODULES	:= Broker \
		   Core \
		   DataSeries \
		   Feed \
		   Indicator \
		   Optimizer \
		   StgyAnalyzer \
		   Strategy \
		   Tools
SRC_DIRS	:= $(addprefix source/, $(MODULES))
BUILD_DIR := build

STRATEGY_SRCS := samples/sample.cpp

SRCS := $(foreach sdir, $(SRC_DIRS), $(wildcard $(sdir)/*.cpp))
SRCS += $(STRATEGY_SRCS)
OBJS := $(patsubst %.cpp, %.o, $(SRCS))
INCLUDES := $(addprefix -I, $(SRC_DIRS))
CPPFLAGS := -Wall -O2


vpath %.cpp $(SRC_DIRS)

define make-goal
$1/%.o: %.cpp
	$(CC) $(INCLUDES) $(CPPFLAGS) -c $$< -o $$@
endef

define make-dirs
	for dir in $(MODULES); \
	do \
		mkdir -p $(BUILD_DIR)/$$dir; \
	done	
endef

.PHONY: all checkdirs clean

all: checkdirs $(BUILD_DIR)/xBacktest.exe

build/xBacktest.exe: $(OBJS)
	$(LD) $^ -o $@ 

checkdirs: $(BUILD_DIR)

$(BUILD_DIR):
	@mkdir -p $@
#	@$(call make-dirs)

clean:
#	@rm -rf $(BUILD_DIR)
	@rm -rf $(OBJS)
	
#$(foreach bdir, $(BUILD_DIR), $(eval $(call make-goal, $(bdir))))

%.o: %.cpp
	$(CC) $(INCLUDES) $(CPPFLAGS) -c $< -o $@ 