# Use an ET machine to make this program. This makefile has been
# custumized for compilation on ET machines. -WSF

#	Makefile for c68 and c386
#
#	N.B.  Some modules will generate empty source files
#		according to the #define statements in config.h
#
#	As well as entries for c68 and c386, there are also
#	entries for the compilers that are typically used to
#	boot the c68/c386 compilers for the first time.

PROG=c86

CFLGS=  -DNDEBUG -O
#CFLGS= -g

SRCDIR=
OBJDIR=unixobj/
BINDIR=../bin/
MKDIR_P=mkdir -p

# cxref flags
CXFLAGS=-d -l -s

# indent flags
INDENTFLAGS= -bad -br -ce -di8 -i4 -npsl \
	-TADDRESS -TAMODE -TBITSIZE -TBLOCK -TBOOL -TBTYPE -TCHAR -TCODE \
	-TCONDITION -TCSE -TDEEP -TEXPR -TEXPRTYPE -TFLAGS -TILEN \
	-TIVAL -TLABEL -TLEVEL -TLINE -TMODEL -TMSGNUM -TOPCODE -TOPTENUM \
	-TOPTION -TOPTIONS -TPEEPFLAGS -TPEEPINFO -TQUALIFIER -TRANGE \
	-TREG -TREGBITMAP -TREGLIST -TREGMASK -TREGTYPE -TREGUSAGE -TRVAL \
	-TSEQNUM -TSIZE -TSTATE -TSTATUS -TSTMT -TSTMTTYPE -TSTORAGE \
	-TSTRING -TSWITCH -TSYM -TTABLE -TTARGET -TTOKEN -TTYP -TUSES -TUVAL \
	-TVISIBILITY

# File suffix seperator (default)
S=.
# Object file extension
O=o
# Flag for name of object file
OF = -o 
# Additional libraries
LIB =
# Command to delete files
RM=rm -f

#------------------------------------------------------------------------------
# Use the following if compiling c68/c386 with c68 on Minix
# CCFLAGS= -Qwarn=8 -Qerror=3 -Qstackopt=3 -D__STDC__=1
# LDFLAGS= -m250000
#------------------------------------------------------------------------------
# Use the following if compiling c68/c386 with c386 on Minix
# CCFLAGS= -Qwarn=8 -Qerror=3 -Qstackopt=3 -D__STDC__=1
# LDFLAGS= -m250000
#------------------------------------------------------------------------------
# Use the following if compiling c68 with ACK (68k version)
# CCFLAGS=
# LDFLAGS= -m250000
#------------------------------------------------------------------------------
# Use the following if compiling c68 with Microsoft Visual C
# CCFLAGS= -Zl -Od
# LDFLAGS= -ML -MDd
# CC = cl -nologo
# O = obj
# OF = -Fo
# RM = del
# LIB =
#------------------------------------------------------------------------------
# Use the following if compiling c68/c386 with GNU C on Minix
# CCFLAGS=	-Wall -ansi -pedantic -Wshadow -Wpointer-arith \
#	-Wwrite-strings -Wcast-qual -Wwrite-strings -Wcast-align \
#	-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
#	-Wredundant-decls -Wnested-externs
# LDFLAGS= -s -m250000
#------------------------------------------------------------------------------
# Use the following if compiling c386 with bcc on Minix
# CCFLAGS=
# LDFLAGS=
#------------------------------------------------------------------------------
# Use the following if compiling c68 with c68 on QDOS
# CCFLAGS= -Qmaxerr=3 -Qwarn=8 -Qerror=3 -Qstackopt=maximum
# CCFLAGS= -Qmaxerr=10 -Qwarn=8 -Qerror=8 -Qstackopt=maximum
# LDFLAGS = -bufp400K -ms
# S=_
# SRCDIR=
# OBJDIR=
# BINDIR=ram3_
#------------------------------------------------------------------------------
# Use the following if compiling c86 with TopSpeed on DOS
# and then do
#	MAKE sibo		to build both versions
#	MAKE c86check		to build code checking only version
#	MAKE c86code		to build code check and generation version
#CC=tsc /zq /fpC86.PR
#O=obj
#RM=del
#------------------------------------------------------------------------------
# Use the following if compiling c86 with GNU C
#CCFLAGS=  -Wall -ansi -pedantic -Wshadow -Wpointer-arith \
#	-Wwrite-strings -Wcast-qual -Wwrite-strings -Wcast-align \
#	-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
#	-Wredundant-decls -Wnested-externs
CC=gcc
CCFLAGS=  -Wall -ansi -std=c99 -pedantic -Wshadow -Wpointer-arith \
	-Wwrite-strings -Wcast-qual -Wwrite-strings -Wcast-align \
	-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
	-Wredundant-decls -Wnested-externs

