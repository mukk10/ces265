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
* @file H265GOPCompressor.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods that handles GOP compression functions
*/

#include <InputParameters.h>
#include <ImageParameters.h>
#include <BitStreamHandler.h>
#include <H265SliceCompressor.h>
#include <H265GOPCompressor.h>
#include <stdio.h>
#include <time.h>


// GOP
H265GOPCompressor::H265GOPCompressor(InputParameters *pcInputParam, ImageParameters *pcImageParam, BitStreamHandler ***& pppcBitStreamPerSlice)
{
	m_pcInputParam = pcInputParam;
	m_pcImageParam = pcImageParam;
	m_uiNumSliceThreads = m_pcInputParam->m_uiNumSliceThreads;
	m_pppcBitStreamPerSlice = pppcBitStreamPerSlice;

	m_ppcH265SliceCompressor = new H265SliceCompressor*[m_uiNumSliceThreads];
	for(u32 i=0;i<m_uiNumSliceThreads;i++)
	{
		m_ppcH265SliceCompressor[i] = new H265SliceCompressor(m_pcInputParam,m_pcImageParam,m_pppcBitStreamPerSlice[i]);
	}

	m_pcTimePerSlice = new clock_t[m_uiNumSliceThreads];
}

H265GOPCompressor::~H265GOPCompressor()
{
	for(u32 i=0;i<m_uiNumSliceThreads;i++)
	{
		delete m_ppcH265SliceCompressor[i];
	}
	delete [] m_ppcH265SliceCompressor;
	delete [] m_pcTimePerSlice;
}

void H265GOPCompressor::CompressGOP(byte **ppbYBuff, byte **ppbCbBuff, byte **ppbCrBuff,
									u32 uiStartSliceNum)
{
	// Compress each slice individually
	for(u32 i=0;i<m_pcInputParam->m_uiGopSize/m_uiNumSliceThreads;i++)
	{
		for(u32 j=0;j<m_uiNumSliceThreads;j++)
		{
			m_ppcH265SliceCompressor[j]->CompressSlice(
				ppbYBuff[i*m_uiNumSliceThreads+j],
				ppbCbBuff[i*m_uiNumSliceThreads+j],
				ppbCrBuff[i*m_uiNumSliceThreads+j],
				uiStartSliceNum);
			if(m_pcInputParam->m_bVerbose)
				printf("Trace: Slice %u encoded.\n",uiStartSliceNum++);
			m_pcTimePerSlice[j] = m_ppcH265SliceCompressor[j]->GetTimePerSlice();
		}
	}
}

BitStreamHandler* H265GOPCompressor::GetSliceBitStreamHandler(u32 uiSliceNum)
{
	MAKE_SURE((uiSliceNum < m_pcInputParam->m_uiGopSize),"The Slice number is not correct");
	return m_ppcH265SliceCompressor[uiSliceNum]->GetSliceBitStreamHandler();
}

BitStreamHandler* H265GOPCompressor::GetSliceBitStreamHandler(u32 uiSliceNum, u32 uiTileNum)
{
	MAKE_SURE((uiSliceNum < m_pcInputParam->m_uiGopSize),"The Slice number is not correct");
	return m_ppcH265SliceCompressor[uiSliceNum]->GetSliceBitStreamHandler(uiTileNum);
}
