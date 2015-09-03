// 
// Simptris.cpp: Simptris game code
//
// This file was originally part of the SIMP architecute simulator.
// This module has been modified slightly (by Wade Fife) for use in 
// the Emu86 8086 simulator. The biggest change was from a C module to
// a C++ class. The original header comments are below.
//
//////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include "Simptris.h"
#include "SimptrisBoard.h"
#include "Bios.h"

#ifndef random
#define random(num) ((int)(((long)rand()*(num))/(RAND_MAX+1)))
#endif

/*
   This file contains routines that support the simptris game in the
   SIMP simulator for EE 425 at BYU.  Simptris is a simple tetris game
   with a small screen and only two piece types. It was designed to
   present a "real time" challenge: sending desired commands to
   rapidly falling pieces before they touch down.  However, it was
   originally written very quickly and is not as efficient as it could
   be. 

   revamped 27 Nov 2001 by jka: this functionality was originally in
   the process associated with the display.  Now it is part of the
   simulator (hopefully to get consistent results across platforms and
   workloads) and must communicate with the display process via a
   pipe, but only for output.
*/

CSimptris::CSimptris()
{
	// Deactivate until call to init_simptris
	game_over = true;
}

CSimptris::~CSimptris()
{
}

/* This function was moved to Bios.cpp
// Signals interrupts for the various simptris events.
void CSimptris::signal_interrupt(int intID, int a, int b, int c, int d)
{
	switch (intID)
	{
	case TET_GAME_OVER:
		// Signal that the game has ended
		FlagInterrupt(SIMP_GAME_OVER_INT);
		break;
	case TET_TOUCHDOWN:
		// Signal that a touchdown has occured
		// a is piece id
		// b through d are the 96 bits for the playing screen bitmap
		wrw(TET_TOUCHDOWN_ADDR, a);

		// Store bits, 1 col per word; MSB is bottom of coloumn
		wrw(TET_BITMAP_ADDR+0x0, b >> 16);	// Col 1 (leftmost)
		wrw(TET_BITMAP_ADDR+0x2, b);		// Col 2
		wrw(TET_BITMAP_ADDR+0x4, c >> 16);	// Col 3
		wrw(TET_BITMAP_ADDR+0x6, c);		// Col 4
		wrw(TET_BITMAP_ADDR+0x8, d >> 16);	// Col 5
		wrw(TET_BITMAP_ADDR+0xA, d);		// Col 6 (rightmost)
		FlagInterrupt(SIMP_TOUCHDOWN_INT);
		break;
	case TET_PIECE_A:
		// Signal that a new corner peice has entered scene (type A)
		// a is piece id
		// b is piece orientation
		// c is entry column
		wrw(TET_PIECE_TYPE_ADDR, TET_TYPE_A);
		wrw(TET_PIECE_ID_ADDR, a);
		wrw(TET_ORIENT_ADDR, b);
		wrw(TET_COLUMN_ADDR, c);
		FlagInterrupt(SIMP_NEW_PIECE_INT);

		//printf("\nNew piece of type A created. ID: %d    Orientation: %d    Column: %d\n\n", a, b, c);

		break;
	case TET_PIECE_B:
		// Signal that a new straight peice has entered scene (type B)
		// a is piece id
		// b is piece orientation
		// c is entry column
		wrw(TET_PIECE_TYPE_ADDR, TET_TYPE_B);
		wrw(TET_PIECE_ID_ADDR, a);
		wrw(TET_ORIENT_ADDR, b);
		wrw(TET_COLUMN_ADDR, c);
		FlagInterrupt(SIMP_NEW_PIECE_INT);

		//printf("\nNew piece of type B created. ID: %d    Orientation: %d    Column: %d\n\n", a, b, c);

		break;
	case TET_RECEIVED:
		// Signal that last command was received and Tetris is ready for a new command
		FlagInterrupt(SIMP_RECEIVED_INT);
		
		// printf("Command received\n");
		
		break;
	case TET_LINE_CLEAR:
		// Signal that a line has been cleared
		FlagInterrupt(SIMP_LINE_CLEAR_INT);
		break;
	default:
		return;
	}
}
*/

