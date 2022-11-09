all: build

build: elf2ld

clean:
	$(RM) elf2ld

.PHONY: clean build all

elf2ld: elf2ld.cxx
	g++ -Wall -Wextra $< -o $@
