/* 
File: lab6app.c
Revision date: 4 November 2009
Description: Application code for EE 425 lab 6 (Message queues)
*/

#include "clib.h"
#include "yakk.h"                   /* contains kernel definitions */
#include "lab6defs.h"               /* contains definitions for this lab */

#define TASK_STACK_SIZE   512       /* stack size in words */
#define MSGQSIZE          10

struct msg MsgArray[MSGARRAYSIZE];  /* buffers for message content */

int ATaskStk[TASK_STACK_SIZE];      /* a stack for each task */
int BTaskStk[TASK_STACK_SIZE];
int STaskStk[TASK_STACK_SIZE];

int GlobalFlag;

void *MsgQ[MSGQSIZE];           /* space for message queue */
YKQ *MsgQPtr;                   /* actual name of queue */

void ATask(void)                /* processes data in messages */
{
    struct msg *tmp;
    int min, max, count;
        
    min = 100;
    max = 0;
    count = 0;
    
    while (1)
    {
        tmp = (struct msg *) YKQPend(MsgQPtr); /* get next msg */
                
        /* check sequence count in msg; were msgs dropped? */
        if (tmp->tick != count+1)
        {
            print("! Dropped msgs: tick ", 21);
            if (tmp->tick - (count+1) > 1) {
                printInt(count+1);
                printChar('-');
                printInt(tmp->tick-1);
                printNewLine();
            }
            else {
                printInt(tmp->tick-1);
                printNewLine();
            }
        }
                
        /* update sequence count */
        count = tmp->tick;
                
        /* process data; update statistics for this sample */
        if (tmp->data < min)
            min = tmp->data;
        if (tmp->data > max)
            max = tmp->data;
                
        /* output min, max, tick values */
        print("Ticks: ", 7);
        printInt(count);
        print("\t", 1);
        print("Min: ", 5);
        printInt(min);
        print("\t", 1);
        print("Max: ", 5);
        printInt(max);
        printNewLine();
    }
}

void BTask(void)                /* saturates the CPU for 5 ticks */
{
    int busycount, curval, j, flag, chcount;
    unsigned tickNum;
        
    curval = 1001;
    chcount = 0;
    
    while (1)
    {
        YKDelayTask(2);
        
        if (GlobalFlag == 1)
        {                           /* flag set -- loop for 5 ticks */
            YKEnterMutex();
            busycount = YKTickNum;
            YKExitMutex();
            
            while (1)
            {
                YKEnterMutex();
                tickNum = YKTickNum;
                YKExitMutex();
                if(tickNum >= busycount + 5) break;                
                
                curval += 2;        /* evaluate next number */
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
                    printChar('.'); /* output a marker for each prime */
                    if (++chcount > 75)
                    {
                        printNewLine();
                        chcount = 0;
                    }
                }
            }
            printNewLine();
            chcount = 0;
            GlobalFlag = 0;        /* clear flag */
        }
    }
}

void STask(void)                /* tracks statistics */
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

    YKNewTask(BTask, (void *) &BTaskStk[TASK_STACK_SIZE], 10);
    YKNewTask(ATask, (void *) &ATaskStk[TASK_STACK_SIZE], 20);
    
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
    
    /* create queue, at least one user task, etc. */
    GlobalFlag = 0;
    MsgQPtr = YKQCreate(MsgQ, MSGQSIZE);
    YKNewTask(STask, (void *) &STaskStk[TASK_STACK_SIZE], 30);
    
    YKRun();
}
