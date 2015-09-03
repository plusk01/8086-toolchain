//
// SimWindow.cpp: implementation of the CSimWindow class.
//
// This class handles I/O to the simulator window (as apposed to the
// program output window). It was created to make I/O independent
// of the operating system. The class could be modified to allow
// for a GUI interface to the simulator.
//
// Note that there is a UNIX/Linux and a windows version of the code.
//
//////////////////////////////////////////////////////////////////////

#include "SimWindow.h"

#include "x86emu/x86emui.h"
#include "Utils.h"
#include <stdarg.h>



//////////////////////////////////////////////////////////////////////
// UNIX/Linux Console Version: ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#ifdef TERMIOS_CONSOLE

#include "Conio.h"


CSimWindow::CSimWindow()
{
}

CSimWindow::~CSimWindow()
{
	closeConio();
}

// Initializes terminal
bool CSimWindow::InitWindow(void)
{
	initConio();

	return true;
}


// For printing normal strings. It supports \n, \r, \t, and \b correctly.
void CSimWindow::PrintText(const char *string)
{
	printf(string);
}


// Like printf. Prints simulator output messages.
// WARNING: The formated message must never exceed MAX_MESSAGE_LENGTH
int CSimWindow::PrintMessage(const char *format, ...)
{
	va_list argptr;
	int cnt;
	
	va_start(argptr, format);
	cnt = vprintf(format, argptr);
	va_end(argptr);

	return(cnt);
}


// For printing characters (7-bit ASCII). This function prints only valid characters, otherwise prints a period.
void CSimWindow::DumpChar(char c)
{
	if(c >= 32) putchar(c);
	else putchar('.');
}


int CSimWindow::GetHistory(int dir, char *dest, int clearChars)
{
	const char *string;
	int i;

	// Clear text at command line
	for(; clearChars > 0; clearChars--) printf("\b \b");

	// Get next or previos command from command history
	if(dir == GET_PREVIOUS) string = history.GetPrevious();
	else string = history.GetNext();
	
	// Copy command from history
	strcpy(dest, string);
	i = strlen(string);
	printf(string);
	
	return i;
}


// Get command string from user of length no more than maxBytes (including NULL).
// Return number of characters in string (excluding NULL);
// Also includes history functionality using up and down arrow keys (using CmdHistory class).
int CSimWindow::GetCommandString(char *dest, int maxBytes)
{
	int i = 0;
	int c;
	
	while(true) {
		c = getchar();

		// Handle special characters
		if(c == 27) {
			if(extendedKeyPress() && getchar() == 91) {
				c = getchar();
				if(c == 65) {	// Up arrow
					// Get previous entry from history
					i = GetHistory(GET_PREVIOUS, dest, i);
				}
				else if(c == 66) {	// Down arrow
					// Get more recent entry from history
					i = GetHistory(GET_NEXT, dest, i);
				}
			}
			else {		// Escape key
				// Clear current command
				history.Reset();
				for(; i > 0; i--) printf("\b \b");
				continue;
			}
			while(extendedKeyPress()) getchar();	// Clear any buffered characters
		}
		else if(c == 14) {	// Ctrl+n
			// Get more recent entry from history
			i = GetHistory(GET_NEXT, dest, i);
		}
		else if(c == 16) {	// Ctrl+p
			// Get previous entry from history
			i = GetHistory(GET_PREVIOUS, dest, i);
		}
		else if(c == '\n') {
			putchar('\n');
			break;
		}
		else if(c == '\b' || c == 127) {	// Check for backspace or delete
			if(i > 0) {
				// Erase character
				printf("\b \b");
				i--;
			}
		}
		else if((char)c < 32) continue;	// Ignore any other unusual characters

		// Normal case:
		else if(i < maxBytes-1) {
			putchar(c);
			dest[i] = (char)c;
			i++;
		}
	}
	dest[i] = '\0';	// Null terminate string
	
	// Add to history (if not empty)
	if(dest[0] != '\0') history.Add(dest);

	return i;
}


#endif // #ifdef TERMIOS_CONSOLE


//////////////////////////////////////////////////////////////////////
// End of UNIX/Linux Console Version /////////////////////////////////
//////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////
// Windows Console Version: //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#ifdef WIN32_CONSOLE

#include <conio.h>


CSimWindow::CSimWindow()
{
	hSimOutput = INVALID_HANDLE_VALUE;
}

CSimWindow::~CSimWindow()
{
	SMALL_RECT consoleWindowDim;
	COORD coord;
	int numCells;
	DWORD temp;

	if(hSimOutput != INVALID_HANDLE_VALUE && hSimInput != INVALID_HANDLE_VALUE) {
		// Set console size to 0
		consoleWindowDim.Bottom = 0;
		consoleWindowDim.Top = 0;
		consoleWindowDim.Left = 0;
		consoleWindowDim.Right = 0;
		SetConsoleWindowInfo(hSimOutput, TRUE, &consoleWindowDim);

		// Reset console attributes
		SetConsoleScreenBufferSize(hSimOutput, oldBufferSize);
		SetConsoleWindowInfo(hSimOutput, FALSE, &oldWindowSize);
		SetConsoleTextAttribute(hSimOutput, oldAttributes);

		// Clear console
		coord.X = 0;
		coord.Y = 0;
		numCells = (oldWindowSize.Right - oldWindowSize.Left + 1) * (oldWindowSize.Bottom - oldWindowSize.Top + 1);
		FillConsoleOutputAttribute(hSimOutput, oldAttributes, numCells, coord, &temp);
		FillConsoleOutputCharacter(hSimOutput, ' ', numCells, coord, &temp);

		// Set cursor position to (0,0)
		SetConsoleCursorPosition(hSimOutput, coord);
	}
}

