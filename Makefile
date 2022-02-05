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
LDFLAGS=-luser32 -lkernel32 -nostdlib -lshell32 -lvulkan-1 -Wl,-entry:__entry,-subsystem:windows
STRIPC=llvm-strip
RC=llvm-rc
PACKC=upx

TARGET=pl.exe
DBGTARGET=pl_debug.exe
TARGETDEFINE=-DWINDOWS -DTARGET=\"$(TARGET)\"
DBGTARGETDEFINE=-DWINDOWS -DTARGET=\"$(DBGTARGET)\"
OBJS:=$(OBJS) windowsResource.res

# Actual build rules.
# These are supposed to be everything that might be edited.
TXTS:=$(TXTS) $(wildcard ./*.txt) ./Makefile ./README.md ./windowsResource.rc
SRCS:=$(SRCS) $(wildcard ./*.h) $(wildcard ./*.c)
CS:=$(CS) $(wildcard ./*.c)
OBJS:=$(OBJS) $(CS:.c=.o)
$(OBJS): Makefile

include deps.txt

# windres
windowsResource.res: windowsResource.rc pl.ico
	$(RC) $<

# Override defaults
%.o: %.c
	$(CC) $(CCINCFLAG) $(CCFLAGS) $< -o $@

$(TARGET): $(OBJS)
	$(LD) $^ -o $@ $(LDFLAGS)
	$(STRIP)
	$(PACK)

$(DBGTARGET): $(OBJS)
	$(LD) $^ -o $@ $(LDFLAGS)
	$(STRIP)
	$(PACK)



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
	rm -f ./*.o ./*.res ./$(TARGET) ./$(DBGTARGET)

.PHONY: backup
backup: clean release
	make clean releasedebug
	git add -A
	git commit -a -m "$(shell cat ./message.txt)" || true
	git push -u origin master

.PHONY: depend
depend:
	clang $(CCINCFLAG) -MM $(CS) > ./deps.txt

.PHONY: run
run: 
	./$(TARGET)

.PHONY: dbgrun
dbgrun: 
	./$(DBGTARGET)
