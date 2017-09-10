// ProgramWindow.h: interface for the CProgramWindow class.
//
// See ProgramWindow.cpp for a description.
//
/////////////////////////////////////////////////////////////////////

#ifndef PROGRAMWINDOW_H
#define PROGRAMWINDOW_H

#include "SysConfig.h"
#include "CRT/CRTCom.h"

// Character Constants //
#define KEY_NONE	0
#define KEY_RESET	18		// Ctrl+R
#define KEY_TICK	20		// Ctrl+T
#define CTRL_B		2		// Ctrl+B
#define ESC_KEY		27

// InitProgramWindow return values

#define PIPE_CREATION_FAILURE		1	// Couldn't create communication pipe
#define PROCESS_CREATION_FAILURE	2	// Couldn't create the process
#define WINDOW_SETUP_FAILURE		3	// Could set up window properties
#define CRT_OK						4	// Pipe and process created successfully



// UNIX/Linux Console Version /////////////////////////////////////////////

#ifdef TERMIOS_CONSOLE

#include "CRT/UNIXCRT.h"

class CProgramWindow  
{
private:
	bool initDone;
	bool initSimpDone;
	int hOutputPipe;
	int hSimpPipe;
	char *outputPipeName;
	char *simpPipeName;
	char *CRTExecName;
	char outBuffer[CRT_OUT_BUFFER_SIZE];

public:
	// NOTE: This interface shouldn't change. Same for both UNIX and win32
	bool KeyPress(void);
	char GetProgramInput(void);
	int InitProgramWindow(char *execPath);
	void SendMessage(Message *message, char *buffer);
	void ClearWindow(void);
	void PrintChar(char c);
	void SendString(char *string, int length);
	int PrintText(const char *format, ...);
	// Simptris Specific:
	int InitSimptris(void);
	void SendBoard(char *string);
	void SendScore(int number);
	void KillSimptris(void);
	void ClearSimpWindow(void);
	//
	CProgramWindow();
	virtual ~CProgramWindow();
};

#endif	// #ifdef TERMIOS_CONSOLE //////////////////////////////////////

/////////////////////////////////////////////////////////////////////


// Win32 Console Version ////////////////////////////////////////////

#ifdef WIN32_CONSOLE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "CRT/win32CRT.h"

class CProgramWindow  
{
private:
	int CreateProcessWindow(char *executable, char *arguments, char *windowTitle, LPPROCESS_INFORMATION process);
	bool MessageReady(void);

	PROCESS_INFORMATION outputProcess;
	PROCESS_INFORMATION simpProcess;
	HANDLE hOutputPipe;
	HANDLE hSimpPipe;
	char *CRTExecName;
	bool initDone;
	bool initSimpDone;

	char outBuffer[CRT_OUT_BUFFER_SIZE];


public:
	// NOTE: This interface shouldn't change. Same for both UNIX and win32
	BOOL KeyPress(void);
	char GetProgramInput(void);
	int InitProgramWindow(char *execPath);
	void SendMessage(Message *message, char *buffer);
	void ClearWindow(void);
	void PrintChar(char c);
	void SendString(char *string, int length);
	int PrintText(const char *format, ...);
	// Simptris Specific:
	int InitSimptris(void);
	void SendBoard(char *string);
	void SendScore(int number);
	void KillSimptris(void);
	void ClearSimpWindow(void);
	//
	CProgramWindow();
	virtual ~CProgramWindow();
};

#endif // #ifdef WIN32_CONSOLE //////////////////////////////////////

#endif