/* returns number in range 0..limit-1 */
int CSimptris::random_num(int limit)
{
#if defined HPUX_CONSOLE || defined LINUX_X86_CONSOLE
	return (int) (drand48() * (double) limit);
#else
	return random(limit);
#endif
}

void CSimptris::seed(int number)
{
    if(!seeded) {
#if defined HPUX_CONSOLE || defined LINUX_X86_CONSOLE
		srand48((long) number);
#else
		srand(number);
#endif		
		seeded = 1;
    }
}

void CSimptris::addpiececell(int y,int x)
{
    if (y < 0 || y >= BOARD_HEIGHT || x < 0 || x >= BOARD_WIDTH)
	fprintf(stderr, "Bad parameter in addpiececell: x=%d, y=%d\n",x,y);
    displayboard[x][y] = FALL_CHAR;
}
  
void CSimptris::send_displayboard()
{
    /* stuff board array and falling pieces into display board, then
       stick all that into a string of chars, then call send_board()
    */

    int col,row,bufindex;
    struct Piece* piece=firstpiece;
    char buf[200];

    for (col = 0; col < BOARD_WIDTH; col++)
	for (row = 0; row < BOARD_HEIGHT; row++)
	    if (board[col][row])
		displayboard[col][row] = DOWN_CHAR;
	    else
		displayboard[col][row] = BLANK_CHAR;

    while (piece != NULL)
    {
	row = piece->pos.row;
	col = piece->pos.col;

	switch(piece->ptype)
	{
	case straight:
	    switch(piece->orientation)
	    {
	    case 0: /* horizontal */
		addpiececell(row,col-1);
		addpiececell(row,col  );
		addpiececell(row,col+1);
		break;
	    case 1: /* vertical */
		addpiececell(row-1,col);
		addpiececell(row  ,col);
		addpiececell(row+1,col);
		break;
	    default:
		fprintf(stderr, 
			"\nSIMPTRIS ERROR: bad straight piece orient: %d\n", 
			piece->orientation);
	    }
	    break;
	case corner:
	    switch(piece->orientation)
	    {
	    case 0: /* |_ shape */
		addpiececell(row+1,col  );
		addpiececell(row  ,col  );
		addpiececell(row  ,col+1);
		break;
	    case 1: /* _| shape */
		addpiececell(row+1,col  );
		addpiececell(row  ,col  );
		addpiececell(row  ,col-1);
		break;
	    case 2: /* shape -| */
		addpiececell(row  ,col  );
		addpiececell(row  ,col-1);
		addpiececell(row-1,col  );  
		break;
	    case 3: /* shape |- */
		addpiececell(row  ,col  );
		addpiececell(row  ,col+1);
		addpiececell(row-1,col  );  
		break;
	    default:
		fprintf(stderr,
			"\nSIMPTRIS ERROR: bad corner piece orient: %d\n",
			piece->orientation);
	    }    
	}
	piece=piece->next;
    }

    /* now stuff into string and send (via pipe) to display process */
    bufindex = 0;
	for (row = 0; row < BOARD_HEIGHT; row++)
    for (col = 0; col < BOARD_WIDTH; col++)
	    buf[bufindex++] = displayboard[col][row];
    buf[bufindex] = '\0';	/* add EOS */
    //send_board(0, buf); // send_board(fifo_out, buf);
	window->SendBoard(buf);
}
  
void CSimptris::init_simptris(void)
{
    int w,h;
    for (w = 0; w < BOARD_WIDTH; w++)
	for (h = 0; h < BOARD_HEIGHT; h++)
	    board[w][h] = false;
    freq_min = INIT_FREQ;
    fall_rate=FALL_RATE;
    game_over = false;
    next_id = 0;
    simpticks = 0;
    simpwait = 0;
    score = -1;			/* init so that correct after initial call */
    firstpiece = NULL;
	seeded = 0;

	doSlideCount = 0;
	doRotateCount = 0;

    update_score();		/* will increment score to zero, display */
    send_displayboard();

}