#------------------------------------------------------------------------------
CFLAGS=-c $(CCFLAGS) $(CFLGS)

#------------------------------------------------------------------------------
#	Source files common to all versions of c68/c386
SRC1 =	$(SRCDIR)analyze$(S)c \
		$(SRCDIR)cglbdef$(S)c \
		$(SRCDIR)cmain$(S)c \
		$(SRCDIR)decl$(S)c \
		$(SRCDIR)expr$(S)c \
		$(SRCDIR)extern$(S)c
SRC2 =	$(SRCDIR)genicode$(S)c \
		$(SRCDIR)geninit$(S)c \
		$(SRCDIR)genstmt$(S)c \
		$(SRCDIR)genutil$(S)c \
		$(SRCDIR)getsym$(S)c \
		$(SRCDIR)init$(S)c
SRC3 =	$(SRCDIR)intexpr$(S)c \
		$(SRCDIR)list$(S)c \
		$(SRCDIR)memmgt$(S)c \
		$(SRCDIR)msgout$(S)c \
		$(SRCDIR)optimize$(S)c
SRC4 =	$(SRCDIR)outgen$(S)c \
		$(SRCDIR)stmt$(S)c \
		$(SRCDIR)symbol$(S)c \
		$(SRCDIR)system$(S)c \
		$(SRCDIR)types$(S)c
SRC5 =	$(SRCDIR)outdbg_d$(S)c \
		$(SRCDIR)gendbg$(S)c \
		$(SRCDIR)inline$(S)c
SRC =	$(SRC1) $(SRC2) $(SRC3) $(SRC4) $(SRC5)

HDR =	$(SRCDIR)chdr$(S)h \
		$(SRCDIR)cglbdec$(S)h \
		$(SRCDIR)config$(S)h \
		$(SRCDIR)expr$(S)h \
		$(SRCDIR)message$(S)h \
		$(SRCDIR)outproto$(S)h \
		$(SRCDIR)version$(S)h \
		$(SRCDIR)proto$(S)h \
		$(SRCDIR)check$(S)h

#	Source files specific to c68 680X0 versions
SRC68K =$(SRCDIR)flow68k$(S)c \
		$(SRCDIR)gen68k$(S)c \
		$(SRCDIR)genffp$(S)c \
		$(SRCDIR)genieee$(S)c \
		$(SRCDIR)peep68k$(S)c \
		$(SRCDIR)out68k_a$(S)c \
		$(SRCDIR)out68k_c$(S)c \
		$(SRCDIR)out68k_g$(S)c \
		$(SRCDIR)out68k_q$(S)c \
		$(SRCDIR)reg68k$(S)c
HDR68K =$(SRCDIR)gen68k$(S)h

#	Source files specific to INTEL versions
SRCX86 =$(SRCDIR)peepx86$(S)c \
		$(SRCDIR)regx86$(S)c \
		$(SRCDIR)outx86_a$(S)c \
		$(SRCDIR)outx86_b$(S)c \
		$(SRCDIR)outx86_g$(S)c \
		$(SRCDIR)outx86_n$(S)c \
		$(SRCDIR)outx86_s$(S)c
HDRX86 =$(SRCDIR)genx86$(S)h

#	Source files specific to c386 INTEL 386 versions
SRC386 =$(SRCDIR)gen386$(S)c
HDR386 =$(SRCDIR)gen386$(S)h

#	Source files specific to c386 INTEL 386 versions
SRC86  =$(SRCDIR)gen86$(S)c
HDR86  =$(SRCDIR)gen86$(S)h

#	Source files specific to carm ARM versions
SRCARM =$(SRCDIR)genarm$(S)c \
		$(SRCDIR)peeparm$(S)c \
		$(SRCDIR)outarm_a$(S)c \
		$(SRCDIR)outarm_o$(S)c \
		$(SRCDIR)regarm$(S)c
HDRARM =$(SRCDIR)genarm$(S)h

#	Source files specific to c30 TI TMS320C30 versions
SRCC30 =$(SRCDIR)flowc30$(S)c \
		$(SRCDIR)genc30$(S)c \
		$(SRCDIR)peepc30$(S)c \
		$(SRCDIR)outc30_r$(S)c \
		$(SRCDIR)regc30$(S)c
