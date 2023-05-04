/*
File: lab5app.c
Revision date: 13 November 2009
Description: Application code for EE 425 lab 5 (Semaphores)
*/

#include "clib.h"
#include "yakk.h"

#define TASK_STACK_SIZE 512           /* stack size in words */

int TaskWStk[TASK_STACK_SIZE];        /* stacks for each task */
int TaskSStk[TASK_STACK_SIZE];
int TaskPStk[TASK_STACK_SIZE];
int TaskStatStk[TASK_STACK_SIZE];
int TaskPRMStk[TASK_STACK_SIZE];

YKSEM *PSemPtr;                        /* YKSEM must be defined in yakk.h */
YKSEM *SSemPtr;
YKSEM *WSemPtr;
YKSEM *NSemPtr;

void TaskWord(void)
{
    while (1)
    {
        YKSemPend(WSemPtr);
        printString("Hey");
        YKSemPost(PSemPtr);
                
        YKSemPend(WSemPtr); 
        printString("it");
        YKSemPost(SSemPtr);
                
        YKSemPend(WSemPtr); 
        printString("works");
        YKSemPost(PSemPtr);
    }
}

void TaskSpace(void)
{
    while (1)
    {
        YKSemPend(SSemPtr);
        printChar(' ');
        YKSemPost(WSemPtr);
    }
}

void TaskPunc(void)
{
    while (1)
    {
        YKSemPend(PSemPtr);
        printChar('"');
        YKSemPost(WSemPtr);
                
        YKSemPend(PSemPtr);
        printChar(',');
        YKSemPost(SSemPtr);
                
        YKSemPend(PSemPtr);
        printString("!\"\r\n");
        YKSemPost(PSemPtr);
                
        YKDelayTask(6);
    }
}

void TaskPrime(void)            /* task that actually computes primes */
{
    int curval = 1001;
    int j,flag,lncnt;
    int endval;
    
    while (1)
    {
        YKSemPend(NSemPtr);
	
        /* compute next range of primes */
        lncnt = 0;
        endval = curval + 500;
        for ( ; curval < endval; curval += 2)
        {
            flag = 0;
            for (j = 3; (j*j) < curval; j += 2)
            {
                if (curval % j == 0)
                {
                    flag = 1;
                    break;
                }
            }
            if (!flag)
            {
                printChar(' ');
                printInt(curval);
                lncnt++;
                if (lncnt > 9)
                {
                    printNewLine();
                    lncnt = 0;
                }
            }
        }
        printNewLine();
    }
}

void TaskStat(void)                /* a task to track statistics */
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

    YKNewTask(TaskPrime, (void *) &TaskPRMStk[TASK_STACK_SIZE], 32);
    YKNewTask(TaskWord,  (void *) &TaskWStk[TASK_STACK_SIZE], 10);
    YKNewTask(TaskSpace, (void *) &TaskSStk[TASK_STACK_SIZE], 11);
    YKNewTask(TaskPunc,  (void *) &TaskPStk[TASK_STACK_SIZE], 12);
    
    while (1)
    {
        YKDelayTask(20);
        
        YKEnterMutex();
        switchCount = YKCtxSwCount;
        idleCount = YKIdleCount;
        YKExitMutex();
        
        printString ("<<<<< Context switches: ");
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
    
    /* create all semaphores, at least one user task, etc. */
    PSemPtr = YKSemCreate(1);
    SSemPtr = YKSemCreate(0);
    WSemPtr = YKSemCreate(0);
    NSemPtr = YKSemCreate(0);
    YKNewTask(TaskStat, (void *) &TaskStatStk[TASK_STACK_SIZE], 30);
    
    YKRun();
}