void CSimptris::handle_touchdown(int pieceid)
{
    /* As of F01, the bitmap includes only the foundation on the bottom of the
       screen and no falling pieces.
    */
  
    int col,row,imgx, imgy, imgz;

    imgx = 0;
    imgy = 0;
    imgz = 0;

    for(col=0; col<=1; col++)
	for(row=0; row<BOARD_HEIGHT; row++)
	    imgx = (imgx<<1) | (int)board[col][row];
    for(col=2; col<=3; col++)
	for(row=0; row<BOARD_HEIGHT; row++)
	    imgy = (imgy<<1) | (int)board[col][row];
    for(col=4; col<=5; col++)
	for(row=0; row<BOARD_HEIGHT; row++)
	    imgz = (imgz<<1) | (int)board[col][row];
  
    signal_interrupt(TET_TOUCHDOWN, pieceid, imgx, imgy, imgz);
}

void CSimptris::update_score()
{   /* a line was cleared: update the score */
    score++;
    //send_score(0, score); // send_score(fifo_out, score);
	window->SendScore(score);
    if (((score%INCREASE_STEP)==0) && (score!=0))
    {
	if((freq_min-FREQ_STEP)>MIN_FREQ)
	    freq_min-=FREQ_STEP;
	if((score>INCREASE_SCORE) && ((score%(FALL_INCREASE_FACTOR*INCREASE_STEP))==0))
        {
	    fall_rate-=FALL_STEP;
	    if(fall_rate<1)
		fall_rate=1;
        }
    }
}

void CSimptris::check_lineclear(int row)
{
    /* Regardless of the piece dropped, this routine checks three rows to see if
       they are complete and can be cleared: the current row, plus the rows
       above and below.  If any of those lines is complete, this code clears the
       line and causes the new board to be displayed.
    */
  
    int i, j, k, current_row, row_count;
    bool clear_row, change;

    current_row = row-1;
    row_count = 0;
    change = false;

    if(row >= 12)
    {
	for(i=0; i<BOARD_WIDTH; i++)
	    for(j=13; j<=15; j++)
		if(board[i][j])
		    game_over=true;
	if(game_over)
	    signal_interrupt(TET_GAME_OVER,0,0,0,0);
    }

    if(!game_over)
    {
	for(i=0; i<3; i++)
	{ 
	    clear_row = true;
	    j=0;
	    while((j<BOARD_WIDTH) && (clear_row))
	    {
		if(!board[j][current_row])
		    clear_row = false;
		j++;
	    }
	  
	    if(clear_row) /* move down all rows above this one */
            /* note: current_row will stay the same when done because the rows have moved down */
	    {
		update_score();
		for(j=current_row; j<BOARD_HEIGHT-3; j++)
		    for(k=0; k<BOARD_WIDTH; k++)
			board[k][j]=board[k][j+1];
		change = true;
		row_count++;
	    }
	    else /* check the next highest row */
	    {
		current_row++;
	    }
	}
      
    }
    if (change)
    {
	signal_interrupt(TET_LINE_CLEAR,0,0,0,0);
	send_displayboard();
    }
}