HDRC30 =$(SRCDIR)genc30$(S)h

#	Source files specific to PowerPC versions
SRCPPC =$(SRCDIR)genppc$(S)c \
		$(SRCDIR)peepppc$(S)c \
		$(SRCDIR)outppc$(S)c \
		$(SRCDIR)regppc$(S)c
HDRPPC =$(SRCDIR)genppc$(S)h

#	Source files specific to the SIBO build enviroment
SRCSIBO=$(SRCDIR)c86$(S)c \
		$(SRCDIR)c86$(S)pic \
		$(SRCDIR)c86$(S)afl \
		$(SRCDIR)c86$(S)shd \
		$(SRCDIR)c86$(S)pr \
		$(SRCDIR)config$(S)chk \
		$(SRCDIR)config$(S)gen \
		$(SRCDIR)config$(S)bat

#	Source files specific to the EPOC build enviroment
SRCEPOC=$(SRCDIR)c68$(S)mmp

#	PC Lint files
SRCLINT=$(SRCDIR)c68$(S)lnt \
		$(SRCDIR)include$(S)lnt

#	Extra files
SRCEXTR=$(SRCDIR)cmains$(S)c \
		$(SRCDIR)c86$(S)c \
		$(SRCDIR)c86$(S)dsw \
		$(SRCDIR)c86$(S)dsp \
		$(SRCDIR)c68$(S)dsw \
		$(SRCDIR)c68$(S)dsp \
		$(SRCDIR)config$(S)c86 \
		$(SRCDIR)config$(S)c68 \
		$(SRCDIR)config$(S)txt \
		$(SRCDIR)lltest$(S)c \
		$(SRCDIR)lltest$(S)dsp

#------------------------------------------------------------------------------
#	Object files common to all c68/c386 variants
OBJ =	$(OBJDIR)analyze$(S)$(O) \
		$(OBJDIR)cglbdef$(S)$(O) \
		$(OBJDIR)cmain$(S)$(O) \
		$(OBJDIR)decl$(S)$(O) \
		$(OBJDIR)expr$(S)$(O) \
		$(OBJDIR)extern$(S)$(O) \
		$(OBJDIR)genicode$(S)$(O) \
		$(OBJDIR)geninit$(S)$(O) \
		$(OBJDIR)genstmt$(S)$(O) \
		$(OBJDIR)genutil$(S)$(O) \
		$(OBJDIR)getsym$(S)$(O) \
		$(OBJDIR)init$(S)$(O) \
		$(OBJDIR)inline$(S)$(O) \
		$(OBJDIR)intexpr$(S)$(O) \
		$(OBJDIR)list$(S)$(O) \
		$(OBJDIR)memmgt$(S)$(O) \
		$(OBJDIR)msgout$(S)$(O) \
		$(OBJDIR)optimize$(S)$(O) \
		$(OBJDIR)outgen$(S)$(O) \
		$(OBJDIR)stmt$(S)$(O) \
		$(OBJDIR)symbol$(S)$(O) \
		$(OBJDIR)system$(S)$(O) \
		$(OBJDIR)types$(S)$(O) \
		$(OBJDIR)gendbg$(S)$(O) \
		$(OBJDIR)outdbg_d$(S)$(O)

#	Object files specific to Motorola 680X0 variants
OBJ68K =$(OBJDIR)flow68k$(S)$(O) \
		$(OBJDIR)gen68k$(S)$(O) \
		$(OBJDIR)genffp$(S)$(O) \
		$(OBJDIR)genieee$(S)$(O) \
		$(OBJDIR)peep68k$(S)$(O) \
		$(OBJDIR)out68k_a$(S)$(O) \
		$(OBJDIR)out68k_c$(S)$(O) \
		$(OBJDIR)out68k_g$(S)$(O) \
		$(OBJDIR)out68k_q$(S)$(O) \
		$(OBJDIR)reg68k$(S)$(O)

#	Object files specific to INTEL variants
OBJX86 =$(OBJDIR)peepx86$(S)$(O) \
		$(OBJDIR)regx86$(S)$(O) \
		$(OBJDIR)outx86_a$(S)$(O) \
		$(OBJDIR)outx86_b$(S)$(O) \
		$(OBJDIR)outx86_g$(S)$(O) \
		$(OBJDIR)outx86_n$(S)$(O) \
		$(OBJDIR)outx86_s$(S)$(O)

