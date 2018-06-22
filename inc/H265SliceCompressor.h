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
* @file H265SliceCompressor.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the class that handles Slice compression functions
*/

#ifndef __H265SLICECOMPRESSOR_H__
#define __H265SLICECOMPRESSOR_H__

#include <Defines.h>
#include <TypeDefs.h>

class InputParameters;
class ImageParameters;
class H265Transform;
class BitStreamHandler;
class H265Headers;
class Cabac;
class WorkQueue;
class WorkItem;
class ThreadHandler;
class H265TileCompressor;

/**
*	Tile job arguments.
*	Use this structure to feed the tile work-queue for multi-threading.
*/
typedef struct _TileJobArgs
{
	i32					iNum;
	byte				*pbYBuff;
	byte				*pbCbBuff;
	byte				*pbCrBuff;
	Cabac				*pcCabac;
	BitStreamHandler	*pcBitStreamHandler;
	H265TileCompressor	*pcTileCompressor;
}TileJobArgs_t;

/**
*	Slice compressor.
*	Compress a full slice.
*/
class H265SliceCompressor
{
private:
	InputParameters const	*m_pcInputParam;					//!< Input parameters
	ImageParameters const	*m_pcImageParam;					//!< Image parameters
	H265TileCompressor		**m_ppcH265TileCompressor;			//!< Tile compressor
	Cabac					**m_ppcCabac;						//!< Cabac handler class for each tile
	u32						m_uiTotalTiles;						//!< Total tiles
	pixel					*m_pcTileStartCTUPel;				//!< Tile starting CTU TL location (included in Tile)
	pixel					*m_pcTileEndCTUPel;					//!< Tile ending CTU TL location (included in Tile)
	eSliceType				m_eSliceType;						//!< Type of slice (I, P, B etc.)
	BitStreamHandler		**m_ppcBitStreamHandler;			//!< Bitstream handler
	BitStreamHandler		*m_pcSliceBitStreamHandler;			//!< Bitstream handler of the slice
	BitStreamHandler		*m_pcSliceHeaderBitStreamHandler;	//!< Stores the slice header bits (required for tiles)
	H265Headers				*m_pcH265Headers;					//!< For generating slice header
	u32						*m_pcTimePerTile;					//!< For storing the time consumption of each tile
	u64						*m_pu64TotalBytesPerTile;			//!< Bytes per Tile
	u64						m_u64TotalBytesPerSlice;			//!< Bytes for the current slice
	WorkQueue				*m_pcTileWorkQueue;					//!< Queue for holding the jobs for tile threads
	u32						m_uiTotalTileThreads;				//!< Total threads allocated for tiles
	TileJobArgs_t			**m_ppcTileJobArgs;					//!< Arguments for the tile thread function
	ThreadHandler			**m_ppcTileThreadHandler;			//!< Thread handlers for the tiles
	WorkItem				**m_ppcWorkItem;					//!< Work items per tile thread job
	u32						m_ctTimeForSlice;					//!< Total time per slice
	/**
	*	Generate slice bounding rectangle.
	*/
	void					GenTileBoundingPixels();

	/**
	*	Write slice header information.
	*/
	void					WriteSliceHeader(u32 uiCurrSliceNum);

	/**
	*	Concatenate bitstream of Tiles.
	*	A linked list is made which attaches the proceeding BitStreamHandler to the previous one.
	*/
	void					CatTileBitStreamHandlers();

	/**
	*	Encode the tile entry information in the bitstream.
	*	At the end of the slice with multiple tiles, the information about the start of the tile
	*	bitstream in the concatenated bitstream must be encoded to enable parallel decoding.
	*/
	void					WriteTilesEntryPointInSliceHeader();

	/**
	*	Fix the last byte of the NAL unit.
	*	If there are multiple tiles for a given slice, then do it only for the last tile.
	*/
	void					FixZeroTermination();

	/**
	*	Initialize the bitstreams.
	*	Reset counters and pointers.
	*/
	void					InitialToCompression();

	/**
	*	Make threads for tile compressor.
	*	Will only be activated if USE_THREADS is enabled.
	*/
	void					MakeTileThreadsPool();

public:

	/**
	*	Constructor.
	*	@param pcInputParam Input parameters to the program.
	*	@param pcImageParam Image parameters of a video frame.
	*	@param ppcBitStreamHandler Bitstream handlers for the slice.
	*/
	H265SliceCompressor(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, BitStreamHandler **& ppcBitStreamHandler);
	~H265SliceCompressor();
	/**
	*	Compress a slice.
	*	@param pbYBuff Contains luma samples.
	*	@param pbCbBuff Contains Cb samples.
	*	@param pbCrBuff Contains Cr samples.
	*	@param uiCurrSliceNum Slice number of the current slice.
	*/
	void					CompressSlice(byte *pbYBuff, byte *pbCbBuff, byte *pbCrBuff, u32 uiCurrSliceNum);

	/**
	*	Get the slice bitstream handler.
	*	@return The bitstream handler for the complete slice.
	*/
	BitStreamHandler		*GetSliceBitStreamHandler(){return m_pcSliceBitStreamHandler;}

	/**
	*	Get the slice bitstream handler for a specific tile.
	*	@param uiTileNum Tile number for which the bitstream handler is required.
	*	@return Bitstream handler of the tile.
	*/
	BitStreamHandler		*GetSliceBitStreamHandler(u32 uiTileNum){return m_ppcBitStreamHandler[uiTileNum];}

	/**
	*	Get total bytes per tile.
	*	@return Total bytes of all the tiles.
	*/
	u64						*GetBytesPerTile(){return m_pu64TotalBytesPerTile;}

	/**
	*	Get total bytes of a specific tile.
	*	@return Total bytes written for the requested tile.
	*/
	u64						GetBytesPerTile(u32 uiTileNum){return m_pu64TotalBytesPerTile[uiTileNum];}

	/**
	*	Get total bytes for the current slice.
	*	@return Total bytes for a complete slice.
	*/
	u64						GetTotalBytes(){return m_u64TotalBytesPerSlice;}

	/**
	*	Get total time per tile.
	*	@return Total time in msec for all the tiles in the slice.
	*/
	u32						*GetTimePerTile(){return m_pcTimePerTile;}

	/**
	*	Get time per slice.
	*	@return Total time in msec for the complete slice.
	*/	
	u32						GetTimePerSlice(){return m_ctTimeForSlice;}

	/**
	*	Get maximum intra angles for the tile.
	*	@param uiTileNum Number of the tile.
	*	@return Total intra angular modes tested for the tile.
	*/	
	u32						GetTileIntraAng(u32 uiTileNum);
};

#endif // __H265SLICECOMPRESSOR_H__