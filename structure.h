/*
 * structure.h
 *
 *  Created on: Feb 9, 2018
 *      Author: amithuda
 */

#ifndef STRUCTURE_H_
#define STRUCTURE_H_



typedef struct
{
	timer_t TimerId;
	int iIntervalTimerFlag ;
	void (*ThreadTimerHandler) (int);
}TimerInfo;

typedef struct
{
	TimerInfo stTimerInfo ;
	struct TimerLink* FrontLink ;
}TimerLink;

#endif /* STRUCTURE_H_ */
