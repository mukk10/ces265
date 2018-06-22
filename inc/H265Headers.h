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
* @file H265Headers.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the class that handles header information for H.265 bitstream
*/

#ifndef __H265HEADERS_H__
#define __H265HEADERS_H__

#include <TypeDefs.h>

class InputParameters;
class ImageParameters;
class BitStreamHandler;

/**
*	Header generators.
*	Generates HEVC headers for the bitstream.
*/
class H265Headers
{
private:
	/**
	*	Write the VPS to the bitstream.
	*/
	void WriteVPSInBitstream(BitStreamHandler *&pcBitStreamHandler);

	/**
	*	Write the SPS to the bitstream.
	*/
	void WriteSPSInBitstream(ImageParameters const *pcImageParam, BitStreamHandler *&pcBitStreamHandler);	

	/**
	*	Write the SPS to the bitstream.
	*/
	void WritePPSInBitstream(ImageParameters const *pcImageParam, BitStreamHandler *&pcBitStreamHandler);

	/**
	*	Write the Slice header to the bitstream.
	*/
	void WriteSliceHdrInBitstream(ImageParameters const *pcImageParam, u32 uiCurrSliceNum, BitStreamHandler *&pcBitStreamHandler);
public:
	/**
	*	Generate VPS NAL Unit.
	*	@param pcInputParam Input parameters to the program.
	*	@param pcImageParam Image parameters of a video frame.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void GenVPSNALU(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, BitStreamHandler *&pcBitStreamHandler);

	/**
	*	Generate SPS NAL Unit.
	*	@param pcInputParam Input parameters to the program.
	*	@param pcImageParam Image parameters of a video frame.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void GenSPSNALU(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, BitStreamHandler *&pcBitStreamHandler);

	/**
	*	Generate PPS NAL Unit.
	*	@param pcInputParam Input parameters to the program.
	*	@param pcImageParam Image parameters of a video frame.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void GenPPSNALU(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, BitStreamHandler *&pcBitStreamHandler);
	void GenSliceHeader(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, u32 uiCurrSliceNum, BitStreamHandler *&pcBitStreamHandler);	// Generate slice header
	
	/**
	*	Encode the tile entry information in the slice header.
	*	Only call this function if there are more than 1 tiles per slice.
	*	@param uiNumEntryPointOffsets Total number of offsets in the bitstream where the tiles are written.
	*	@param uiEntryPointOffsets Array which contains the offsets. 
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void WriteTilesEntryPointsInSliceHeader(u32 uiNumEntryPointOffsets, u32 const *uiEntryPointOffsets, BitStreamHandler *&pcBitStreamHandler);

	/**
	*	Constructor.
	*/
	H265Headers();
	~H265Headers();
};

#endif	// __H265HEADERS_H__