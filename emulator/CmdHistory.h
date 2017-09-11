// CmdHistory.h: interface for the CCmdHistory class.
//
// This class allows you to bring up past commands, usually
// by pressing the up-arrow. The work of saving and managing
// the commands is performed by this class.
//
//////////////////////////////////////////////////////////////////////

#ifndef CMD_HISTORY_H
#define CMD_HISTORY_H

#define HISTORY_SIZE	30
#define ENTRY_SIZE		70

class CCmdHistory  
{
public:
	CCmdHistory();
	virtual ~CCmdHistory();

	void Add(char *string);
	const char *GetNext(void);
	const char *GetPrevious(void);
	void Reset(void);

private:
	int historyIndex;	// Number representing current entry (from 0 to HISTORY_SIZE, where 1 is tail of queue)
	int current;		// Index of current entry
	int head;			// Head of queue (where old entries are removed from)
	int tail;			// Tail of queue (where new entries are added)
	int lastIndex;		// Index that was last used to store an entry (used to avoid duplicate entries)
	int size;			// Number of entries in queue
	char history[HISTORY_SIZE][ENTRY_SIZE];		// String data for queue
};

#endif // #ifndef CMD_HISTORY_H

