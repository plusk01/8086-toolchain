//
// ProgramWindow.cpp: implementation of the CProgramWindow class.
//
// This class handles the creation and initialization of the program
// output window. It also handles text I/O for this window.
//
// Note that there is a curses and a windows version of the code.
//
//////////////////////////////////////////////////////////////////////

#include "ProgramWindow.h"
#include "SimptrisBoard.h"


//////////////////////////////////////////////////////////////////////
// UNIX/Linux Terminal Version ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#ifdef TERMIOS_CONSOLE

#include <stdio.h>
#include <stdarg.h>

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>

#include "Conio.h"



CProgramWindow::CProgramWindow()
{
	initDone = false;
	initSimpDone = false;
	outputPipeName = NULL;
	simpPipeName = NULL;
}

CProgramWindow::~CProgramWindow()
{
	pid_t pid;
	Message message;

	// Do some clean up //

	if(initDone) {
		message.action = EXIT;
		message.length = 0;
		write(hOutputPipe, &message, sizeof(message));
		close(hOutputPipe);

		pid = fork();
		if(pid == 0) execlp("rm", "rm", outputPipeName, NULL);
	}

	if(initSimpDone) KillSimptris();

	if(outputPipeName) free(outputPipeName);// delete outputPipeName;
	if(simpPipeName) free(simpPipeName);	// delete simpPipeName;
	if(CRTExecName) free(CRTExecName);		// delete CRTExecName;
}


bool CProgramWindow::KeyPress(void)
{
	return keyPress();
}


char CProgramWindow::GetProgramInput(void)
{
	return getchar();
}

// Creates the pipe for communication with the output window.
// This function was part of the original EE425 SIMP simulator.
int CreatePipe(char *buf, int flag)
{
    /* Creates a pipe (fifo) named (*buf) using the mkfifo command.
       If flag is set and file buf exists, it will remove file buf and
       recreate it.  If flag is not set and file buf exists, it will
       return -1.  Returns 0 if successful, -1 otherwise */

    int error;
    error=mkfifo(buf, S_IRUSR | S_IWUSR);
    if(error && flag)
    {
		if (errno==EEXIST)
		{
			switch(fork())
			{
			case -1:
				{
					/*perror("fifo_comm:");
					fprintf(stderr, "fifo_comm: Unable to fork. Exiting.\n" );
					exit (1);
					break;
					*/
					return -1;
				}
			case 0:
				{
					execlp("rm","rm",buf,NULL);
					break;
				}
			default:
				break;
			}
		}
		else
			perror("fifo_comm:");
    }
    while(error && flag && (errno == EEXIST))
		error=mkfifo(buf,S_IRUSR | S_IWUSR);
      
    return error;
}


// This function creates the output window communications pipe and process.
int CProgramWindow::InitProgramWindow(char *path)
{
	pid_t processID;

	// Generate command line for CRT process (assume it's in same directory as current process)
	// CRTExecName = new char[strlen(path) + strlen(CRT_MODULE_NAME)];
	CRTExecName = (char *)malloc(strlen(path) + strlen(CRT_MODULE_NAME) + 2);
	if(CRTExecName == NULL) return PROCESS_CREATION_FAILURE;
	strcpy(CRTExecName, path);
	strcat(CRTExecName, CRT_MODULE_NAME);
	
	// CREATE PIPE //

	// outputPipeName = new char[strlen(EMU86_OUT_PIPE_NAME) + strlen(getenv("USER")) + 2];
	outputPipeName = (char *)malloc(strlen(EMU86_OUT_PIPE_NAME) + strlen(getenv("USER")) + 2);
    if(outputPipeName == NULL) return PIPE_CREATION_FAILURE;
	strcpy(outputPipeName, EMU86_OUT_PIPE_NAME);
	strcat(outputPipeName, "_");
	strcat(outputPipeName, getenv("USER"));
	if(CreatePipe(outputPipeName, 1) != 0) return PIPE_CREATION_FAILURE;


	// CREATE PROCESS //

	processID = fork();
	if(processID == -1) return PROCESS_CREATION_FAILURE;
	
	// Check if this is now child, if so replace this prcess with CRT
	if(processID == 0) {
		if (execlp("xterm", "xterm", "-bg", "black",
			"-fg", "green" ,"-T", "Program Output", "-j",
			"-sb", "-sl", "500", "-cr", "black",
			"-bd", "blue", "-s", "-geometry", "80x24", "-e",
			CRTExecName, outputPipeName, (void *)NULL) == -1
		) return PROCESS_CREATION_FAILURE;
	}


	// OPEN PIPE //

	hOutputPipe = open(outputPipeName, O_WRONLY);
	if(hOutputPipe < 0) return PIPE_CREATION_FAILURE;


	// Process and pipe successfully created
	initDone = true;
	return CRT_OK;
}

