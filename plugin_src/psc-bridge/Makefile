ifndef OO_PS4_TOOLCHAIN
$(error OO_PS4_TOOLCHAIN is not set)
endif

include $(OO_PS4_TOOLCHAIN)/make/ps4defs.mk

TARGET = dualsense_bridge.prx
ODIR   = build
SDIR   = source
IDIRS  = -Iinclude -I$(OO_PS4_TOOLCHAIN)/include
LDIRS  = -L$(OO_PS4_TOOLCHAIN)/lib
LIBS   = -lScePad_stub -lSceUsbd_stub -lSceLibcInternal_stub

CFILES   = $(wildcard $(SDIR)/*.c)
CPPFILES = $(wildcard $(SDIR)/*.cpp)
OBJS     = $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES)) \
           $(patsubst $(SDIR)/%.cpp, $(ODIR)/%.o, $(CPPFILES))

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(IDIRS)

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(IDIRS)

$(TARGET): $(ODIR) $(OBJS)
	$(LD) $(ODIR)/*.o -o $(ODIR)/$(TARGET) $(LDFLAGS) $(LDIRS) $(LIBS)

$(ODIR):
	@mkdir $@

clean:
	@rm -rf $(ODIR) $(TARGET)

.PHONY: clean
