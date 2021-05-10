/*
 * Timer.c
 *
 *  Created on: Dec 11, 2017
 *      Author: parth
 */

#include"Timer.h"
#include <time.h>
#include <signal.h>
#include "GlobalAndStructure.c"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>



int MakeTimer( timer_t *timerID, int ExpireTimeInSecond, int ExpireTimeInNanoSecond,int IntervalTimeInSecond ,int IntervalTimeInNanoSecond, void (TimerHandler)(int))
{
//	printf("Make timer invoked\n");
	int iReturnVal = TIMER_ERROR_NO_ERROR ;
	int tempIntervalFlag = 0 ;
	if(*timerID == 0)
	{
		struct sigevent te;
		struct itimerspec its;
		struct sigaction sa;
		int sigNo = SIGRTMIN;

		/* Set up signal handler. */
		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = MainTimerHandler;
		sigemptyset(&sa.sa_mask);
		if (sigaction(sigNo, &sa, NULL) == -1)
		{
			perror("sigaction");
			iReturnVal = TIMER_ERROR_SIGACTION_FAIL;
		}
		else
		{
			/* Set and enable alarm */
			te.sigev_notify = SIGEV_SIGNAL;
			te.sigev_signo = sigNo;
			te.sigev_value.sival_ptr = timerID;
			if(-1 == timer_create(CLOCK_REALTIME, &te, timerID))
			{
				perror("timer_create");
				iReturnVal = TIMER_ERROR_TIMER_CREATE_FAIL;
			}
			else
			{
				its.it_interval.tv_sec = IntervalTimeInSecond;
				its.it_interval.tv_nsec = IntervalTimeInNanoSecond;
				its.it_value.tv_sec = ExpireTimeInSecond;
				its.it_value.tv_nsec = ExpireTimeInNanoSecond;
				if((its.it_interval.tv_sec != 0 ) || (its.it_interval.tv_nsec!= 0 ))
				{
					tempIntervalFlag = 1 ;
				}
				if(-1 == timer_settime(*timerID, 0, &its, NULL))
				{
					perror("timer_settime");
					iReturnVal = TIMER_ERROR_TIMER_SET_TIME_FAIL;
				}
				else
				{
//					printf("Timer created and going to be add in timer database\n");
					iReturnVal = AddTimerInDataBase(*timerID,TimerHandler,tempIntervalFlag);
//					printf("Timer created and added in timer database\n");
				}
			}
		}
	}
	else
	{
		iReturnVal = TIMER_ERROR_ID_NOT_ZER0 ;
	}
	return iReturnVal;
}


int ResetTimerVal( timer_t timerID, int ExpireTimeInSecond, int ExpireTimeInNanoSecond,int IntervalTimeInSecond ,int IntervalTimeInNanoSecond)
{
	int iReturnVal = TIMER_ERROR_NO_ERROR ;
	struct itimerspec its;
	TimerLink* ptrTempLink = Head,*ptrPrevNode = NULL ;

	while(ptrTempLink != NULL)
	{
		if(ptrTempLink->stTimerInfo.TimerId == timerID)
		{
			its.it_interval.tv_sec = IntervalTimeInSecond;
			its.it_interval.tv_nsec = IntervalTimeInNanoSecond;
			its.it_value.tv_sec = ExpireTimeInSecond;
			its.it_value.tv_nsec = ExpireTimeInNanoSecond;
			if((its.it_interval.tv_sec != 0 ) || (its.it_interval.tv_nsec!= 0 ))
			{
				ptrTempLink->stTimerInfo.iIntervalTimerFlag = 1 ;
			}
			if(-1 == timer_settime(timerID, 0, &its, NULL))
			{
				perror("timer_settime");
				iReturnVal = TIMER_ERROR_TIMER_SET_TIME_FAIL;
			}
			break;
		}
		ptrTempLink = ptrTempLink->FrontLink;
	}

	if(ptrTempLink == NULL)
	{
		iReturnVal = TIMER_ERROR_TIMER_NOT_PRESENT ;
	}
	return iReturnVal ;
}


int DeleteTimerFromKernel(timer_t timerID)
{
	int iReturnVal = TIMER_ERROR_NO_ERROR ;
	if(timerID != 0)
	{
		if(-1 == timer_delete(timerID))
		{
			perror("__TIMER__ timer_delete() ");
			iReturnVal = TIMER_ERROR_TIMER_DELETE_FAIL ;
		}
		else
		{
			timerID = 0 ;
		}
	}
//	printf("timer deletion : %d\n",iReturnVal );
	return iReturnVal ;
}

static void MainTimerHandler( int sig, siginfo_t *si, void *uc )
{
//	printf("Main timer Handler invoked\n");
	timer_t *tidp;
	tidp = si->si_value.sival_ptr;
	TimerLink* ptrTempLink = Head ;
	while(ptrTempLink != NULL)
	{
		if(ptrTempLink->stTimerInfo.TimerId == *tidp)
		{
			if(ptrTempLink->stTimerInfo.iIntervalTimerFlag == 0)
			{
				DeleteTimerFromDataBase(ptrTempLink->stTimerInfo.TimerId);
//								printf("Delete timer 1  : %d\n",DeleteTimerFromDataBase(ptrTempLink->stTimerInfo.TimerId));
			}
			ptrTempLink->stTimerInfo.ThreadTimerHandler(*tidp);
			break;
		}
		ptrTempLink = ptrTempLink->FrontLink;
	}
//	PrintDataBase();
}


