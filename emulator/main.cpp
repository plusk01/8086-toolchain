//
// main.cpp: Starting point for Emu86 simulator.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdarg.h>

#include <stdlib.h>

#include "SimControl.h"

#include "x86emu/x86emui.h"

// Simulator class
CSimControl SimControl;

// The following is a peice of error output code for the SciTech simulator (it was copied from validate.c)
// The code has been modified so that it is only called when the SciTech code is in debug mode. - WSF
void printk(const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    vfprintf(stdout, fmt, argptr);
    fflush(stdout);
    va_end(argptr);
}


#ifdef TERMIOS_CONSOLE
void closeConio(void);
#endif


int main(int argc, char *argv[])
{	
	bool status;

	// Intialize simulator
	status = SimControl.InitSim(argv[0]);
	if(!status) return 1;

	// Begin main execution loop
	SimControl.ExecLoop();

#ifdef TERMIOS_CONSOLE
	/*
		The following is a bit of a hack. This call is in the destructor for SimControl, but for some reason, isn't
		getting called on the UNIX version. This is a quick fix. Neglecting this call causes keyboard echo to be
		turned off when using bash after exiting.
	*/
	closeConio();
#endif

	return 0;
}