#	Object files specific to INTEL 386 variants
OBJ386 =$(OBJDIR)gen386$(S)$(O)

#	Object files specific to INTEL 86 variants
OBJ86  =$(OBJDIR)gen86$(S)$(O)

#	Object files specific to ARM variants
OBJARM =$(OBJDIR)genarm$(S)$(O) \
		$(OBJDIR)peeparm$(S)$(O) \
		$(OBJDIR)outarm_a$(S)$(O) \
		$(OBJDIR)outarm_o$(S)$(O) \
		$(OBJDIR)regarm$(S)$(O)

#	Object files specific to TI TMS320C30 versions
OBJC30 =$(OBJDIR)flowc30$(S)$(O) \
		$(OBJDIR)genc30$(S)$(O) \
		$(OBJDIR)peepc30$(S)$(O) \
		$(OBJDIR)outc30_r$(S)$(O) \
		$(OBJDIR)regc30$(S)$(O)

#	Object files specific to PowerPC versions
OBJPPC =$(OBJDIR)genppc$(S)$(O) \
		$(OBJDIR)peepppc$(S)$(O) \
		$(OBJDIR)outppc$(S)$(O) \
		$(OBJDIR)regppc$(S)$(O)

#	All object files
OBJS   =$(OBJ) $(OBJ68K) $(OBJX86) $(OBJ386) $(OBJ86) $(OBJARM) $(OBJC30) \
	$(OBJPPC)

#------------------------------------------------------------------------------
#	The following is only required if your library does not
#	already contain this routine.
# SRCLIB = $(SRCDIR)vfprintf$(S)c
# OBJLIB = $(OBJDIR)vfprintf$(S)o
#	Use this setting if you already have vfprintf
SRCLIB =
OBJLIB =

#------------------------------------------------------------------------------
LNS    =$(OBJS:.o=.ln)
CXS    =$(OBJS:.o=.cx)

.SUFFIXES:	$(S)ln $(S)cx $(S)obj $(S)o

#------------------------------------------------------------------------------
all:		$(BINDIR)$(PROG)

$(S)c$(S)obj:
		$(CC) $(CFLAGS) $*

$(S)c$(S)ln:
		lint $(CFLAGS) -m -u -c $<

$(S)c$(S)cx:
		cxref $(CXFLAGS) -C $<

$(BINDIR)$(PROG):	directory $(OBJS)
		$(CC) $(LDFLAGS) $(OF)$@ $(OBJS) $(LIB)

directory:
	$(MKDIR_P) $(BINDIR)

$(OBJ): 	$(HDR)
$(OBJ68K):	$(HDR) $(HDR68K)
$(OBJX86):	$(HDR) $(HDR86)  $(HDRX86)
$(OBJ386):	$(HDR) $(HDR386) $(HDRX86)
$(OBJ86):	$(HDR) $(HDR86) $(HDRX86)
$(OBJARM):	$(HDR) $(HDRASM)
$(OBJC30):	$(HDR) $(HDRC30)
$(OBJPPC):	$(HDR) $(HDRPPC)
$(HDR) :

#	Object files common to all c68/c386 variants
$(OBJDIR)analyze$(S)$(O) : $(SRCDIR)analyze$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)analyze$(S)$(O) $(SRCDIR)analyze$(S)c
$(OBJDIR)cglbdef$(S)$(O) : $(SRCDIR)cglbdef$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)cglbdef$(S)$(O) $(SRCDIR)cglbdef$(S)c
$(OBJDIR)cmain$(S)$(O) : $(SRCDIR)cmain$(S)c
	$(CC) -ansi $(CFLAGS) $(OF)$(OBJDIR)cmain$(S)$(O) $(SRCDIR)cmain$(S)c
