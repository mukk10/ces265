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
* @file Utilities.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the general utility functions.
*/

#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <TypeDefs.h>

/**
*	Get time in mili seconds.
*/
u32 GetTimeInMiliSec();

/**
*	Bubble sort the values and their indexes.
*	Help is taken from: http://rosettacode.org/wiki/Sorting_algorithms/Bubble_sort#C
*	@param piVals Pointer of values.
*	@param piIdx Pointer of indexes.
*	@param uiSize Size of the array.
*/
void BubbleSortValIndex(i32 *piVals, i32 *piIdx, u32 uiSize);

/**
*	Get current date and time.
*	@param piBuff Array where date and time are written.
*	@param uiBuffSize Size of the array.
*/
void GetCurrentDateTime(i8 *piBuff, u32 uiBuffSize);

#endif	// __UTILITIES_H__