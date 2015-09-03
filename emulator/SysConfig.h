//
// SysConfig.h: Target system configuration file
//
// This file tells the compiler what platform the simulator will run
// on. It automatically detects win32 and sets up defenitions to 
// compile for that platform. Otherwise, it's configured for HPUX.
//
//////////////////////////////////////////////////////////////////////

#ifndef SYSCONFIG_H
#define SYSCONFIG_H

/********************************************************************
The following detects the win32 compiler platform (if present) and 
sets defines appropriately.

If not using win32:

- For HPUX (HPPA processors) define the HPUX_CONSOLE defenition.
- If compiling under x86 Linux, define LINUX_X86_CONSOLE.

The win32 defines have been commented out.
********************************************************************/

//#ifdef _WIN32
//#define WIN32_CONSOLE

//#else

//#define	HPUX_CONSOLE
//#undef	LINUX_X86_CONSOLE

//#endif

#define    LINUX_X86_CONSOLE
#undef     _WIN32
#undef     HPUX_CONSOLE

/*******************************************************************/



/////////////////////////////////////////////////////////////////////
// The following are settings required for compilation //////////////
/////////////////////////////////////////////////////////////////////


// Win32 defines ////////////////////////////////////////////////////

#ifdef WIN32_CONSOLE

#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif

#endif // WIN32_CONSOLE


// Linux defines ////////////////////////////////////////////////////

#ifdef LINUX_X86_CONSOLE

#define TERMIOS_CONSOLE

#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif

#endif // LINUX_X86_CONSOLE


// HPUX defines /////////////////////////////////////////////////////

#ifdef HPUX_CONSOLE

#define TERMIOS_CONSOLE

#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__
#endif

#endif // HPUX_CONSOLE


#endif //SYSCONFIG_H

