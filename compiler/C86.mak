# Microsoft Developer Studio Generated NMAKE File, Based on C86.dsp
!IF "$(CFG)" == ""
CFG=c86 - Win32 Release
!MESSAGE No configuration specified. Defaulting to c86 - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "c86 - Win32 Release" && "$(CFG)" != "c86 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "C86.mak" CFG="c86 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "c86 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "c86 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "c86 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\c86.exe"


CLEAN :
	-@erase "$(INTDIR)\Analyze.obj"
	-@erase "$(INTDIR)\Cglbdef.obj"
	-@erase "$(INTDIR)\Cmain.obj"
	-@erase "$(INTDIR)\Decl.obj"
	-@erase "$(INTDIR)\Expr.obj"
	-@erase "$(INTDIR)\Extern.obj"
	-@erase "$(INTDIR)\Flow68k.obj"
	-@erase "$(INTDIR)\Flowc30.obj"
	-@erase "$(INTDIR)\Gen386.obj"
	-@erase "$(INTDIR)\Gen68k.obj"
	-@erase "$(INTDIR)\Gen86.obj"
	-@erase "$(INTDIR)\Genarm.obj"
	-@erase "$(INTDIR)\Genc30.obj"
	-@erase "$(INTDIR)\Genffp.obj"
	-@erase "$(INTDIR)\Genicode.obj"
	-@erase "$(INTDIR)\Genieee.obj"
	-@erase "$(INTDIR)\geninit.obj"
	-@erase "$(INTDIR)\Genppc.obj"
	-@erase "$(INTDIR)\Genstmt.obj"
	-@erase "$(INTDIR)\Genutil.obj"
	-@erase "$(INTDIR)\Getsym.obj"
	-@erase "$(INTDIR)\Init.obj"
	-@erase "$(INTDIR)\Intexpr.obj"
	-@erase "$(INTDIR)\List.obj"
	-@erase "$(INTDIR)\Memmgt.obj"
	-@erase "$(INTDIR)\Msgout.obj"
	-@erase "$(INTDIR)\Optimize.obj"
	-@erase "$(INTDIR)\Out68k_a.obj"
	-@erase "$(INTDIR)\Out68k_c.obj"
	-@erase "$(INTDIR)\Out68k_g.obj"
	-@erase "$(INTDIR)\Out68k_q.obj"
	-@erase "$(INTDIR)\Outarm_o.obj"
	-@erase "$(INTDIR)\Outc30_r.obj"
	-@erase "$(INTDIR)\Outgen.obj"
	-@erase "$(INTDIR)\Outppc.obj"
	-@erase "$(INTDIR)\Outx86_a.obj"
	-@erase "$(INTDIR)\Outx86_b.obj"
	-@erase "$(INTDIR)\Outx86_g.obj"
	-@erase "$(INTDIR)\outx86_n.obj"
	-@erase "$(INTDIR)\Outx86_s.obj"
	-@erase "$(INTDIR)\Peep68k.obj"
	-@erase "$(INTDIR)\Peeparm.obj"
	-@erase "$(INTDIR)\Peepc30.obj"
	-@erase "$(INTDIR)\Peepppc.obj"
	-@erase "$(INTDIR)\Peepx86.obj"
	-@erase "$(INTDIR)\Reg68k.obj"
	-@erase "$(INTDIR)\Regarm.obj"
	-@erase "$(INTDIR)\Regc30.obj"
	-@erase "$(INTDIR)\Regppc.obj"
	-@erase "$(INTDIR)\Regx86.obj"
	-@erase "$(INTDIR)\Stmt.obj"
	-@erase "$(INTDIR)\Symbol.obj"
	-@erase "$(INTDIR)\System.obj"
	-@erase "$(INTDIR)\Types.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\c86.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\C86.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\c86.pdb" /machine:I386 /out:"$(OUTDIR)\c86.exe" 
