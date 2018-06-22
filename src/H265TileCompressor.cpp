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
* @file H265TileCompressor.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods that handles Tile compression functions
*/

#include <InputParameters.h>
#include <ImageParameters.h>
#include <BitStreamHandler.h>
#include <H265CTUCompressor.h>
#include <Cabac.h>
#include <H265TileCompressor.h>
#include <Utilities.h>

// Tile
H265TileCompressor::H265TileCompressor(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, 
									   pixel cTileStartCTUPelTL, pixel cTileEndCTUPelTL, u32 uiTileID)
{
	m_pcInputParam = pcInputParam;
	m_pcImageParam = pcImageParam;

	m_cTileStartCTUPelTL.x = cTileStartCTUPelTL.x;
	m_cTileStartCTUPelTL.y = cTileStartCTUPelTL.y;

	m_cTileEndCTUPelTL.x = cTileEndCTUPelTL.x;
	m_cTileEndCTUPelTL.y = cTileEndCTUPelTL.y;

	// Get tile statistics
	m_uiTileWidthInPels = m_cTileEndCTUPelTL.x - m_cTileStartCTUPelTL.x + CTU_WIDTH;
	m_uiTileHeightInPels = m_cTileEndCTUPelTL.y - m_cTileStartCTUPelTL.y + CTU_HEIGHT;
	m_uiTileWidthInCTUs = (m_uiTileWidthInPels + 1)/CTU_WIDTH;
	m_uiTileHeightInCTUs = (m_uiTileHeightInPels + 1)/CTU_HEIGHT;
	m_uiTotalCTUsInTile = m_uiTileWidthInCTUs*m_uiTileHeightInCTUs;
	
	m_puiCTUAddrMapX = new u32[m_uiTotalCTUsInTile];
	m_puiCTUAddrMapY = new u32[m_uiTotalCTUsInTile];

	// Make a map of addresses for the CTUs in the tile. This will help in compression at CTU level
	MakeCTUAddrMap();
	// Make only one CTU compressor class. This means that we will not implement threads for this class
	m_pcH265CTUCompressor = new H265CTUCompressor(m_pcInputParam,m_pcImageParam,m_cTileStartCTUPelTL,m_cTileEndCTUPelTL);
	m_uiTileID = uiTileID;

	m_uiTotalBytes = 0;
	
}

void H265TileCompressor::MakeCTUAddrMap()
{
	for(u32 i=0,iAddrY=0;i<m_uiTileHeightInCTUs;i++,iAddrY+=CTU_HEIGHT)
	{
		for(u32 j=0,iAddrX=0;j<m_uiTileWidthInCTUs;j++,iAddrX+=CTU_WIDTH)
		{
			m_puiCTUAddrMapX[i*m_uiTileWidthInCTUs+j] = m_cTileStartCTUPelTL.x + iAddrX;
			m_puiCTUAddrMapY[i*m_uiTileWidthInCTUs+j] = m_cTileStartCTUPelTL.y + iAddrY;
		}
	}
}

H265TileCompressor::~H265TileCompressor()
{
	delete m_pcH265CTUCompressor;
	delete [] m_puiCTUAddrMapX;
	delete [] m_puiCTUAddrMapY;
}

void H265TileCompressor::CompressTile(byte *pbYBuff, byte *pbCbBuff, byte *pbCrBuff, 
									  Cabac *& pcCabac, BitStreamHandler *& pcBitstreamHandler)
{
	u32 uiAddrX;
	u32 uiAddrY;
	m_ctTimeForTile = GetTimeInMiliSec();
	m_pcH265CTUCompressor->InitBuffersNewTile();

	for(u32 i=0;i<m_uiTotalCTUsInTile;i++)
	{
		// 1- Compress
		// 2- Encode
		// 3- Update
		uiAddrX = m_puiCTUAddrMapX[i];
		uiAddrY = m_puiCTUAddrMapY[i];
		m_pcH265CTUCompressor->CompressCTU(uiAddrX, uiAddrY, pbYBuff, pbCbBuff, pbCrBuff);
		m_pcH265CTUCompressor->EncodeCTU(uiAddrX, uiAddrY, pcCabac, pcBitstreamHandler);
		m_pcH265CTUCompressor->UpdateBuffers(uiAddrX, uiAddrY, pbYBuff, pbCbBuff, pbCrBuff);
		if(m_pcInputParam->m_bVerbose)
			printf("Trace: CTU at (%u,%u) encoded in %u msec.\n",uiAddrX,uiAddrY,m_pcH265CTUCompressor->GetTimePerCTU());
	}
	m_ctTimeForTile = GetTimeInMiliSec() - m_ctTimeForTile;
	m_uiTotalBytes = pcBitstreamHandler->GetTotalBytesWritten();
	if(m_pcInputParam->m_bVerbose)
		printf("Trace: Total bytes for tile %u = %llu.\n",m_uiTileID,pcBitstreamHandler->GetTotalBytesWritten());
}