#	$(CC) $(CFLAGS) $(OF)$(OBJDIR)cmain$(S)$(O) $(SRCDIR)cmain$(S)c
$(OBJDIR)decl$(S)$(O) : $(SRCDIR)decl$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)decl$(S)$(O) $(SRCDIR)decl$(S)c
$(OBJDIR)expr$(S)$(O) : $(SRCDIR)expr$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)expr$(S)$(O) $(SRCDIR)expr$(S)c
$(OBJDIR)extern$(S)$(O) : $(SRCDIR)extern$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)extern$(S)$(O) $(SRCDIR)extern$(S)c
$(OBJDIR)genicode$(S)$(O) : $(SRCDIR)genicode$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)genicode$(S)$(O) $(SRCDIR)genicode$(S)c
$(OBJDIR)geninit$(S)$(O) : $(SRCDIR)geninit$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)geninit$(S)$(O) $(SRCDIR)geninit$(S)c
$(OBJDIR)genstmt$(S)$(O) : $(SRCDIR)genstmt$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)genstmt$(S)$(O) $(SRCDIR)genstmt$(S)c
$(OBJDIR)genutil$(S)$(O) : $(SRCDIR)genutil$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)genutil$(S)$(O) $(SRCDIR)genutil$(S)c
$(OBJDIR)getsym$(S)$(O) : $(SRCDIR)getsym$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)getsym$(S)$(O) $(SRCDIR)getsym$(S)c
$(OBJDIR)init$(S)$(O) : $(SRCDIR)init$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)init$(S)$(O) $(SRCDIR)init$(S)c
$(OBJDIR)inline$(S)$(O) : $(SRCDIR)inline$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)inline$(S)$(O) $(SRCDIR)inline$(S)c
$(OBJDIR)intexpr$(S)$(O) : $(SRCDIR)intexpr$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)intexpr$(S)$(O) $(SRCDIR)intexpr$(S)c
$(OBJDIR)list$(S)$(O) : $(SRCDIR)list$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)list$(S)$(O) $(SRCDIR)list$(S)c
$(OBJDIR)memmgt$(S)$(O) : $(SRCDIR)memmgt$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)memmgt$(S)$(O) $(SRCDIR)memmgt$(S)c
$(OBJDIR)msgout$(S)$(O) : $(SRCDIR)msgout$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)msgout$(S)$(O) $(SRCDIR)msgout$(S)c
$(OBJDIR)optimize$(S)$(O) : $(SRCDIR)optimize$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)optimize$(S)$(O) $(SRCDIR)optimize$(S)c
$(OBJDIR)outgen$(S)$(O) : $(SRCDIR)outgen$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outgen$(S)$(O) $(SRCDIR)outgen$(S)c
$(OBJDIR)stmt$(S)$(O) : $(SRCDIR)stmt$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)stmt$(S)$(O) $(SRCDIR)stmt$(S)c
$(OBJDIR)symbol$(S)$(O) : $(SRCDIR)symbol$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)symbol$(S)$(O) $(SRCDIR)symbol$(S)c
$(OBJDIR)system$(S)$(O) : $(SRCDIR)system$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)system$(S)$(O) $(SRCDIR)system$(S)c
$(OBJDIR)types$(S)$(O) : $(SRCDIR)types$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)types$(S)$(O) $(SRCDIR)types$(S)c
$(OBJDIR)gendbg$(S)$(O) : $(SRCDIR)gendbg$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)gendbg$(S)$(O) $(SRCDIR)gendbg$(S)c
$(OBJDIR)outdbg_d$(S)$(O) : $(SRCDIR)outdbg_d$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outdbg_d$(S)$(O) $(SRCDIR)outdbg_d$(S)c
#	Object files specific to Motorola 680X0 variants
$(OBJDIR)flow68k$(S)$(O) : $(SRCDIR)flow68k$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)flow68k$(S)$(O) $(SRCDIR)flow68k$(S)c
$(OBJDIR)gen68k$(S)$(O) : $(SRCDIR)gen68k$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)gen68k$(S)$(O) $(SRCDIR)gen68k$(S)c
$(OBJDIR)genffp$(S)$(O) : $(SRCDIR)genffp$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)genffp$(S)$(O) $(SRCDIR)genffp$(S)c
$(OBJDIR)genieee$(S)$(O) : $(SRCDIR)genieee$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)genieee$(S)$(O) $(SRCDIR)genieee$(S)c
$(OBJDIR)peep68k$(S)$(O) : $(SRCDIR)peep68k$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)peep68k$(S)$(O) $(SRCDIR)peep68k$(S)c
$(OBJDIR)out68k_a$(S)$(O) : $(SRCDIR)out68k_a$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)out68k_a$(S)$(O) $(SRCDIR)out68k_a$(S)c
$(OBJDIR)out68k_c$(S)$(O) : $(SRCDIR)out68k_c$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)out68k_c$(S)$(O) $(SRCDIR)out68k_c$(S)c
$(OBJDIR)out68k_g$(S)$(O) : $(SRCDIR)out68k_g$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)out68k_g$(S)$(O) $(SRCDIR)out68k_g$(S)c
$(OBJDIR)out68k_q$(S)$(O) : $(SRCDIR)out68k_q$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)out68k_q$(S)$(O) $(SRCDIR)out68k_q$(S)c
$(OBJDIR)reg68k$(S)$(O) : $(SRCDIR)reg68k$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)reg68k$(S)$(O) $(SRCDIR)reg68k$(S)c
#	Object files specific to INTEL variants
$(OBJDIR)peepx86$(S)$(O) : $(SRCDIR)peepx86$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)peepx86$(S)$(O) $(SRCDIR)peepx86$(S)c
$(OBJDIR)regx86$(S)$(O) : $(SRCDIR)regx86$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)regx86$(S)$(O) $(SRCDIR)regx86$(S)c
$(OBJDIR)outx86_a$(S)$(O) : $(SRCDIR)outx86_a$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outx86_a$(S)$(O) $(SRCDIR)outx86_a$(S)c
$(OBJDIR)outx86_b$(S)$(O) : $(SRCDIR)outx86_b$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outx86_b$(S)$(O) $(SRCDIR)outx86_b$(S)c
$(OBJDIR)outx86_g$(S)$(O) : $(SRCDIR)outx86_g$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outx86_g$(S)$(O) $(SRCDIR)outx86_g$(S)c
$(OBJDIR)outx86_n$(S)$(O) : $(SRCDIR)outx86_n$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outx86_n$(S)$(O) $(SRCDIR)outx86_n$(S)c
$(OBJDIR)outx86_s$(S)$(O) : $(SRCDIR)outx86_s$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outx86_s$(S)$(O) $(SRCDIR)outx86_s$(S)c
#	Object files specific to INTEL 386 variants
$(OBJDIR)gen386$(S)$(O) : $(SRCDIR)gen386$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)gen386$(S)$(O) $(SRCDIR)gen386$(S)c
#	Object files specific to INTEL 86 variants
$(OBJDIR)gen86$(S)$(O) : $(SRCDIR)gen86$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)gen86$(S)$(O) $(SRCDIR)gen86$(S)c
#	Object files specific to ARM variants
$(OBJDIR)genarm$(S)$(O) : $(SRCDIR)genarm$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)genarm$(S)$(O) $(SRCDIR)genarm$(S)c
$(OBJDIR)peeparm$(S)$(O) : $(SRCDIR)peeparm$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)peeparm$(S)$(O) $(SRCDIR)peeparm$(S)c
$(OBJDIR)outarm_a$(S)$(O) : $(SRCDIR)outarm_a$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outarm_a$(S)$(O) $(SRCDIR)outarm_a$(S)c
$(OBJDIR)outarm_o$(S)$(O) : $(SRCDIR)outarm_o$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outarm_o$(S)$(O) $(SRCDIR)outarm_o$(S)c
$(OBJDIR)regarm$(S)$(O) : $(SRCDIR)regarm$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)regarm$(S)$(O) $(SRCDIR)regarm$(S)c
#	Object files specific to TI TMS320C30 versions
$(OBJDIR)flowc30$(S)$(O) : $(SRCDIR)flowc30$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)flowc30$(S)$(O) $(SRCDIR)flowc30$(S)c
$(OBJDIR)genc30$(S)$(O) : $(SRCDIR)genc30$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)genc30$(S)$(O) $(SRCDIR)genc30$(S)c
$(OBJDIR)peepc30$(S)$(O) : $(SRCDIR)peepc30$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)peepc30$(S)$(O) $(SRCDIR)peepc30$(S)c
$(OBJDIR)outc30_r$(S)$(O) : $(SRCDIR)outc30_r$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outc30_r$(S)$(O) $(SRCDIR)outc30_r$(S)c
$(OBJDIR)regc30$(S)$(O) : $(SRCDIR)regc30$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)regc30$(S)$(O) $(SRCDIR)regc30$(S)c
#	Object files specific to PowerPC versions
$(OBJDIR)genppc$(S)$(O) : $(SRCDIR)genppc$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)genppc$(S)$(O) $(SRCDIR)genppc$(S)c
$(OBJDIR)peepppc$(S)$(O) : $(SRCDIR)peepppc$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)peepppc$(S)$(O) $(SRCDIR)peepppc$(S)c
$(OBJDIR)outppc$(S)$(O) : $(SRCDIR)outppc$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)outppc$(S)$(O) $(SRCDIR)outppc$(S)c
$(OBJDIR)regppc$(S)$(O) : $(SRCDIR)regppc$(S)c
	$(CC) $(CFLAGS) $(OF)$(OBJDIR)regppc$(S)$(O) $(SRCDIR)regppc$(S)c

