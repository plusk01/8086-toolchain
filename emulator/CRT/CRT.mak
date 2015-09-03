# Microsoft Developer Studio Generated NMAKE File, Based on CRT.dsp
!IF "$(CFG)" == ""
CFG=CRT - Win32 Debug
!MESSAGE No configuration specified. Defaulting to CRT - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "CRT - Win32 Release" && "$(CFG)" != "CRT - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CRT.mak" CFG="CRT - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CRT - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "CRT - Win32 Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "CRT - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\Release\ProgramCRT.exe"


CLEAN :
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\win32CRT.obj"
	-@erase "..\Release\ProgramCRT.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\CRT.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\CRT.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\ProgramCRT.pdb" /machine:I386 /out:"../Release/ProgramCRT.exe" 
LINK32_OBJS= \
	"$(INTDIR)\win32CRT.obj"

"..\Release\ProgramCRT.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "CRT - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\Execute\ProgramCRT.exe" "$(OUTDIR)\CRT.bsc"


CLEAN :
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\win32CRT.obj"
	-@erase "$(INTDIR)\win32CRT.sbr"
	-@erase "$(OUTDIR)\CRT.bsc"
	-@erase "$(OUTDIR)\ProgramCRT.pdb"
	-@erase "..\Execute\ProgramCRT.exe"
	-@erase "..\Execute\ProgramCRT.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\CRT.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\CRT.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\win32CRT.sbr"

"$(OUTDIR)\CRT.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\ProgramCRT.pdb" /debug /machine:I386 /out:"../Execute/ProgramCRT.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\win32CRT.obj"

"..\Execute\ProgramCRT.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("CRT.dep")
!INCLUDE "CRT.dep"
!ELSE 
!MESSAGE Warning: cannot find "CRT.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "CRT - Win32 Release" || "$(CFG)" == "CRT - Win32 Debug"
SOURCE=.\win32CRT.cpp

!IF  "$(CFG)" == "CRT - Win32 Release"


"$(INTDIR)\win32CRT.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "CRT - Win32 Debug"


"$(INTDIR)\win32CRT.obj"	"$(INTDIR)\win32CRT.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

