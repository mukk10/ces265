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
* @file TypeDefs.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the m_iType definitions and macros used in this project.
* Help for some types is taken from http://msdn.microsoft.com/en-us/library/s3f49ktz(v=vs.80).aspx
*/


#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__

#include <Defines.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#ifdef _MSC_VER
#include <intrin.h>	// @todo For linux, I think I can use x86intrin.h
#endif

#define			I32_MIN						-(1<<31)				//!<	Min 32-bit signed
#define			I32_MAX						((1<<30)-1)				//!<	Max 32-bit signed
#define			U32_MAX						((1<<30)-1)				//!<	Max 32-bit unsigned
#define			I8_MIN						-(1<<7)					//!<	Min 8-bit signed
#define			I8_MAX						((1<<7)-1)				//!<	Max 8-bit signed
#define			U8_MAX						((1<<8)-1)				//!<	Max 8-bit unsigned
#define			I16_MIN						-(1<<15)				//!<	Min 16-bit signed
#define			I16_MAX						((1<<15)-1)				//!<	Max 16-bit signed
#define			U16_MAX						((1<<16)-1)				//!<	Max 16-bit unsigned

typedef			signed int					i32;
typedef			unsigned int				u32;
typedef			char						i8;
typedef			unsigned char				u8;
typedef			unsigned char				byte;
typedef			signed short				i16;
typedef			unsigned short				u16;
typedef			bool						bit;
typedef			float						f32;
typedef			double						f64;

#if defined WIN32 || defined WIN64
typedef __int64 i64;
typedef unsigned __int64 u64;
#define			I64_MIN 					(-9223372036854775807i64 - 1i64)
#define			I64_MAX						(9223372036854775807i64)
#define			U64_MAX						(18446744073709551615u64)	// @todo Check this
#else
typedef long long i64;
typedef unsigned long long u64;
#define			I64_MIN 					(-9223372036854775807LL - 1LL)
#endif

/**
*	Pixel structure.
*	Stores the x and y locations.
*/
typedef struct _pixel
{
	u32	x;
	u32	y;
}pixel;

/**
*	Slice Type.
*/
typedef enum {
	P_SLICE = 0,		 //!< P-slice (only past references)
	B_SLICE,			 //!< B-slice (past and future references)
	I_SLICE				 //!< I-slice (no references)
} eSliceType;

/**
*	Neighborhood availability.
*	Used for Intra modes.
*/
typedef enum {
	VALID_LB = 0,	    //!< Left Bottom
	VALID_L,			//!< Left
	VALID_LT,			//!< Left Top
	VALID_T,			//!< Top
	VALID_TR,			//!< Top Right
} eValidIdx;

/**
*	Coefficient scanning type.
*	Used in ACS.
*/
typedef enum {
	SCAN_ZIGZAG = 0,    //!< typical zigzag scan
	SCAN_HOR    = 1,    //!< horizontal first scan
	SCAN_VER    = 2,    //!< vertical first scan
	SCAN_DIAG   = 3     //!< up-right diagonal scan
} eScanType;

// General macros

/**
*	Max of 2 numbers.
*/
#ifndef max
#define 		max(a, b) 					(((a) > (b)) 	? 	(a) 		: 	(b))
#endif

/**
*	Min of 2 numbers.
*/
#ifndef min
#define 		min(a, b) 					(((a) < (b)) 	? 	(a) 		: 	(b))
#endif

/**
*	Swappint 2 numbers.
*	Before using SWAP, define a variable tmp of the same class as A and B.
*/
#define			SWAP(A,B)											\
	do{																\
		tmp = (A);													\
		(A) = (B);													\
		(B) = tmp;													\
	}while(0)														\

#define 		ABS(A) 						((A)<(0) ? (-(A)):(A)) 								//!< Absolute of a number.
#define 		Clip1(a)            		((a)>255?255:((a)<0?0:(a)))							//!< Saturate from 0 to 255.
#define 		Clip3(min,max,val) 			(((val)<(min))?(min):(((val)>(max))?(max):(val)))	//!< Saturate a number between min and max.
#define			ROUND(x)					floor((x)+0.5)										//!< Round a number.
#ifndef			PI
#define			PI							3.141592654
#endif