clobber:	clean
		-$(RM) $(BINDIR)$(PROG)
		-$(RM) *$(S)exe
		-$(RM) *$(S)img
clean:
		-$(RM) *$(S)cx
		-$(RM) *$(S)ln
		-$(RM) $(OBJDIR)*$(S)$(O)
		-$(RM) $(OBJDIR)*$(S)i
		-$(RM) $(OBJDIR)*$(S)s
		-$(RM) *$(S)uue
		-$(RM) *$(S)rs?
		-$(RM) *$(S)rzc

#------------------------------------------------------------------------------
#	Source code diagnostic tools

lint:		$(LNS)
		lint $(LNS)

lclint:
		lclint $(SRC)

pclint:
		lin $(SRC)

#	Use PCLINT with settings appropriate to QDOS/SMS systems
qllint:
		lint-nt ql.lnt c68.lnt

cxref:		$(CXS)
		cxref $(CXFLAGS) -c $(CXS)

#------------------------------------------------------------------------------
# Special section to handle building programs 
# under the SIBO SDK (TopSpeed) compiler on DOS
# This uses completely different syntax to virtually
# all the other options.

TSOBJS	 =$(OBJ) $(OBJX86) $(OBJ86) 

sibo:  c86check.img c86code.img
c86code:   c86code.img
c86check:  c86check.img