LINK32_OBJS= \
	"$(INTDIR)\Analyze.obj" \
	"$(INTDIR)\Cglbdef.obj" \
	"$(INTDIR)\Cmain.obj" \
	"$(INTDIR)\Decl.obj" \
	"$(INTDIR)\Expr.obj" \
	"$(INTDIR)\Extern.obj" \
	"$(INTDIR)\Flow68k.obj" \
	"$(INTDIR)\Flowc30.obj" \
	"$(INTDIR)\Gen386.obj" \
	"$(INTDIR)\Gen68k.obj" \
	"$(INTDIR)\Gen86.obj" \
	"$(INTDIR)\Genarm.obj" \
	"$(INTDIR)\Genc30.obj" \
	"$(INTDIR)\Genffp.obj" \
	"$(INTDIR)\Genicode.obj" \
	"$(INTDIR)\Genieee.obj" \
	"$(INTDIR)\geninit.obj" \
	"$(INTDIR)\Genppc.obj" \
	"$(INTDIR)\Genstmt.obj" \
	"$(INTDIR)\Genutil.obj" \
	"$(INTDIR)\Getsym.obj" \
	"$(INTDIR)\Init.obj" \
	"$(INTDIR)\Intexpr.obj" \
	"$(INTDIR)\List.obj" \
	"$(INTDIR)\Memmgt.obj" \
	"$(INTDIR)\Msgout.obj" \
	"$(INTDIR)\Optimize.obj" \
	"$(INTDIR)\Out68k_a.obj" \
	"$(INTDIR)\Out68k_c.obj" \
	"$(INTDIR)\Out68k_g.obj" \
	"$(INTDIR)\Out68k_q.obj" \
	"$(INTDIR)\Outarm_o.obj" \
	"$(INTDIR)\Outc30_r.obj" \
	"$(INTDIR)\Outgen.obj" \
	"$(INTDIR)\Outppc.obj" \
	"$(INTDIR)\Outx86_a.obj" \
	"$(INTDIR)\Outx86_b.obj" \
	"$(INTDIR)\Outx86_g.obj" \
	"$(INTDIR)\outx86_n.obj" \
	"$(INTDIR)\Outx86_s.obj" \
	"$(INTDIR)\Peep68k.obj" \
	"$(INTDIR)\Peeparm.obj" \
	"$(INTDIR)\Peepc30.obj" \
	"$(INTDIR)\Peepppc.obj" \
	"$(INTDIR)\Peepx86.obj" \
	"$(INTDIR)\Reg68k.obj" \
	"$(INTDIR)\Regarm.obj" \
	"$(INTDIR)\Regc30.obj" \
	"$(INTDIR)\Regppc.obj" \
	"$(INTDIR)\Regx86.obj" \
	"$(INTDIR)\Stmt.obj" \
	"$(INTDIR)\Symbol.obj" \
	"$(INTDIR)\System.obj" \
	"$(INTDIR)\Types.obj"

"$(OUTDIR)\c86.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\C86.exe" "$(OUTDIR)\C86.bsc"