void CSimptris::generate_piece()
{
    int new_type,new_orient,new_col;
    struct Piece* new_piece, *temp;
 
    /* generate new piece type, orientation, and position */

    new_type=random_num(2);
    if (new_type == 0)		/* straight */
	new_orient=random_num(2);
    else if (new_type == 1)	/* corner */
	new_orient=random_num(4);
    else
	printf("Bad type in generate_piece: %d\n", new_type);

    switch(new_type) 
    { 
    case 0:			/* straight */
	if (new_orient==0) 
	    new_col=random_num(BOARD_WIDTH-2)+1; /* horizontal */
	else 
	    new_col=random_num(BOARD_WIDTH); /* vertical */
	break; 
    case 1:			/* corner */
	if ((new_orient==0) || (new_orient==3)) /* one cell to right */
	    new_col=random_num(BOARD_WIDTH-1); 
	else			/* one cell to left */
	    new_col=random_num(BOARD_WIDTH-1)+1; 
	break; 
    } 

    /* initialize new piece */
    new_piece =              (Piece *)malloc(sizeof(struct Piece));
    new_piece->id =          next_id++;
    new_piece->pos.col =     new_col;
    new_piece->pos.row =     BOARD_HEIGHT-2;
    new_piece->ptype =       (Piece_type)new_type;
    new_piece->orientation = new_orient;
    new_piece->next =        NULL;

    /* now insert new piece in list */
    if(firstpiece == NULL) /* there are no active pieces on the screen */
    {
	firstpiece = new_piece;
	new_piece->prev = NULL; 
    }
    else
    {
	temp = firstpiece;
	while(temp->next != NULL)
	    temp = temp->next; /* go to end of list */
	temp->next = new_piece;
	new_piece->prev = temp; 
    }

    /* set up for interrupt by putting id, orientation and col in memory */
    if (new_type == corner)
	signal_interrupt(TET_PIECE_A, new_piece->id, new_piece->orientation,
		       new_piece->pos.col,0);
    else
	signal_interrupt(TET_PIECE_B, new_piece->id, new_piece->orientation,
		       new_piece->pos.col,0);
    send_displayboard();
}

struct Piece* CSimptris::find_piece(int find_id)
{
    struct Piece *temp, *found;
  
    temp = firstpiece;
    found = NULL;
    while(temp!=NULL)
    {
	if(temp->id == find_id)
	{
	    found = temp;
	    temp = NULL;
	}
	else
	    temp = temp->next;
    }
    return found;
}


// Queue rotate piece to occur in TET_COM_DELAY simpticks
void CSimptris::SlidePieceCmd(int pieceid, int dir)
{
	nextSlideID = pieceid;
	nextSlideDir = dir;
	doSlideCount = simpticks + TET_COM_DELAY;
}