// Cabac
#define			RET_1_IF_TRUE(A)			( (A) != 0 ? 1 : 0 )	
#define			GET_CTX_QT_CBF(A,B)			( (B) ? ((A) != 4 ? 1 : 0) : 0)
#define			GET_STATE(A)				((A)>>1)
#define			GET_MPS(A)					((u32(A)) & 0x1)
#define			UPDATE_LPS(A)				((A)=g_pbNextStateLPS[(A)])
#define			UPDATE_MPS(A)				((A)=g_pbNextStateMPS[(A)])
// Cabac initialization macro
#define INIT_CABAC(n,m,v)													\
	do																		\
	{																		\
		InitCabacEntry((n)*(m), uiQP, pbContextModels, (v)[eCurrSliceType]);\
		pbContextModels += (n)*(m);											\
		uiOffset += (n)*(m);												\
	}while(0)																\

/**
* Bitstream writing.
* Write one byte at a time and check for emulation prevention if required.
* For the first flush, invalid reads of 2 bytes will occur, but we ignore it
* and hope that they are not 0 and 0. However, I code in such a way that for
* the first flushing, the emulation prevention is not tested (keep it false
* for the start sequence).
*/
#define			FLUSH_BITS(dst,src,numbits,emuprev)					\
	do{																\
		u32 uiTmpWord = src;										\
		for(i32 i=0;i<((numbits)>>3);i++)							\
		{															\
			byte ubTmpByte = byte(uiTmpWord >> 24);					\
			uiTmpWord <<= 8;										\
			if((emuprev) && dst[-2] == 0 &&							\
				dst[-1] == 0 && ubTmpByte <= 3)						\
				*(dst)++ = 0x03;									\
			*(dst)++ = ubTmpByte;									\
		}															\
	}while(0)														\

/**
*	Log2.
*	Help from http://stackoverflow.com/questions/6834868/macro-to-compute-number-of-bits-needed-to-store-a-number-n
*/
#define			NBITS2(n)					(((n)&2)?1:0)
#define			NBITS4(n)					(((n)&0xC)?(2+NBITS2((n)>>2)):(NBITS2(n)))
#define			NBITS8(n)					(((n)&0xF0)?(4+NBITS4((n)>>4)):(NBITS4(n)))
#define			NBITS16(n)					(((n)&0xFF00)?(8+NBITS8((n)>>8)):(NBITS8(n)))
#define			NBITS32(n)					(((n)&0xFFFF0000)?(16+NBITS16((n)>>16)):(NBITS16(n)))
#define			LOG2(n)						((n)==0?0:NBITS32(n)+1)

#define			IS_INTRA(x)					(x >= 0 && x < TOTAL_INTRA_MODES)	//!< CU type checking. See Defines.h for the total intra modes
#define			GET_INTERLEAVED_BITS(x)		(((((x) * 0x01010101) & 0x40100401) * 0x8040201 ) >> 27)

/**
*	Assertion.
*	The run should stop here in case of an error.
*/
#if defined _MSC_VER && defined _DEBUG
#define			MY_DEBUGBREAK()				__debugbreak()
#else
#define			MY_DEBUGBREAK()				assert(0)			// @todo I need to change this
#endif

/**
*	Equivalent of assert().
*/
#define			MAKE_SURE(cond, str)							\
	do{															\
		if(!(cond))												\
		{														\
			printf("Error: %s\n",(str));						\
			printf("%s: line %d\n",__FILE__, __LINE__);			\
			MY_DEBUGBREAK();									\
		}														\
	}while(0)													\

/** 
*	Sleep.
*/
#ifdef _MSC_VER
#include <Windows.h>
#define		sleep(A)		Sleep(A)
#endif

#define			SATURATE_HIGH(A,B)			((A)>(B)?(B):(A))				//!< Saturate A after B.
#define			SATURATE_LOW(A,B)			((A)<(B)?(B):(A))				//!< Saturate A before B.
#define			SATURATE(A,B,C)				((A)<(B)?(B):((A)>(C)?(C):(A)))	//!< Saturate A between B (low) and C (high)

#endif	// __TYPEDEFS_H__
