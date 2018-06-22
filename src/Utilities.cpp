/*
CES265, a multi-threaded HEVC encoder.
Copyright (C) 2013-2014, CES265 project.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file Utilities.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the general utility functions
*/

#include <Utilities.h>
#include <time.h>
#ifdef _MSC_VER
#include <Windows.h>
#endif

u32 GetTimeInMiliSec()
{
#ifdef _MSC_VER
	return GetTickCount();
#else
	// http://stackoverflow.com/questions/275004/c-timer-function-to-provide-time-in-nano-seconds
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ((u32)ts.tv_sec * 1000LL + (u32)ts.tv_nsec / 1000000LL);
#endif
}

void BubbleSortValIndex(i32 *piVals, i32 *piIdx, u32 uiSize)
{
	// Modified from: http://rosettacode.org/wiki/Sorting_algorithms/Bubble_sort#C
	int j, t = 1;
	while (uiSize-- && t)
		for (j = t = 0; j < i32(uiSize); j++) 
		{
			if (piVals[j] > piVals[j+1]) continue;
			t = piVals[j], piVals[j] = piVals[j+1], piVals[j+1] = t;
			t = piIdx[j], piIdx[j] = piIdx[j+1], piIdx[j+1] = t;
			t=1;
		}
}

void GetCurrentDateTime(i8 *piBuff, u32 uiBuffSize)
{
	// http://stackoverflow.com/questions/997946/c-get-current-time-and-date
	time_t now = time(0);
	struct tm tstruct;
	tstruct = *localtime(&now);
	// Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
	// for more information about date/time format
	strftime(piBuff, uiBuffSize, "%d-%m-%Y @ %X", &tstruct);
}