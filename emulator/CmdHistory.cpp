// CmdHistory.cpp: implementation of the CCmdHistory class.
//
// This class allows you to bring up past commands by pressing the
// up-arrow. The work of saving and managing the commands is 
// performed by this class.
//
//////////////////////////////////////////////////////////////////////

#include "CmdHistory.h"
#include <string.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCmdHistory::CCmdHistory()
{
	head = tail = lastIndex = current = historyIndex = size = 0;
}

CCmdHistory::~CCmdHistory()
{
}

// Add a command in string form to the command history
void CCmdHistory::Add(char *string)
{
	// Avoid duplicating last entry
	if(!strcmp(string, history[lastIndex])) {
		Reset();
		return;
	}

	// Add new entry at tail
	strncpy(history[tail], string, ENTRY_SIZE);
	history[tail][ENTRY_SIZE-1] = '\0';
	lastIndex = tail;

	// Adjust indices
	tail++;
	if(tail == HISTORY_SIZE) tail = 0;
	
	// See if head entry needs to be removed
	if(size < HISTORY_SIZE) size++;
	else {
		// Remove entry at head
		head++;
		if(head == HISTORY_SIZE) head = 0;
	}

	Reset();
}

// Get the next command from the history (i.e., move forward in history).
const char *CCmdHistory::GetNext(void)
{
	const char *returnString;

	// Check if we've reached end of queue
	if(historyIndex > 1) {
		// Return next entry
		historyIndex--;

		current++;
		if(current == HISTORY_SIZE) current = 0;

		returnString = (const char *)history[current];
	}
	else {
		// Return nothing since at beginning
		current = tail;
		returnString = "\0";
	}

	return returnString;
}

// Get the previous command from the history (i.e., move backwards in history).
const char *CCmdHistory::GetPrevious(void)
{
	const char *returnString;

	// Check if at head of queue
	if(historyIndex < size) {
		// Not at head, give previous entry
		historyIndex++;
		
		current--;
		if(current < 0) current = HISTORY_SIZE - 1;

		returnString = (const char *)history[current];
	}
	else if(size > 0) {
		// At head
		current = head;
		returnString = (const char *)history[head];
	}
	else {
		// Return nothing since queue empty
		returnString = "\0";
	}

	return returnString;
}

// Reset to top of command history stack.
void CCmdHistory::Reset(void)
{
	current = tail;		// Set to start of queue
	historyIndex = 0;	// Currently at entry 0
}

