CXXFLAGS += \
	-I .. \
	-std=c++23 \
	-fno-stack-protector \
	-fno-stack-check \
	-fpic \
	-fpie \
	-fexec-charset=IBM-1047 \
#	-fsanitize=undefined

LDFLAGS += \
	-Tlinker.ld \
	-nostdlib \
	-static-libgcc

PROGRAMS := hello.exe calc.exe mplay.exe run.exe tour.exe system.exe
DEPS := $(PROGRAMS:.o=.d)

all: build

build: $(PROGRAMS)

clean:
	$(RM) *.elf
	$(RM) *.o *.d

.PHONY: clean build all

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -MMD -c $< -o $@

-include $(DEPS)
