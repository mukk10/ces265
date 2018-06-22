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
* @file ImageParameters.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the class that handles image related functions
*/

#ifndef __IMAGEPARAMETERS_H__
#define __IMAGEPARAMETERS_H__

#include <Defines.h>
#include <TypeDefs.h>

class CTU;
class InputParameters;

/**
*	Image parameters of a video frame.
*	Currently, I am keeping everything public of this class. 
*/
class ImageParameters
{
public:
	// Frame characteristics
	u32 	m_uiFrameWidth;                 //!<	Image uiWidth in number of pels
	u32 	m_uiFrameWidthChroma;           //!<	Image uiWidth in number of pels chroma
	u32 	m_uiFrameHeight;                //!<	Number of lines
	u32 	m_uiFrameHeightChroma;       	//!<	Number of lines  chroma
	u32 	m_uiFramePelsLuma;				//!<	Total pixels in luman
	u32 	m_uiFramePelsChroma;			//!<	Total pixels in chroma
	u32		m_uiFrameWidthInCTUs;			//!<	Width of the luma frame in CTUs
	u32		m_uiFrameHeightInCTUs;			//!<	Height of the luma frame in CTUs
	u32		m_uiFrameSizeInCTUs;			//!<	Total CTUs in the frame
	u32		m_uiFrameSizeInTiles;			//!<	Total Tiles in one frame
	u32		m_uiFrameWidthInTiles;			//!<	Total columns of the tiles in one frame
	u32		*m_puiTileWidthInCTUs;			//!<	Array to store the width of all the tiles
	u32		*m_puiTileHeightInCTUs;			//!<	Array to store the height of all the tiles
	u32		*m_puiTileCTUNumX;				//!<	Array to store the right-most CTU number of the tile
	u32		*m_puiTileCTUNumY;				//!<	Array to store the bottom-most CTU number of the tile
	u32		m_uiFrameHeightInTiles;			//!<	Total rows of tiles in one frame
	eSliceType	m_eSliceType;				//!<	Slice type
	u64		m_u64TotalBytesPerTile;			//!<	Total bytes allocated to a tile

	// Frame processing
	u32 	m_uiQP;                     	//!<	Current quantization
	i32 	m_iType;						//!<	Frame m_iType (I, P, B)
	i32 	m_uiCurrCTUNum;					//!<	Current CTU number
	i32 	m_uiCurrSliceNum;				//!<	Current slice number
	i32 	m_uiCurrFrameNum;				//!<	Current frame number
	i8		m_cOutputFolder[FILE_NAME_LEN];	//!<	The output file name 
	u32		m_uiMaxCUSize;					//!<	Maximum size of a CU
	u32		m_uiMinCUSize;					//!<	Minimum CU size
	u32		m_uiMaxCUDepth;					//!<	Maximum depth of a CU
	u32		m_uiQuadTreeTULog2MinSize;		//!<	Minimum TU size
	u32		m_uiQuadTreeTULog2MaxSize;		//!<	Maximum TU size
	u32		m_uiQuadTreeTUMaxDepthInter;	//!<	Maximum TU depth for Inter
	u32		m_uiQuadTreeTUMaxDepthIntra;	//!<	Maximum TU depth for Intra
	u32		m_uiMaxNumRefFrames;			//!<	Maximum reference frames
	u32		m_uiBitsForPOC;					//!<	Total bits for presenting POC
	u32		m_uiMaxMergeCands;				//!<	Totoal merge candidates
	u32		m_uiTileCodingSync;				//!<	Denotes if more than one tile per slice

	/**
	*	Constructor.
	*/
	ImageParameters();
	~ImageParameters();

	/**
	*	Initialize the image properties.
	*	@param pcInputParam Input parameters set by the user.
	*/
	void	InitImgProp(InputParameters *pcInputParam);

	/**
	*	Get CTU location in pixels.
	*	CTU location is in pixel units, given the CTU number.
	*	@param uiCTUNum CTU number in raster-scan order.
	*	@param uiPelX Displacement from the left-boundary of the frame.
	*	@param uiPelY Displacement form the top-boundary of the frame.
	*/
	void	GetCTUStartPel(u32 uCTUNum, u32 &uiPelX, u32 &uiPelY) const;

	/**
	*	Get CTU Number from the given pixel locations.
	*	If the pixel location does not point exactly to the CTU start, then
	*	the CTU number corresponding to the top-left of the pixel locations is generated.
	*	@param uiPelX Displacement from the left-boundary of the frame.
	*	@param uiPelY Displacement form the top-boundary of the frame.
	*	@param uiCTUNum CTU number in raster-scan order.
	*/
	void	GetCTUNum(u32 uiPelX, u32 uiPelY, u32 &uiCTUNum);

	/**
	*	Get the log_2(size) of the CU.
	*	@param uiSize Size of the CU.
	*	@return log_2(uiSize).
	*/
	u32		GetLog2FromCUSize(u32 uiSize);

	/**
	*	Set the tile sizes and their locations.
	*	@param uiFrameWidth Width of the frame.
	*	@param uiFrameHeight Height of the frame.
	*	@param uiFrameSizeInTiles Total tiles within a frame.
	*	@param uiFrameWidthInTiles Width of the frame in tiles.
	*	@param uiFrameHeightInTiles Height of the frame in tiles.
	*	@param uiMaxTileWidthInCTUs Gives the width of the tile in CTUs with the largest width.
	*	@param uiMaxTileHeightInCTUs Gives the height of the tile in CTUs with the largest height.
	*	@param puiTileWidthInCTUs A pointer to array with widths of all tiles in CTUs. Its size must be equal to uiFrameSizeInTiles.
	*	@param puiTileHeightInCTUs A pointer to array with heights of all tiles in CTUs. Its size must be equal to uiFrameSizeInTiles.
	*	@param puiTileCTUNumX Displacement from the left-boundary of the frame of the right edge CTU of all the tiles. Its size must be equal to uiFrameSizeInTiles.
	*	@param puiTileCTUNumY Displacement from the top-boundary of the frame of the bottom edge CTU of all the tiles. Its size must be equal to uiFrameSizeInTiles.
	*/
	void	SetTileStruct(u32 uiFrameWidth, u32 uiFrameHeight, u32 uiFrameSizeInTiles, u32 uiFrameWidthInTiles, u32 uiFrameHeightInTiles, 
							u32 & uiMaxTileWidthInCTUs, u32 & uiMaxTileHeightInCTUs,
							u32 *puiTileWidthInCTUs, u32 *puiTileHeightInCTUs, u32 *puiTileCTUNumX, u32 *puiTileCTUNumY);
};

#endif	// __IMAGEPARAMETERS_H__