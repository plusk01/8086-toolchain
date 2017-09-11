#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <curses.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "UNIXCRT.h"
#include "CRTCom.h"
#include "../SimptrisBoard.h"

#define BOARD_TOP	2	// Position of top of board
#define BOARD_LEFT	5	// Position of left side of board

bool simptris;
int hOutputPipe;
char buffer[CRT_OUT_BUFFER_SIZE+1];

const char initialScreen[] = 
	"........"
	".      ."
	".      ."
	".      ."
	"|      |"
	"|      |"
	"|      |"
	"|      |"
	"|      |"
	"|      |"
	"|      |"
	"|      |"
	"|      |"
	"|      |"
	"|      |"
	"|      |"
	"|      |"
	"+------+"
;


//////////////////////////////////////////////////////////////////////////////
// Terminal functions ////////////////////////////////////////////////////////

void GetTermSize(int *rows, int *cols)
{
	struct winsize win;
	if(ioctl(0, TIOCGWINSZ, &win) != -1) {	// 0 is fd for stdin
		*rows = win.ws_row;
		*cols = win.ws_col;
		return;
	}
	
	/* This is an older method that can also be used
	struct ttysize ttywin;
	if(ioctl(0, TIOCGSIZE, &ttywin) != -1) {	// 0 is fd for stdin
		*rows = ttywin.ts_lines;
		*cols = ttywin.ts_cols;
		return;
	}
	*/
	
	// If the above fails, use env variables.
	// NOTE: These values will be incorect if the window has been resized.
	*rows = atoi(getenv("LINES"));
	*cols = atoi(getenv("COLUMNS"));
}

void DisableEcho(void)
{
	struct termios new_settings;
	tcgetattr(0,&new_settings);
	new_settings.c_lflag &= (~ECHO);
	tcsetattr(0,TCSANOW,&new_settings);
}


//////////////////////////////////////////////////////////////////////////////
// Simptris Functions ////////////////////////////////////////////////////////

void PrintScore(unsigned score)
{
	char string[12];
	sprintf(string, "%u", score);
	mvaddstr(BOARD_TOP + BOARD_HEIGHT + 2*BOARD_BORDER, BOARD_LEFT+BOARD_WIDTH, string);
}


void PrintBoard(char *data)
{
	int i, y;

	y = BOARD_HEIGHT + 2;
	for(i = 0; i < BOARD_HEIGHT; i++) {
		mvaddnstr(y, BOARD_LEFT+BOARD_BORDER, &data[(BOARD_WIDTH)*i], BOARD_WIDTH);
		y--;
	}

	move(0, 0);
}

void DrawBoardFrame(void)
{
	int y;

	for(y = 0; y < BOARD_HEIGHT+2*BOARD_BORDER; y++) {
		mvaddnstr(y+BOARD_TOP, BOARD_LEFT, (const char *)&initialScreen[(BOARD_WIDTH+2)*y], BOARD_WIDTH+2);
	}
	mvaddstr(y+BOARD_TOP, BOARD_LEFT, "Score");
	PrintScore(0);
}


bool InitSimptris(void)
{
	// Setup curses //
	if(initscr() == NULL) return false;
	if(noecho() == ERR) return false;
	clear();
	scrollok(stdscr, false);	// Disable scrolling
	idlok(stdscr, false);		// Disable terminal line insert (used for scrolling)
	
	// Display board //
	DrawBoardFrame();

	refresh();

	return true;
}

void EndSimptris(void)
{
	if(simptris) {
		clear();
		refresh();
		endwin();
	}
}

//////////////////////////////////////////////////////////////////////////////
// Text Output ///////////////////////////////////////////////////////////////

void printString(char *string, int n)
{
	for(; n; n--) putchar(*string++);
}



//////////////////////////////////////////////////////////////////////////////
// Message Handler //////////////////////////////////////////////////////////

/* 
This is what DOS Int 21h does:
0 NULL (service 9h, 40h, and 2h print space)
7 BELL (does this ring in dos?)
8 BACKSPACE (service 9h, 40h, and 2h backspace but don't overwrite)
9 TAB	(service 9h, 40h, and 2h all tab to nearest 8th char)
10 LF	(service 9h, 40h, and 2h all do a simple line feed)
13 CR	(service 9h, 40h, and 2h all do a carriage return)
26 SUBSTITUTE	(service 40h won't print this char or beyond this character)
				(service 9h and 2h print the little right arrow)
255 EOF/WHITESPACE (like space)
*/

void ProcessIncoming(void)
{
	Message message;
	unsigned long int numBytesRead, numBytesWritten;
	int status;
	int numErrors;
	char c;
	int i;

	numErrors = 0;

	while(true) {
		// Get message header
		if(read(hOutputPipe, &message, sizeof(message)) <= 0) {
			if(simptris) EndSimptris();
			return;
		}

		// Get message body
		if(message.length > 0) {
			if(message.length >= CRT_OUT_BUFFER_SIZE) message.length = CRT_OUT_BUFFER_SIZE - 1;
			read(hOutputPipe, buffer, message.length);
		}

		switch(message.action) {
		case EXIT:
			if(simptris) EndSimptris();
			return;
		case CLEAR_SCREEN:
			if(simptris) {
				clear();
				DrawBoardFrame();
				refresh();
			}
			else {
				int i, numRows, numCols;
				GetTermSize(&numRows, &numCols);
				numCols--;
				printf("<END OF OUTPUT>");
				for(i = 0; i < 24; i++) putchar('\n');	// Add blank lines
				while(numCols-- > 0) putchar('_');			// Make separator
				while(numRows-- > 0) putchar('\n');			// Scroll screen
	
				printf("\033[2J");	// Wipe screen
				printf("\033[H");	// Return cursor to home position (or "\033[0;0f" would work)
				fflush(stdout);		// Ensure redraw of screen
			}
			break;
		case PRINT_STRING:
			printString(buffer, message.length);
			fflush(stdout);
			break;
		case PRINT_CHAR:
			putchar(buffer[0]);
			fflush(stdout);
			break;
		case INIT_BOARD:
			if(!InitSimptris()) {
				printf("Simptris couldn't be initialized.\nPress Enter to terminate...");
				getchar();
				return;
			}
			else simptris = true;
			break;
		case PRINT_BOARD:
			PrintBoard(buffer);
			refresh();
			break;
		case PRINT_SCORE:
			PrintScore(*((int *)buffer));
			refresh();
			break;
		default:
			numErrors++;
			if(!simptris) printf("<COMMUNICATION ERROR>\n");
			else printw("<COMMUNICATION ERROR>\n");

			if(numErrors >= MAX_COM_ERRORS) {
				if(simptris) EndSimptris();
				printf("Too many communication errors!\nPress any key to terminate...");
				return;
			}
		}
	}
}


int main(int argc, char *argv[])
{
	if(argc != 2) {
		printf("BAD ARGUMENT: Pipe name required as parameter.\nPress Enter to continue...");
		getchar();
		return 1;
	}

	// Connect to pipe
	hOutputPipe = open(argv[1], O_RDONLY);
	if(hOutputPipe < 0) {
		printf("ERROR: Couldn't open pipe.\nPress Enter to terminate...\n");
		getchar();
		return 2;
	}

	simptris = 0;

	// Turn off character echo (so keys hit by user don't show up)
	DisableEcho();

	ProcessIncoming();

	return 0;
}



