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
* @file H265Transform.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods related to the H265 transform class
*/

#include <H265Transform.h>
#include <cassert>

/**
*	4x4 transform.
*	Table using 8-279.
*/
const i8 g_pbT4[4*4] = {
   64, 64, 64, 64,
   83, 36,-36,-83,
   64,-64,-64, 64,
   36,-83, 83,-36
};

/**
*	8x8 transform.
*/
const i8 g_pbT8[8*8] = {
   64, 64, 64, 64, 64, 64, 64, 64,
   89, 75, 50, 18,-18,-50,-75,-89,
   83, 36,-36,-83,-83,-36, 36, 83,
   75,-18,-89,-50, 50, 89, 18,-75,
   64,-64,-64, 64, 64,-64,-64, 64,
   50,-89, 18, 75,-75,-18, 89,-50,
   36,-83, 83,-36,-36, 83,-83, 36,
   18,-50, 75,-89, 89,-75, 50,-18
};

/**
*	16x16 transform.
*/
const i8 g_pbT16[16*16] = {
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   90, 87, 80, 70, 57, 43, 25,  9, -9,-25,-43,-57,-70,-80,-87,-90,
   89, 75, 50, 18,-18,-50,-75,-89,-89,-75,-50,-18, 18, 50, 75, 89,
   87, 57,  9,-43,-80,-90,-70,-25, 25, 70, 90, 80, 43, -9,-57,-87,
   83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83,
   80,  9,-70,-87,-25, 57, 90, 43,-43,-90,-57, 25, 87, 70, -9,-80,
   75,-18,-89,-50, 50, 89, 18,-75,-75, 18, 89, 50,-50,-89,-18, 75,
   70,-43,-87,  9, 90, 25,-80,-57, 57, 80,-25,-90, -9, 87, 43,-70,
   64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64,
   57,-80,-25, 90, -9,-87, 43, 70,-70,-43, 87,  9,-90, 25, 80,-57,
   50,-89, 18, 75,-75,-18, 89,-50,-50, 89,-18,-75, 75, 18,-89, 50,
   43,-90, 57, 25,-87, 70,  9,-80, 80, -9,-70, 87,-25,-57, 90,-43,
   36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36,
   25,-70, 90,-80, 43,  9,-57, 87,-87, 57, -9,-43, 80,-90, 70,-25,
   18,-50, 75,-89, 89,-75, 50,-18,-18, 50,-75, 89,-89, 75,-50, 18,
	9,-25, 43,-57, 70,-80, 87,-90, 90,-87, 80,-70, 57,-43, 25, -9,
};

/**
*	32x32 transform.
*/
const i8 g_pbT32[32*32] = {
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13,  4, -4,-13,-22,-31,-38,-46,-54,-61,-67,-73,-78,-82,-85,-88,-90,-90,
   90, 87, 80, 70, 57, 43, 25,  9, -9,-25,-43,-57,-70,-80,-87,-90,-90,-87,-80,-70,-57,-43,-25, -9,  9, 25, 43, 57, 70, 80, 87, 90,
   90, 82, 67, 46, 22, -4,-31,-54,-73,-85,-90,-88,-78,-61,-38,-13, 13, 38, 61, 78, 88, 90, 85, 73, 54, 31,  4,-22,-46,-67,-82,-90,
   89, 75, 50, 18,-18,-50,-75,-89,-89,-75,-50,-18, 18, 50, 75, 89, 89, 75, 50, 18,-18,-50,-75,-89,-89,-75,-50,-18, 18, 50, 75, 89,
   88, 67, 31,-13,-54,-82,-90,-78,-46, -4, 38, 73, 90, 85, 61, 22,-22,-61,-85,-90,-73,-38,  4, 46, 78, 90, 82, 54, 13,-31,-67,-88,
   87, 57,  9,-43,-80,-90,-70,-25, 25, 70, 90, 80, 43, -9,-57,-87,-87,-57, -9, 43, 80, 90, 70, 25,-25,-70,-90,-80,-43,  9, 57, 87,
   85, 46,-13,-67,-90,-73,-22, 38, 82, 88, 54, -4,-61,-90,-78,-31, 31, 78, 90, 61,  4,-54,-88,-82,-38, 22, 73, 90, 67, 13,-46,-85,
   83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83, 83, 36,-36,-83,-83,-36, 36, 83,
   82, 22,-54,-90,-61, 13, 78, 85, 31,-46,-90,-67,  4, 73, 88, 38,-38,-88,-73, -4, 67, 90, 46,-31,-85,-78,-13, 61, 90, 54,-22,-82,
   80,  9,-70,-87,-25, 57, 90, 43,-43,-90,-57, 25, 87, 70, -9,-80,-80, -9, 70, 87, 25,-57,-90,-43, 43, 90, 57,-25,-87,-70,  9, 80,
   78, -4,-82,-73, 13, 85, 67,-22,-88,-61, 31, 90, 54,-38,-90,-46, 46, 90, 38,-54,-90,-31, 61, 88, 22,-67,-85,-13, 73, 82,  4,-78,
   75,-18,-89,-50, 50, 89, 18,-75,-75, 18, 89, 50,-50,-89,-18, 75, 75,-18,-89,-50, 50, 89, 18,-75,-75, 18, 89, 50,-50,-89,-18, 75,
   73,-31,-90,-22, 78, 67,-38,-90,-13, 82, 61,-46,-88, -4, 85, 54,-54,-85,  4, 88, 46,-61,-82, 13, 90, 38,-67,-78, 22, 90, 31,-73,
   70,-43,-87,  9, 90, 25,-80,-57, 57, 80,-25,-90, -9, 87, 43,-70,-70, 43, 87, -9,-90,-25, 80, 57,-57,-80, 25, 90,  9,-87,-43, 70,
   67,-54,-78, 38, 85,-22,-90,  4, 90, 13,-88,-31, 82, 46,-73,-61, 61, 73,-46,-82, 31, 88,-13,-90, -4, 90, 22,-85,-38, 78, 54,-67,
   64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64, 64,-64,-64, 64,
   61,-73,-46, 82, 31,-88,-13, 90, -4,-90, 22, 85,-38,-78, 54, 67,-67,-54, 78, 38,-85,-22, 90,  4,-90, 13, 88,-31,-82, 46, 73,-61,
   57,-80,-25, 90, -9,-87, 43, 70,-70,-43, 87,  9,-90, 25, 80,-57,-57, 80, 25,-90,  9, 87,-43,-70, 70, 43,-87, -9, 90,-25,-80, 57,
   54,-85, -4, 88,-46,-61, 82, 13,-90, 38, 67,-78,-22, 90,-31,-73, 73, 31,-90, 22, 78,-67,-38, 90,-13,-82, 61, 46,-88,  4, 85,-54,
   50,-89, 18, 75,-75,-18, 89,-50,-50, 89,-18,-75, 75, 18,-89, 50, 50,-89, 18, 75,-75,-18, 89,-50,-50, 89,-18,-75, 75, 18,-89, 50,
   46,-90, 38, 54,-90, 31, 61,-88, 22, 67,-85, 13, 73,-82,  4, 78,-78, -4, 82,-73,-13, 85,-67,-22, 88,-61,-31, 90,-54,-38, 90,-46,
   43,-90, 57, 25,-87, 70,  9,-80, 80, -9,-70, 87,-25,-57, 90,-43,-43, 90,-57,-25, 87,-70, -9, 80,-80,  9, 70,-87, 25, 57,-90, 43,
   38,-88, 73, -4,-67, 90,-46,-31, 85,-78, 13, 61,-90, 54, 22,-82, 82,-22,-54, 90,-61,-13, 78,-85, 31, 46,-90, 67,  4,-73, 88,-38,
   36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36, 36,-83, 83,-36,-36, 83,-83, 36,
   31,-78, 90,-61,  4, 54,-88, 82,-38,-22, 73,-90, 67,-13,-46, 85,-85, 46, 13,-67, 90,-73, 22, 38,-82, 88,-54, -4, 61,-90, 78,-31,
   25,-70, 90,-80, 43,  9,-57, 87,-87, 57, -9,-43, 80,-90, 70,-25,-25, 70,-90, 80,-43, -9, 57,-87, 87,-57,  9, 43,-80, 90,-70, 25,
   22,-61, 85,-90, 73,-38, -4, 46,-78, 90,-82, 54,-13,-31, 67,-88, 88,-67, 31, 13,-54, 82,-90, 78,-46,  4, 38,-73, 90,-85, 61,-22,
   18,-50, 75,-89, 89,-75, 50,-18,-18, 50,-75, 89,-89, 75,-50, 18, 18,-50, 75,-89, 89,-75, 50,-18,-18, 50,-75, 89,-89, 75,-50, 18,
   13,-38, 61,-78, 88,-90, 85,-73, 54,-31,  4, 22,-46, 67,-82, 90,-90, 82,-67, 46,-22, -4, 31,-54, 73,-85, 90,-88, 78,-61, 38,-13,
	9,-25, 43,-57, 70,-80, 87,-90, 90,-87, 80,-70, 57,-43, 25, -9, -9, 25,-43, 57,-70, 80,-87, 90,-90, 87,-80, 70,-57, 43,-25,  9,
	4,-13, 22,-31, 38,-46, 54,-61, 67,-73, 78,-82, 85,-88, 90,-90, 90,-90, 88,-85, 82,-78, 73,-67, 61,-54, 46,-38, 31,-22, 13, -4,
};

