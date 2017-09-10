#include <windows.h>
#include <stdio.h>
#include <conio.h>

#include "win32CRT.h"
#include "CRTCom.h"
#include "../SimptrisBoard.h"

#define BOARD_TOP		2	// Position of top of board
#define BOARD_LEFT		5	// Position of left side of board

HANDLE hOutput;
HANDLE hOutputPipe;

bool simptris;
char buffer[CRT_OUT_BUFFER_SIZE];



const unsigned char initialScreen[] = {
//	214, 196, 196, 196, 196, 196, 196, 183,
//	186,  32,  32,  32,  32,  32,  32, 186,
//	186,  32,  32,  32,  32,  32,  32, 186,
//	199,  32,  32,  32,  32,  32,  32, 182,
	218, 196, 196, 196, 196, 196, 196, 191,
	179,  32,  32,  32,  32,  32,  32, 179,
	179,  32,  32,  32,  32,  32,  32, 179,
	210,  32,  32,  32,  32,  32,  32, 210,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	186,  32,  32,  32,  32,  32,  32, 186,
	200, 205, 205, 205, 205, 205, 205, 188,
};



void ClearWindow(void)
{
	CONSOLE_SCREEN_BUFFER_INFO screenInfo;
	COORD coord;
	unsigned long numCharsWritten;

	GetConsoleScreenBufferInfo(hOutput, &screenInfo);

	coord.X = coord.Y = 0;
	FillConsoleOutputCharacter(hOutput, ' ',  screenInfo.dwSize.X * screenInfo.dwSize.Y, coord, &numCharsWritten);

	SetConsoleCursorPosition(hOutput, coord);
}


/*
int MessageReady(void)
{
	unsigned long numBytesRead;
	unsigned long numBytesAvailable;
	unsigned long numBytesLeft;
	int error;
	BOOL status;
	Message message;

	// Peek at message
	status = PeekNamedPipe(
		hOutputPipe,				// handle to pipe to copy from
		&message,					// pointer to data buffer
		sizeof(message),			// size, in bytes, of data buffer
		&numBytesRead,				// pointer to number of bytes read
		&numBytesAvailable,			// pointer to total number of bytes available
		&numBytesLeft				// pointer to unread bytes in this message
	);

	if(status && numBytesAvailable >= (unsigned)message.length + sizeof(message)) return MESSAGE_READY;
	else if(!status) {
		error = GetLastError();
		if(error == ERROR_BROKEN_PIPE || error == ERROR_INVALID_HANDLE) return BAD_PIPE;
	}

	return NO_MESSAGE;
}*/


void PrintScore(unsigned score)
{
	char string[12];
	int length;
	COORD pos;
	unsigned long numBytesWritten;

	sprintf(string, "%u", score);
	for(length = 0; string[length] != '\0'; length++);
	pos.X = BOARD_LEFT + (BOARD_WIDTH+BOARD_BORDER);
	pos.Y = BOARD_TOP + (BOARD_HEIGHT+2*BOARD_BORDER);
	WriteConsoleOutputCharacter(hOutput, string, length, pos, &numBytesWritten);
}


void PrintBoard(char *data)
{
	COORD pos;
	unsigned long numBytesWritten;
	int i;

	pos.X = BOARD_LEFT + BOARD_BORDER;
	pos.Y = BOARD_TOP + BOARD_HEIGHT;

	for(i = 0; i < BOARD_HEIGHT; i++) {
		WriteConsoleOutputCharacter(hOutput, (const char *)&data[(BOARD_WIDTH)*i], BOARD_WIDTH, pos, &numBytesWritten);
		pos.Y--;
	}
}


void DrawBoardFrame(void)
{
	COORD pos;
	int i;
	unsigned long numBytesWritten;

	// Display board //
	pos.X = BOARD_LEFT;
	pos.Y = BOARD_TOP;
	for(i = 0; i < BOARD_HEIGHT+2*BOARD_BORDER; i++) {
		WriteConsoleOutputCharacter(hOutput, (const char *)&initialScreen[(BOARD_WIDTH+2)*i], BOARD_WIDTH+2, pos, &numBytesWritten);
		pos.Y++;
	}
	WriteConsoleOutputCharacter(hOutput, "Lines:", 7, pos, &numBytesWritten);

	PrintScore(0);
}


