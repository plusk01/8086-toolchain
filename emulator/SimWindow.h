//
// SimWindow.h: interface for the CSimWindow class.
//
// See SimWindow.cpp for a description of this class.
//
//////////////////////////////////////////////////////////////////////

#ifndef SIMWINDOW_H
#define SIMWINDOW_H

#include "SysConfig.h"
#include "CmdHistory.h"

#define GET_PREVIOUS	0
#define GET_NEXT		1

#ifdef WIN32_CONSOLE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define SIM_CONSOLE_WIDTH	80
#define SIM_CONSOLE_HEIGHT	25
#define SIM_CONSOLE_BUFFER_WIDTH 80
#define SIM_CONSOLE_BUFFER_HEIGHT 1000

class CSimWindow  
{
private:
#ifdef WIN32_CONSOLE
	HANDLE hSimOutput;
	HANDLE hSimInput;

	COORD oldBufferSize;
	SMALL_RECT oldWindowSize;
	WORD oldAttributes;

#endif
	CCmdHistory history;

public:
	CSimWindow();
	virtual ~CSimWindow();
	bool InitWindow(void);
	void PrintText(const char *string);
	int PrintMessage(const char *format, ...);
	void DumpChar(char c);
	int GetHistory(int dir, char *dest, int clearChars);
	int GetCommandString(char *dest, int maxBytes);
};

#endif

