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
* @file H265GOPCompressor.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the class that handles GOP compression functions
*/

#ifndef __H265GOPCOMPRESSOR_H__
#define __H265GOPCOMPRESSOR_H__

#include <Defines.h>
#include <TypeDefs.h>
#include <time.h>

class InputParameters;
class ImageParameters;
class BitStreamHandler;
class H265SliceCompressor;

/**
*	GOP compressor.
*	Compress a full GOP.
*/
class H265GOPCompressor
{
private:
	InputParameters			*m_pcInputParam;			//!< Input parameters
	ImageParameters			*m_pcImageParam;			//!< Image parameters
	u32						m_uiNumSliceThreads;		//!< Total Slice threads
	H265SliceCompressor		**m_ppcH265SliceCompressor;	//!< Slice compressor
	BitStreamHandler		***m_pppcBitStreamPerSlice;	//!< Bitstream handler per slice
	clock_t					*m_pcTimePerSlice;			//!< For storing the time consumption of each slice
public:
	/**
	*	Constructor.
	*	@param pcInputParam Input parameters to the program.
	*	@param pcImageParam Image parameters of a video frame.
	*	@param pppcBitStreamPerSlice Bitstream handlers for the GOP.
	*/
	H265GOPCompressor(InputParameters *pcInputParam, ImageParameters *pcImageParam, BitStreamHandler ***& pppcBitStreamPerSlice);
	~H265GOPCompressor();
	/**
	*	Compress a GOP.
	*	Give the number of the starting slice/frame of the GOP (starts from 0).
	*	@param ppbYBuff Contains luma samples.
	*	@param ppbCbBuff Contains Cb samples.
	*	@param ppbCrBuff Contains Cr samples.
	*	@param uiStartSliceNum The number of the starting slice within the GOP.
	*/
	void					CompressGOP(byte **ppbYBuff, byte **ppbCbBuff, byte **ppbCrBuff, u32 uiStartSliceNum);

	/**
	*	Get compressed bitstream of a slice.
	*	Each slice has one or many tiles, but they can be accessed due to the linked-list structure
	*	of the BitStreamHanlders.
	*	@param uiSliceNum Slice number withtin the GOP.
	*	@return Bitstream handler of the slice.
	*/
	BitStreamHandler		*GetSliceBitStreamHandler(u32 uiSliceNum);

	/**
	*	Get compressed bitstream of a tile within a slice.
	*	Each slice has one or many tiles.
	*	@param uiSliceNum Slice number withtin the GOP.
	*	@param uiTileNum Tile number within the slice.
	*	@return Bitstream handler of the slice.
	*/
	BitStreamHandler		*GetSliceBitStreamHandler(u32 uiSliceNum, u32 uiTileNum);

	/**
	*	Get the Slice compressor.
	*	@param uiSliceNum Slice number within the GOP.
	*	@return Slice compressor object.
	*/
	H265SliceCompressor		*GetSliceCompressor(u32 uiSliceNum){return m_ppcH265SliceCompressor[uiSliceNum];}
};


#endif	// __H265GOPCOMPRESSOR_H__