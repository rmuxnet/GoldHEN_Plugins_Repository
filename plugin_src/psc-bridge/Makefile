CC      := clang
CXX     := clang++
ODIR    := build
SDIR    := source
IDIRS   := -I. -Iinclude -I../../common -I../../SDK/include -I$(OO_PS4_TOOLCHAIN)/include
LDIRS   := -L. -L../../SDK -L$(OO_PS4_TOOLCHAIN)/lib
CFLAGS  := $(DEBUG_FLAGS) -O2 -fno-builtin -nostartfiles -nostdlib -Wall -masm=intel -march=btver2 -mtune=btver2 -m64 -mabi=sysv -mcmodel=small -fpie -fPIC
CXXFLAGS:= $(CFLAGS) -std=c++11
LFLAGS  := -Xlinker -T ../../SDK/linker.x -Xlinker -Ttext=0x0 -Wl,--build-id=none

CFILES   := $(wildcard $(SDIR)/*.c)
CPPFILES := $(wildcard $(SDIR)/*.cpp)
OBJS     := $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES)) \
            $(patsubst $(SDIR)/%.cpp, $(ODIR)/%.o, $(CPPFILES))

LIBS     := -lScePad_stub -lSceUsbd_stub -lSceLibcInternal_stub -lkernel -lGoldHEN_Hook

TARGET = $(shell basename $(CURDIR)).prx

$(TARGET): $(ODIR) $(OBJS)
	$(CXX) $(ODIR)/*.o -o $(ODIR)/$(TARGET) $(CXXFLAGS) $(LFLAGS) $(LDIRS) $(LIBS)

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(IDIRS)

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(IDIRS)

$(ODIR):
	@mkdir $@

clean:
	@rm -rf $(TARGET) $(ODIR)
