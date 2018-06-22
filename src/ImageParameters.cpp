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
* @file ImageParameters.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods for the ImageParameters class
*/

#include <ImageParameters.h>
#include <InputParameters.h>
#include <string.h>
#include <cassert>

ImageParameters::ImageParameters()
{
	m_uiMaxCUSize = CTU_WIDTH;
	m_uiMaxCUDepth = LOG2(m_uiMaxCUSize-1)-2;
	m_uiMinCUSize = CTU_WIDTH >> (m_uiMaxCUDepth-1);
	m_uiQuadTreeTULog2MinSize = 2;	
	m_uiQuadTreeTULog2MaxSize = 5;	
	m_uiQuadTreeTUMaxDepthInter = 1;
	m_uiQuadTreeTUMaxDepthIntra = 1;
	m_uiMaxNumRefFrames = 1;		
	m_uiBitsForPOC = 8;			
	m_uiMaxMergeCands = 5;
}

ImageParameters::~ImageParameters()
{
	delete [] m_puiTileWidthInCTUs;
	delete [] m_puiTileHeightInCTUs;
	delete [] m_puiTileCTUNumX;
	delete [] m_puiTileCTUNumY;
}

void ImageParameters::InitImgProp(InputParameters *pcInputParam)
{
	m_uiFrameWidth			=	(pcInputParam->m_uiFrameWidth);
	m_uiFrameWidthChroma	=	((pcInputParam->m_uiFrameWidth) / 2);
	m_uiFrameHeight			=	(pcInputParam->m_uiFrameHeight);
	m_uiFrameHeightChroma	=	((pcInputParam->m_uiFrameHeight) / 2);

	m_uiFramePelsLuma 		= 	m_uiFrameWidth*m_uiFrameHeight;
	m_uiFramePelsChroma		= 	m_uiFrameWidthChroma*m_uiFrameHeightChroma;

	m_uiFrameWidthInCTUs	= 	(pcInputParam->m_uiFrameWidth)/CTU_WIDTH;	// Make sure this is divisible completely
	m_uiFrameHeightInCTUs 	= 	(pcInputParam->m_uiFrameHeight)/CTU_HEIGHT;	// Make sure this is divisible completely
	m_uiFrameSizeInCTUs   	= 	m_uiFrameWidthInCTUs*m_uiFrameHeightInCTUs;

	m_uiCurrCTUNum 			= 	0;
	m_uiCurrSliceNum 		= 	0;
	m_uiCurrFrameNum 		= 	0;
	m_uiQP					= 	pcInputParam->m_uiQP;
	m_iType 				= 	I_SLICE;  //set manually here...in ref code set by 'SetImgType' func invoked in main

	strcpy(m_cOutputFolder,"./CES_H265_output");

	// Tile structure
	m_uiFrameSizeInTiles	=	pcInputParam->m_uiTilesPerFrame;
	m_uiFrameWidthInTiles	=	pcInputParam->m_uiFrameWidthInTiles;		
	m_uiFrameHeightInTiles	=	pcInputParam->m_uiFrameHeightInTiles;
	MAKE_SURE((m_uiFrameWidthInTiles*m_uiFrameHeightInTiles)==m_uiFrameSizeInTiles,
		"Error: The Tile sizes are incorrect or they do not match the total tiles in a frame");

	m_uiTileCodingSync = RET_1_IF_TRUE(m_uiFrameSizeInTiles > 1);

	// Determine Tiles widths and heights
	m_puiTileWidthInCTUs	=	new u32[m_uiFrameSizeInTiles];
	m_puiTileHeightInCTUs	=	new u32[m_uiFrameSizeInTiles];
	m_puiTileCTUNumX	=	new	u32[m_uiFrameSizeInTiles];
	m_puiTileCTUNumY	=	new	u32[m_uiFrameSizeInTiles];
	u32 uiMaxTileWidthInCTUs = 0;
	u32 uiMaxTileHeightInCTUs = 0;

	SetTileStruct(m_uiFrameWidth,m_uiFrameHeight,m_uiFrameSizeInTiles,m_uiFrameWidthInTiles,m_uiFrameHeightInTiles,
		uiMaxTileWidthInCTUs, uiMaxTileHeightInCTUs,
		m_puiTileWidthInCTUs,m_puiTileHeightInCTUs,m_puiTileCTUNumX,m_puiTileCTUNumY);

	// Entropy buffer size
	m_u64TotalBytesPerTile	=	uiMaxTileWidthInCTUs*uiMaxTileHeightInCTUs*BYTES_PER_CTU;
}