CLEAN :
	-@erase "$(INTDIR)\Analyze.obj"
	-@erase "$(INTDIR)\Analyze.sbr"
	-@erase "$(INTDIR)\Cglbdef.obj"
	-@erase "$(INTDIR)\Cglbdef.sbr"
	-@erase "$(INTDIR)\Cmain.obj"
	-@erase "$(INTDIR)\Cmain.sbr"
	-@erase "$(INTDIR)\Decl.obj"
	-@erase "$(INTDIR)\Decl.sbr"
	-@erase "$(INTDIR)\Expr.obj"
	-@erase "$(INTDIR)\Expr.sbr"
	-@erase "$(INTDIR)\Extern.obj"
	-@erase "$(INTDIR)\Extern.sbr"
	-@erase "$(INTDIR)\Flow68k.obj"
	-@erase "$(INTDIR)\Flow68k.sbr"
	-@erase "$(INTDIR)\Flowc30.obj"
	-@erase "$(INTDIR)\Flowc30.sbr"
	-@erase "$(INTDIR)\Gen386.obj"
	-@erase "$(INTDIR)\Gen386.sbr"
	-@erase "$(INTDIR)\Gen68k.obj"
	-@erase "$(INTDIR)\Gen68k.sbr"
	-@erase "$(INTDIR)\Gen86.obj"
	-@erase "$(INTDIR)\Gen86.sbr"
	-@erase "$(INTDIR)\Genarm.obj"
	-@erase "$(INTDIR)\Genarm.sbr"
	-@erase "$(INTDIR)\Genc30.obj"
	-@erase "$(INTDIR)\Genc30.sbr"
	-@erase "$(INTDIR)\Genffp.obj"
	-@erase "$(INTDIR)\Genffp.sbr"
	-@erase "$(INTDIR)\Genicode.obj"
	-@erase "$(INTDIR)\Genicode.sbr"
	-@erase "$(INTDIR)\Genieee.obj"
	-@erase "$(INTDIR)\Genieee.sbr"
	-@erase "$(INTDIR)\geninit.obj"
	-@erase "$(INTDIR)\geninit.sbr"
	-@erase "$(INTDIR)\Genppc.obj"
	-@erase "$(INTDIR)\Genppc.sbr"
	-@erase "$(INTDIR)\Genstmt.obj"
	-@erase "$(INTDIR)\Genstmt.sbr"
	-@erase "$(INTDIR)\Genutil.obj"
	-@erase "$(INTDIR)\Genutil.sbr"
	-@erase "$(INTDIR)\Getsym.obj"
	-@erase "$(INTDIR)\Getsym.sbr"
	-@erase "$(INTDIR)\Init.obj"
	-@erase "$(INTDIR)\Init.sbr"
	-@erase "$(INTDIR)\Intexpr.obj"
	-@erase "$(INTDIR)\Intexpr.sbr"
	-@erase "$(INTDIR)\List.obj"
	-@erase "$(INTDIR)\List.sbr"
	-@erase "$(INTDIR)\Memmgt.obj"
	-@erase "$(INTDIR)\Memmgt.sbr"
	-@erase "$(INTDIR)\Msgout.obj"
	-@erase "$(INTDIR)\Msgout.sbr"
	-@erase "$(INTDIR)\Optimize.obj"
	-@erase "$(INTDIR)\Optimize.sbr"
	-@erase "$(INTDIR)\Out68k_a.obj"
	-@erase "$(INTDIR)\Out68k_a.sbr"
	-@erase "$(INTDIR)\Out68k_c.obj"
	-@erase "$(INTDIR)\Out68k_c.sbr"
	-@erase "$(INTDIR)\Out68k_g.obj"
	-@erase "$(INTDIR)\Out68k_g.sbr"
	-@erase "$(INTDIR)\Out68k_q.obj"
	-@erase "$(INTDIR)\Out68k_q.sbr"
	-@erase "$(INTDIR)\Outarm_o.obj"
	-@erase "$(INTDIR)\Outarm_o.sbr"
	-@erase "$(INTDIR)\Outc30_r.obj"
	-@erase "$(INTDIR)\Outc30_r.sbr"
	-@erase "$(INTDIR)\Outgen.obj"
	-@erase "$(INTDIR)\Outgen.sbr"
	-@erase "$(INTDIR)\Outppc.obj"
	-@erase "$(INTDIR)\Outppc.sbr"
	-@erase "$(INTDIR)\Outx86_a.obj"
	-@erase "$(INTDIR)\Outx86_a.sbr"
	-@erase "$(INTDIR)\Outx86_b.obj"
	-@erase "$(INTDIR)\Outx86_b.sbr"
	-@erase "$(INTDIR)\Outx86_g.obj"
	-@erase "$(INTDIR)\Outx86_g.sbr"
	-@erase "$(INTDIR)\outx86_n.obj"
	-@erase "$(INTDIR)\outx86_n.sbr"
	-@erase "$(INTDIR)\Outx86_s.obj"
	-@erase "$(INTDIR)\Outx86_s.sbr"
	-@erase "$(INTDIR)\Peep68k.obj"
	-@erase "$(INTDIR)\Peep68k.sbr"
	-@erase "$(INTDIR)\Peeparm.obj"
	-@erase "$(INTDIR)\Peeparm.sbr"
	-@erase "$(INTDIR)\Peepc30.obj"
	-@erase "$(INTDIR)\Peepc30.sbr"
	-@erase "$(INTDIR)\Peepppc.obj"
	-@erase "$(INTDIR)\Peepppc.sbr"
	-@erase "$(INTDIR)\Peepx86.obj"
	-@erase "$(INTDIR)\Peepx86.sbr"
	-@erase "$(INTDIR)\Reg68k.obj"
	-@erase "$(INTDIR)\Reg68k.sbr"
	-@erase "$(INTDIR)\Regarm.obj"
	-@erase "$(INTDIR)\Regarm.sbr"
	-@erase "$(INTDIR)\Regc30.obj"
	-@erase "$(INTDIR)\Regc30.sbr"
	-@erase "$(INTDIR)\Regppc.obj"
	-@erase "$(INTDIR)\Regppc.sbr"
	-@erase "$(INTDIR)\Regx86.obj"
	-@erase "$(INTDIR)\Regx86.sbr"
	-@erase "$(INTDIR)\Stmt.obj"
	-@erase "$(INTDIR)\Stmt.sbr"
	-@erase "$(INTDIR)\Symbol.obj"
	-@erase "$(INTDIR)\Symbol.sbr"
	-@erase "$(INTDIR)\System.obj"
	-@erase "$(INTDIR)\System.sbr"
	-@erase "$(INTDIR)\Types.obj"
	-@erase "$(INTDIR)\Types.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\C86.bsc"
	-@erase "$(OUTDIR)\C86.exe"
	-@erase "$(OUTDIR)\C86.ilk"
	-@erase "$(OUTDIR)\C86.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W4 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\C86.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\Analyze.sbr" \
	"$(INTDIR)\Cglbdef.sbr" \
	"$(INTDIR)\Cmain.sbr" \
	"$(INTDIR)\Decl.sbr" \
	"$(INTDIR)\Expr.sbr" \
	"$(INTDIR)\Extern.sbr" \
	"$(INTDIR)\Flow68k.sbr" \
	"$(INTDIR)\Flowc30.sbr" \
	"$(INTDIR)\Gen386.sbr" \
	"$(INTDIR)\Gen68k.sbr" \
	"$(INTDIR)\Gen86.sbr" \
	"$(INTDIR)\Genarm.sbr" \
	"$(INTDIR)\Genc30.sbr" \
	"$(INTDIR)\Genffp.sbr" \
	"$(INTDIR)\Genicode.sbr" \
	"$(INTDIR)\Genieee.sbr" \
	"$(INTDIR)\geninit.sbr" \
	"$(INTDIR)\Genppc.sbr" \
	"$(INTDIR)\Genstmt.sbr" \
	"$(INTDIR)\Genutil.sbr" \
	"$(INTDIR)\Getsym.sbr" \
	"$(INTDIR)\Init.sbr" \
	"$(INTDIR)\Intexpr.sbr" \
	"$(INTDIR)\List.sbr" \
	"$(INTDIR)\Memmgt.sbr" \
	"$(INTDIR)\Msgout.sbr" \
	"$(INTDIR)\Optimize.sbr" \
	"$(INTDIR)\Out68k_a.sbr" \
	"$(INTDIR)\Out68k_c.sbr" \
	"$(INTDIR)\Out68k_g.sbr" \
	"$(INTDIR)\Out68k_q.sbr" \
	"$(INTDIR)\Outarm_o.sbr" \
	"$(INTDIR)\Outc30_r.sbr" \
	"$(INTDIR)\Outgen.sbr" \
	"$(INTDIR)\Outppc.sbr" \
	"$(INTDIR)\Outx86_a.sbr" \
	"$(INTDIR)\Outx86_b.sbr" \
	"$(INTDIR)\Outx86_g.sbr" \
	"$(INTDIR)\outx86_n.sbr" \
	"$(INTDIR)\Outx86_s.sbr" \
	"$(INTDIR)\Peep68k.sbr" \
	"$(INTDIR)\Peeparm.sbr" \
	"$(INTDIR)\Peepc30.sbr" \
	"$(INTDIR)\Peepppc.sbr" \
	"$(INTDIR)\Peepx86.sbr" \
	"$(INTDIR)\Reg68k.sbr" \
	"$(INTDIR)\Regarm.sbr" \
	"$(INTDIR)\Regc30.sbr" \
	"$(INTDIR)\Regppc.sbr" \
	"$(INTDIR)\Regx86.sbr" \
	"$(INTDIR)\Stmt.sbr" \
	"$(INTDIR)\Symbol.sbr" \
	"$(INTDIR)\System.sbr" \
	"$(INTDIR)\Types.sbr"