// Initializes windows console.
bool CSimWindow::InitWindow()
{
	// Set up console window
	WORD wAttributes;
	COORD pos;
	SMALL_RECT consoleWindowDim;
	CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
	unsigned long int temp;

	// Get console handles
	hSimOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	hSimInput = GetStdHandle(STD_INPUT_HANDLE);

	if(hSimOutput == INVALID_HANDLE_VALUE  || hSimInput == INVALID_HANDLE_VALUE) {
		printf("ERROR: Couldn't get console I/O handles.\n");
		return false;
	}

	// Save current console info
	GetConsoleScreenBufferInfo(hSimOutput, &bufferInfo);
	oldBufferSize = bufferInfo.dwSize;
	oldWindowSize = bufferInfo.srWindow;
	oldAttributes = bufferInfo.wAttributes;

	// Set console title
	SetConsoleTitle("Emu86 Simulator");

	// Set console to small size for buffer change:
	consoleWindowDim.Bottom = 0;
	consoleWindowDim.Top = 0;
	consoleWindowDim.Left = 0;
	consoleWindowDim.Right = 0;
	SetConsoleWindowInfo(hSimOutput, TRUE, &consoleWindowDim);

	// Set buffer size
	pos.X = SIM_CONSOLE_BUFFER_WIDTH;
	pos.Y = SIM_CONSOLE_BUFFER_HEIGHT;
	SetConsoleScreenBufferSize(hSimOutput, pos);

	// Set console size
	consoleWindowDim.Bottom = SIM_CONSOLE_HEIGHT - 1;
	consoleWindowDim.Top = 0;
	consoleWindowDim.Left = 0;
	consoleWindowDim.Right = SIM_CONSOLE_WIDTH - 1;
	SetConsoleWindowInfo(hSimOutput, TRUE, &consoleWindowDim);


	// Clear console, set to black, white text
	pos.X = 0;
	pos.Y = 0;
	wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	FillConsoleOutputAttribute(hSimOutput, wAttributes, SIM_CONSOLE_BUFFER_WIDTH * SIM_CONSOLE_BUFFER_HEIGHT, pos, &temp);
	FillConsoleOutputCharacter(hSimOutput, ' ', SIM_CONSOLE_BUFFER_WIDTH * SIM_CONSOLE_BUFFER_HEIGHT, pos, &temp);

	// Move curor to top left
	SetConsoleCursorPosition(hSimOutput, pos);

	return true;
}

// For printing normal strings. It supports \n, \r, \t, and \b correctly.
void CSimWindow::PrintText(const char *string)
{
	WriteConsole(hSimOutput, string, strlen(string), NULL, NULL);
}

// Like printf. Prints simulator output messages.
int CSimWindow::PrintMessage(const char *format, ...)
{
	va_list argptr;
	int cnt;
	
	va_start(argptr, format);
	cnt = vprintf(format, argptr);
	va_end(argptr);
	
	return(cnt);
}


int CSimWindow::GetHistory(int dir, char *dest, int clearChars)
{
	const char *string;
	int i;

	// Clear text at command line
	for(; clearChars > 0; clearChars--) printf("\b \b");

	// Get next or previos command from command history
	if(dir == GET_PREVIOUS) string = history.GetPrevious();
	else string = history.GetNext();
	
	// Copy command from history
	strcpy(dest, string);
	i = strlen(string);
	printf(string);
	
	return i;
}


// Get command string from user of length no more than maxBytes (including NULL).
// Return number of characters in string (excluding NULL);
// Also includes history functionality using up and down arrow keys (using CmdHistory).
int CSimWindow::GetCommandString(char *dest, int maxBytes)
{
	int i = 0;
	int c;
	
	while(true) {
		c = getch();

		// Handle special characters

		if(c == 224) {
			c = getch();
			if(c == 72) {	// Up arrow
				// Get previous entry from history
				i = GetHistory(GET_PREVIOUS, dest, i);
			}
			else if(c == 80) {	// Down arrow
				// Get more recent entry from history
				i = GetHistory(GET_NEXT, dest, i);
			}
			else while(kbhit()) getch();
			
			continue;
		}
		else if(c == 27) {	// ESC character
			// Clear current command
			history.Reset();
			for(; i > 0; i--) printf("\b \b");
			continue;
		}
		else if(c == 14) {	// Ctrl+n
			// Get more recent entry from history
			i = GetHistory(GET_NEXT, dest, i);
		}
		else if(c == 16) {	// Ctrl+p
			// Get previous entry from history
			i = GetHistory(GET_PREVIOUS, dest, i);
		}
		else if(c == '\r') { //(c == '\n') {
			putchar('\n');
			break;
		}
		else if(c == '\b') {	// Check for backspace
			if(i > 0) {
				// Erase character
				printf("\b \b");
				i--;
			}
		}
		else if(c < 32 || c > 128) continue;	// Ignore any other unusual characters

		// Normal case:
		else if(i < maxBytes-1) {
			putchar(c);
			dest[i] = (char)c;
			i++;
		}
	}
	dest[i] = '\0';	// Null terminate string

	// Add to history (if not empty)
	if(dest[0] != '\0') history.Add(dest);

	return i;
}

// For printing characters (extended ASCII). This function prints only valid characters, otherwise prints a period.
void CSimWindow::DumpChar(char c)
{
	if((unsigned char)c >= 32) putch(c);
	else putch('.');
}


#endif // #ifdef WIN32_CONSOLE //////////////////////////////////////