void ImageParameters::SetTileStruct(u32 uiFrameWidth, u32 uiFrameHeight, u32 uiFrameSizeInTiles, u32 uiFrameWidthInTiles, u32 uiFrameHeightInTiles, 
									u32 & uiMaxTileWidthInCTUs, u32 & uiMaxTileHeightInCTUs,
									u32 *puiTileWidthInCTUs, u32 *puiTileHeightInCTUs, u32 *puiTileCTUNumX, u32 *puiTileCTUNumY)
{
	u32 uiTileIdx;
	u32 uiFrameWidthInCTUs = uiFrameWidth/CTU_WIDTH;
	u32 uiFrameHeightInCTUs = uiFrameHeight/CTU_HEIGHT;
	for(u32 i=0;i<uiFrameHeightInTiles;i++)
	{
		for(u32 j=0;j<uiFrameWidthInTiles;j++)
		{
			uiTileIdx = i*uiFrameWidthInTiles + j;
			puiTileWidthInCTUs[uiTileIdx] = (j+1)*uiFrameWidthInCTUs/uiFrameWidthInTiles
				- j*uiFrameWidthInCTUs/uiFrameWidthInTiles;
			if(puiTileWidthInCTUs[uiTileIdx] > uiMaxTileWidthInCTUs)
				uiMaxTileWidthInCTUs = puiTileWidthInCTUs[uiTileIdx];
		}
	}
	for(u32 i=0;i<uiFrameWidthInTiles;i++)
	{
		for(u32 j=0;j<uiFrameHeightInTiles;j++)
		{
			uiTileIdx = i + j*uiFrameWidthInTiles;
			puiTileHeightInCTUs[uiTileIdx] = (j+1)*uiFrameHeightInCTUs/uiFrameHeightInTiles
				- j*uiFrameHeightInCTUs/uiFrameHeightInTiles;
			if(puiTileHeightInCTUs[uiTileIdx] > uiMaxTileHeightInCTUs)
				uiMaxTileHeightInCTUs = puiTileHeightInCTUs[uiTileIdx];
		}
	}

	// Generate the tile boundaries
	for(u32 i=0;i<uiFrameHeightInTiles;i++)
	{
		for(u32 j=0;j<uiFrameWidthInTiles;j++)
		{
			uiTileIdx = i*uiFrameWidthInTiles + j;
			// Initialize the right edge of each tile
			puiTileCTUNumX[uiTileIdx] = 0;
			for(u32 m=0;m<j;m++)
				puiTileCTUNumX[uiTileIdx] += puiTileWidthInCTUs[i*uiFrameWidthInTiles+m];

			// Initialize the right edge of each tile
			puiTileCTUNumY[uiTileIdx] = 0;
			for(u32 m=0;m<i;m++)
				puiTileCTUNumY[uiTileIdx] += puiTileHeightInCTUs[j+m*uiFrameWidthInTiles];
		}
	}
}

void ImageParameters::GetCTUStartPel(u32 uiCTUNum, u32 &uiPelX, u32 &uiPelY) const
{
	uiPelX = (uiCTUNum % m_uiFrameWidthInCTUs)*CTU_WIDTH;
	uiPelY = (uiCTUNum / m_uiFrameWidthInCTUs)*CTU_HEIGHT;
}

void ImageParameters::GetCTUNum(u32 uiPelX, u32 uiPelY, u32 &uiCTUNum)
{
	uiCTUNum = (uiPelY/CTU_HEIGHT)*m_uiFrameWidthInCTUs + uiPelX/CTU_WIDTH;
}

u32 ImageParameters::GetLog2FromCUSize(u32 uiSize)
{
	// @todo Remove this after debugging
	MAKE_SURE((uiSize >= 4) && (uiSize <= CTU_WIDTH),"CU size is not correct");

	u32 uiLog2Size = 0;
	while(uiSize > 0)
	{
		uiSize >>= 1;
		uiLog2Size++;
	}
	return uiLog2Size;
}