/**
*	For quanitzation.
*/
const i16 g_piQuantScales[6] = {
	26214,23302,20560,18396,16384,14564
};

/**
*	For inverse quantization.
*/
const i8 g_piInvQuantScales[6] = {
	40, 45, 51, 57, 64, 72
};

void H265Transform::DST4(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift)
{
	// See 8.6.4.2 in draft
	i32 iRound = 1<<(uiShift-1);
	i32 c0, c1, c2, c3, c4;

	for(u32 i=0;i<4;i++) 
	{
		// Intermediate Variables from 8-276 and 8-277
		c0 = piSrc[i*uiStride+0] + piSrc[i*uiStride+3];
		c1 = piSrc[i*uiStride+1] + piSrc[i*uiStride+3];
		c2 = piSrc[i*uiStride+0] - piSrc[i*uiStride+1];
		c3 = 74* piSrc[i*uiStride+2];
		c4 = (piSrc[i*uiStride+0] + piSrc[i*uiStride+1] - piSrc[i*uiStride+3]);

		piDest[0*uiStride+i] =  ( 29 * c0 + 55 * c1 + c3 + iRound ) >> uiShift;
		piDest[1*uiStride+i] =  ( 74 * c4                + iRound ) >> uiShift;
		piDest[2*uiStride+i] =  ( 29 * c2 + 55 * c0 - c3 + iRound ) >> uiShift;
		piDest[3*uiStride+i] =  ( 55 * c2 - 29 * c1 + c3 + iRound ) >> uiShift;
	}
}

void H265Transform::DCT4(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift)
{
	// See 8.6.4.2 in draft
	i32 iRound = 1<<(uiShift-1);
	i32 E0, E1, O0, O1;

	for(u32 i=0;i<uiTrLines;i++) 
	{
		// Intermediate even and odd variable
		E0 = piSrc[i*uiStride+0] + piSrc[i*uiStride+3];
		O0 = piSrc[i*uiStride+0] - piSrc[i*uiStride+3];
		E1 = piSrc[i*uiStride+1] + piSrc[i*uiStride+2];
		O1 = piSrc[i*uiStride+1] - piSrc[i*uiStride+2];

		piDest[0*uiStride+i] =  ( g_pbT4[0*4+0]*E0 + g_pbT4[0*4+1]*E1 + iRound ) >> uiShift;
		piDest[2*uiStride+i] =  ( g_pbT4[2*4+0]*E0 + g_pbT4[2*4+1]*E1 + iRound ) >> uiShift;
		piDest[1*uiStride+i] =  ( g_pbT4[1*4+0]*O0 + g_pbT4[1*4+1]*O1 + iRound ) >> uiShift;
		piDest[3*uiStride+i] =  ( g_pbT4[3*4+0]*O0 + g_pbT4[3*4+1]*O1 + iRound ) >> uiShift;
	}
}

void H265Transform::DCT8(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift)
{
	i32 iRound = 1<<(uiShift-1);
	i32 E[4], O[4];
	i32 EE[2], EO[2];
	for(u32 i=0; i<uiTrLines; i++) 
	{
		/* E and O */
		E[0] = piSrc[i*uiStride+0] + piSrc[i*uiStride+7];
		O[0] = piSrc[i*uiStride+0] - piSrc[i*uiStride+7];
		E[1] = piSrc[i*uiStride+1] + piSrc[i*uiStride+6];
		O[1] = piSrc[i*uiStride+1] - piSrc[i*uiStride+6];
		E[2] = piSrc[i*uiStride+2] + piSrc[i*uiStride+5];
		O[2] = piSrc[i*uiStride+2] - piSrc[i*uiStride+5];
		E[3] = piSrc[i*uiStride+3] + piSrc[i*uiStride+4];
		O[3] = piSrc[i*uiStride+3] - piSrc[i*uiStride+4];

		/* EE and EO */
		EE[0] = E[0] + E[3];
		EO[0] = E[0] - E[3];
		EE[1] = E[1] + E[2];
		EO[1] = E[1] - E[2];

		piDest[0*uiStride+i] = (g_pbT8[0*8+0]*EE[0] + g_pbT8[0*8+1]*EE[1] + iRound) >> uiShift;
		piDest[4*uiStride+i] = (g_pbT8[4*8+0]*EE[0] + g_pbT8[4*8+1]*EE[1] + iRound) >> uiShift;
		piDest[2*uiStride+i] = (g_pbT8[2*8+0]*EO[0] + g_pbT8[2*8+1]*EO[1] + iRound) >> uiShift;
		piDest[6*uiStride+i] = (g_pbT8[6*8+0]*EO[0] + g_pbT8[6*8+1]*EO[1] + iRound) >> uiShift;

		piDest[1*uiStride+i] = (g_pbT8[1*8+0]*O[0] + g_pbT8[1*8+1]*O[1] + g_pbT8[1*8+2]*O[2] + g_pbT8[1*8+3]*O[3] + iRound) >> uiShift;
		piDest[3*uiStride+i] = (g_pbT8[3*8+0]*O[0] + g_pbT8[3*8+1]*O[1] + g_pbT8[3*8+2]*O[2] + g_pbT8[3*8+3]*O[3] + iRound) >> uiShift;
		piDest[5*uiStride+i] = (g_pbT8[5*8+0]*O[0] + g_pbT8[5*8+1]*O[1] + g_pbT8[5*8+2]*O[2] + g_pbT8[5*8+3]*O[3] + iRound) >> uiShift;
		piDest[7*uiStride+i] = (g_pbT8[7*8+0]*O[0] + g_pbT8[7*8+1]*O[1] + g_pbT8[7*8+2]*O[2] + g_pbT8[7*8+3]*O[3] + iRound) >> uiShift;
	}
}

