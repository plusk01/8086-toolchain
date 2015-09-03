###############################################################################
# Makefile for Emu86, the 8086 simulator built for EE425 by Wade S. Fife under
# the direction of James K. Archibald.
# Last updated September 4, 2003 by Wade S. Fife

# There is also be a VC++ 6.0 project and workspace (Emu86.dsw) for
# building a windows version.

# The file SysConfig.h can also be modified for a Linux (little endian) version.

# Depending on the status of machines in the EE department or ET college,
# building emu86 and ProgramCRT may be tricky. Over the past two years
# I have had to update this makefile serveral times to ensure that
# the libraries are appropriately linked (either statically or 
# dynamically). If make fails, chances are the sysops have made a change
# to the library locations or to where certain libraries are searched
# for. Update the link stages as needed and try both EE and ET machines
# for linking both emu86 and ProgramCRT. Changing the compiler for
# ProgramCRT from gcc to g++ or vice-versa may also work.

# The simulator is made of 4 parts, all needed make the executables:
# (1) EE425 custom code for the simulator interface and I/O, (2) NASM
# disassembler code for instruction dumping, (3) and SciTech emulator code
# to simulate 8086 instructions. Also needed is (4) a second executable,
# named CRTexe, which reads a pipe and ouputs the data, acting as a
# separate output window.

CC = g++ -m32
PCRTCC = gcc

# Nasm Source Directory
ND = NasmDisassemble/

# NOTE: In order to be portable from EE to ET machines
# libstdc++ must be statically linked (-static).

# Emu86 Executable
ExecRelease/emu86:	ExecRelease/ProgramCRT \
		Bios.o ProgramWindow.o SimControl.o SimWindow.o Simptris.o Utils.o CmdHistory.o SymbolTable.o \
		nasm_disasm.o Conio.o \
		debug.o decode.o fpu.o main.o ops.o ops2.o prim_ops.o sys.o PIC.o
		$(CC) \
		Bios.o ProgramWindow.o SimControl.o SimWindow.o Simptris.o Utils.o CmdHistory.o SymbolTable.o \
		nasm_disasm.o Conio.o \
		debug.o decode.o fpu.o main.o ops.o ops2.o prim_ops.o sys.o PIC.o\
		-o ExecRelease/emu86

# CRT Output Console
ExecRelease/ProgramCRT:	CRT/UNIXCRT.cpp CRT/UNIXCRT.h
			$(CC) -lncurses CRT/UNIXCRT.cpp -o ExecRelease/ProgramCRT

##############################################################################

# Custom EE425 Files
Bios.o:		Bios.cpp Bios.h Utils.h x86emu/x86emui.h
		$(CC) -c Bios.cpp
ProgramWindow.o:	ProgramWindow.cpp ProgramWindow.h SysConfig.h CRT/UNIXCRT.h SimptrisBoard.h
		$(CC) -c ProgramWindow.cpp
SimControl.o:	SimControl.cpp SimControl.h Help.h ProgramWindow.h SimWindow.h Utils.h SymbolTable.h \
		Bios.h EXEDecode.h x86emu/x86emui.h $(ND)disasm.h
		$(CC) -c SimControl.cpp
SimWindow.o:	SimWindow.cpp SimWindow.h SysConfig.h x86emu/x86emui.h Utils.h
		$(CC) -c SimWindow.cpp
Simptris.o:	Simptris.cpp Simptris.h SimptrisBoard.h
		$(CC) -c Simptris.cpp
Utils.o:	Utils.cpp Utils.h
		$(CC) -c Utils.cpp
CmdHistory.o:	CmdHistory.cpp CmdHistory.h
		$(CC) -c CmdHistory.cpp
SymbolTable.o:	SymbolTable.cpp SymbolTable.h
		$(CC) -c SymbolTable.cpp

# Custom Console Rutines (for non blocking UNIX terminal input)
Conio.o:	Conio.cpp Conio.h
		$(CC) -c Conio.cpp

# NASM Disassembler files
nasm_disasm.o:	$(ND)nasm_disasm.cpp $(ND)nasm.h $(ND)disasm.h \
		$(ND)insns.h $(ND)names.c $(ND)insnsd.c $(ND)regdis.c \
		$(ND)insnsi.h $(ND)regs.h $(ND)insnsn.c $(ND)regs.c
		$(CC) -c $(ND)nasm_disasm.cpp

# SciTech Files
debug.o:	debug.cpp x86emu/x86emui.h
		$(CC) -c debug.cpp
decode.o:	decode.cpp x86emu/x86emui.h
		$(CC) -c decode.cpp
fpu.o:		fpu.cpp x86emu/x86emui.h
		$(CC) -c fpu.cpp
main.o:		main.cpp x86emu/x86emui.h
		$(CC) -c main.cpp
ops.o:		ops.cpp x86emu/x86emui.h
		$(CC) -c ops.cpp
ops2.o:		ops2.cpp x86emu/x86emui.h
		$(CC) -c ops2.cpp
prim_ops.o:	prim_ops.cpp x86emu/x86emui.h
		$(CC) -c prim_ops.cpp
sys.o:		sys.cpp x86emu.h x86emu/regs.h x86emu/debug.h x86emu/prim_ops.h
		$(CC) -c sys.cpp
# The following module adds to the SciTech code but was written by Wade Fife.
PIC.o:		PIC.cpp PIC.h x86emu/x86emui.h
		$(CC) -c PIC.cpp

# Cleanup
clean:		
		rm *.o ExecRelease/emu86 ExecRelease/ProgramCRT