"$(OUTDIR)\C86.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\C86.pdb" /debug /machine:I386 /out:"$(OUTDIR)\C86.exe" 
LINK32_OBJS= \
	"$(INTDIR)\Analyze.obj" \
	"$(INTDIR)\Cglbdef.obj" \
	"$(INTDIR)\Cmain.obj" \
	"$(INTDIR)\Decl.obj" \
	"$(INTDIR)\Expr.obj" \
	"$(INTDIR)\Extern.obj" \
	"$(INTDIR)\Flow68k.obj" \
	"$(INTDIR)\Flowc30.obj" \
	"$(INTDIR)\Gen386.obj" \
	"$(INTDIR)\Gen68k.obj" \
	"$(INTDIR)\Gen86.obj" \
	"$(INTDIR)\Genarm.obj" \
	"$(INTDIR)\Genc30.obj" \
	"$(INTDIR)\Genffp.obj" \
	"$(INTDIR)\Genicode.obj" \
	"$(INTDIR)\Genieee.obj" \
	"$(INTDIR)\geninit.obj" \
	"$(INTDIR)\Genppc.obj" \
	"$(INTDIR)\Genstmt.obj" \
	"$(INTDIR)\Genutil.obj" \
	"$(INTDIR)\Getsym.obj" \
	"$(INTDIR)\Init.obj" \
	"$(INTDIR)\Intexpr.obj" \
	"$(INTDIR)\List.obj" \
	"$(INTDIR)\Memmgt.obj" \
	"$(INTDIR)\Msgout.obj" \
	"$(INTDIR)\Optimize.obj" \
	"$(INTDIR)\Out68k_a.obj" \
	"$(INTDIR)\Out68k_c.obj" \
	"$(INTDIR)\Out68k_g.obj" \
	"$(INTDIR)\Out68k_q.obj" \
	"$(INTDIR)\Outarm_o.obj" \
	"$(INTDIR)\Outc30_r.obj" \
	"$(INTDIR)\Outgen.obj" \
	"$(INTDIR)\Outppc.obj" \
	"$(INTDIR)\Outx86_a.obj" \
	"$(INTDIR)\Outx86_b.obj" \
	"$(INTDIR)\Outx86_g.obj" \
	"$(INTDIR)\outx86_n.obj" \
	"$(INTDIR)\Outx86_s.obj" \
	"$(INTDIR)\Peep68k.obj" \
	"$(INTDIR)\Peeparm.obj" \
	"$(INTDIR)\Peepc30.obj" \
	"$(INTDIR)\Peepppc.obj" \
	"$(INTDIR)\Peepx86.obj" \
	"$(INTDIR)\Reg68k.obj" \
	"$(INTDIR)\Regarm.obj" \
	"$(INTDIR)\Regc30.obj" \
	"$(INTDIR)\Regppc.obj" \
	"$(INTDIR)\Regx86.obj" \
	"$(INTDIR)\Stmt.obj" \
	"$(INTDIR)\Symbol.obj" \
	"$(INTDIR)\System.obj" \
	"$(INTDIR)\Types.obj"