c86check.img : checkok c86.rss $(TSOBJS)
		tsc /m c86
		copy c86.img c86check.img


c86code.img : codeok c86.rss $(TSOBJS)
		tsc /m c86
		copy c86.img c86code.img
checkok:
		config config.chk
codeok:
		config config.gen

c86.rss : c86.c message.h config.h
		$(RM) c86.i
		$(RM) c86.rss
		tsc c86.c /option(pre_proc=on) /zq
		mv c86.i c86.rss
		rcomp16 c86.rss
		rchuf hwim -v -ic86.rsc

#------------------------------------------------------------------------------
# reformat source files to ensure consistant style
indent:
		@for file in $(SRC) $(SRC68K) $(SRCX86) $(SRC386) \
			$(SRC86) $(SRCARM) $(SRCC30) $(SRCPPC) \
			$(HDR) $(HDR68K) $(HDRX86) $(HDR386) $(HDR86) \
			$(HDRARM) $(HDRC30) $(HDRPPC) ; do \
			echo $$file:; \
			indent $(INDENTFLAGS) $$file && rm $$file~; \
		done

#------------------------------------------------------------------------------
# build the whole compiler as one huge .c file.   This can make the
# resulting binary smaller on some systems.
c68s:	$(SRC) $(HDR) $(HDR68K)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OF)$@ cmains$(S)c $(LIB)

#------------------------------------------------------------------------------
# updating david via email
david:	zip
	uuencode $(PROG)src$(S)zip $(PROG)src$(S)zip \
		| mail -s "Latest $(PROG)" d.j.walker@x400.icl.co.uk
	rm -f $(PROG)src$(S)zip

#------------------------------------------------------------------------------
# putting source on floppy to take home
home:
	tar cvf /var/tmp/$(PROG)$(S)tar \
		Makefile $(SRCSIBO) $(SRCEPOC) $(SRCLINT) \
	   $(SRC) $(SRC68K) $(SRCX86) $(SRC386) $(SRC86) $(SRCARM) $(SRCC30) \
	   $(SRCPPC) $(SRCEXTR) \
	   $(HDR) $(HDR68K) $(HDRX86) $(HDR386) $(HDR86) $(HDRARM) $(HDRC30) \
	   $(HDRPPC)
	gzip /var/tmp/$(PROG)$(S)tar
	rcp /var/tmp/$(PROG)$(S)tar$(S)gz melody:/var/tmp/$(PROG)$(S)tgz
	rm /var/tmp/$(PROG)$(S)tar$(S)gz
	rsh melody dd if=/var/tmp/$(PROG)$(S)tgz of=/dev/rdsk/f03ht bs=18k
	rsh melody rm /var/tmp/$(PROG)$(S)tgz

#------------------------------------------------------------------------------
# putting source on floppy
floppy:
	tar zcvf /dev/fd0 \
	   Makefile $(SRCSIBO) $(SRCEPOC) $(SRCLINT) \
	   $(SRC) $(SRC68K) $(SRCX86) $(SRC386) $(SRC86) $(SRCARM) $(SRCC30) \
	   $(SRCPPC) $(SRCEXTR) \
	   $(HDR) $(HDR68K) $(HDRX86) $(HDR386) $(HDR86) $(HDRARM) $(HDRC30) \
	   $(HDRPPC)

