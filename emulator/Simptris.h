//
// Simptris.h: Header file for Simptris game
//
// See Simptris.cpp for more information. The timing of Simptris
// can be changed by modifying these defenitions.
//
//////////////////////////////////////////////////////////////////////

#ifndef SIMPTRIS_H
#define SIMPTRIS_H

#include "SimptrisBoard.h"
#include "ProgramWindow.h"

#define SIMP_TIME_SCALE	100		// Number of instructions to cycle before calling handle_simptris

#define TET_COM_DELAY 10	// Number of times handle_simptis is called before simptris commands take effect
#define INIT_FREQ 450
#define FREQ_STEP 50		// Amount to increase frequency
#define INCREASE_STEP 5		// Num lines needed to increase frequency
#define FALL_INCREASE_FACTOR 4
#define FALL_RATE 20
#define FALL_STEP 1
#define MIN_FREQ 0
#define MIN_FALL 1
#define MAJ_VERSION 4
#define MIN_VERSION 0
#define INCREASE_SCORE 100

// Tetris interrupt event IDs
#define TET_GAME_OVER	0
#define TET_PIECE_A		1
#define TET_PIECE_B		2
#define TET_TOUCHDOWN	3
#define TET_RECEIVED	4
#define TET_LINE_CLEAR	5

// Piece types (Values writtent to memory for user code)
#define TET_TYPE_A	0
#define TET_TYPE_B	1


enum Piece_type {straight = 0, corner = 1};


typedef struct
{
  unsigned int row; 
  unsigned int col;
} Position;

struct Piece
{
  Position pos;
  int id;
  enum Piece_type ptype;
  int orientation;
  struct Piece *next;
  struct Piece *prev;
};


class CSimptris
{
private:
	void slide_piece(int pieceid, int dir);
	void rotate_piece(int pieceid, int dir);
	// void signal_interrupt(int intID, int a, int b, int c, int d);	Moved to Bios.cpp
	int random_num(int limit);
	void addpiececell(int y,int x);
	void send_displayboard();
	void handle_touchdown(int pieceid);
	void update_score();
	void check_lineclear(int row);
	void generate_piece();
	struct Piece* find_piece(int find_id);
	void drop_pieces();

	int nextSlideID, nextSlideDir, doSlideCount;
	int nextRotateID, nextRotateDir, doRotateCount;

	struct Piece *firstpiece;
	bool board[BOARD_WIDTH][BOARD_HEIGHT];
	char displayboard[BOARD_WIDTH][BOARD_HEIGHT];
	unsigned int freq_min, next_id;
	int fall_rate, score;
	bool game_over;
	int simpticks, simpwait, seeded;

public:
	CSimptris();
	~CSimptris();

	bool GameOver(void) { return game_over; }
	void SetWindow(CProgramWindow *programWindow) { window = programWindow; }
	void EndGame(void) { game_over = true; }
	void SlidePieceCmd(int pieceid, int dir);
	void RotatePieceCmd(int pieceid, int dir);
	void seed(int number);
	void init_simptris();
	void handle_simptris();

	CProgramWindow *window;
};

#endif	// #ifndef SIMPTRIS_H