// Queue rotate piece to occur in TET_COM_DELAY simpticks
void CSimptris::RotatePieceCmd(int pieceid, int dir)
{
	nextRotateID = pieceid;
	nextRotateDir = dir;
	doRotateCount = simpticks + TET_COM_DELAY;
}

  
void CSimptris::slide_piece(int pieceid, int dir)
{
    int row, col;
    struct Piece *piece;
    bool move_ok;

    /* find piece with this id */
    piece = find_piece(pieceid);

    move_ok = false;
    /* do we have a piece? */
    if(piece != NULL)
    {   /* slide it if it can slide */
	row = piece->pos.row;
	col = piece->pos.col;
	switch(piece->ptype)
	{
	case straight:
	    switch(piece->orientation)
	    {
	    case 0: /* horizontal */
		if((dir==0) && (col != 1))
		{
		    if(!board[col-2][row])
			move_ok=true;
		}
		else if((dir==1) && (col != BOARD_WIDTH-2))
		    if(!board[col+2][row])
			move_ok=true;
		break;
	    case 1: /* vertical */
		if((dir==0) && (col != 0))
		{
		    if( (!board[col-1][row]  ) &&
			(!board[col-1][row-1]) &&
			(!board[col-1][row+1])   )
			move_ok=true;
		}
		else if((dir==1) && (col != BOARD_WIDTH-1))
		    if( (!board[col+1][row]  ) &&
			(!board[col+1][row-1]) &&
			(!board[col+1][row+1])   )
			move_ok=true;
		break;
	    }
	    break;

	case corner:
	    switch(piece->orientation)
	    {
	    case 0: /* |_ orientation */
		if((dir==0) && (col != 0))
		{
		    if((!board[col-1][row]) && (!board[col-1][row+1]))
			move_ok=true;
		}
		else if((dir==1) && (col!=BOARD_WIDTH-2))
		    if((!board[col+2][row]) && (!board[col+1][row+1]))
			move_ok=true;
		break;
	    case 1: /* _| orientation */
		if((dir==0) && (col != 1))
		{
		    if((!board[col-2][row]) && (!board[col-1][row+1]))
			move_ok=true;
		}
		else if((dir==1) && (col!=BOARD_WIDTH-1))
		    if((!board[col+1][row]) && (!board[col+1][row+1]))
			move_ok=true;
		break;
	    case 2: /* -| orientation */
		if((dir==0) && (col != 1))
		{
		    if((!board[col-2][row]) && (!board[col-1][row-1]))
			move_ok=true;
		}
		else if((dir==1) && (col!=BOARD_WIDTH-1))
		    if((!board[col+1][row]) && (!board[col+1][row-1]))
			move_ok=true;
		break;
	    case 3: /* |- orientation */
		if((dir==0) && (col != 0))
		{
		    if((!board[col-1][row]) && (!board[col-1][row-1]))
			move_ok=true;
		}
		else if((dir==1) && (col!=BOARD_WIDTH-2))
		    if((!board[col+2][row]) && (!board[col+1][row-1]))
			move_ok=true;
		break;
	    }
	}

	if(move_ok)		/* move piece if the move is okay */
	{
	    if(dir==0)
		piece->pos.col-=1;
	    else
		piece->pos.col+=1;
	    send_displayboard();
	}
    }
    /* signal SIMP that next command can be sent */
    signal_interrupt(TET_RECEIVED,0,0,0,0);
}

void CSimptris::rotate_piece(int pieceid, int dir)
{
    int row,col,i;
    struct Piece *piece;
    bool move_ok;

    /* find piece with this id */
    piece = find_piece(pieceid);

    move_ok = false;
    /* do we have a piece? */
    if(piece != NULL)
    {	/* rotate it if it can rotate */
	row = piece->pos.row;
	col = piece->pos.col;
	switch(piece->ptype)
	{
	case straight:
	    switch(piece->orientation)
	    {
	    case 0: /* horizontal */
		if(row!=0)
		{
		    move_ok=true;
		    for(i=col-1; i<=col+1; i++)
			if(board[i][row+1] || board[i][row-1])
			    move_ok = false;
		}
		break;
	    case 1: /* vertical */
		if ((col!=0) && (col!=5))
		{
		    move_ok=true;
		    for(i=row-1; i<=row+1; i++)
			if(board[col-1][i] || board[col+1][i])
			    move_ok = false;
		}
		break;
	    }
	  
	    if(move_ok)
	    {
		if(piece->orientation == 0)
		    piece->orientation = 1;
		else
		    piece->orientation = 0;
		send_displayboard();
	    }
	    break;
	
	case corner:
	    switch(piece->orientation)
	    {
	    case 0: /* |_ orientation */
		if((dir==0) && (col!=0))
		{
		    if( (!board[col-1][row  ]) &&
			(!board[col-1][row+1]) &&
			(!board[col+1][row+1])   )
			move_ok = true;
		}
		else if((dir==1) && (row!=0))
		{
		    if( (!board[col  ][row-1]) &&
			(!board[col+1][row-1]) &&
			(!board[col+1][row+1])   )
			move_ok = true;
		}
		break;
	      
	    case 1: /* _| orientation */
		if((dir==1) && col!=BOARD_WIDTH-1)
		{
		    if( (!board[col+1][row  ]) &&
			(!board[col+1][row+1]) &&
			(!board[col-1][row+1])   )
			move_ok = true;
		}
		else if((dir==0) && (row!=0))
		{
		    if( (!board[col  ][row-1]) &&
			(!board[col-1][row-1]) &&
			(!board[col-1][row+1])   )
			move_ok = true;
		}
		break;
	      
	    case 2: /* -| orientation */
		if((dir==0) && col!=BOARD_WIDTH-1)
		{
		    if( (!board[col+1][row  ]) &&
			(!board[col+1][row-1]) &&
			(!board[col-1][row-1])   )
			move_ok = true;
		}
		else if(dir==1)
		{
		    if( (!board[col  ][row+1]) &&
			(!board[col-1][row+1]) &&
			(!board[col-1][row-1])   )
			move_ok = true;
		}
		break;

	    case 3: /* |- orientation */
		if((dir==1) && col!=0)
		{
		    if( (!board[col-1][row  ]) &&
			(!board[col-1][row-1]) &&
			(!board[col+1][row-1])   )
			move_ok = true;
		}
		else if(dir==0)
		{
		    if( (!board[col  ][row+1]) &&
			(!board[col+1][row+1]) &&
			(!board[col+1][row-1])   )
			move_ok = true;
		}
		break;
	    }
	    if(move_ok)
	    {
		if(dir==0)
		{
		    piece->orientation++;
		    if(piece->orientation > 3)
			piece->orientation = 0;
		}
		else if(dir==1)
		{
		    piece->orientation--;
		    if(piece->orientation == -1)
			piece->orientation = 3;
		}
		send_displayboard();
	    }
	    break;
	}
    }
    
    /* signal SIMP that next command can be sent */
    signal_interrupt(TET_RECEIVED,0,0,0,0);
}