#------------------------------------------------------------------------------
# putting source on 720K floppies to transfer to Atari ST minix system
floppy720:
	tar cvf /dev/fd0 [a-fh-o]??*$(S)c ???*$(S)h c86$(S)*
	@echo -n "Insert 2nd diskette:"; read DUMMY
	tar cvf /dev/fd0 [g]??*$(S)c
	@echo -n "Insert 3nd diskette:"; read DUMMY
	tar cvf /dev/fd0 [p-z]??*$(S)c

#------------------------------------------------------------------------------
# creating a uuencoded gziped tar archive
tar:
	tar cvf $(PROG)$(S)tar Makefile $(SRCSIBO) $(SRCEPOC) $(SRCLINT) \
	   $(SRC) $(SRC68K) $(SRCX86) $(SRC386) $(SRC86) $(SRCARM) $(SRCC30) \
	   $(SRCPPC) $(SRCEXTR) \
	   $(HDR) $(HDR68K) $(HDRX86) $(HDR386) $(HDR86) $(HDRARM) $(HDRC30) \
	   $(HDRPPC)
	gzip $(PROG)$(S)tar
	uuencode $(PROG)$(S)tar$(S)gz $(PROG)$(S)tgz >$(PROG)$(S)uue
	rm -f $(PROG)$(S)tar$(S)gz

#------------------------------------------------------------------------------
# creating a zip archive
zip:
	zip $(PROG)src$(S)zip Makefile $(SRCSIBO) $(SRCEPOC) $(SRCLINT) \
	   $(SRC) $(SRC68K) $(SRCX86) $(SRC386) $(SRC86) $(SRCARM) $(SRCC30) \
	   $(SRCPPC) $(SRCEXTR) \
	   $(HDR) $(HDR68K) $(HDRX86) $(HDR386) $(HDR86) $(HDRARM) $(HDRC30) \
	   $(HDRPPC)

#------------------------------------------------------------------------------
# creating a zip archive
winzip:
	wzzip $(PROG)src$(S)zip Makefile $(SRCSIBO) $(SRCEPOC) $(SRCLINT) \
	   $(SRC) $(SRC68K) $(SRCX86) $(SRC386) $(SRC86) $(SRCARM) $(SRCC30) \
	   $(SRCPPC) $(SRCEXTR) \
	   $(HDR) $(HDR68K) $(HDRX86) $(HDR386) $(HDR86) $(HDRARM) $(HDRC30) \
	   $(HDRPPC)

#------------------------------------------------------------------------------
# creating a pkzip archive
# (because of limitations on command line length
#  we have to build up the archive in bits!)
pkzip:
	pkzip $(PROG)src$(S)zip Makefile $(SRC1) 
	pkzip $(PROG)src$(S)zip $(SRC2) 
	pkzip $(PROG)src$(S)zip $(SRC3) 
	pkzip $(PROG)src$(S)zip $(SRC4) 
	pkzip $(PROG)src$(S)zip $(SRC68K)
	pkzip $(PROG)src$(S)zip $(SRCX86)
	pkzip $(PROG)src$(S)zip $(SRC386)
	pkzip $(PROG)src$(S)zip $(SRC86)
	pkzip $(PROG)src$(S)zip $(SRCARM)
	pkzip $(PROG)src$(S)zip $(SRCC30)
	pkzip $(PROG)src$(S)zip $(SRCPPC)
	pkzip $(PROG)src$(S)zip $(SRCEXTR)
	pkzip $(PROG)src$(S)zip $(SRCSIBO)
	pkzip $(PROG)src$(S)zip $(SRCEPOC)
	pkzip $(PROG)src$(S)zip $(SRCLINT)
	pkzip $(PROG)src$(S)zip $(HDR) 
	pkzip $(PROG)src$(S)zip $(HDR68K)
	pkzip $(PROG)src$(S)zip $(HDRX86)
	pkzip $(PROG)src$(S)zip $(HDR386)
	pkzip $(PROG)src$(S)zip $(HDR86)
	pkzip $(PROG)src$(S)zip $(HDRARM)
	pkzip $(PROG)src$(S)zip $(HDRC30)
	pkzip $(PROG)src$(S)zip $(HDRPPC)





