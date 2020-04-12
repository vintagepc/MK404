# 
# 	Copyright 2008-2011 Michel Pollet <buserror@gmail.com>
#
#	This file is part of simavr.
#
#	simavr is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	simavr is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with simavr.  If not, see <http://www.gnu.org/licenses/>.

target=	Einsy
firm_src = ${wildcard at*${board}.c}
firmware = ${firm_src:.c=.axf}
simavr = ../../

IPATH = .
IPATH += ./hwDefs // Custom parts first..
IPATH += ../parts
IPATH += ${simavr}/include
IPATH += ${simavr}/simavr/sim
IPATH += ${simavr}/simavr/cores

VPATH = .
VPATH += ./hwDefs
VPATH += ../parts


LDFLAGS += -lpthread -lutil

include ../Makefile.opengl

.PHONY: MK3S.afx

all: obj ${target} MK3S.afx 

MK3S.afx:
	./build-fw.sh
	cp ../Prusa-Firmware-build/Firmware.ino.elf MK3S.afx;
	

OBJdump: MK3S.afx
	avr-objdump -d MK3S.afx > MK3S.txt
	
buildRun: all 
	./Einsy.elf
	
include ${simavr}/Makefile.common

board = ${OBJ}/${target}.elf

${board} : ${OBJ}/rotenc.o
${board} : ${OBJ}/fan.o
${board} : ${OBJ}/button.o
${board} : ${OBJ}/uart_pty.o
${board} : ${OBJ}/hd44780.o
${board} : ${OBJ}/hd44780_glut.o
${board} : ${OBJ}/thermistor.o
${board} : ${OBJ}/w25x20cl.o
${board} : ${OBJ}/Einsy_EEPROM.o
${board} : ${OBJ}/${target}.o

${target}: ${board}
	if [ ! -f ${target}.elf ]; then ln -s ${OBJ}/${target}.elf ${target}.elf; fi
	@echo $@ done

clean: clean-${OBJ}
	rm -rf *.a *.axf ${target} *.vcd .*.swo .*.swp .*.swm .*.swn
