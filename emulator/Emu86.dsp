# Microsoft Developer Studio Project File - Name="Emu86" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Emu86 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Emu86.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Emu86.mak" CFG="Emu86 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Emu86 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Emu86 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Emu86 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"ExecRelease/Emu86.exe"

!ELSEIF  "$(CFG)" == "Emu86 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"ExecDebug/Emu86.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Emu86 - Win32 Release"
# Name "Emu86 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Bios.cpp
# End Source File
# Begin Source File

SOURCE=.\CmdHistory.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\decode.cpp
# End Source File
# Begin Source File

SOURCE=.\fpu.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\NasmDisassemble\nasm_disasm.cpp
# End Source File
# Begin Source File

SOURCE=.\ops.cpp
# End Source File
# Begin Source File

SOURCE=.\ops2.cpp
# End Source File
# Begin Source File

SOURCE=.\PIC.cpp
# End Source File
# Begin Source File

SOURCE=.\prim_ops.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgramWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\SimControl.cpp
# End Source File
# Begin Source File

SOURCE=.\Simptris.cpp
# End Source File
# Begin Source File

SOURCE=.\SimWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\SymbolTable.cpp
# End Source File
# Begin Source File

SOURCE=.\sys.cpp
# End Source File
# Begin Source File

SOURCE=.\Utils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Bios.h
# End Source File
# Begin Source File

SOURCE=.\CmdHistory.h
# End Source File
# Begin Source File

SOURCE=.\CRT\CRTCom.h
# End Source File
# Begin Source File

SOURCE=.\x86emu\debug.h
# End Source File
# Begin Source File

SOURCE=.\x86emu\decode.h
# End Source File
# Begin Source File

SOURCE=.\EXEDecode.h
# End Source File
# Begin Source File

SOURCE=.\x86emu\fpu.h
# End Source File
# Begin Source File

SOURCE=.\x86emu\fpu_regs.h
# End Source File
# Begin Source File

SOURCE=.\Help.h
# End Source File
# Begin Source File

SOURCE=.\x86emu\ops.h
# End Source File
# Begin Source File

SOURCE=.\PIC.h
# End Source File
# Begin Source File

SOURCE=.\x86emu\prim_ops.h
# End Source File
# Begin Source File

SOURCE=.\ProgramWindow.h
# End Source File
# Begin Source File

SOURCE=.\x86emu\regs.h
# End Source File
# Begin Source File

SOURCE=.\SimControl.h
# End Source File
# Begin Source File

SOURCE=.\Simptris.h
# End Source File
# Begin Source File

SOURCE=.\SimptrisBoard.h
# End Source File
# Begin Source File

SOURCE=.\SimWindow.h
# End Source File
# Begin Source File

SOURCE=.\SymbolTable.h
# End Source File
# Begin Source File

SOURCE=.\SysConfig.h
# End Source File
# Begin Source File

SOURCE=.\x86emu\types.h
# End Source File
# Begin Source File

SOURCE=.\Utils.h
# End Source File
# Begin Source File

SOURCE=.\CRT\win32CRT.h
# End Source File
# Begin Source File

SOURCE=.\x86emu.h
# End Source File
# Begin Source File

SOURCE=.\x86emu\x86emui.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
