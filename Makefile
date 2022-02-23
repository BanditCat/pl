################################################################################
##     Copyright (c) Jon DuBois 2022. This file is part of pseudoluminal.     ##
##                                                                            ##
## This program is free software: you can redistribute it and#or modify       ##
## it under the terms of the GNU Affero General Public License as published   ##
## by the Free Software Foundation, either version 3 of the License, or       ##
## (at your option) any later version.                                        ##
##                                                                            ##
## This program is distributed in the hope that it will be useful,            ##
## but WITHOUT ANY WARRANTY; without even the implied warranty of             ##
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              ##
## GNU Affero General Public License for more details.                        ##
##                                                                            ##
## You should have received a copy of the GNU Affero General Public License   ##
## along with this program.  If not, see <https://www.gnu.org/licenses/>.     ##
##                                                                            ##
################################################################################
## Project makefile.                                                          ##
################################################################################


# Targets in this section are the ones meant to be used by users.
.PHONY: all
all: depend debug dbgrun
.PHONY: rall
rall: depend release run
.PHONY: rdall
rdall: depend releasedebug dbgrun


# Toolchain.
CC=clang
CCFLAGS=-m64 -std=c17 -Wall -fno-exceptions -Wextra -Werror -ffreestanding -c
CCINCFLAG=
LD=clang
LDFLAGS=-luser32 -luxtheme -lkernel32 -nostdlib -lCabinet -lshell32 -lbuild/vulkan-1 -Wl,-entry:__entry,-subsystem:windows
STRIPC=llvm-strip
RC=llvm-rc
PACKC=upx
GLSLC=glslc

TARGET=./build/pl.exe
DBGTARGET=./build/pl_debug.exe
TARGETDEFINE=-DWINDOWS -DTARGET=\"$(TARGET)\"
DBGTARGETDEFINE=-DWINDOWS -DTARGET=\"$(DBGTARGET)\"
RES=./build/windowsResource.res
CRES=./build/cres

TXTS:=$(TXTS) $(wildcard ./*.txt) ./Makefile ./README.md ./windowsResource.rc
SRCS:=$(SRCS) $(wildcard ./*.h) $(wildcard ./*.c) $(SHADERS)
CS:=$(CS) $(wildcard ./*.c)
SHADERS:=$(SHADERS) $(wildcard ./shaders/*.vert) $(wildcard ./shaders/*.frag)
SOBJS:=$(SHADERS:./shaders/%.vert=./res/shaders/%.spv) 
SOBJS:=$(SOBJS:./shaders/%.frag=./res/shaders/%.spv) 
OBJS:=$(RES) $(CS:%.c=./build/%.o)
DOBJS:=$(RES) $(CS:%.c=./build/%_dbg.o)
$(OBJS): Makefile

include deps.txt


# Actual build rules.
$(RES): windowsResource.rc graphics/pl.ico $(CRES)
	$(RC) $<
	mv ./windowsResource.res $@

./res/shaders/%.spv: ./shaders/%.vert
	$(GLSLC) $< -o $@
./res/shaders/%.spv: ./shaders/%.frag
	$(GLSLC) $< -o $@

./build/%.o: %.c
	$(CC) $(CCINCFLAG) $(CCFLAGS) $< -o $@
./build/%_dbg.o: %.c
	$(CC) $(CCINCFLAG) $(CCFLAGS) $< -o $@

$(TARGET): $(OBJS) 
	$(LD) $^ -o $@ $(LDFLAGS)
	$(STRIP)
	$(PACK)

$(DBGTARGET): $(DOBJS)
	$(LD) $^ -o $@ $(LDFLAGS)
	$(STRIP)
	$(PACK)

$(CRES): $(SOBJS)
	./pl.exe -compressDir=res -compressorOutput=cres
	mv ./cres ./build

.PHONY: release 
release: $(TARGET)
release: CCFLAGS:=-O3 $(TARGETDEFINE) $(CCFLAGS)
release: LDFLAGS:=-O3 -flto $(LDFLAGS)
release: STRIP:=$(STRIPC) -s $(TARGET)
release: PACK:=$(PACKC) --best $(TARGET)

.PHONY: releasedebug
releasedebug: $(DBGTARGET)
releasedebug: CCFLAGS:=-O3 $(DBGTARGETDEFINE) -DDEBUG $(CCFLAGS)
releasedebug: LDFLAGS:=-O3 -flto $(LDFLAGS)
releasedebug: STRIP:=$(STRIPC) -s $(DBGTARGET)
releasedebug: PACK:=$(PACKC) --best $(DBGTARGET)


.PHONY: debug 
debug: $(DBGTARGET)
debug: CCFLAGS:=$(DBGTARGETDEFINE) -O0 -g -DDEBUG $(CCFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJS) $(DOBJS) $(SOBJS) $(TARGET) $(DBGTARGET) $(CRES)



.PHONY: backup
backup: release releasedebug
	cp ./build/pl.exe ./
	cp ../../home/.emacs ./emacs.txt
	git add -A
	git commit -a -m "$(shell cat ./message.txt)" || true
	git push -u origin master

.PHONY: run
run: $(TARGET)
	$(TARGET)

.PHONY: dbgrun
dbgrun: $(DBGTARGET)
	$(DBGTARGET)

.PHONY: depend
depend:
	clang $(CCINCFLAG) -MM $(CS) > ./deps.txt && cat ./deps.txt | sed -re 's/(.*)\.o:/\1_dbg.o:/' >> ./deps.txt

