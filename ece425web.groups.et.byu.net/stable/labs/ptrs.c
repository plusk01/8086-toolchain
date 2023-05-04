/* Examples of TCB structure declarations and linked list operations.
   I won't guarantee that these are flawless, but I did lift them more
   or less intact from my code.  These should give you a pretty good
   idea how to use linked list structures.  Note that the solutions
   are not completely general -- the code assumes, for example, that
   the ready list always has an entry for the idle task, except for
   the very first insertion, when the new task is created.  (Can you
   identify what parts of the code assume this?)  My purpose in making
   this code available is so that you have something to study to
   understand the general concepts.  My code is almost certainly based
   on assumptions about how things in my program work that might not
   be true in your program, so use this code at your own risk.  In
   addition, I've included only those parts of the code that deal with
   pointers, so you may have to add a few non-pointer related things
   here and there.

   If you are struggling a bit with doubly linked lists, keep in mind
   that insertion of a new node requires the updating of four
   pointers: (1) the forward pointer of the previous node in the list
   (if any), (2) the backward pointer of the new node, (3) the forward
   pointer of the new node, and (4) the backward pointer of the next
   node in the list (if any).  Study the code to see how these four
   pointers are taken care of in each case.  In general, routines of
   this type have to deal with special cases of inserting into an
   empty list, inserting at the head of a list, and inserting at the
   end of a list.  Forgetting to cover all the possible cases that
   might arise at each point can cause some debugging headaches. 

   James Archibald 
*/

#define MAXTASKS 3		/* count of user tasks */

typedef struct taskblock *TCBptr;
typedef struct taskblock
{				/* the TCB struct definition */
    void *stackptr;		/* pointer to current top of stack */
    int state;			/* current state */
    int priority;		/* current priority */
    int delay;			/* #ticks yet to wait */
    TCBptr next;		/* forward ptr for dbl linked list */
    TCBptr prev;		/* backward ptr for dbl linked list */
}  TCB;

TCBptr YKRdyList;		/* a list of TCBs of all ready tasks
				   in order of decreasing priority */ 
TCBptr YKSuspList;		/* tasks delayed or suspended */
TCBptr YKAvailTCBList;		/* a list of available TCBs */
TCB    YKTCBArray[MAXTASKS+1];	/* array to allocate all needed TCBs
				   (extra one is for the idle task) */

void code_examples(void)
{
    int i;
    TCBptr tmp, tmp2;

    /* code to construct singly linked available TCB list from initial
       array */ 

    YKAvailTCBList = &(YKTCBArray[0]);
    for (i = 0; i < MAXTASKS; i++)
	YKTCBArray[i].next = &(YKTCBArray[i+1]);
    YKTCBArray[MAXTASKS].next = NULL;
    
    /* code to grab an unused TCB from the available list */

    tmp = YKAvailTCBList;
    YKAvailTCBList = tmp->next;

    /* code to insert an entry in doubly linked ready list sorted by
       priority numbers (lowest number first).  tmp points to TCB
       to be inserted */ 

    if (YKRdyList == NULL)	/* is this first insertion? */
    {
	YKRdyList = tmp;
	tmp->next = NULL;
	tmp->prev = NULL;
    }
    else			/* not first insertion */
    {
	tmp2 = YKRdyList;	/* insert in sorted ready list */
	while (tmp2->priority < tmp->priority)
	    tmp2 = tmp2->next;	/* assumes idle task is at end */
	if (tmp2->prev == NULL)	/* insert in list before tmp2 */
	    YKRdyList = tmp;
	else
	    tmp2->prev->next = tmp;
	tmp->prev = tmp2->prev;
	tmp->next = tmp2;
	tmp2->prev = tmp;
    }

    /* code to remove an entry from the ready list and put in
       suspended list, which is not sorted.  (This only works for the
       current task, so the TCB of the task to be suspended is assumed
       to be the first entry in the ready list.)   */

    tmp = YKRdyList;		/* get ptr to TCB to change */
    YKRdyList = tmp->next;	/* remove from ready list */
    tmp->next->prev = NULL;	/* ready list is never empty */
    tmp->next = YKSuspList;	/* put at head of delayed list */
    YKSuspList = tmp;
    tmp->prev = NULL;
    if (tmp->next != NULL)	/* susp list may be empty */
	tmp->next->prev = tmp;

    /* code to remove an entry from the suspended list and insert it
       in the (sorted) ready list.  tmp points to the TCB that is to
       be moved. */

    if (tmp->prev == NULL)	/* fix up suspended list */
	YKSuspList = tmp->next;
    else
	tmp->prev->next = tmp->next;
    if (tmp->next != NULL)
	tmp->next->prev = tmp->prev;

    tmp2 = YKRdyList;		/* put in ready list (idle task always
				 at end) */
    while (tmp2->priority < tmp->priority)
	tmp2 = tmp2->next;
    if (tmp2->prev == NULL)	/* insert before TCB pointed to by tmp2 */
	YKRdyList = tmp;
    else
	tmp2->prev->next = tmp;
    tmp->prev = tmp2->prev;
    tmp->next = tmp2;
    tmp2->prev = tmp;
}
