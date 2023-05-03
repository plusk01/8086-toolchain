/* 
File: lab4c_app.c
Revision date: 23 December 2003
Description: Application code for EE 425 lab 4C (Kernel essentials C)
*/

#include "clib.h"
#include "yakk.h"

#define STACKSIZE 256          /* Size of task's stack in words */

int TaskStack[STACKSIZE];      /* Space for task's stack */

void Task(void);               /* Function prototype for task code */

void main(void)
{
    YKInitialize();
    
    printString("Creating task...\n");
    YKNewTask(Task, (void *) &TaskStack[STACKSIZE], 0);

    printString("Starting kernel...\n");
    YKRun();
}

void Task(void)
{
    unsigned idleCount;
    unsigned numCtxSwitches;

    printString("Task started.\n");
    while (1)
    {
        printString("Delaying task...\n");

        YKDelayTask(2);

        YKEnterMutex();
        numCtxSwitches = YKCtxSwCount;
        idleCount = YKIdleCount;
        YKIdleCount = 0;
        YKExitMutex();

        printString("Task running after ");
        printUInt(numCtxSwitches);
        printString(" context switches! YKIdleCount is ");
        printUInt(idleCount);
        printString(".\n");
    }
}

