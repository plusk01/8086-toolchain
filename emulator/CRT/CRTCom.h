//
// CRTCom.h: interface between Emu86 and the CRT process
//
// This file contains information to ensure that when Emu86 and the 
// CRT process communicate, the communication is understood. When
// this file is modified, both Emu86 and the CRT process must be
// recompiled.
//
//////////////////////////////////////////////////////////////////////


typedef struct {
	int action;
	int length;
} Message;


#define PROGRAM_CONSOLE_WIDTH	80
#define PROGRAM_CONSOLE_HEIGHT	25


#define MAX_COM_ERRORS	10


// Program Output Messages ///////////////////////////////////////////

#define EXIT			0
#define CLEAR_SCREEN	1
#define PRINT_STRING	2
#define PRINT_CHAR		3
// Emtris Specific
#define INIT_BOARD		4
#define PRINT_BOARD		5
#define PRINT_SCORE		6
//