// Sends a message to the program window process.
void CProgramWindow::SendMessage(Message *message, char *buffer)
{
	write(hOutputPipe, &message, sizeof(message));
	if(message->length > 0) write(hOutputPipe, buffer, message->length);
}

// Sends message to clear the program window.
void CProgramWindow::ClearWindow(void)
{
	Message message;

	message.action = CLEAR_SCREEN;
	message.length = 0;
	write(hOutputPipe, &message, sizeof(message));
}

// Sends a message to print character.
void CProgramWindow::PrintChar(char c)
{
	Message message;

	message.action = PRINT_CHAR;
	message.length = 1;

	write(hOutputPipe, &message, sizeof(message));
	write(hOutputPipe, &c, 1);
}

// Sends string of size length to program window via message (string may or may not be null terminated)
void CProgramWindow::SendString(char *string, int length)
{
	Message message;

	// See if string + null will fit
	if(length + sizeof(message) > CRT_OUT_BUFFER_SIZE) {
		length = CRT_OUT_BUFFER_SIZE - sizeof(message);
	}

	message.action = PRINT_STRING;
	message.length = length;

	write(hOutputPipe, &message, sizeof(message));
	write(hOutputPipe, string, length);
}

// Like printf. Prints to the program output window.
int CProgramWindow::PrintText(const char *format, ...)
{
	Message message;
	va_list argptr;
	
	va_start(argptr, format);
	message.length = vsprintf(outBuffer, format, argptr);
	va_end(argptr);

	// See if string + null will fit
	if(message.length + sizeof(message) > CRT_OUT_BUFFER_SIZE) {
		message.length = CRT_OUT_BUFFER_SIZE - sizeof(message);
	}

	message.action = PRINT_STRING;

	write(hOutputPipe, &message, sizeof(message));
	return write(hOutputPipe, outBuffer, message.length);
}

// Creates simptris window pipe and process.
// Return true if successful, otherwise false.
int CProgramWindow::InitSimptris(void)
{
	pid_t processID;

	if(initSimpDone) return CRT_OK;
	
	// CREATE SIMPTRIS PIPE //

	// simpPipeName = new char[strlen(EMU86_SIMP_PIPE_NAME) + strlen(getenv("USER")) + 2];
	simpPipeName = (char *)malloc(strlen(EMU86_SIMP_PIPE_NAME) + strlen(getenv("USER")) + 2);
    if(simpPipeName == NULL) return PIPE_CREATION_FAILURE;
	strcpy(simpPipeName, EMU86_SIMP_PIPE_NAME);
	strcat(simpPipeName, "_");
	strcat(simpPipeName, getenv("USER"));
	if(CreatePipe(simpPipeName, 1) != 0) return PIPE_CREATION_FAILURE;
	
	
	// CREATE SIMPTRIS CRT PROCESS //

	processID = fork();
	if(processID == -1) return PROCESS_CREATION_FAILURE;
	
	// Check if this is now child, if so replace this prcess with CRT
	if(processID == 0) {
		if (execlp("xterm", "xterm", "-bg", "black",
			"-fg", "green" ,"-T", "Simptris",
			"+sb", "-sl", "0", "-cr", "black",
			"-s", "-geometry", "18x23", "-e",
			CRTExecName, simpPipeName, (void *)NULL) == -1
		) return PROCESS_CREATION_FAILURE;
	}


	// OPEN SIMPTRIS PIPE //

	hSimpPipe = open(simpPipeName, O_WRONLY);
	if(hSimpPipe < 0) return PIPE_CREATION_FAILURE;


	// Process and pipe successfully created
	initSimpDone = true;

	Message message;

	message.action = INIT_BOARD;
	message.length = 0;

	write(hSimpPipe, &message, sizeof(message));

	return CRT_OK;
}

