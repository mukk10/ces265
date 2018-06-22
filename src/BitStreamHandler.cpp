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
* @file BitStreamHandler.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods related to the BitStreamHandler class.
*/

#include <BitStreamHandler.h>
#include <Defines.h>

BitStreamHandler::BitStreamHandler(u64 uiTotalBytes)
{
	m_uiCurrWord				= 	0;
	m_iBitLocPtr				= 	32;
	m_u64BytesRBSP				=	0;
	m_u64BytesPerFrame			=	0;
	m_u64AllocBytes				=	uiTotalBytes;
	m_pbByteBuffer				=	new byte[u32(uiTotalBytes)];
	m_pbByteBuffer[0]			=	0xFF;
	m_pbByteBuffer[1]			=	0xFF;
	m_pbRBSPBuffer				=	m_pbByteBuffer+2;	// To eliminate invalid read in emulation prevention
	m_pbCurrRBSPBuffer			=	m_pbRBSPBuffer;
	m_pcNextBitStreamHandler	=	NULL;
	m_pcPrevBitStreamHandler	=	NULL;
}

BitStreamHandler::~BitStreamHandler()
{
	delete [] m_pbByteBuffer;
}

void BitStreamHandler::ResetRBSPBufferSize(u64 uiTotalBytes)
{
	if(m_pbByteBuffer)
		delete [] m_pbByteBuffer;
	m_pbByteBuffer		=	new byte[u32(uiTotalBytes)];
	m_pbByteBuffer[0]	=	0xFF;
	m_pbByteBuffer[1]	=	0xFF;
	m_pbRBSPBuffer		=	m_pbByteBuffer+2;	// To eliminate invalid read in emulation prevention
	m_pbCurrRBSPBuffer	=	m_pbRBSPBuffer;
}

void BitStreamHandler::InitBitStreamWordLevel(bit isNewBuffer)
{
	m_uiCurrWord		= 	0;
	m_iBitLocPtr		= 	32;
	m_u64BytesRBSP		=	0;
	if(isNewBuffer)
	{
		m_pbCurrRBSPBuffer	=	m_pbRBSPBuffer;
		m_u64BytesPerFrame	=	0;	// @todo Currently, the bytes per Frame and the total RBSP bytes are the same
	}
}

void BitStreamHandler::PutStartCodeWordLevel()
{
	// Check that the current writing location is at a byte boundary
	MAKE_SURE((m_iBitLocPtr == 32),
		"Error: The start code is placed at a location where it must not be");
	PutCodeInBitstream(0x00000001, 32, false);
	m_u64BytesPerFrame = u64(m_pbCurrRBSPBuffer - m_pbRBSPBuffer);
	m_u64BytesRBSP = m_u64BytesPerFrame;
}

void BitStreamHandler::PutCodeInBitstream(u32 uiCode, i32 iLength, bit bEmuPrevActivate)
{
	m_iBitLocPtr -= iLength;
	if(m_iBitLocPtr > 0)	// The word buffer is not completed yet. @todo Check if > or >= will work
		m_uiCurrWord = ((m_uiCurrWord) | (uiCode << m_iBitLocPtr));
	else // The word buffer is full, perform emulation prevention and write to bitstream
	{
		m_uiCurrWord = ((m_uiCurrWord) | (uiCode >> (-m_iBitLocPtr)));	// Fill the remaining word buffer
		FLUSH_BITS(m_pbCurrRBSPBuffer,m_uiCurrWord,32,bEmuPrevActivate);	// Flust the full word to the bitstream buffer

		m_u64BytesPerFrame = u64(m_pbCurrRBSPBuffer - m_pbRBSPBuffer);
		m_u64BytesRBSP = m_u64BytesPerFrame;

		MAKE_SURE(m_u64BytesPerFrame < m_u64AllocBytes, "Error: Bitstream buffer overflow detected.");

		m_iBitLocPtr += 32;
		if(m_iBitLocPtr == 32)
			m_uiCurrWord = 0;
		else
			m_uiCurrWord = (uiCode << m_iBitLocPtr);	// Put the remaining data of the code in the word buffer
	}
}

void BitStreamHandler::PutUNInBitstream(u32 uiCode, i32 iLength, char *pbTraceString)
{
	MAKE_SURE((iLength > 0),"Error: Length of the code is <= 0");
	PutCodeInBitstream(uiCode,iLength,true);
}

void BitStreamHandler::PutUVInBitstream(u32 uiCode, char *pbTraceString)
{
	u32 uiLocCode = ++uiCode;
	i32 iLength = LOG2(uiLocCode) - 1;
	MAKE_SURE((iLength >= 0),"Error: Length of the code is < 0");	// The length of the code can be 0 here
	PutCodeInBitstream(0,iLength,true);
	PutCodeInBitstream(uiLocCode,iLength+1,true);
}

void BitStreamHandler::PutSVInBitstream(i32 iCode, char *pbTraceString)
{
	u32 uiCode = (iCode > 0 ? (iCode<<1)-1 : (-iCode)<<1);	// Convert signed to unsigned
	PutUVInBitstream(uiCode,pbTraceString);
}

void BitStreamHandler::FlushRemBytes()
{
	MAKE_SURE(((m_iBitLocPtr & 0x7)==0),
		"Error:The word-level cache is not properly byte algined");
	FLUSH_BITS(m_pbCurrRBSPBuffer,m_uiCurrWord,(32-m_iBitLocPtr),true);
	m_u64BytesPerFrame = u64(m_pbCurrRBSPBuffer - m_pbRBSPBuffer);
	m_u64BytesRBSP = m_u64BytesPerFrame;
	m_uiCurrWord = 0;
}

void BitStreamHandler::WriteAlignZeroBits()
{
	i32 iLength = (8-(32-m_iBitLocPtr)) & 0x7;	// Complete a full byte that must be written to the bitstream
	MAKE_SURE((iLength < 8),
		"Error: The byte alignment wrongly requires more than 8-bits");
	PutCodeInBitstream(0,iLength,true);
}

void BitStreamHandler::WriteRBSPTrailingBits()
{
	PutCodeInBitstream(1,1,true);
	WriteAlignZeroBits();
}

void BitStreamHandler::FixZeroTermination()
{
	/* 7.4.1 and 7.4.1.1
	 * ... when the last byte of the RBSP data is equal to 0x00 (which can
	 * only occur when the RBSP ends in a cabac_zero_word), a final byte equal
	 * to 0x03 is appended to the end of the data.
	 */
	// @todo Check this
	if(m_pbRBSPBuffer[GetTotalBytesWritten()-1] == 0)
		FLUSH_BITS(m_pbCurrRBSPBuffer,0x03,8,false);
}