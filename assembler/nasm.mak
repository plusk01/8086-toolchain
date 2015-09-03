# Microsoft Developer Studio Generated NMAKE File, Based on nasm.dsp
!IF "$(CFG)" == ""
CFG=nasm - Win32 Debug
!MESSAGE No configuration specified. Defaulting to nasm - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "nasm - Win32 Release" && "$(CFG)" != "nasm - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nasm.mak" CFG="nasm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nasm - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "nasm - Win32 Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "nasm - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\nasm.exe"


CLEAN :
	-@erase "$(INTDIR)\assemble.obj"
	-@erase "$(INTDIR)\eval.obj"
	-@erase "$(INTDIR)\float.obj"
	-@erase "$(INTDIR)\insnsa.obj"
	-@erase "$(INTDIR)\labels.obj"
	-@erase "$(INTDIR)\listing.obj"
	-@erase "$(INTDIR)\nasm.obj"
	-@erase "$(INTDIR)\nasmlib.obj"
	-@erase "$(INTDIR)\outaout.obj"
	-@erase "$(INTDIR)\outas86.obj"
	-@erase "$(INTDIR)\outbin.obj"
	-@erase "$(INTDIR)\outcoff.obj"
	-@erase "$(INTDIR)\outdbg.obj"
	-@erase "$(INTDIR)\outelf.obj"
	-@erase "$(INTDIR)\outform.obj"
	-@erase "$(INTDIR)\outieee.obj"
	-@erase "$(INTDIR)\outobj.obj"
	-@erase "$(INTDIR)\outrdf2.obj"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\preproc.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\nasm.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "./" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\nasm.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\nasm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\nasm.pdb" /machine:I386 /out:"$(OUTDIR)\nasm.exe" 
LINK32_OBJS= \
	"$(INTDIR)\assemble.obj" \
	"$(INTDIR)\eval.obj" \
	"$(INTDIR)\float.obj" \
	"$(INTDIR)\insnsa.obj" \
	"$(INTDIR)\labels.obj" \
	"$(INTDIR)\listing.obj" \
	"$(INTDIR)\nasm.obj" \
	"$(INTDIR)\nasmlib.obj" \
	"$(INTDIR)\outaout.obj" \
	"$(INTDIR)\outas86.obj" \
	"$(INTDIR)\outbin.obj" \
	"$(INTDIR)\outcoff.obj" \
	"$(INTDIR)\outdbg.obj" \
	"$(INTDIR)\outelf.obj" \
	"$(INTDIR)\outform.obj" \
	"$(INTDIR)\outieee.obj" \
	"$(INTDIR)\outobj.obj" \
	"$(INTDIR)\outrdf2.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\preproc.obj"

"$(OUTDIR)\nasm.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\nasm.exe" "$(OUTDIR)\nasm.bsc"


CLEAN :
	-@erase "$(INTDIR)\assemble.obj"
	-@erase "$(INTDIR)\assemble.sbr"
	-@erase "$(INTDIR)\eval.obj"
	-@erase "$(INTDIR)\eval.sbr"
	-@erase "$(INTDIR)\float.obj"
	-@erase "$(INTDIR)\float.sbr"
	-@erase "$(INTDIR)\insnsa.obj"
	-@erase "$(INTDIR)\insnsa.sbr"
	-@erase "$(INTDIR)\labels.obj"
	-@erase "$(INTDIR)\labels.sbr"
	-@erase "$(INTDIR)\listing.obj"
	-@erase "$(INTDIR)\listing.sbr"
	-@erase "$(INTDIR)\nasm.obj"
	-@erase "$(INTDIR)\nasm.sbr"
	-@erase "$(INTDIR)\nasmlib.obj"
	-@erase "$(INTDIR)\nasmlib.sbr"
	-@erase "$(INTDIR)\outaout.obj"
	-@erase "$(INTDIR)\outaout.sbr"
	-@erase "$(INTDIR)\outas86.obj"
	-@erase "$(INTDIR)\outas86.sbr"
	-@erase "$(INTDIR)\outbin.obj"
	-@erase "$(INTDIR)\outbin.sbr"
	-@erase "$(INTDIR)\outcoff.obj"
	-@erase "$(INTDIR)\outcoff.sbr"
	-@erase "$(INTDIR)\outdbg.obj"
	-@erase "$(INTDIR)\outdbg.sbr"
	-@erase "$(INTDIR)\outelf.obj"
	-@erase "$(INTDIR)\outelf.sbr"
	-@erase "$(INTDIR)\outform.obj"
	-@erase "$(INTDIR)\outform.sbr"
	-@erase "$(INTDIR)\outieee.obj"
	-@erase "$(INTDIR)\outieee.sbr"
	-@erase "$(INTDIR)\outobj.obj"
	-@erase "$(INTDIR)\outobj.sbr"
	-@erase "$(INTDIR)\outrdf2.obj"
	-@erase "$(INTDIR)\outrdf2.sbr"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\parser.sbr"
	-@erase "$(INTDIR)\preproc.obj"
	-@erase "$(INTDIR)\preproc.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\nasm.bsc"
	-@erase "$(OUTDIR)\nasm.exe"
	-@erase "$(OUTDIR)\nasm.ilk"
	-@erase "$(OUTDIR)\nasm.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\nasm.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\nasm.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\assemble.sbr" \
	"$(INTDIR)\eval.sbr" \
	"$(INTDIR)\float.sbr" \
	"$(INTDIR)\insnsa.sbr" \
	"$(INTDIR)\labels.sbr" \
	"$(INTDIR)\listing.sbr" \
	"$(INTDIR)\nasm.sbr" \
	"$(INTDIR)\nasmlib.sbr" \
	"$(INTDIR)\outaout.sbr" \
	"$(INTDIR)\outas86.sbr" \
	"$(INTDIR)\outbin.sbr" \
	"$(INTDIR)\outcoff.sbr" \
	"$(INTDIR)\outdbg.sbr" \
	"$(INTDIR)\outelf.sbr" \
	"$(INTDIR)\outform.sbr" \
	"$(INTDIR)\outieee.sbr" \
	"$(INTDIR)\outobj.sbr" \
	"$(INTDIR)\outrdf2.sbr" \
	"$(INTDIR)\parser.sbr" \
	"$(INTDIR)\preproc.sbr"