void H265Transform::DCT16(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift)
{

	i32 iRound = 1<<(uiShift-1);
	i32 E[8],O[8];
	i32 EE[4],EO[4];
	i32 EEE[2],EEO[2];

	for(u32 i=0;i<uiTrLines;i++) 
	{
		/* Even and Odd */
		E[0] = piSrc[i*uiStride+0] + piSrc[i*uiStride+15];
		O[0] = piSrc[i*uiStride+0] - piSrc[i*uiStride+15];
		E[1] = piSrc[i*uiStride+1] + piSrc[i*uiStride+14];
		O[1] = piSrc[i*uiStride+1] - piSrc[i*uiStride+14];
		E[2] = piSrc[i*uiStride+2] + piSrc[i*uiStride+13];
		O[2] = piSrc[i*uiStride+2] - piSrc[i*uiStride+13];
		E[3] = piSrc[i*uiStride+3] + piSrc[i*uiStride+12];
		O[3] = piSrc[i*uiStride+3] - piSrc[i*uiStride+12];
		E[4] = piSrc[i*uiStride+4] + piSrc[i*uiStride+11];
		O[4] = piSrc[i*uiStride+4] - piSrc[i*uiStride+11];
		E[5] = piSrc[i*uiStride+5] + piSrc[i*uiStride+10];
		O[5] = piSrc[i*uiStride+5] - piSrc[i*uiStride+10];
		E[6] = piSrc[i*uiStride+6] + piSrc[i*uiStride+ 9];
		O[6] = piSrc[i*uiStride+6] - piSrc[i*uiStride+ 9];
		E[7] = piSrc[i*uiStride+7] + piSrc[i*uiStride+ 8];
		O[7] = piSrc[i*uiStride+7] - piSrc[i*uiStride+ 8];

		/* EE and EO */
		EE[0] = E[0] + E[7];
		EO[0] = E[0] - E[7];
		EE[1] = E[1] + E[6];
		EO[1] = E[1] - E[6];
		EE[2] = E[2] + E[5];
		EO[2] = E[2] - E[5];
		EE[3] = E[3] + E[4];
		EO[3] = E[3] - E[4];

		/* EEE and EEO */
		EEE[0] = EE[0] + EE[3];
		EEO[0] = EE[0] - EE[3];
		EEE[1] = EE[1] + EE[2];
		EEO[1] = EE[1] - EE[2];

		piDest[ 0*uiStride+i] = (g_pbT16[ 0*16+0]*EEE[0] + g_pbT16[ 0*16+1]*EEE[1] + iRound) >> uiShift;
		piDest[ 8*uiStride+i] = (g_pbT16[ 8*16+0]*EEE[0] + g_pbT16[ 8*16+1]*EEE[1] + iRound) >> uiShift;
		piDest[ 4*uiStride+i] = (g_pbT16[ 4*16+0]*EEO[0] + g_pbT16[ 4*16+1]*EEO[1] + iRound) >> uiShift;
		piDest[12*uiStride+i] = (g_pbT16[12*16+0]*EEO[0] + g_pbT16[12*16+1]*EEO[1] + iRound) >> uiShift;

		piDest[ 2*uiStride+i] = (g_pbT16[ 2*16+0]*EO[0] + g_pbT16[ 2*16+1]*EO[1] + g_pbT16[ 2*16+2]*EO[2] + g_pbT16[ 2*16+3]*EO[3] + iRound) >> uiShift;
		piDest[ 6*uiStride+i] = (g_pbT16[ 6*16+0]*EO[0] + g_pbT16[ 6*16+1]*EO[1] + g_pbT16[ 6*16+2]*EO[2] + g_pbT16[ 6*16+3]*EO[3] + iRound) >> uiShift;
		piDest[10*uiStride+i] = (g_pbT16[10*16+0]*EO[0] + g_pbT16[10*16+1]*EO[1] + g_pbT16[10*16+2]*EO[2] + g_pbT16[10*16+3]*EO[3] + iRound) >> uiShift;
		piDest[14*uiStride+i] = (g_pbT16[14*16+0]*EO[0] + g_pbT16[14*16+1]*EO[1] + g_pbT16[14*16+2]*EO[2] + g_pbT16[14*16+3]*EO[3] + iRound) >> uiShift;

		piDest[ 1*uiStride+i] = (g_pbT16[ 1*16+0]*O[0] + g_pbT16[ 1*16+1]*O[1] + g_pbT16[ 1*16+2]*O[2] + g_pbT16[ 1*16+3]*O[3] +
							  g_pbT16[ 1*16+4]*O[4] + g_pbT16[ 1*16+5]*O[5] + g_pbT16[ 1*16+6]*O[6] + g_pbT16[ 1*16+7]*O[7] + iRound) >> uiShift;
		piDest[ 3*uiStride+i] = (g_pbT16[ 3*16+0]*O[0] + g_pbT16[ 3*16+1]*O[1] + g_pbT16[ 3*16+2]*O[2] + g_pbT16[ 3*16+3]*O[3] +
							  g_pbT16[ 3*16+4]*O[4] + g_pbT16[ 3*16+5]*O[5] + g_pbT16[ 3*16+6]*O[6] + g_pbT16[ 3*16+7]*O[7] + iRound) >> uiShift;
		piDest[ 5*uiStride+i] = (g_pbT16[ 5*16+0]*O[0] + g_pbT16[ 5*16+1]*O[1] + g_pbT16[ 5*16+2]*O[2] + g_pbT16[ 5*16+3]*O[3] +
							  g_pbT16[ 5*16+4]*O[4] + g_pbT16[ 5*16+5]*O[5] + g_pbT16[ 5*16+6]*O[6] + g_pbT16[ 5*16+7]*O[7] + iRound) >> uiShift;
		piDest[ 7*uiStride+i] = (g_pbT16[ 7*16+0]*O[0] + g_pbT16[ 7*16+1]*O[1] + g_pbT16[ 7*16+2]*O[2] + g_pbT16[ 7*16+3]*O[3] +
							  g_pbT16[ 7*16+4]*O[4] + g_pbT16[ 7*16+5]*O[5] + g_pbT16[ 7*16+6]*O[6] + g_pbT16[ 7*16+7]*O[7] + iRound) >> uiShift;
		piDest[ 9*uiStride+i] = (g_pbT16[ 9*16+0]*O[0] + g_pbT16[ 9*16+1]*O[1] + g_pbT16[ 9*16+2]*O[2] + g_pbT16[ 9*16+3]*O[3] +
							  g_pbT16[ 9*16+4]*O[4] + g_pbT16[ 9*16+5]*O[5] + g_pbT16[ 9*16+6]*O[6] + g_pbT16[ 9*16+7]*O[7] + iRound) >> uiShift;
		piDest[11*uiStride+i] = (g_pbT16[11*16+0]*O[0] + g_pbT16[11*16+1]*O[1] + g_pbT16[11*16+2]*O[2] + g_pbT16[11*16+3]*O[3] +
							  g_pbT16[11*16+4]*O[4] + g_pbT16[11*16+5]*O[5] + g_pbT16[11*16+6]*O[6] + g_pbT16[11*16+7]*O[7] + iRound) >> uiShift;
		piDest[13*uiStride+i] = (g_pbT16[13*16+0]*O[0] + g_pbT16[13*16+1]*O[1] + g_pbT16[13*16+2]*O[2] + g_pbT16[13*16+3]*O[3] +
							  g_pbT16[13*16+4]*O[4] + g_pbT16[13*16+5]*O[5] + g_pbT16[13*16+6]*O[6] + g_pbT16[13*16+7]*O[7] + iRound) >> uiShift;
		piDest[15*uiStride+i] = (g_pbT16[15*16+0]*O[0] + g_pbT16[15*16+1]*O[1] + g_pbT16[15*16+2]*O[2] + g_pbT16[15*16+3]*O[3] +
							  g_pbT16[15*16+4]*O[4] + g_pbT16[15*16+5]*O[5] + g_pbT16[15*16+6]*O[6] + g_pbT16[15*16+7]*O[7] + iRound) >> uiShift;
	}

}

