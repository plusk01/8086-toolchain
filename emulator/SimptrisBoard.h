//
// SimptrisBoard.h: Description of simptris board
//
// This file contains defentions that define the appearence of the 
// Simptris board. See also the appropriate CRT process file.
//
//////////////////////////////////////////////////////////////////////

#ifndef EMTRIS_H
#define EMTRIS_H

#include "SysConfig.h"

#define BOARD_BORDER 1		// Size of board's border
#define BOARD_HEIGHT 16		// Hight of playing board (without border)
#define BOARD_WIDTH 6		// Width of playing board (without border)

#ifdef WIN32_CONSOLE
#define FALL_CHAR (char)178
#define DOWN_CHAR (char)219
#define BLANK_CHAR ' '
#endif

#if defined(HPUX_CONSOLE) || defined(LINUX_X86_CONSOLE)
#define FALL_CHAR '#'
#define DOWN_CHAR '*'
#define BLANK_CHAR ' '
#endif


#endif