bool InitSimptris(void)
{
	COORD screenSize;
	SMALL_RECT consoleWindowDim;
	int status;

	// Resize window //

	consoleWindowDim.Bottom = BOARD_TOP + BOARD_HEIGHT + 2*BOARD_BORDER + BOARD_TOP;// (BOARD_HEIGHT-1+2) + 6;
	consoleWindowDim.Top = 0;
	consoleWindowDim.Left = 0;
	consoleWindowDim.Right = BOARD_LEFT + BOARD_WIDTH + 2*BOARD_BORDER + BOARD_LEFT - 1;// (BOARD_WIDTH-1+2)  + 10;
	status = SetConsoleWindowInfo(hOutput, TRUE, &consoleWindowDim);
	if(!status) return false;

	// Set buffers size to minimum //
	screenSize.Y = BOARD_TOP + BOARD_HEIGHT + 2*BOARD_BORDER + BOARD_TOP + 1;	//(BOARD_HEIGHT+2)+6; 23
	screenSize.X = BOARD_LEFT + BOARD_WIDTH + 2*BOARD_BORDER + BOARD_LEFT;		//(BOARD_WIDTH+2)+10;
	status = SetConsoleScreenBufferSize(hOutput, screenSize);
	if(!status) return false;

	DrawBoardFrame();	

	simptris = true;

	return true;
}


void ProcessIncoming(void)
{
	Message message;
	unsigned long int numBytesRead;
	int i;
	
	while(true) {
		// Get message header (If bad pipe, exit)
		if(!ReadFile(hOutputPipe, &message, sizeof(message), &numBytesRead, NULL)) return;
		
		// Get message body
		if(message.length > 0) {
			if(message.length >= CRT_OUT_BUFFER_SIZE) message.length = CRT_OUT_BUFFER_SIZE - 1;
			ReadFile(hOutputPipe, buffer, message.length, &numBytesRead, NULL);
		}

		switch(message.action) {
		case CLEAR_SCREEN:
			ClearWindow();
			if(simptris) DrawBoardFrame();
			break;
		case PRINT_STRING:
			i = 0;
			while(i < message.length) {
				putchar(buffer[i]);
				i++;
			}
			break;
		case PRINT_CHAR:
			putchar(*buffer);
			break;
		case INIT_BOARD:
			if(!InitSimptris()) {
				printf("Could not set up simptris window.\nPress any key to terminate...");
				getch();
				return;
			}
			break;
		case PRINT_BOARD:
			PrintBoard(buffer);
			break;
		case PRINT_SCORE:
			PrintScore(*((int *)buffer));
			break;
		}
	}
}


bool InitWindow(void)
{
	//	AllocConsole();  Not needed since process created with CREATE_NEW_CONSOLE attribute
	CONSOLE_CURSOR_INFO cursorInfo;
	SMALL_RECT consoleWindowDim;
	COORD screenSize;

	hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	if(hOutput == INVALID_HANDLE_VALUE) return false;

	// Set console size
	consoleWindowDim.Bottom = PROGRAM_CONSOLE_HEIGHT-1;
	consoleWindowDim.Top = 0;
	consoleWindowDim.Left = 0;
	consoleWindowDim.Right = PROGRAM_CONSOLE_WIDTH-1;
	SetConsoleWindowInfo(hOutput, TRUE, &consoleWindowDim);

	// Set buffer size //
	screenSize.X = PROGRAM_CONSOLE_WIDTH;
	screenSize.Y = 500;
	int status = SetConsoleScreenBufferSize(hOutput, screenSize);


	// Hide cursor
	cursorInfo.bVisible = false;
	cursorInfo.dwSize = 10;
	SetConsoleCursorInfo(hOutput, &cursorInfo);

	return true;
}


int main(int argc, char *argv[])
{
//	InitWindow();
//	InitSimptris();
//	getch();
//	return 0;

	if(argc != 2) {
		printf("BAD ARGUMENT: Pipe name required as parameter.\n");
		return 1;
	}

	simptris = false;
	if(!InitWindow()) {
		printf("ERROR: Could not get both input and output handles.\n");
		return 1;
	}

	printf("Waiting for pipe \"%s\"...\n", argv[1]);

	// Wait for server to make output pipe //
	WaitNamedPipe(argv[1], NMPWAIT_WAIT_FOREVER);

	// Connect to output pipe
	hOutputPipe = CreateFile(
		argv[1],						// name of the pipe (or file)
		GENERIC_READ,					// read/write access (must match the pipe)
		0,								// usually 0 (no share) for pipes
		NULL,							// access privileges
		OPEN_EXISTING,					// must be OPEN_EXISTING for pipes
		0,								// write-through and overlapping modes
		NULL							// ignored with OPEN_EXISTING
	);

	if(hOutputPipe == INVALID_HANDLE_VALUE) {
		printf("ERROR: Could not connect to pipe.\nPress any key...");
		getch();
		return 2;
	}

	ClearWindow();

	ProcessIncoming();

	return 0;
}
