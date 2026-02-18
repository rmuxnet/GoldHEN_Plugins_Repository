CC      := clang
ODIR    := build
SDIR    := source
IDIRS   := -I. -Iinclude -I../../common -I../../SDK/include -I$(OO_PS4_TOOLCHAIN)/include -I$(OO_PS4_TOOLCHAIN)/include/orbis
LDIRS   := -L. -L../../SDK -L$(OO_PS4_TOOLCHAIN)/lib
CFLAGS  := $(DEBUG_FLAGS) -O2 -fno-builtin -nostartfiles -nostdlib -Wall -masm=intel -march=btver2 -mtune=btver2 -m64 -mabi=sysv -mcmodel=small -fpie -fPIC
LFLAGS  := -Xlinker -T ../../SDK/linker.x -Xlinker -Ttext=0x0 -Wl,--build-id=none

CFILES   := $(wildcard $(SDIR)/*.c)
OBJS     := $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES))

LIBS     := -lScePad_stub -lSceUsbd_stub -lSceLibcInternal_stub -lkernel -lGoldHEN_Hook

TARGET = $(shell basename $(CURDIR)).prx

$(TARGET): $(ODIR) $(OBJS)
	$(CC) $(ODIR)/*.o -o $(ODIR)/$(TARGET) $(CFLAGS) $(LFLAGS) $(LDIRS) $(LIBS)

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(IDIRS)

$(ODIR):
	@mkdir $@

clean:
	@rm -rf $(TARGET) $(ODIR)