void H265Transform::DCT32(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift)
{
	i32 E[16],O[16];
	i32 EE[8],EO[8];
	i32 EEE[4],EEO[4];
	i32 EEEE[2],EEEO[2];
	i32 iRound = 1<<(uiShift-1);

	for(u32 i=0;i<uiTrLines;i++) 
	{
		/* E and O */
		for(u32 k=0; k<16; k++ ) 
		{
			E[k] = piSrc[i*uiStride+k] + piSrc[i*uiStride+31-k];
			O[k] = piSrc[i*uiStride+k] - piSrc[i*uiStride+31-k];
		}
		/* EE and EO */
		for(u32 k=0; k<8; k++ ) 
		{
			EE[k] = E[k] + E[15-k];
			EO[k] = E[k] - E[15-k];
		}
		/* EEE and EEO */
		for(u32 k=0; k<4; k++ ) 
		{
			EEE[k] = EE[k] + EE[7-k];
			EEO[k] = EE[k] - EE[7-k];
		}
		/* EEEE and EEEO */
		EEEE[0] = EEE[0] + EEE[3];
		EEEO[0] = EEE[0] - EEE[3];
		EEEE[1] = EEE[1] + EEE[2];
		EEEO[1] = EEE[1] - EEE[2];

		// 0, 8, 16, 24
		piDest[ 0*uiStride+i] = (g_pbT32[ 0*32+0]*EEEE[0] + g_pbT32[ 0*32+1]*EEEE[1] + iRound) >> uiShift;
		piDest[16*uiStride+i] = (g_pbT32[16*32+0]*EEEE[0] + g_pbT32[16*32+1]*EEEE[1] + iRound) >> uiShift;
		piDest[ 8*uiStride+i] = (g_pbT32[ 8*32+0]*EEEO[0] + g_pbT32[ 8*32+1]*EEEO[1] + iRound) >> uiShift;
		piDest[24*uiStride+i] = (g_pbT32[24*32+0]*EEEO[0] + g_pbT32[24*32+1]*EEEO[1] + iRound) >> uiShift;

		// 4, 12, 20, 28
		for(u32 k=4; k<32; k+=8 ) 
			piDest[k*uiStride+i] = (g_pbT32[k*32+0]*EEO[0] + g_pbT32[k*32+1]*EEO[1] + g_pbT32[k*32+2]*EEO[2] + g_pbT32[k*32+3]*EEO[3] + iRound) >> uiShift;

		// 2, 6, 10, 14, 18, 22, 26, 30
		for(u32 k=2; k<32; k+=4 ) 
			piDest[k*uiStride+i] = (g_pbT32[k*32+0]*EO[0] + g_pbT32[k*32+1]*EO[1] + g_pbT32[k*32+2]*EO[2] + g_pbT32[k*32+3]*EO[3] +
								 g_pbT32[k*32+4]*EO[4] + g_pbT32[k*32+5]*EO[5] + g_pbT32[k*32+6]*EO[6] + g_pbT32[k*32+7]*EO[7] + iRound) >> uiShift;

		// 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31
		for(u32 k=1; k<32; k+=2 ) 
			piDest[k*uiStride+i]=(g_pbT32[k*32+ 0]*O[ 0] + g_pbT32[k*32+ 1]*O[ 1] + g_pbT32[k*32+ 2]*O[ 2] + g_pbT32[k*32+ 3]*O[ 3] +
								 g_pbT32[k*32+ 4]*O[ 4] + g_pbT32[k*32+ 5]*O[ 5] + g_pbT32[k*32+ 6]*O[ 6] + g_pbT32[k*32+ 7]*O[ 7] +
								 g_pbT32[k*32+ 8]*O[ 8] + g_pbT32[k*32+ 9]*O[ 9] + g_pbT32[k*32+10]*O[10] + g_pbT32[k*32+11]*O[11] +
								 g_pbT32[k*32+12]*O[12] + g_pbT32[k*32+13]*O[13] + g_pbT32[k*32+14]*O[14] + g_pbT32[k*32+15]*O[15] + iRound) >> uiShift;
	}
}

void H265Transform::IDST4(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift)
{
	i32 iRound = 1 << (uiShift-1);
	i32 c0, c1, c2, c3, c4;

	for(u32 i=0; i<4; i++ ) 
	{
		// Intermediate Variables
		c0 = piSrc[0*uiStride+i] + piSrc[2*uiStride+i];
		c1 = piSrc[2*uiStride+i] + piSrc[3*uiStride+i];
		c2 = piSrc[0*uiStride+i] - piSrc[3*uiStride+i];
		c3 = 74* piSrc[1*uiStride+i];
		c4 = piSrc[0*uiStride+i] - piSrc[2*uiStride+i] + piSrc[3*uiStride+i];

		piDest[i*uiStride+0] = ( 29 * c0 + 55 * c1 + c3 + iRound ) >> uiShift;
		piDest[i*uiStride+1] = ( 55 * c2 - 29 * c1 + c3 + iRound ) >> uiShift;
		piDest[i*uiStride+2] = ( 74 * c4                + iRound ) >> uiShift;
		piDest[i*uiStride+3] = ( 55 * c0 + 29 * c2 - c3 + iRound ) >> uiShift;

		piDest[i*uiStride+0] = Clip3( -32768, 32767, piDest[i*uiStride+0] );
		piDest[i*uiStride+1] = Clip3( -32768, 32767, piDest[i*uiStride+1] );
		piDest[i*uiStride+2] = Clip3( -32768, 32767, piDest[i*uiStride+2] );
		piDest[i*uiStride+3] = Clip3( -32768, 32767, piDest[i*uiStride+3] );
  }
}