void CSimptris::drop_pieces()
{
    struct Piece *current = firstpiece;
    struct Piece *temp1;
    struct Piece *temp2;
    int row, col, touchdown;
    bool stop;
  
    /* Goes through each falling piece and checks to see if it has hit something
       and will stop */
  
    touchdown = -1;
    while((current != NULL) && (!game_over))
    {

	stop = false;
	row = current->pos.row;
	col = current->pos.col;
      
	switch (current->ptype)
	{
	case straight:
	    switch(current->orientation)
	    {

	    case 0:  /* horizontal */
		/* Check to see if we have hit bottom */
		if (row == 0) 
		    stop=true;
		/* Check to see if the three spaces below this 
		   piece are occupied */
		else if ( board[col-1][row-1] ||
			  board[col  ][row-1] ||
			  board[col+1][row-1]    )
		    stop=true;
		if(stop) /* Occupy board if piece has stopped */
		{
		    board[col-1][row]=true;
		    board[col  ][row]=true;
		    board[col+1][row]=true;
		}
		break;

	    case 1: /* vertical */
		/* Check to see if we have hit bottom */
		if ((row-1) == 0)
		    stop=true;
		/* Check to see if the one space below this
		   piece is occupied */
		else if (board[col][row-2])
		    stop=true;
		if(stop) /* Occupy board if piece has stopped */
		{
		    board[col][row+1]=true;
		    board[col][row  ]=true;
		    board[col][row-1]=true;
		}
		break;
	    }
	    break;
	  
	case corner:
	    switch (current->orientation)
	    {
	    case 0: /* |_ shape */
		/* Check to see if we have hit bottom */
		if (row == 0) 
		{
		    stop=true;
		}
		/* Check to see if the three spaces below this 
		   piece are occupied */
		else if (	board[col  ][row-1] ||
				board[col+1][row-1]    )
		{
		    stop=true;
		}
		if(stop) /* Occupy board if piece has stopped */
		{
		    board[col  ][row+1]=true;
		    board[col  ][row  ]=true;
		    board[col+1][row  ]=true;
		}
		break;
	    
	    case 1: /* _| shape */
		/* Check to see if we have hit bottom */
		if (row == 0) 
		    stop=true;
		/* Check to see if the three spaces below this 
		   piece are occupied */
		else if (	board[col  ][row-1] ||
				board[col-1][row-1]    )
		    stop=true;
		if(stop) /* Occupy board if piece has stopped */
		{
		    board[col  ][row+1]=true;
		    board[col  ][row  ]=true;
		    board[col-1][row  ]=true;
		}
		break;
	    
	    case 2: /* -| shape */
		/* Check to see if we have hit bottom */
		if ((row-1) == 0) 
		    stop=true;
		/* Check to see if the three spaces below this 
		   piece are occupied */
		else if (	board[col-1][row-1] ||
				board[col  ][row-2]    )
		    stop=true;
		if(stop) /* Occupy board if piece has stopped */
		{
		    board[col-1][row  ]=true;
		    board[col  ][row  ]=true;
		    board[col  ][row-1]=true;
		}
		break;
	    
	    case 3: /* |- shape */
		/* Check to see if we have hit bottom */
		if ((row-1) == 0) 
		{
		    stop=true;
		}
		/* Check to see if the three spaces below this 
		   piece are occupied */
		else if (	board[col+1][row-1] ||
				board[col  ][row-2]    )
		{
		    stop=true;
		}
		if(stop) /* Occupy board if piece has stopped */
		{
		    board[col+1][row  ]=true;
		    board[col  ][row  ]=true;
		    board[col  ][row-1]=true;
		}
		break;
	    }
	    break;
	}

	if(stop==false) /* move piece down one row */
	{
	    current->pos.row-=1;            /* lower piece one row */
	    current=current->next; 
	}
	else      /* remove piece from list */
	{
	    if (current == firstpiece)      /* piece is first one in list */
	    {
		temp1=current->next;
		firstpiece = temp1;
	    }
	    else /* there is another piece in the list before this one */
	    {
		temp1 = current->prev; 
		temp1->next = current->next; 
		if(current->next!=NULL)
		{
		    temp2 = current->next;
		    temp2->prev = temp1;
		}
	    }
	    touchdown = current->id;  /* remember id */
	    check_lineclear(current->pos.row); /* check to see if any lines are cleared */
	    temp1=current->next;
	    free(current); 
	    current=temp1;
	}
    }
    send_displayboard();	/* picture will always be changed */
    if (touchdown >= 0)		/* then a piece touched down */
	handle_touchdown(touchdown); /* send interrupt, set up memory, etc. */
}