"$(OUTDIR)\C86.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("C86.dep")
!INCLUDE "C86.dep"
!ELSE 
!MESSAGE Warning: cannot find "C86.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "c86 - Win32 Release" || "$(CFG)" == "c86 - Win32 Debug"
SOURCE=.\Analyze.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Analyze.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Analyze.obj"	"$(INTDIR)\Analyze.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Cglbdef.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Cglbdef.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Cglbdef.obj"	"$(INTDIR)\Cglbdef.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Cmain.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Cmain.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Cmain.obj"	"$(INTDIR)\Cmain.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Decl.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Decl.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Decl.obj"	"$(INTDIR)\Decl.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Expr.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Expr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Expr.obj"	"$(INTDIR)\Expr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Extern.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Extern.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Extern.obj"	"$(INTDIR)\Extern.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Flow68k.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Flow68k.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Flow68k.obj"	"$(INTDIR)\Flow68k.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Flowc30.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Flowc30.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Flowc30.obj"	"$(INTDIR)\Flowc30.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Gen386.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Gen386.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Gen386.obj"	"$(INTDIR)\Gen386.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Gen68k.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Gen68k.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Gen68k.obj"	"$(INTDIR)\Gen68k.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Gen86.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Gen86.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Gen86.obj"	"$(INTDIR)\Gen86.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Genarm.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genarm.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genarm.obj"	"$(INTDIR)\Genarm.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Genc30.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genc30.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genc30.obj"	"$(INTDIR)\Genc30.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Genffp.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genffp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genffp.obj"	"$(INTDIR)\Genffp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Genicode.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genicode.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genicode.obj"	"$(INTDIR)\Genicode.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Genieee.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genieee.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genieee.obj"	"$(INTDIR)\Genieee.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\geninit.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\geninit.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\geninit.obj"	"$(INTDIR)\geninit.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Genppc.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genppc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genppc.obj"	"$(INTDIR)\Genppc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Genstmt.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genstmt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genstmt.obj"	"$(INTDIR)\Genstmt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Genutil.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Genutil.obj"	"$(INTDIR)\Genutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Getsym.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Getsym.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Getsym.obj"	"$(INTDIR)\Getsym.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Init.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Init.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Init.obj"	"$(INTDIR)\Init.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Intexpr.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Intexpr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Intexpr.obj"	"$(INTDIR)\Intexpr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\List.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\List.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\List.obj"	"$(INTDIR)\List.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Memmgt.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Memmgt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Memmgt.obj"	"$(INTDIR)\Memmgt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Msgout.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Msgout.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Msgout.obj"	"$(INTDIR)\Msgout.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Optimize.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Optimize.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Optimize.obj"	"$(INTDIR)\Optimize.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Out68k_a.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Out68k_a.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Out68k_a.obj"	"$(INTDIR)\Out68k_a.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Out68k_c.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Out68k_c.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Out68k_c.obj"	"$(INTDIR)\Out68k_c.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Out68k_g.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Out68k_g.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Out68k_g.obj"	"$(INTDIR)\Out68k_g.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Out68k_q.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Out68k_q.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Out68k_q.obj"	"$(INTDIR)\Out68k_q.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Outarm_o.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outarm_o.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outarm_o.obj"	"$(INTDIR)\Outarm_o.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Outc30_r.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outc30_r.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outc30_r.obj"	"$(INTDIR)\Outc30_r.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Outgen.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outgen.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outgen.obj"	"$(INTDIR)\Outgen.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Outppc.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outppc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outppc.obj"	"$(INTDIR)\Outppc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Outx86_a.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outx86_a.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outx86_a.obj"	"$(INTDIR)\Outx86_a.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Outx86_b.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outx86_b.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outx86_b.obj"	"$(INTDIR)\Outx86_b.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Outx86_g.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outx86_g.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outx86_g.obj"	"$(INTDIR)\Outx86_g.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\outx86_n.c