"$(OUTDIR)\nasm.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\nasm.pdb" /debug /machine:I386 /out:"$(OUTDIR)\nasm.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\assemble.obj" \
	"$(INTDIR)\eval.obj" \
	"$(INTDIR)\float.obj" \
	"$(INTDIR)\insnsa.obj" \
	"$(INTDIR)\labels.obj" \
	"$(INTDIR)\listing.obj" \
	"$(INTDIR)\nasm.obj" \
	"$(INTDIR)\nasmlib.obj" \
	"$(INTDIR)\outaout.obj" \
	"$(INTDIR)\outas86.obj" \
	"$(INTDIR)\outbin.obj" \
	"$(INTDIR)\outcoff.obj" \
	"$(INTDIR)\outdbg.obj" \
	"$(INTDIR)\outelf.obj" \
	"$(INTDIR)\outform.obj" \
	"$(INTDIR)\outieee.obj" \
	"$(INTDIR)\outobj.obj" \
	"$(INTDIR)\outrdf2.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\preproc.obj"

"$(OUTDIR)\nasm.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("nasm.dep")
!INCLUDE "nasm.dep"
!ELSE 
!MESSAGE Warning: cannot find "nasm.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "nasm - Win32 Release" || "$(CFG)" == "nasm - Win32 Debug"
SOURCE=.\assemble.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\assemble.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\assemble.obj"	"$(INTDIR)\assemble.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\eval.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\eval.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\eval.obj"	"$(INTDIR)\eval.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\float.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\float.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\float.obj"	"$(INTDIR)\float.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\insnsa.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\insnsa.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\insnsa.obj"	"$(INTDIR)\insnsa.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\labels.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\labels.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\labels.obj"	"$(INTDIR)\labels.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\listing.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\listing.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\listing.obj"	"$(INTDIR)\listing.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\nasm.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\nasm.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\nasm.obj"	"$(INTDIR)\nasm.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\nasmlib.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\nasmlib.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\nasmlib.obj"	"$(INTDIR)\nasmlib.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\output\outaout.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\outaout.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\outaout.obj"	"$(INTDIR)\outaout.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\output\outas86.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\outas86.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\outas86.obj"	"$(INTDIR)\outas86.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\output\outbin.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\outbin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\outbin.obj"	"$(INTDIR)\outbin.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\output\outcoff.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\outcoff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\outcoff.obj"	"$(INTDIR)\outcoff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\output\outdbg.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\outdbg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\outdbg.obj"	"$(INTDIR)\outdbg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\output\outelf.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\outelf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\outelf.obj"	"$(INTDIR)\outelf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\outform.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\outform.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\outform.obj"	"$(INTDIR)\outform.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\output\outieee.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\outieee.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\outieee.obj"	"$(INTDIR)\outieee.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\output\outobj.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\outobj.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\outobj.obj"	"$(INTDIR)\outobj.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\output\outrdf2.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\outrdf2.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\outrdf2.obj"	"$(INTDIR)\outrdf2.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\parser.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\parser.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\parser.obj"	"$(INTDIR)\parser.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\preproc.c

!IF  "$(CFG)" == "nasm - Win32 Release"


"$(INTDIR)\preproc.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nasm - Win32 Debug"


"$(INTDIR)\preproc.obj"	"$(INTDIR)\preproc.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