// Terminate the simptris pipe and process.
void CProgramWindow::KillSimptris(void)
{
	pid_t pid;
	Message message;

	if(!initSimpDone) return;
	
	message.action = EXIT;
	message.length = 0;
	write(hSimpPipe, &message, sizeof(message));
	close(hSimpPipe);
	
	pid = fork();
	if(pid == 0) execlp("rm", "rm", simpPipeName, NULL);
	free(simpPipeName); // delete simpPipeName;
	simpPipeName = 0;

	initSimpDone = false;
}

// Sends fixed length string representing board content
void CProgramWindow::SendBoard(char *string)
{
	Message message;

	message.action = PRINT_BOARD;
	message.length = BOARD_WIDTH * BOARD_HEIGHT;

	write(hSimpPipe, &message, sizeof(message));
	write(hSimpPipe, string, message.length);
}

// Sends the score through the fifo
void CProgramWindow::SendScore(int number)
{
	Message message;

	message.action = PRINT_SCORE;
	message.length = sizeof(number);


	write(hSimpPipe, &message, sizeof(message));
	write(hSimpPipe, &number, message.length);
}

// Clears the simptris window.
void CProgramWindow::ClearSimpWindow(void)
{
	Message message;

	message.action = CLEAR_SCREEN;
	message.length = 0;
	write(hSimpPipe, &message, sizeof(message));
}



#endif // #ifdef TERMIOS_CONSOLE ///////////////////////////////////////











//////////////////////////////////////////////////////////////////////
// Windows Console Version////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#ifdef WIN32_CONSOLE

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>


CProgramWindow::CProgramWindow()
{
	initDone = false;
	initSimpDone = false;
	CRTExecName = NULL;
}

CProgramWindow::~CProgramWindow()
{
	if(CRTExecName) free(CRTExecName);		// delete CRTExecName;

	// Kill CRT process
	if(initDone) {
		TerminateProcess(outputProcess.hProcess, 0);
		DisconnectNamedPipe(hOutputPipe);
		CloseHandle(hOutputPipe);
	}
	if(initSimpDone) KillSimptris();
}

int CProgramWindow::CreateProcessWindow(char *executable, char *arguments, char *windowTitle, LPPROCESS_INFORMATION process)
{
	int status;
	char commandLine[1024];

	// Combine executable with arguments
	strncpy(commandLine, executable, sizeof(commandLine) - 2);
	strncat(commandLine, " ", 1);
	strncat(commandLine, arguments, sizeof(commandLine) - 2 - strlen(commandLine));

	LPCTSTR lpApplicationName;				// pointer to name of executable module
	LPTSTR lpCommandLine;						// pointer to command line string
	LPSECURITY_ATTRIBUTES lpProcessAttributes;// process security attributes
	LPSECURITY_ATTRIBUTES lpThreadAttributes;	// thread security attributes
	BOOL bInheritHandles;						// handle inheritance flag
	DWORD dwCreationFlags;					// creation flags
	LPVOID lpEnvironment;						// pointer to new environment block
	LPCTSTR lpCurrentDirectory;				// pointer to current directory name
	STARTUPINFO startupInfo;				// pointer to STARTUPINFO
	//PROCESS_INFORMATION processInformation;	// pointer to PROCESS_INFORMATION

	lpApplicationName = NULL;
	lpCommandLine = commandLine;
	lpProcessAttributes = NULL;
	lpThreadAttributes = NULL;
	bInheritHandles = FALSE;
	//dwCreationFlags = DETACHED_PROCESS;
	dwCreationFlags = CREATE_NEW_CONSOLE;
	lpEnvironment = NULL;
	lpCurrentDirectory = NULL;

  	startupInfo.cb = sizeof(STARTUPINFO);
	startupInfo.lpReserved = NULL;
	startupInfo.lpDesktop = NULL;
	startupInfo.lpTitle = windowTitle;
	startupInfo.cbReserved2 = 0;
	startupInfo.lpReserved2 = 0;
	startupInfo.dwFlags = STARTF_USECOUNTCHARS | STARTF_USEFILLATTRIBUTE;
	startupInfo.dwXCountChars = 80;
	startupInfo.dwYCountChars = 1000;
	startupInfo.dwFillAttribute = FOREGROUND_GREEN;


	status = CreateProcess(
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		&startupInfo,
		process
	);

	if(!status) return 0;
	
	return 1;
}

