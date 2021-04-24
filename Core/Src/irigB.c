/*
 * irigB.c
 *
 * 	IRIG-B Protocol decoder
 *	To decode the signal, handleInterrupt must be called with the pulse length given as parameter in microseconds
 *	(use a timer interrupt in Input Capture mode)
 *
 *	Functionality:
 *	- Frame error checking
 *	- Frame decoded in the timeFrame structure
 *	- Frame statistics (signal quality) in the frameTiming structure
 *
 *  Created on: Apr 22, 2021
 *      Author: Andrei
 */

#include "irigB.h"

uint8_t frameBuffer[100];
IRIG_B_FRAME timeFrame;

#if GENERATE_STATISTICS
IRIG_B_TIMING frameTiming;
#endif

void handleInterrupt(uint32_t microSeconds)
{
	static uint8_t receivedIndexBit = 0;
	static uint8_t currentPosition = 0;

	if(microSeconds > INDEX_TIME)
	{
#if GENERATE_STATISTICS
		zeroBitAverage(microSeconds);
#endif
		if(receivedIndexBit == 1)
		{
			/* Start of frame */
			receivedIndexBit++;
			currentPosition = 1;
			return;
		}
		else if(receivedIndexBit == 0)
		{
			/* Might be an index, might be end of frame
			 * Let checkIndexPosition() handle it */
			if(currentPosition)
			{
				checkIndexPosition(currentPosition);
				currentPosition++;
			}
			receivedIndexBit = 1;
			return;
		}
		else
		{
			/* WTF? */
			return;
		}
	}
	if(!currentPosition)
	{
		/* We've not captured a frame start sequence yet, don't do anything */
		return;
	}

	if(currentPosition > IRIG_MAX_INDEX)
		return;
	receivedIndexBit = 0;
	if(microSeconds > ONE_TIME)
	{
#if GENERATE_STATISTICS
		oneBitAverage(microSeconds);
#endif
		/* Store a one in the frame buffer */
		frameBuffer[currentPosition] = 1;
		currentPosition++;
		return;
	}

	if(microSeconds > ZERO_TIME)
	{
#if GENERATE_STATISTICS
		zeroBitAverage(microSeconds);
#endif
		/* Store a zero in the frame buffer */
		frameBuffer[currentPosition] = 0;
		currentPosition++;
		return;
	}
}

void parseSeconds()
{
	timeFrame.seconds = frameBuffer[1] * 1 +
						frameBuffer[2] * 2 +
						frameBuffer[3] * 4 +
						frameBuffer[4] * 8 +
						frameBuffer[6] * 10 +
						frameBuffer[7] * 20 +
						frameBuffer[8] * 40;
}

void parseMinutes()
{
	timeFrame.minutes = frameBuffer[10] * 1 +
						frameBuffer[11] * 2 +
						frameBuffer[12] * 4 +
						frameBuffer[13] * 8 +
						frameBuffer[15] * 10 +
						frameBuffer[16] * 20 +
						frameBuffer[17] * 40;
}

void parseHours()
{
	timeFrame.hours = frameBuffer[20] * 1 +
					  frameBuffer[21] * 2 +
					  frameBuffer[22] * 4 +
					  frameBuffer[23] * 8 +
					  frameBuffer[25] * 10 +
					  frameBuffer[26] * 20;
}

void parseDays()
{
	timeFrame.days = frameBuffer[30] * 1 +
					 frameBuffer[31] * 2 +
					 frameBuffer[32] * 4 +
					 frameBuffer[33] * 8 +
					 frameBuffer[35] * 10 +
					 frameBuffer[36] * 20 +
					 frameBuffer[37] * 40 +
					 frameBuffer[38] * 80 +
					 frameBuffer[40] * 100 +
					 frameBuffer[41] * 200;
}

void parseYears()
{
	timeFrame.years = frameBuffer[50] * 1 +
					  frameBuffer[51] * 2 +
					  frameBuffer[52] * 4 +
					  frameBuffer[53] * 8 +
					  frameBuffer[55] * 10 +
					  frameBuffer[56] * 20 +
					  frameBuffer[57] * 40 +
					  frameBuffer[58] * 80;
}

void parseTimeQuality()
{
	/* Bit order not clear in the specifications, the order might be reversed */
	timeFrame.timeQuality = (frameBuffer[71]) |
							(frameBuffer[72] << 1) |
							(frameBuffer[73] << 2) |
							(frameBuffer[74] << 3);
}

void parseTimeOfDay()
{
	/* To be implemented */
	;
}

#if GENERATE_STATISTICS

void indexBitAverage(uint32_t microSeconds)
{
	uint16_t i;
	uint32_t currentAvg = 0;
	if(frameTiming.indexCount < IRIG_STATS_BUFFER_SIZE)
	{
		frameTiming.indexCount++;
	}
	frameTiming.index[frameTiming.iIndex] = microSeconds;
	frameTiming.iIndex++;
	frameTiming.iIndex%=frameTiming.indexCount;
	/* Add all the timings in the buffer */
	for(i=0;i<frameTiming.indexCount;i++)
		currentAvg += frameTiming.index[i];
	/* Average out the timings */
	currentAvg /= frameTiming.indexCount;
	frameTiming.averageIndex = currentAvg;
}

void oneBitAverage(uint32_t microSeconds)
{
	uint16_t i;
	uint32_t currentAvg = 0;
	if(frameTiming.oneCount < IRIG_STATS_BUFFER_SIZE)
	{
		frameTiming.oneCount++;
	}
	frameTiming.one[frameTiming.iOne] = microSeconds;
	frameTiming.iOne++;
	frameTiming.iOne%=frameTiming.oneCount;
	/* Add all the timings in the buffer */
	for(i=0;i<frameTiming.oneCount;i++)
		currentAvg += frameTiming.one[i];
	/* Average out the timings */
	currentAvg /= frameTiming.oneCount;
	frameTiming.averageOne = currentAvg;
}

void zeroBitAverage(uint32_t microSeconds)
{
	uint16_t i;
	uint32_t currentAvg = 0;
	if(frameTiming.zeroCount < IRIG_STATS_BUFFER_SIZE)
	{
		frameTiming.zeroCount++;
	}
	frameTiming.zero[frameTiming.iZero] = microSeconds;
	frameTiming.iZero++;
	frameTiming.iZero%=frameTiming.zeroCount;
	/* Add all the timings in the buffer */
	for(i=0;i<frameTiming.zeroCount;i++)
		currentAvg += frameTiming.zero[i];
	/* Average out the timings */
	currentAvg /= frameTiming.zeroCount;
	frameTiming.averageZero = currentAvg;
}

#endif

void checkIndexPosition(uint8_t index)
{
	if((index + 1) % 10 == 0)
	{
		timeFrame.frameOK = 1;
		timeFrame.receivedOkIndexCount++;
		if(index == 99)
		{
			/* End of frame, increase the received frames counter */
			timeFrame.receivedFrames++;
		}
		switch (index) {
			case 9:
				parseSeconds();
				break;
			case 19:
				parseMinutes();
				break;
			case 29:
				parseHours();
				break;
			case 49:
				parseDays();
				break;
			case 59:
				parseYears();
				break;
			case 79:
				parseTimeQuality();
				break;
			case 99:
				parseTimeOfDay();
				break;
			default:
				break;
		}
	}
	else
	{
		timeFrame.frameOK = 0;
		timeFrame.receivedBadIndexCount++;
	}
}