int AddTimerInDataBase(timer_t timerID,void (TimerHandler)(int),int iTempIntervalFlag)
{
//	printf("AddTimerInDataBase invoked\n");
	int iReturnVal = TIMER_ERROR_NO_ERROR ;
	TimerLink* ptrTempLink = Head,*ptrPrevNode = NULL ;

	//go to last node
	while(ptrTempLink != NULL)
	{
		ptrPrevNode = ptrTempLink;
		ptrTempLink = ptrTempLink->FrontLink;
	}

	//add node
	ptrTempLink = (TimerLink*)malloc(sizeof(TimerLink));
	if(ptrTempLink == NULL)
	{
		perror("malloc :");
		iReturnVal = TIMER_ERROR_MALLOC_FAIL ;
	}
	else
	{
		ptrTempLink->stTimerInfo.TimerId = timerID;
		ptrTempLink->stTimerInfo.ThreadTimerHandler = TimerHandler;
		if(iTempIntervalFlag == 1)
		{
			ptrTempLink->stTimerInfo.iIntervalTimerFlag = 1;
		}
		ptrTempLink->FrontLink = NULL;
		if(Head==NULL)
		{
			Head = ptrTempLink ;
		}
		else
		{
			ptrPrevNode->FrontLink = ptrTempLink;
		}
	}
//	PrintDataBase();
	return iReturnVal;
}

int DeleteTimer(timer_t *timerID)
{
//	printf("Delete database invoked\n");
	int iReturnVal = TIMER_ERROR_NO_ERROR ;

	//take head data for temprary purpose
	TimerLink* ptrTempLink = Head, *ptrPrevNode;

	// if we need to delete first timer
//	printf("__TIMER__ Timer id : %d\n",ptrTempLink->stTimerInfo.TimerId);
//	printf("__TIMER__ Link : %d\n",ptrTempLink);
//	printf("__TIMER__ condition : %d\n",(ptrTempLink != NULL && ptrTempLink->stTimerInfo.TimerId == *timerID));

	if (ptrTempLink != NULL && ptrTempLink->stTimerInfo.TimerId == *timerID)
	{
//		printf("__TIMER__ Here to delete first node Tid : %d\n",ptrTempLink->stTimerInfo.TimerId);
		//change head node
		Head = ptrTempLink->FrontLink;
		iReturnVal = DeleteTimerFromKernel(ptrTempLink->stTimerInfo.TimerId);
		if(iReturnVal == TIMER_ERROR_NO_ERROR)
		{
			*timerID = 0 ;
			free(ptrTempLink);               // free old head
		}
	}
	else
	{

//		printf("__TIMER__ Here to delete other node Tid : %d\n",ptrTempLink->stTimerInfo.TimerId);

		//		//copy first node adddress
		//		ptrTempLink = ptrTempLink->FrontLink;

		//search timer id in all the node,if found delete node via free() function and remove node from list
		while (ptrTempLink != NULL && ptrTempLink->stTimerInfo.TimerId != *timerID)
		{
			ptrPrevNode = ptrTempLink;
			ptrTempLink = ptrTempLink->FrontLink;
		}

		// If timer is not present in link list
		if (ptrTempLink == NULL)
		{
			iReturnVal = TIMER_ERROR_TIMER_NOT_PRESENT ;
		}
		else
		{
			// Remove node from list
			ptrPrevNode->FrontLink = ptrTempLink->FrontLink;

			//Actual timer deletion
			iReturnVal = DeleteTimerFromKernel(ptrTempLink->stTimerInfo.TimerId);

			// Free memory
			if(iReturnVal == TIMER_ERROR_NO_ERROR)
			{
				*timerID = 0 ;
				free(ptrTempLink);
			}
		}
	}
//	PrintDataBase();
	return iReturnVal;
}




void PrintDataBase()
{
		printf("printdatabase invoked\n");
	int iCnt = 1 ;
	TimerLink* ptrTempLink = Head;

	while (ptrTempLink != NULL)
	{
		printf("sr_no_%d__TimerID__%d___IntervalFlag__%d \n",iCnt++, ptrTempLink->stTimerInfo.TimerId,ptrTempLink->stTimerInfo.iIntervalTimerFlag);
		ptrTempLink = ptrTempLink->FrontLink;
	}
}




int DeleteTimerFromDataBase(timer_t timerID)
{
//	printf("Delete database invoked\n");
	int iReturnVal = TIMER_ERROR_NO_ERROR ;

	//take head data for temprary purpose
	TimerLink* ptrTempLink = Head, *ptrPrevNode;

	// if we need to delete first timer
	if (ptrTempLink != NULL && ptrTempLink->stTimerInfo.TimerId == timerID)
	{
		//change head node
		Head = ptrTempLink->FrontLink;
		// Free memory
		free(ptrTempLink);               // free old head
	}
	else
	{
		//		//copy first node adddress
		//		ptrTempLink = ptrTempLink->FrontLink;

		//search timer id in all the node,if found delete node via free() function and remove node from list
		while (ptrTempLink != NULL && ptrTempLink->stTimerInfo.TimerId != timerID)
		{
			ptrPrevNode = ptrTempLink;
			ptrTempLink = ptrTempLink->FrontLink;
		}

		// If timer is not present in link list
		if (ptrTempLink == NULL)
		{
			iReturnVal = TIMER_ERROR_TIMER_NOT_PRESENT ;
		}
		else
		{
			// Remove node from list
			ptrPrevNode->FrontLink = ptrTempLink->FrontLink;

			// Free memory
			free(ptrTempLink);
		}
	}
//	PrintDataBase();
	return iReturnVal;
}

