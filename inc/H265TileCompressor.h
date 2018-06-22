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
* @file H265TileCompressor.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the class that handles Slice compression functions
*/

#ifndef __H265TILECOMPRESSOR_H__
#define __H265TILECOMPRESSOR_H__

#include <Defines.h>
#include <TypeDefs.h>

class InputParameters;
class ImageParameters;
class BitStreamHandler;
class Cabac;
class H265CTUCompressor;

/**
*	Tile compressor.
*	Compresses one full tile.
*/
class H265TileCompressor
{
private:
	InputParameters const	*m_pcInputParam;					//!< Input parameters
	ImageParameters const	*m_pcImageParam;					//!< Image parameters
	H265CTUCompressor		*m_pcH265CTUCompressor;				//!< CTU compressor (This is not threaded. But it can be made to)
	u32						*m_puiCTUAddrMapX;					//!< Address X of the CTUs to process in the frame
	u32						*m_puiCTUAddrMapY;					//!< Address Y of the CTUs to process in the frame
	u32						m_uiTotalCTUsInTile;				//!< Total CTUs to process in the tile
	pixel					m_cTileStartCTUPelTL;				//!< Starting pixel of the tile
	pixel					m_cTileEndCTUPelTL;					//!< Ending pixel of the tile
	u32						m_uiTileWidthInPels;				//!< Width of the tile in pixel units
	u32						m_uiTileHeightInPels;				//!< Height of the tile in pixel units
	u32						m_uiTileWidthInCTUs;				//!< Width of tile in CTU units
	u32						m_uiTileHeightInCTUs;				//!< Height of tile in CTU units
	u32						m_ctTimeForTile;					//!< Total tics the tile compressor takes
	u64						m_uiTotalBytes;						//!< Total bytes written for the tile
	u32						m_uiTileID;							//!< Tile ID
public:

	/**
	*	Constructor
	*	@param pcInputParam Input parameters to the program.
	*	@param pcImageParam Image parameters of a video frame.
	*	@param cTileStartCTUPelTL Top left (x,y) pixel locations of the the top left CTU of the tile which contains this CTU compressor.
	*	@param cTileEndCTUPelTL Top left (x,y) pixel location of the bottom right CTU of the tile which contains this CTU compressor.
	*	@param uitileID ID of the tile.
	*/
	H265TileCompressor(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, 
		pixel cTileStartCTUPelTL, pixel cTileEndCTUPelTL, u32 uiTileID);
		//u32 *uiAddrMapX, u32 *uiAddrMapY, u32 uiTotalCTUsInTile);
	~H265TileCompressor();
	/**
	*	Compress a Tile.
	*	@param pbYBuff Contains luma samples.
	*	@param pbCbBuff Contains Cb samples.
	*	@param pbCrBuff Contains Cr samples.
	*	@param pcCabac CABAC encoder.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void					CompressTile(byte *pbYBuff, byte *pbCbBuff, byte *pbCrBuff,
		Cabac *& pcCabac, BitStreamHandler *& pcBitStreamHandler);

	/**
	*	Get the time tics for the current tile.
	*	@return Time in msec consumed by the tile.
	*/
	u32						GetTimePerTile(){return m_ctTimeForTile;}

	/**
	*	Get the total bytes written for the tile.
	*	@return Total bytes written for the current tile in the bitstream.
	*/
	u64						GetTotalBytesWritten(){return m_uiTotalBytes;}

	/**
	*	Make an address map for Tiles.
	*	Depending upon the Tiles, determine the CTUs that will be used in tiles.
	*/
	void					MakeCTUAddrMap();

	/**
	*	Set tile ID.
	*	@param uiID ID of the tile.
	*/
	void					SetTileID(u32 uiID){m_uiTileID = uiID;}

};


#endif	// __H265TILECOMPRESSOR_H__