!IF  "$(CFG)" == "c86 - Win32 Release"


"$(INTDIR)\outx86_n.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"


"$(INTDIR)\outx86_n.obj"	"$(INTDIR)\outx86_n.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Outx86_s.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outx86_s.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Outx86_s.obj"	"$(INTDIR)\Outx86_s.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Peep68k.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Peep68k.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Peep68k.obj"	"$(INTDIR)\Peep68k.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Peeparm.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Peeparm.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Peeparm.obj"	"$(INTDIR)\Peeparm.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Peepc30.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Peepc30.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Peepc30.obj"	"$(INTDIR)\Peepc30.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Peepppc.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Peepppc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Peepppc.obj"	"$(INTDIR)\Peepppc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Peepx86.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Peepx86.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Peepx86.obj"	"$(INTDIR)\Peepx86.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Reg68k.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Reg68k.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Reg68k.obj"	"$(INTDIR)\Reg68k.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Regarm.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Regarm.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Regarm.obj"	"$(INTDIR)\Regarm.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Regc30.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Regc30.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Regc30.obj"	"$(INTDIR)\Regc30.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Regppc.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Regppc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Regppc.obj"	"$(INTDIR)\Regppc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Regx86.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Regx86.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Regx86.obj"	"$(INTDIR)\Regx86.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Stmt.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Stmt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Stmt.obj"	"$(INTDIR)\Stmt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Symbol.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Symbol.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Symbol.obj"	"$(INTDIR)\Symbol.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\System.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\System.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\System.obj"	"$(INTDIR)\System.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Types.c

!IF  "$(CFG)" == "c86 - Win32 Release"

CPP_SWITCHES=/nologo /ML /W4 /GX /O2 /I "E:\DJW\PSION\SRC\C86" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Types.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "c86 - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "E:\DJW\PSION\SRC\C86" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__STDC__" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\C86.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Types.obj"	"$(INTDIR)\Types.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