/*bool CProgramWindow::MessageReady(void)
{
	unsigned long numBytesRead;
	unsigned long numBytesAvailable;
	unsigned long numBytesLeft;
	BOOL status;
	Message message;

	// Peek at message
	status = PeekNamedPipe(
		hInputPipe,					// handle to pipe to copy from
		&message,					// pointer to data buffer
		sizeof(message),			// size, in bytes, of data buffer
		&numBytesRead,				// pointer to number of bytes read
		&numBytesAvailable,			// pointer to total number of bytes available
		&numBytesLeft				// pointer to unread bytes in this message
	);

	if(status && numBytesAvailable >= (unsigned)message.length + sizeof(message)) return true;

	return false;
}*/


BOOL CProgramWindow::KeyPress(void)
{
	return kbhit();
}

char CProgramWindow::GetProgramInput(void)
{
	return (char)getch();
}

int CProgramWindow::InitProgramWindow(char *path)
{
	// Generate command line for CRT process (assume it's in same directory as current process)
	// CRTExecName = new char[strlen(path) + strlen(CRT_MODULE_NAME)];
	CRTExecName = (char *)malloc(strlen(path) + strlen(CRT_MODULE_NAME) + 2);
	if(CRTExecName == NULL) return PROCESS_CREATION_FAILURE;
	strcpy(CRTExecName, path);
	strcat(CRTExecName, CRT_MODULE_NAME);



	// Create the output pipe //
	hOutputPipe = CreateNamedPipe( 
		EMU86_OUT_PIPE_NAME,		// pipe name 
		PIPE_ACCESS_OUTBOUND,		// read/write access 
		PIPE_TYPE_BYTE |			// message type pipe 
		PIPE_READMODE_BYTE |		// message-read mode 
		PIPE_WAIT,					// blocking mode 
		1,							// max. instances  
		CRT_OUT_BUFFER_SIZE,		// output buffer size 
		0,							// input buffer size 
		0,							// client time-out 
		NULL);						// no security attribute 

	if(hOutputPipe == INVALID_HANDLE_VALUE) return PIPE_CREATION_FAILURE;
	
	// Create the process window
	if(!CreateProcessWindow(CRTExecName, EMU86_OUT_PIPE_NAME, "Program Output", &outputProcess)) 
		return PROCESS_CREATION_FAILURE;

	initDone = true;
	
	return CRT_OK;
}

void CProgramWindow::SendMessage(Message *message, char *buffer)
{
	unsigned long int numBytesWritten;

	WriteFile(hOutputPipe, &message, sizeof(message), &numBytesWritten, NULL);
	if(message->length > 0) WriteFile(hOutputPipe, message, message->length, &numBytesWritten, NULL);
}

void CProgramWindow::ClearWindow(void)
{
	unsigned long numBytesWritten;
	Message message;

	message.action = CLEAR_SCREEN;
	message.length = 0;

	WriteFile(hOutputPipe, &message, sizeof(message), &numBytesWritten, NULL);
}

void CProgramWindow::PrintChar(char c)
{
	unsigned long numBytesWritten;
	Message message;

	message.action = PRINT_CHAR;
	message.length = 1;

	WriteFile(hOutputPipe, &message, sizeof(message), &numBytesWritten, NULL);
	WriteFile(hOutputPipe, &c, 1, &numBytesWritten, NULL);
}

