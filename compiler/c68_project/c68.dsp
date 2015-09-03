# Microsoft Developer Studio Project File - Name="c68" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=c68 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "c68.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "c68.mak" CFG="c68 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "c68 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "c68 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "c68 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "c68 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D __STDC__=1 /FR /YX /FD /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "c68 - Win32 Release"
# Name "c68 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\analyze.c
# End Source File
# Begin Source File

SOURCE=.\cglbdef.c
# End Source File
# Begin Source File

SOURCE=.\cmain.c
# End Source File
# Begin Source File

SOURCE=.\decl.c
# End Source File
# Begin Source File

SOURCE=.\expr.c
# End Source File
# Begin Source File

SOURCE=.\extern.c
# End Source File
# Begin Source File

SOURCE=.\flow68k.c
# End Source File
# Begin Source File

SOURCE=.\flowc30.c
# End Source File
# Begin Source File

SOURCE=.\gen386.c
# End Source File
# Begin Source File

SOURCE=.\gen68k.c
# End Source File
# Begin Source File

SOURCE=.\gen86.c
# End Source File
# Begin Source File

SOURCE=.\genarm.c
# End Source File
# Begin Source File

SOURCE=.\genc30.c
# End Source File
# Begin Source File

SOURCE=.\genffp.c
# End Source File
# Begin Source File

SOURCE=.\genicode.c
# End Source File
# Begin Source File

SOURCE=.\genieee.c
# End Source File
# Begin Source File

SOURCE=.\geninit.c
# End Source File
# Begin Source File

SOURCE=.\genppc.c
# End Source File
# Begin Source File

SOURCE=.\genstmt.c
# End Source File
# Begin Source File

SOURCE=.\genutil.c
# End Source File
# Begin Source File

SOURCE=.\getsym.c
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# Begin Source File

SOURCE=.\inline.c
# End Source File
# Begin Source File

SOURCE=.\intexpr.c
# End Source File
# Begin Source File

SOURCE=.\list.c
# End Source File
# Begin Source File

SOURCE=.\memmgt.c
# End Source File
# Begin Source File

SOURCE=.\msgout.c
# End Source File
# Begin Source File

SOURCE=.\optimize.c
# End Source File
# Begin Source File

SOURCE=.\out68k_a.c
# End Source File
# Begin Source File

SOURCE=.\out68k_c.c
# End Source File
# Begin Source File

SOURCE=.\out68k_g.c
# End Source File
# Begin Source File

SOURCE=.\out68k_q.c
# End Source File
# Begin Source File

SOURCE=.\outarm_a.c
# End Source File
# Begin Source File

SOURCE=.\outarm_o.c
# End Source File
# Begin Source File

SOURCE=.\outc30_r.c
# End Source File
# Begin Source File

SOURCE=.\outgen.c
# End Source File
# Begin Source File

SOURCE=.\outppc.c
# End Source File
# Begin Source File

SOURCE=.\outx86_a.c
# End Source File
# Begin Source File

SOURCE=.\outx86_b.c
# End Source File
# Begin Source File

SOURCE=.\outx86_g.c
# End Source File
# Begin Source File

SOURCE=.\outx86_n.c
# End Source File
# Begin Source File

SOURCE=.\outx86_s.c
# End Source File
# Begin Source File

SOURCE=.\peep68k.c
# End Source File
# Begin Source File

SOURCE=.\peeparm.c
# End Source File
# Begin Source File

SOURCE=.\peepc30.c
# End Source File
# Begin Source File

SOURCE=.\peepppc.c
# End Source File
# Begin Source File

SOURCE=.\peepx86.c
# End Source File
# Begin Source File

SOURCE=.\reg68k.c
# End Source File
# Begin Source File

SOURCE=.\regarm.c
# End Source File
# Begin Source File

SOURCE=.\regc30.c
# End Source File
# Begin Source File

SOURCE=.\regppc.c
# End Source File
# Begin Source File

SOURCE=.\regx86.c
# End Source File
# Begin Source File

SOURCE=.\stmt.c
# End Source File
# Begin Source File

SOURCE=.\symbol.c
# End Source File
# Begin Source File

SOURCE=.\system.c
# End Source File
# Begin Source File

SOURCE=.\types.c
# End Source File
# End Group
# Begin Group "Header files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cglbdec.h
# End Source File
# Begin Source File

SOURCE=.\chdr.h
# End Source File
# Begin Source File

SOURCE=.\check.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\expr.h
# End Source File
# Begin Source File

SOURCE=.\gen386.h
# End Source File
# Begin Source File

SOURCE=.\gen68k.h
# End Source File
# Begin Source File

SOURCE=.\gen86.h
# End Source File
# Begin Source File

SOURCE=.\genarm.h
# End Source File
# Begin Source File

SOURCE=.\genc30.h
# End Source File
# Begin Source File

SOURCE=.\genppc.h
# End Source File
# Begin Source File

SOURCE=.\genx86.h
# End Source File
# Begin Source File

SOURCE=.\message.h
# End Source File
# Begin Source File

SOURCE=.\outproto.h
# End Source File
# Begin Source File

SOURCE=.\proto.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# End Group
# Begin Group "Support Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Makefile
# End Source File
# End Group
# Begin Group "Lint Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\c68-Options.lnt"
# End Source File
# Begin Source File

SOURCE=.\c68.lnt
# End Source File
# End Group
# End Target
# End Project
