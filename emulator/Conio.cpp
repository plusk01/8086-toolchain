#include <stdlib.h>
#include <stdio.h>

#include <termios.h>
#include <string.h>

#include <sys/select.h>

#include "Conio.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define STDIN_FD 0

static struct termios stored_settings;
static fd_set stdin_wait_set;
static struct timeval tm_zero;

static int conioEnabled = FALSE;


// Disable automatic keyboard echo (not normal mode).
void disableEcho(void)
{
	struct termios new_settings;
	tcgetattr(0, &new_settings);
	new_settings.c_lflag &= (~ECHO);
	tcsetattr(0, TCSANOW, &new_settings);
}


// Enable automatic keyboard echo (normal mode).
void enableEcho(void)
{
	struct termios new_settings;
	tcgetattr(0, &new_settings);
	new_settings.c_lflag |= ECHO;
	tcsetattr(0, TCSANOW, &new_settings);
}


// Enable non-canonical mode. Causes character read functions to block
// only until the first key is pressed (not until CR is entered).
void enableKeyPress(void)
{
    struct termios new_settings;

    tcgetattr(0, &new_settings);

    // Disable canonical mode, and set buffer size to 1 byte
    new_settings.c_lflag &= (~ICANON);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    
    tcsetattr(0, TCSANOW, &new_settings);
}


// Restore canonical keyboard mode (i.e., block until CR is read or buffer
// is full).
void disableKeyPress(void)
{
    struct termios new_settings;

    tcgetattr(0, &new_settings);

    // Disable canonical mode, and set buffer size to 1 byte
    new_settings.c_lflag |= ICANON;
    new_settings.c_cc[VTIME] = stored_settings.c_cc[VTIME];
    new_settings.c_cc[VMIN] = stored_settings.c_cc[VMIN];

    tcsetattr(0, TCSANOW, &new_settings);
}


// Returns the value of the key or extended key waiting to be read.
// If no key is waiting, returns 0.
// Use this function after the value 27 (ESC) is read to check if
// there are more values to follow for extended key informationg.
// For example, up arrow key usually sends three values: 27, 91, then 65.
// keyPress() will not detect these extended values.
char extendedKeyPress(void)
{
	struct termios new_settings;
	char c;
	int old_VMIN;

	// Disable blocking of getchar
	tcgetattr(0,&new_settings);
	old_VMIN = new_settings.c_cc[VMIN];
	new_settings.c_cc[VMIN] = 0;
	tcsetattr(0, TCSANOW, &new_settings);

	c = getchar();
	
	// Restore blocking of getchar
	new_settings.c_cc[VMIN] = old_VMIN;
	tcsetattr(0, TCSANOW, &new_settings);

	if(c != EOF) {
		ungetc(c, stdin);
		return c;
	}
	return 0;
}


// Returns key if a key has been pressed and is waiting to be read.
bool keyPress(void)
{
	FD_SET(STDIN_FD, &stdin_wait_set);

	if(select(1, &stdin_wait_set, NULL, NULL, &tm_zero) > 0) return true;
	return false;
}


// Initializes the conio routines. Call BEFORE using any conio functions.
// Saves terminal settings to be restored later with closeConio(),
// then it calls disableEcho() and enableKeyPress().
void initConio(void)
{
	if(conioEnabled) return;

	FD_ZERO(&stdin_wait_set);
	FD_SET(STDIN_FD, &stdin_wait_set);

	tm_zero.tv_sec = 0;
	tm_zero.tv_usec = 0;
	
	tcgetattr(0, &stored_settings);
	disableEcho();
	enableKeyPress();

	conioEnabled = TRUE;
}


// Restores terminal settings to what they were before initConio() was called.
void closeConio(void)
{
	if(!conioEnabled) return;

	tcsetattr(0, TCSANOW, &stored_settings);

	conioEnabled = FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// DEMO CODE //////////////////////////////////////////////////////////////////
/*
int main(void)
{
	char c;
	
	initConio();
	
	printf("Demo Started. Press some keys (0 to quit).\n");
	
	while(1) {
		c = getch();

		if(c == '0') break;
		
		if(c == 27 && extendedKeyPress()) {
			c = getchar();
			if(c == 91) {
				c = getchar();
				if(c == 65) printf("UP ARROW (27,91,65)\n");
				else if(c == 66) printf("DOWN ARROW (27,91,66)\n");
				else if(c == 67) printf("RIGHT ARROW (27,91,67)\n");
				else if(c == 68) printf("LEFT ARROW (27,91,68)\n");
				else if(c == 53) printf("PGUP (27,91,53,126)\n");
				else if(c == 54) printf("PGDN (27,91,54,126)\n");
			}
			while(extendedKeyPress()) getchar();	// Clean up excess chars
		}
		else if(c == 8) printf("BACKSPACE (8)\n");
		else if(c == 9) printf("TAB (9)\n");
		else if(c == 10) printf("ENTER (10)\n");
		else if(c == 27) printf("ESC (27)\n");
		else if(c == 32) printf("SPACE (32)\n");
		else if(c == 127) printf("DEL (127)\n");
		else printf("\'%c\' (%d)\n", c, (int)c);	
	}
	
	closeConio();
	
	return 0;
}
*/