// Sends string of size length directly to CRT (string may or may not be null terminated)
void CProgramWindow::SendString(char *string, int length)
{
	Message message;
	unsigned long numBytesWritten;

	// See if string + null will fit
	if(length + sizeof(message) > CRT_OUT_BUFFER_SIZE) {
		length = CRT_OUT_BUFFER_SIZE - sizeof(message);
	}

	message.length = length;
	message.action = PRINT_STRING;

	WriteFile(hOutputPipe, &message, sizeof(message), &numBytesWritten, NULL);
	WriteFile(hOutputPipe, string, length, &numBytesWritten, NULL);
}

// Like printf. Prints the simulated programs output.
int CProgramWindow::PrintText(const char *format, ...)
{
	Message message;
	va_list argptr;
	unsigned long numBytesWritten;
	
	va_start(argptr, format);
	message.length = vsprintf(outBuffer, format, argptr);
	va_end(argptr);

	// See if string + null will fit
	if(message.length + sizeof(message) > CRT_OUT_BUFFER_SIZE) {
		message.length = CRT_OUT_BUFFER_SIZE - sizeof(message);
	}

	message.action = PRINT_STRING;

	WriteFile(hOutputPipe, &message, sizeof(message), &numBytesWritten, NULL);
	WriteFile(hOutputPipe, outBuffer, message.length, &numBytesWritten, NULL);

	return numBytesWritten;
}

// Send signal to setup and initialize board
int CProgramWindow::InitSimptris(void)
{
	if(initSimpDone) return CRT_OK;

	// Create the Simptris output pipe //
	hSimpPipe = CreateNamedPipe( 
		EMU86_SIMP_PIPE_NAME,		// pipe name 
		PIPE_ACCESS_OUTBOUND,		// read/write access 
		PIPE_TYPE_BYTE |			// message type pipe 
		PIPE_READMODE_BYTE |		// message-read mode 
		PIPE_WAIT,					// blocking mode 
		1,							// max. instances  
		CRT_SIMP_BUFFER_SIZE,		// output buffer size 
		0,							// input buffer size 
		0,							// client time-out 
		NULL);						// no security attribute 

	if(hSimpPipe == INVALID_HANDLE_VALUE) return PIPE_CREATION_FAILURE;
	
	// Create the Simptris CRT process window
	if(!CreateProcessWindow(CRTExecName, EMU86_SIMP_PIPE_NAME, "Simptris", &simpProcess)) 
		return PROCESS_CREATION_FAILURE;

	initSimpDone = true;

	// Wait for process to open pipe //
	ConnectNamedPipe(hSimpPipe, NULL);

	// Send initialization command //
	unsigned long numBytesWritten;
	Message message;
	message.action = INIT_BOARD;
	message.length = 0;
	WriteFile(hSimpPipe, &message, sizeof(message), &numBytesWritten, NULL);

	if(numBytesWritten != sizeof(message)) return PIPE_CREATION_FAILURE;


	return CRT_OK;
}

void CProgramWindow::KillSimptris(void)
{
	if(!initSimpDone) return;
	TerminateProcess(simpProcess.hProcess, 0);
	DisconnectNamedPipe(hSimpPipe);
	CloseHandle(hSimpPipe);
	initSimpDone = false;
}

// Sends fixed length string representing board content
void CProgramWindow::SendBoard(char *string)
{
	unsigned long numBytesWritten;
	Message message;

	message.action = PRINT_BOARD;
	message.length = BOARD_WIDTH * BOARD_HEIGHT;

	WriteFile(hSimpPipe, &message, sizeof(message), &numBytesWritten, NULL);
	WriteFile(hSimpPipe, string, message.length, &numBytesWritten, NULL);
}

// Sends the score through the fifo
void CProgramWindow::SendScore(int number)
{
	unsigned long numBytesWritten;
	Message message;

	message.action = PRINT_SCORE;
	message.length = sizeof(number);

	WriteFile(hSimpPipe, &message, sizeof(message), &numBytesWritten, NULL);
	WriteFile(hSimpPipe, &number, message.length, &numBytesWritten, NULL);
}

void CProgramWindow::ClearSimpWindow(void)
{
	unsigned long numBytesWritten;
	Message message;

	message.action = CLEAR_SCREEN;
	message.length = 0;

	WriteFile(hSimpPipe, &message, sizeof(message), &numBytesWritten, NULL);
}

#endif // #ifdef WIN32_CONSOLE //////////////////////////////////////

