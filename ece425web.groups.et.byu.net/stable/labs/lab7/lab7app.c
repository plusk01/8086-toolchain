/* 
File: lab7app.c
Revision date: 10 November 2005
Description: Application code for EE 425 lab 7 (Event flags)
*/

#include "clib.h"
#include "yakk.h"                     /* contains kernel definitions */
#include "lab7defs.h"

#define TASK_STACK_SIZE   512         /* stack size in words */



YKEVENT *charEvent;
YKEVENT *numEvent;

int CharTaskStk[TASK_STACK_SIZE];     /* a stack for each task */
int AllCharsTaskStk[TASK_STACK_SIZE];
int AllNumsTaskStk[TASK_STACK_SIZE];
int STaskStk[TASK_STACK_SIZE];



void CharTask(void)        /* waits for any events triggered by letter keys */
{
    unsigned events;

    printString("Started CharTask     (2)\n");

    while(1) {
        events = YKEventPend(charEvent, 
                             EVENT_A_KEY | EVENT_B_KEY | EVENT_C_KEY,
                             EVENT_WAIT_ANY);
        
        if(events == 0) {
            printString("Oops! At least one event should be set "
                        "in return value!\n");
        }

        if(events & EVENT_A_KEY) {
            printString("CharTask     (A)\n");
            YKEventReset(charEvent, EVENT_A_KEY);
        }
        
        if(events & EVENT_B_KEY) {
            printString("CharTask     (B)\n");
            YKEventReset(charEvent, EVENT_B_KEY);
        }
        
        if(events & EVENT_C_KEY) {
            printString("CharTask     (C)\n");
            YKEventReset(charEvent, EVENT_C_KEY);
        }
    }
}


void AllCharsTask(void)    /* waits for all events triggered by letter keys */
{
    unsigned events;

    printString("Started AllCharsTask (3)\n");

    while(1) {
        events = YKEventPend(charEvent, 
                             EVENT_A_KEY | EVENT_B_KEY | EVENT_C_KEY,
                             EVENT_WAIT_ALL);
        // To be reset by WaitForAny task
        
        if(events != 0) {
            printString("Oops! Char events weren't reset by CharTask!\n");
        }

        printString("AllCharsTask (D)\n");
    }
}


void AllNumsTask(void)     /* waits for events triggered by number keys */
{
    unsigned events;

    printString("Started AllNumsTask  (1)\n");

    while(1) {
        events = YKEventPend(numEvent, 
                             EVENT_1_KEY | EVENT_2_KEY | EVENT_3_KEY,
                             EVENT_WAIT_ALL);
        
        if(events != (EVENT_1_KEY | EVENT_2_KEY | EVENT_3_KEY)) {
            printString("Oops! All events should be set in return value!\n");
        }

        printString("AllNumsTask  (123)\n");

        YKEventReset(numEvent, EVENT_1_KEY | EVENT_2_KEY | EVENT_3_KEY);
    }
}


void STask(void)           /* tracks statistics */
{
    unsigned max, switchCount, idleCount;
    int tmp;

    YKDelayTask(1);
    printString("Welcome to the YAK kernel\r\n");
    printString("Determining CPU capacity\r\n");
    YKDelayTask(1);
    YKIdleCount = 0;
    YKDelayTask(5);
    max = YKIdleCount / 25;
    YKIdleCount = 0;

    YKNewTask(CharTask, (void *) &CharTaskStk[TASK_STACK_SIZE], 2);
    YKNewTask(AllNumsTask, (void *) &AllNumsTaskStk[TASK_STACK_SIZE], 1);
    YKNewTask(AllCharsTask, (void *) &AllCharsTaskStk[TASK_STACK_SIZE], 3);
    
    while (1)
    {
        YKDelayTask(20);
        
        YKEnterMutex();
        switchCount = YKCtxSwCount;
        idleCount = YKIdleCount;
        YKExitMutex();
        
        printString("<<<<< Context switches: ");
        printInt((int)switchCount);
        printString(", CPU usage: ");
        tmp = (int) (idleCount/max);
        printInt(100-tmp);
        printString("% >>>>>\r\n");
        
        YKEnterMutex();
        YKCtxSwCount = 0;
        YKIdleCount = 0;
        YKExitMutex();
    }
}   


void main(void)
{
    YKInitialize();

    charEvent = YKEventCreate(0);
    numEvent = YKEventCreate(0);
    YKNewTask(STask, (void *) &STaskStk[TASK_STACK_SIZE], 0);
    
    YKRun();
}
