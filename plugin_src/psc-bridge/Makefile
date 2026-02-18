# Library metadata.
DEBUG_FLAGS = -D__FINAL__=1
LOG_TYPE = -D__USE_PRINTF__
BUILD_TYPE = _final

ifeq ($(DEBUG),1)
    DEBUG_FLAGS = -D__FINAL__=0
    BUILD_TYPE = _debug
endif

TYPE         := $(BUILD_TYPE)
FINAL        := $(DEBUG_FLAGS)
BUILD_FOLDER := $(shell pwd)/../../bin/plugins
OUTPUT_PRX   := dualsense_bridge
TARGET       := $(BUILD_FOLDER)/prx$(TYPE)/$(OUTPUT_PRX)
TARGET_ELF   := $(BUILD_FOLDER)/elf$(TYPE)/$(OUTPUT_PRX)
TARGETSTUB   := $(OUTPUT_PRX).so

# Libraries linked into the ELF.
LIBS := -lSceLibcInternal -lGoldHEN_Hook -lkernel -lSceSysmodule -lScePad -lSceUsbd

EXTRAFLAGS := $(DEBUG_FLAGS) $(LOG_TYPE) -fcolor-diagnostics -Wall

# Root vars
TOOLCHAIN     := $(OO_PS4_TOOLCHAIN)
GH_SDK        := ../../SDK
PROJDIR       := source
INTDIR        := build
INCLUDEDIR    := include
COMMON_DIR    := ../../common

# Define objects to build
CFILES      := $(wildcard $(PROJDIR)/*.c)
CPPFILES    := $(wildcard $(PROJDIR)/*.cpp)
# Only include plugin_common if you need standard logging/notify helpers
COMMONFILES := $(wildcard $(COMMON_DIR)/*.c) 

OBJS        := $(patsubst $(PROJDIR)/%.c, $(INTDIR)/%.o, $(CFILES)) \
               $(patsubst $(PROJDIR)/%.cpp, $(INTDIR)/%.o, $(CPPFILES)) \
               $(patsubst $(COMMON_DIR)/%.c, $(INTDIR)/%.o, $(COMMONFILES))

# Define final C/C++ flags
# Note: Added -I$(TOOLCHAIN)/include explicitly just in case
CFLAGS      := $(FINAL) --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) \
               -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include \
               -I$(GH_SDK)/include -I$(INCLUDEDIR) -I$(COMMON_DIR)

CXXFLAGS    := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -std=c++11

LDFLAGS     := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x -e _init --eh-frame-hdr \
               -L$(TOOLCHAIN)/lib -L$(GH_SDK) $(LIBS) 

# Check for linux vs macOS
UNAME_S     := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
        CC      := clang
        CCX     := clang++
        LD      := ld.lld
        CDIR    := linux
endif
ifeq ($(UNAME_S),Darwin)
        CC      := /usr/local/opt/llvm/bin/clang
        CCX     := /usr/local/opt/llvm/bin/clang++
        LD      := /usr/local/opt/llvm/bin/ld.lld
        CDIR    := macos
endif

all: $(TARGET)

$(TARGET): $(INTDIR) $(OBJS)
	$(LD) $(GH_SDK)/build/crtprx.o $(INTDIR)/*.o -o $(TARGET_ELF).elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-fself -in=$(TARGET_ELF).elf -out=$(TARGET_ELF).oelf --lib=$(TARGET).prx --paid 0x3800000000000011

$(INTDIR)/%.o: $(PROJDIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR)/%.o: $(PROJDIR)/%.cpp
	$(CCX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/%.o: $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

$(INTDIR):
	@mkdir -p $@

clean:
	rm -rf $(TARGET) $(TARGETSTUB) $(INTDIR) $(OBJS)

.PHONY: clean all
