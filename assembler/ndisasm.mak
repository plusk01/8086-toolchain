# Microsoft Developer Studio Generated NMAKE File, Based on ndisasm.dsp
!IF "$(CFG)" == ""
CFG=ndisasm - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ndisasm - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ndisasm - Win32 Release" && "$(CFG)" != "ndisasm - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ndisasm.mak" CFG="ndisasm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ndisasm - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ndisasm - Win32 Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "ndisasm - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\ndisasm.exe"


CLEAN :
	-@erase "$(INTDIR)\disasm.obj"
	-@erase "$(INTDIR)\insnsd.obj"
	-@erase "$(INTDIR)\nasmlib.obj"
	-@erase "$(INTDIR)\ndisasm.obj"
	-@erase "$(INTDIR)\sync.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\ndisasm.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\ndisasm.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ndisasm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\ndisasm.pdb" /machine:I386 /out:"$(OUTDIR)\ndisasm.exe" 
LINK32_OBJS= \
	"$(INTDIR)\insnsd.obj" \
	"$(INTDIR)\nasmlib.obj" \
	"$(INTDIR)\ndisasm.obj" \
	"$(INTDIR)\disasm.obj" \
	"$(INTDIR)\sync.obj"

"$(OUTDIR)\ndisasm.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ndisasm - Win32 Debug"

OUTDIR=.\ndisasm___Win32_Debug
INTDIR=.\ndisasm___Win32_Debug
# Begin Custom Macros
OutDir=.\ndisasm___Win32_Debug
# End Custom Macros

ALL : "$(OUTDIR)\ndisasm.exe"


CLEAN :
	-@erase "$(INTDIR)\disasm.obj"
	-@erase "$(INTDIR)\insnsd.obj"
	-@erase "$(INTDIR)\nasmlib.obj"
	-@erase "$(INTDIR)\ndisasm.obj"
	-@erase "$(INTDIR)\sync.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\ndisasm.exe"
	-@erase "$(OUTDIR)\ndisasm.ilk"
	-@erase "$(OUTDIR)\ndisasm.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\ndisasm.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ  /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ndisasm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\ndisasm.pdb" /debug /machine:I386 /out:"$(OUTDIR)\ndisasm.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\insnsd.obj" \
	"$(INTDIR)\nasmlib.obj" \
	"$(INTDIR)\ndisasm.obj" \
	"$(INTDIR)\disasm.obj" \
	"$(INTDIR)\sync.obj"

"$(OUTDIR)\ndisasm.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("ndisasm.dep")
!INCLUDE "ndisasm.dep"
!ELSE 
!MESSAGE Warning: cannot find "ndisasm.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ndisasm - Win32 Release" || "$(CFG)" == "ndisasm - Win32 Debug"
SOURCE=.\disasm.c

"$(INTDIR)\disasm.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\insnsd.c

"$(INTDIR)\insnsd.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\nasmlib.c

"$(INTDIR)\nasmlib.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ndisasm.c

"$(INTDIR)\ndisasm.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\sync.c

"$(INTDIR)\sync.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

