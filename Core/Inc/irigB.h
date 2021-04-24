/*
 * irigB.h
 *
 *	IRIG-B Protocol decoder
 *	To decode the signal, handleInterrupt must be called with the pulse length given as parameter in microseconds
 *	(use a timer interrupt in Input Capture mode)
 *
 *	Functionality:
 *	- Frame error checking
 *	- Frame decoded in the timeFrame structure (partially, more fields need to be added depending on protocol variant)
 *	- Frame statistics (signal quality) in the frameTiming structure
 *
 *  Created on: Apr 22, 2021
 *      Author: Andrei
 */

#ifndef INC_IRIGB_H_
#define INC_IRIGB_H_

#include "main.h"

#define ZERO_TIME 	1000
#define ONE_TIME	4000
#define INDEX_TIME	7000

#define IRIG_MAX_INDEX			99
#define IRIG_STATS_BUFFER_SIZE	10

/* Disable statistics if CPU overhead is too big, should not be a problem with CPU frequency > 20MHz */
/* Overhead also depends on CPU architecture and compiler optimizations settings */
#define GENERATE_STATISTICS 1

typedef enum
{
	CLOCK_LOCKED,
	TIME_UNRELIABLE = 0xF
}IRIG_TIME_QUALITY;

typedef struct
{
	uint32_t seconds;
	uint32_t minutes;
	uint32_t hours;
	uint32_t days;
	uint32_t years;
	IRIG_TIME_QUALITY timeQuality;
	uint32_t timeOfDay;
	uint8_t frameOK;
	uint32_t receivedFrames;
	uint32_t receivedOkIndexCount;
	uint32_t receivedBadIndexCount;

}IRIG_B_FRAME;

#if GENERATE_STATISTICS
typedef struct
{
	uint16_t one[IRIG_STATS_BUFFER_SIZE],zero[IRIG_STATS_BUFFER_SIZE],index[IRIG_STATS_BUFFER_SIZE];
	uint16_t oneCount,zeroCount,indexCount,iOne,iZero,iIndex;
	uint32_t averageOne,averageZero,averageIndex;
}IRIG_B_TIMING;
#endif



uint8_t frameBuffer[100];
IRIG_B_FRAME timeFrame;
#if GENERATE_STATISTICS
IRIG_B_TIMING frameTiming;
#endif

void handleInterrupt(uint32_t microSeconds);

void parseSeconds();
void parseMinutes();
void parseHours();
void parseDays();
void parseYears();
void parseTimeQuality();
void parseTimeOfDay();

#if GENERATE_STATISTICS
void indexBitAverage(uint32_t microSeconds);
void oneBitAverage(uint32_t microSeconds);
void zeroBitAverage(uint32_t microSeconds);
#endif

void checkIndexPosition(uint8_t index);


#endif /* INC_IRIGB_H_ */