void H265Transform::IDCT4(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift)
{
	i32 iRound = 1<<(uiShift-1);
	i32 O0, O1, E0, E1;

	for(u32 i=0; i<uiTrLines; i++) 
	{
		/* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
		O0 = g_pbT4[1*4+0]*piSrc[1*uiStride+i] + g_pbT4[3*4+0]*piSrc[3*uiStride+i];
		O1 = g_pbT4[1*4+1]*piSrc[1*uiStride+i] + g_pbT4[3*4+1]*piSrc[3*uiStride+i];
		E0 = g_pbT4[0*4+0]*piSrc[0*uiStride+i] + g_pbT4[2*4+0]*piSrc[2*uiStride+i];
		E1 = g_pbT4[0*4+1]*piSrc[0*uiStride+i] + g_pbT4[2*4+1]*piSrc[2*uiStride+i];

		/* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
		piDest[i*uiStride+0] = (E0 + O0 + iRound) >> uiShift;
		piDest[i*uiStride+1] = (E1 + O1 + iRound) >> uiShift;
		piDest[i*uiStride+2] = (E1 - O1 + iRound) >> uiShift;
		piDest[i*uiStride+3] = (E0 - O0 + iRound) >> uiShift;

		piDest[i*uiStride+0] = Clip3( -32768, 32767, piDest[i*uiStride+0] );
		piDest[i*uiStride+1] = Clip3( -32768, 32767, piDest[i*uiStride+1] );
		piDest[i*uiStride+2] = Clip3( -32768, 32767, piDest[i*uiStride+2] );
		piDest[i*uiStride+3] = Clip3( -32768, 32767, piDest[i*uiStride+3] );
	}
}

void H265Transform::IDCT8(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift)
{
	i32 iRound = 1<<(uiShift-1);
	i32 O[4], E[4];
	i32 EO[2], EE[2];

	for(u32 i=0; i<uiTrLines; i++) 
	{
		/* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
		O[0] = g_pbT8[1*8+0]*piSrc[1*uiStride+i] + g_pbT8[3*8+0]*piSrc[3*uiStride+i] + g_pbT8[5*8+0]*piSrc[5*uiStride+i] + g_pbT8[7*8+0]*piSrc[7*uiStride+i];
		O[1] = g_pbT8[1*8+1]*piSrc[1*uiStride+i] + g_pbT8[3*8+1]*piSrc[3*uiStride+i] + g_pbT8[5*8+1]*piSrc[5*uiStride+i] + g_pbT8[7*8+1]*piSrc[7*uiStride+i];
		O[2] = g_pbT8[1*8+2]*piSrc[1*uiStride+i] + g_pbT8[3*8+2]*piSrc[3*uiStride+i] + g_pbT8[5*8+2]*piSrc[5*uiStride+i] + g_pbT8[7*8+2]*piSrc[7*uiStride+i];
		O[3] = g_pbT8[1*8+3]*piSrc[1*uiStride+i] + g_pbT8[3*8+3]*piSrc[3*uiStride+i] + g_pbT8[5*8+3]*piSrc[5*uiStride+i] + g_pbT8[7*8+3]*piSrc[7*uiStride+i];

		EO[0] = g_pbT8[2*8+0]*piSrc[2*uiStride+i] + g_pbT8[6*8+0]*piSrc[6*uiStride+i];
		EO[1] = g_pbT8[2*8+1]*piSrc[2*uiStride+i] + g_pbT8[6*8+1]*piSrc[6*uiStride+i];
		EE[0] = g_pbT8[0*8+0]*piSrc[0*uiStride+i] + g_pbT8[4*8+0]*piSrc[4*uiStride+i];
		EE[1] = g_pbT8[0*8+1]*piSrc[0*uiStride+i] + g_pbT8[4*8+1]*piSrc[4*uiStride+i];

		/* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
		E[0] = EE[0] + EO[0];
		E[3] = EE[0] - EO[0];
		E[1] = EE[1] + EO[1];
		E[2] = EE[1] - EO[1];

		piDest[i*uiStride+0] = (E[0] + O[0] + iRound) >> uiShift;
		piDest[i*uiStride+1] = (E[1] + O[1] + iRound) >> uiShift;
		piDest[i*uiStride+2] = (E[2] + O[2] + iRound) >> uiShift;
		piDest[i*uiStride+3] = (E[3] + O[3] + iRound) >> uiShift;
		piDest[i*uiStride+4] = (E[3] - O[3] + iRound) >> uiShift;
		piDest[i*uiStride+5] = (E[2] - O[2] + iRound) >> uiShift;
		piDest[i*uiStride+6] = (E[1] - O[1] + iRound) >> uiShift;
		piDest[i*uiStride+7] = (E[0] - O[0] + iRound) >> uiShift;
		piDest[i*uiStride+0] = Clip3( -32768, 32767, piDest[i*uiStride+0] );
		piDest[i*uiStride+1] = Clip3( -32768, 32767, piDest[i*uiStride+1] );
		piDest[i*uiStride+2] = Clip3( -32768, 32767, piDest[i*uiStride+2] );
		piDest[i*uiStride+3] = Clip3( -32768, 32767, piDest[i*uiStride+3] );
		piDest[i*uiStride+4] = Clip3( -32768, 32767, piDest[i*uiStride+4] );
		piDest[i*uiStride+5] = Clip3( -32768, 32767, piDest[i*uiStride+5] );
		piDest[i*uiStride+6] = Clip3( -32768, 32767, piDest[i*uiStride+6] );
		piDest[i*uiStride+7] = Clip3( -32768, 32767, piDest[i*uiStride+7] );
	}
}

void H265Transform::IDCT16(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift)
{
	i32 rnd = 1<<(uiShift-1);
	i32 O[8], E[8];
	i32 EO[4], EE[4];
	i32 EEO[2], EEE[2];

	for(u32 i=0; i<uiTrLines; i++) 
	{
		/* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
		O[0] =   g_pbT16[ 1*16+0]*piSrc[ 1*uiStride+i] + g_pbT16[ 3*16+0]*piSrc[ 3*uiStride+i] + g_pbT16[ 5*16+0]*piSrc[ 5*uiStride+i] + g_pbT16[ 7*16+0]*piSrc[ 7*uiStride+i]
		   + g_pbT16[ 9*16+0]*piSrc[ 9*uiStride+i] + g_pbT16[11*16+0]*piSrc[11*uiStride+i] + g_pbT16[13*16+0]*piSrc[13*uiStride+i] + g_pbT16[15*16+0]*piSrc[15*uiStride+i];
		O[1] =   g_pbT16[ 1*16+1]*piSrc[ 1*uiStride+i] + g_pbT16[ 3*16+1]*piSrc[ 3*uiStride+i] + g_pbT16[ 5*16+1]*piSrc[ 5*uiStride+i] + g_pbT16[ 7*16+1]*piSrc[ 7*uiStride+i]
		   + g_pbT16[ 9*16+1]*piSrc[ 9*uiStride+i] + g_pbT16[11*16+1]*piSrc[11*uiStride+i] + g_pbT16[13*16+1]*piSrc[13*uiStride+i] + g_pbT16[15*16+1]*piSrc[15*uiStride+i];
		O[2] =   g_pbT16[ 1*16+2]*piSrc[ 1*uiStride+i] + g_pbT16[ 3*16+2]*piSrc[ 3*uiStride+i] + g_pbT16[ 5*16+2]*piSrc[ 5*uiStride+i] + g_pbT16[ 7*16+2]*piSrc[ 7*uiStride+i]
		   + g_pbT16[ 9*16+2]*piSrc[ 9*uiStride+i] + g_pbT16[11*16+2]*piSrc[11*uiStride+i] + g_pbT16[13*16+2]*piSrc[13*uiStride+i] + g_pbT16[15*16+2]*piSrc[15*uiStride+i];
		O[3] =   g_pbT16[ 1*16+3]*piSrc[ 1*uiStride+i] + g_pbT16[ 3*16+3]*piSrc[ 3*uiStride+i] + g_pbT16[ 5*16+3]*piSrc[ 5*uiStride+i] + g_pbT16[ 7*16+3]*piSrc[ 7*uiStride+i]
		  + g_pbT16[ 9*16+3]*piSrc[ 9*uiStride+i] + g_pbT16[11*16+3]*piSrc[11*uiStride+i] + g_pbT16[13*16+3]*piSrc[13*uiStride+i] + g_pbT16[15*16+3]*piSrc[15*uiStride+i];
		O[4] =   g_pbT16[ 1*16+4]*piSrc[ 1*uiStride+i] + g_pbT16[ 3*16+4]*piSrc[ 3*uiStride+i] + g_pbT16[ 5*16+4]*piSrc[ 5*uiStride+i] + g_pbT16[ 7*16+4]*piSrc[ 7*uiStride+i]
		  + g_pbT16[ 9*16+4]*piSrc[ 9*uiStride+i] + g_pbT16[11*16+4]*piSrc[11*uiStride+i] + g_pbT16[13*16+4]*piSrc[13*uiStride+i] + g_pbT16[15*16+4]*piSrc[15*uiStride+i];
		O[5] =   g_pbT16[ 1*16+5]*piSrc[ 1*uiStride+i] + g_pbT16[ 3*16+5]*piSrc[ 3*uiStride+i] + g_pbT16[ 5*16+5]*piSrc[ 5*uiStride+i] + g_pbT16[ 7*16+5]*piSrc[ 7*uiStride+i]
		  + g_pbT16[ 9*16+5]*piSrc[ 9*uiStride+i] + g_pbT16[11*16+5]*piSrc[11*uiStride+i] + g_pbT16[13*16+5]*piSrc[13*uiStride+i] + g_pbT16[15*16+5]*piSrc[15*uiStride+i];
		O[6] =   g_pbT16[ 1*16+6]*piSrc[ 1*uiStride+i] + g_pbT16[ 3*16+6]*piSrc[ 3*uiStride+i] + g_pbT16[ 5*16+6]*piSrc[ 5*uiStride+i] + g_pbT16[ 7*16+6]*piSrc[ 7*uiStride+i]
		  + g_pbT16[ 9*16+6]*piSrc[ 9*uiStride+i] + g_pbT16[11*16+6]*piSrc[11*uiStride+i] + g_pbT16[13*16+6]*piSrc[13*uiStride+i] + g_pbT16[15*16+6]*piSrc[15*uiStride+i];
		O[7] =   g_pbT16[ 1*16+7]*piSrc[ 1*uiStride+i] + g_pbT16[ 3*16+7]*piSrc[ 3*uiStride+i] + g_pbT16[ 5*16+7]*piSrc[ 5*uiStride+i] + g_pbT16[ 7*16+7]*piSrc[ 7*uiStride+i]
		   + g_pbT16[ 9*16+7]*piSrc[ 9*uiStride+i] + g_pbT16[11*16+7]*piSrc[11*uiStride+i] + g_pbT16[13*16+7]*piSrc[13*uiStride+i] + g_pbT16[15*16+7]*piSrc[15*uiStride+i];

		EO[0] = g_pbT16[ 2*16+0]*piSrc[ 2*uiStride+i] + g_pbT16[ 6*16+0]*piSrc[ 6*uiStride+i] + g_pbT16[10*16+0]*piSrc[10*uiStride+i] + g_pbT16[14*16+0]*piSrc[14*uiStride+i];
		EO[1] = g_pbT16[ 2*16+1]*piSrc[ 2*uiStride+i] + g_pbT16[ 6*16+1]*piSrc[ 6*uiStride+i] + g_pbT16[10*16+1]*piSrc[10*uiStride+i] + g_pbT16[14*16+1]*piSrc[14*uiStride+i];
		EO[2] = g_pbT16[ 2*16+2]*piSrc[ 2*uiStride+i] + g_pbT16[ 6*16+2]*piSrc[ 6*uiStride+i] + g_pbT16[10*16+2]*piSrc[10*uiStride+i] + g_pbT16[14*16+2]*piSrc[14*uiStride+i];
		EO[3] = g_pbT16[ 2*16+3]*piSrc[ 2*uiStride+i] + g_pbT16[ 6*16+3]*piSrc[ 6*uiStride+i] + g_pbT16[10*16+3]*piSrc[10*uiStride+i] + g_pbT16[14*16+3]*piSrc[14*uiStride+i];

		EEO[0] = g_pbT16[4*16+0]*piSrc[4*uiStride+i] + g_pbT16[12*16+0]*piSrc[12*uiStride+i];
		EEO[1] = g_pbT16[4*16+1]*piSrc[4*uiStride+i] + g_pbT16[12*16+1]*piSrc[12*uiStride+i];
		EEE[0] = g_pbT16[0*16+0]*piSrc[0*uiStride+i] + g_pbT16[ 8*16+0]*piSrc[ 8*uiStride+i];
		EEE[1] = g_pbT16[0*16+1]*piSrc[0*uiStride+i] + g_pbT16[ 8*16+1]*piSrc[ 8*uiStride+i];

		/* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
		EE[0] = EEE[0] + EEO[0];
		EE[3] = EEE[0] - EEO[0];
		EE[1] = EEE[1] + EEO[1];
		EE[2] = EEE[1] - EEO[1];

		E[0] = EE[0] + EO[0];
		E[7] = EE[0] - EO[0];
		E[1] = EE[1] + EO[1];
		E[6] = EE[1] - EO[1];
		E[2] = EE[2] + EO[2];
		E[5] = EE[2] - EO[2];
		E[3] = EE[3] + EO[3];
		E[4] = EE[3] - EO[3];

		piDest[i*uiStride+ 0] = (E[0] + O[0] + rnd) >> uiShift;
		piDest[i*uiStride+15] = (E[0] - O[0] + rnd) >> uiShift;
		piDest[i*uiStride+ 1] = (E[1] + O[1] + rnd) >> uiShift;
		piDest[i*uiStride+14] = (E[1] - O[1] + rnd) >> uiShift;
		piDest[i*uiStride+ 2] = (E[2] + O[2] + rnd) >> uiShift;
		piDest[i*uiStride+13] = (E[2] - O[2] + rnd) >> uiShift;
		piDest[i*uiStride+ 3] = (E[3] + O[3] + rnd) >> uiShift;
		piDest[i*uiStride+12] = (E[3] - O[3] + rnd) >> uiShift;
		piDest[i*uiStride+ 4] = (E[4] + O[4] + rnd) >> uiShift;
		piDest[i*uiStride+11] = (E[4] - O[4] + rnd) >> uiShift;
		piDest[i*uiStride+ 5] = (E[5] + O[5] + rnd) >> uiShift;
		piDest[i*uiStride+10] = (E[5] - O[5] + rnd) >> uiShift;
		piDest[i*uiStride+ 6] = (E[6] + O[6] + rnd) >> uiShift;
		piDest[i*uiStride+ 9] = (E[6] - O[6] + rnd) >> uiShift;
		piDest[i*uiStride+ 7] = (E[7] + O[7] + rnd) >> uiShift;
		piDest[i*uiStride+ 8] = (E[7] - O[7] + rnd) >> uiShift;

		piDest[i*uiStride+ 0] = Clip3( -32768, 32767, piDest[i*uiStride+ 0] );
		piDest[i*uiStride+ 1] = Clip3( -32768, 32767, piDest[i*uiStride+ 1] );
		piDest[i*uiStride+ 2] = Clip3( -32768, 32767, piDest[i*uiStride+ 2] );
		piDest[i*uiStride+ 3] = Clip3( -32768, 32767, piDest[i*uiStride+ 3] );
		piDest[i*uiStride+ 4] = Clip3( -32768, 32767, piDest[i*uiStride+ 4] );
		piDest[i*uiStride+ 5] = Clip3( -32768, 32767, piDest[i*uiStride+ 5] );
		piDest[i*uiStride+ 6] = Clip3( -32768, 32767, piDest[i*uiStride+ 6] );
		piDest[i*uiStride+ 7] = Clip3( -32768, 32767, piDest[i*uiStride+ 7] );
		piDest[i*uiStride+ 8] = Clip3( -32768, 32767, piDest[i*uiStride+ 8] );
		piDest[i*uiStride+ 9] = Clip3( -32768, 32767, piDest[i*uiStride+ 9] );
		piDest[i*uiStride+10] = Clip3( -32768, 32767, piDest[i*uiStride+10] );
		piDest[i*uiStride+11] = Clip3( -32768, 32767, piDest[i*uiStride+11] );
		piDest[i*uiStride+12] = Clip3( -32768, 32767, piDest[i*uiStride+12] );
		piDest[i*uiStride+13] = Clip3( -32768, 32767, piDest[i*uiStride+13] );
		piDest[i*uiStride+14] = Clip3( -32768, 32767, piDest[i*uiStride+14] );
		piDest[i*uiStride+15] = Clip3( -32768, 32767, piDest[i*uiStride+15] );
	}
}

void H265Transform::IDCT32(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift)
{
	i32 E[16],O[16];
	i32 EE[8],EO[8];
	i32 EEE[4],EEO[4];
	i32 EEEE[2],EEEO[2];
	int rnd = 1<<(uiShift-1);

	for(u32 i=0; i<32; i++ ) 
	{
		/* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
		for(u32 k=0; k<16; k++ ) 
		{
			O[k] =   g_pbT32[ 1*32+k]*piSrc[ 1*uiStride+i] + g_pbT32[ 3*32+k]*piSrc[ 3*uiStride+i]
				   + g_pbT32[ 5*32+k]*piSrc[ 5*uiStride+i] + g_pbT32[ 7*32+k]*piSrc[ 7*uiStride+i]
				   + g_pbT32[ 9*32+k]*piSrc[ 9*uiStride+i] + g_pbT32[11*32+k]*piSrc[11*uiStride+i]
				   + g_pbT32[13*32+k]*piSrc[13*uiStride+i] + g_pbT32[15*32+k]*piSrc[15*uiStride+i]
				   + g_pbT32[17*32+k]*piSrc[17*uiStride+i] + g_pbT32[19*32+k]*piSrc[19*uiStride+i]
				   + g_pbT32[21*32+k]*piSrc[21*uiStride+i] + g_pbT32[23*32+k]*piSrc[23*uiStride+i]
				   + g_pbT32[25*32+k]*piSrc[25*uiStride+i] + g_pbT32[27*32+k]*piSrc[27*uiStride+i]
				   + g_pbT32[29*32+k]*piSrc[29*uiStride+i] + g_pbT32[31*32+k]*piSrc[31*uiStride+i];
		}

		for(u32 k=0; k<8; k++ ) 
		{
			EO[k] =   g_pbT32[ 2*32+k]*piSrc[ 2*uiStride+i] + g_pbT32[ 6*32+k]*piSrc[ 6*uiStride+i]
					+ g_pbT32[10*32+k]*piSrc[10*uiStride+i] + g_pbT32[14*32+k]*piSrc[14*uiStride+i]
					+ g_pbT32[18*32+k]*piSrc[18*uiStride+i] + g_pbT32[22*32+k]*piSrc[22*uiStride+i]
					+ g_pbT32[26*32+k]*piSrc[26*uiStride+i] + g_pbT32[30*32+k]*piSrc[30*uiStride+i];
		}

		for(u32 k=0; k<4; k++ ) 
		{
			EEO[k] =   g_pbT32[ 4*32+k]*piSrc[ 4*uiStride+i] + g_pbT32[12*32+k]*piSrc[12*uiStride+i]
					 + g_pbT32[20*32+k]*piSrc[20*uiStride+i] + g_pbT32[28*32+k]*piSrc[28*uiStride+i];
		}
		EEEO[0] = g_pbT32[8*32+0]*piSrc[8*uiStride+i] + g_pbT32[24*32+0]*piSrc[24*uiStride+i];
		EEEO[1] = g_pbT32[8*32+1]*piSrc[8*uiStride+i] + g_pbT32[24*32+1]*piSrc[24*uiStride+i];
		EEEE[0] = g_pbT32[0*32+0]*piSrc[0*uiStride+i] + g_pbT32[16*32+0]*piSrc[16*uiStride+i];
		EEEE[1] = g_pbT32[0*32+1]*piSrc[0*uiStride+i] + g_pbT32[16*32+1]*piSrc[16*uiStride+i];

		/* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
		EEE[0] = EEEE[0] + EEEO[0];
		EEE[3] = EEEE[0] - EEEO[0];
		EEE[1] = EEEE[1] + EEEO[1];
		EEE[2] = EEEE[1] - EEEO[1];

		EE[0] = EEE[0] + EEO[0];
		EE[7] = EEE[0] - EEO[0];
		EE[1] = EEE[1] + EEO[1];
		EE[6] = EEE[1] - EEO[1];
		EE[5] = EEE[2] - EEO[2];
		EE[2] = EEE[2] + EEO[2];
		EE[3] = EEE[3] + EEO[3];
		EE[4] = EEE[3] - EEO[3];

		for(u32 k=0; k<8; k++ ) 
		{
			E[k  ] = EE[k  ] + EO[k  ];
			E[k+8] = EE[7-k] - EO[7-k];
		}

		for(u32 k=0; k<16; k++ ) 
		{
			piDest[i*uiStride+k   ] = (E[k   ] + O[k   ] + rnd) >> uiShift;
			piDest[i*uiStride+k+16] = (E[15-k] - O[15-k] + rnd) >> uiShift;
			piDest[i*uiStride+k   ] = Clip3( -32768, 32767, piDest[i*uiStride+k   ]);
			piDest[i*uiStride+k+16] = Clip3( -32768, 32767, piDest[i*uiStride+k+16]);
		}
	}
}

H265Transform::H265Transform()
{
	// For function pointers, see http://www.codeproject.com/Articles/7150/Member-Function-Pointers-and-the-Fastest-Possible
	DCTN[0] = &H265Transform::DST4;
	DCTN[1] = &H265Transform::DCT4;
	DCTN[2] = &H265Transform::DCT8;
	DCTN[3] = &H265Transform::DCT16;
	DCTN[4] = &H265Transform::DCT32;

	IDCTN[0] = &H265Transform::IDST4;
	IDCTN[1] = &H265Transform::IDCT4;
	IDCTN[2] = &H265Transform::IDCT8;
	IDCTN[3] = &H265Transform::IDCT16;
	IDCTN[4] = &H265Transform::IDCT32;
}

void H265Transform::ResDCT(i16 *piOutput, u32 uiWidth, u32 uiHeight, byte *pbSrc, u32 uiSrcStride, byte *pbRef, u32 uiRefStride, i16 *piRes, i16 *piResHorTrans, u32 uiMode)
{
	// TU size should not be more than 32x32
	MAKE_SURE((uiWidth <= 32) && (uiHeight <= 32),"TU size must not exceed 32");

	// See 8.6.4 and 8.6.4.1 in the draft
	u32 uiLog2Width = LOG2(uiWidth-1);
	u32 uiLog2Height = LOG2(uiHeight-1);

	bit bUseDST = IS_INTRA(uiMode) && (uiWidth + uiHeight == 8);		// Use DST 4x4
	
	// Compute residue
	for(u32 i=0;i<uiHeight;i++)
		for(u32 j=0;j<uiWidth;j++)
			piRes[i*uiWidth+j] = pbSrc[i*uiSrcStride+j] - pbRef[i*uiRefStride+j];

	// Generate transform
	// For the first shift, the bitDepth is 8, therefore, uiLog2Width - 1 + bitDepth - 8 is replaced by uiLog2Width-1
	(this->*DCTN[uiLog2Width - 1 - bUseDST])(piResHorTrans,piRes,uiWidth,uiHeight,uiLog2Width-1);		
	(this->*DCTN[uiLog2Height- 1 - bUseDST])(piOutput,piResHorTrans,uiWidth,uiWidth,uiLog2Height+6);
}

u32 H265Transform::Quant(i16 *piOutput, u32 uiOutputStride, u32 uiQP, u32 uiWidth, u32 uiHeight, i16 *piSrc, u32 uiSrcStride, eSliceType eST)
{
	u32 uiQuantSum = 0;	// Denotes if there is a non-zero value and an inverse transform loop is required
	const u32 uiQPDiv6 = uiQP / 6;
	const u32 uiQPMod6 = uiQP % 6;

	u32 uiLog2TrSize = LOG2(uiWidth-1);
	u32 uiQ = g_piQuantScales[uiQPMod6];
	i32 iTransShift = MAX_TR_DYN_RANGE - 8 - uiLog2TrSize;
	i32 iQBits = QUANT_SHIFT + uiQPDiv6 + iTransShift;
	i32 iRound = (eST == I_SLICE ? 171 : 85) << (iQBits - 9);

	i32 iLevel;
	i32 iSign;

	for(u32 i=0;i<uiHeight;i++)
	{
		for(u32 j=0;j<uiWidth;j++)
		{
			iLevel = piSrc[i*uiSrcStride+j];
			iSign = (iLevel < 0 ? -1 : 1);

			iLevel = (ABS(iLevel)*uiQ + iRound) >> iQBits;
			uiQuantSum += iLevel;
			iLevel *= iSign;
			piOutput[i*uiOutputStride+j] = Clip3(-32768,32767,iLevel);
		}
	}
	return uiQuantSum;
}

void H265Transform::InvQuant(i16 *piOutput, u32 uiOutputStride, u32 uiQP, u32 uiWidth, u32 uiHeight, i16 *piSrc, u32 uiSrcStride, eSliceType eST)
{
	const u32 uiQPDiv6 = uiQP / 6;
	const u32 uiQPMod6 = uiQP % 6;

	u32 uiLog2TrSize = LOG2(uiWidth-1);
	i32 iTransShift = MAX_TR_DYN_RANGE - 8 - uiLog2TrSize;
	i32 iShift = IQUANT_SHIFT - QUANT_SHIFT - iTransShift;
	i32 iRound = 1<<(iShift-1);
	i32 iScale = g_piInvQuantScales[uiQPMod6] << uiQPDiv6;
	i32 iIQCoeff;

	for(u32 i=0;i<uiHeight;i++)
	{
		for(u32 j=0;j<uiWidth;j++)
		{
			iIQCoeff = (piSrc[i*uiSrcStride+j]*iScale + iRound) >> iShift;
			piOutput[i*uiOutputStride+j] = Clip3(-32768, 32767, iIQCoeff);
		}
	}
}

void H265Transform::IDCTRec(byte *pbOutput, u32 uiOutputStride, u32 uiWidth, u32 uiHeight, i16 *piSrc, u32 uiSrcStride, 
							byte *pbRef, u32 uiRefStride, i16 *piButterflyOut1, i16 *piButterflyOut2, u32 uiMode)
{
	// TU size should not be more than 32x32
	MAKE_SURE((uiWidth <= 32) && (uiHeight <= 32),"TU size must not exceed 32");

	// See 8.6.4 and 8.6.4.1 in the draft
	u32 uiLog2Width = LOG2(uiWidth-1);
	u32 uiLog2Height = LOG2(uiHeight-1);

	bit bUseDST = IS_INTRA(uiMode) && (uiWidth + uiHeight == 8);		// Use DST 4x4

	// IDCT
	(this->*IDCTN[uiLog2Width - 1 - bUseDST])(piButterflyOut1,piSrc,uiSrcStride,uiHeight,SHIFT_INV_1);
	(this->*IDCTN[uiLog2Height- 1 - bUseDST])(piButterflyOut2,piButterflyOut1,uiSrcStride,uiWidth,SHIFT_INV_2);

	// Reconstruct
	for(u32 i=0;i<uiHeight;i++)
		for(u32 j=0;j<uiWidth;j++)
			pbOutput[i*uiOutputStride+j] = byte(Clip1(piButterflyOut2[i*uiSrcStride+j]+pbRef[i*uiRefStride+j]));
}