void CSimptris::handle_simptris()
{
    /* This routine is called from execute() every TIME_SCALE time units.  This
       routine is responsible for dropping pieces, generating pieces, and
       handling all state changes to keep the game flowing along.  If the state
       is changed in a way that requires the generation of interrupts, they must
       be triggered here also. 
    */
	
	if(!game_over) {
		simpticks++;

		// Check if it's time to do a previously requrested slide or rotate
	    if (doSlideCount && simpticks >= doSlideCount) {
			// call routine to do slide
			doSlideCount = 0;
			slide_piece(nextSlideID, nextSlideDir);
	    }
		else if(doRotateCount && simpticks >= doRotateCount) {
			// call routine to do rotate 
			doRotateCount = 0;
			rotate_piece(nextRotateID, nextRotateDir);
	    }


		/* if cycle_count is appropriate, generate new piece */
		if (((simpticks % freq_min) == 0) && simpwait > 2)
		{
			generate_piece();
			simpwait = 0;
		}
		
		/* if cycle_count is appropriate, have pieces drop a row */
		if ((simpticks % fall_rate) == 0)
		{
			drop_pieces();
			simpwait++;
		}
	}
}
/* Above routine modified for speed. Original body:
    simpticks++;
    // if cycle_count is appropriate, generate new piece 
    if (!game_over && ((simpticks % freq_min) == 0) && simpwait > 2)
    {
	generate_piece();
	simpwait = 0;
    }

    // if cycle_count is appropriate, have pieces drop a row
    if (!game_over && (simpticks % fall_rate) == 0)
    {
	drop_pieces();
	simpwait++;
    }
*/

