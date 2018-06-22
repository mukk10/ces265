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
* @file H265SliceCompressor.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods that handles Slice compression functions
*/

#include <InputParameters.h>
#include <ImageParameters.h>
#include <BitStreamHandler.h>
#include <H265TileCompressor.h>
#include <H265Headers.h>
#include <H265SliceCompressor.h>
#include <Cabac.h>
#include <WorkItem.h>
#include <WorkQueue.h>
#include <ThreadHandler.h>
#include <Utilities.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER
#include <unistd.h>	// For the sleep() function
#endif

/**
*	Work item preparation.
*	Used for threading purposes.
*/
#define			PREPARE_WORK_ITEM													\
	do																				\
	{																				\
		m_ppcTileJobArgs[i]->iNum = i;												\
		m_ppcTileJobArgs[i]->pbYBuff = pbYBuff;										\
		m_ppcTileJobArgs[i]->pbCbBuff = pbCbBuff;									\
		m_ppcTileJobArgs[i]->pbCrBuff = pbCrBuff;									\
		m_ppcTileJobArgs[i]->pcCabac = m_ppcCabac[i];								\
		m_ppcTileJobArgs[i]->pcBitStreamHandler = m_ppcBitStreamHandler[i];			\
		m_ppcTileJobArgs[i]->pcTileCompressor = m_ppcH265TileCompressor[i];			\
		m_ppcWorkItem[i]->m_pfPtrToFunc = CompressTileThread;						\
		m_ppcWorkItem[i]->m_iItemNum = i;											\
		m_ppcWorkItem[i]->m_pArgs = m_ppcTileJobArgs[i];							\
		m_ppcWorkItem[i]->m_iTotArg = 0;											\
	}while(0)																		\

// Slice
H265SliceCompressor::H265SliceCompressor(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, BitStreamHandler **& ppcBitStreamHandler)
{
	m_pcInputParam = pcInputParam;
	m_pcImageParam = pcImageParam;

	m_uiTotalTiles = m_pcImageParam->m_uiFrameSizeInTiles;
	m_ppcBitStreamHandler = ppcBitStreamHandler;

	m_pcTileStartCTUPel = new pixel[m_uiTotalTiles];
	m_pcTileEndCTUPel = new pixel[m_uiTotalTiles];
	
	// Make the CTU address map, in Tile processing order
	GenTileBoundingPixels();

	m_ppcH265TileCompressor = new H265TileCompressor*[m_uiTotalTiles];
	m_ppcCabac = new Cabac*[m_uiTotalTiles];
	for(u32 i=0;i<m_uiTotalTiles;i++)
	{
		m_ppcCabac[i] = new Cabac(m_pcImageParam);
		m_ppcH265TileCompressor[i] = new H265TileCompressor(m_pcInputParam,m_pcImageParam,
			m_pcTileStartCTUPel[i],m_pcTileEndCTUPel[i],i);
	}

	// If there are more than 1 tiles per slice, it means that we need to encode the tile entry information
	// in the bitstream. This information is available at the end of encoding the complete slice, but must
	// be added in the slice header. Therefore, if we have more than 1 tiles, we save the slice header at a
	// different space. Else, the first tile bitstream handler is used to store the slice header
	if(m_uiTotalTiles > 1)	// Tiles are present, there must be separate slice header bitstream handler
		m_pcSliceHeaderBitStreamHandler = new BitStreamHandler(50);
	else
		m_pcSliceHeaderBitStreamHandler = m_ppcBitStreamHandler[0];

	m_pcSliceBitStreamHandler = m_pcSliceHeaderBitStreamHandler;	// Assign the first bitstream handler as the main handler of the frame
	m_pcH265Headers = new H265Headers;
	m_pcTimePerTile = new u32[m_uiTotalTiles];
	m_pu64TotalBytesPerTile = new u64[m_uiTotalTiles];

#if(USE_THREADS)
	MakeTileThreadsPool();
#endif
}

void H265SliceCompressor::GenTileBoundingPixels()
{
	u32 uiTileIdx;
	u32 uiStartCTUNumForTile;
	u32 uiEndCTUNumForTile;
	u32 *puiTileCTUNumX = m_pcImageParam->m_puiTileCTUNumX;
	u32 *puiTileCTUNumY = m_pcImageParam->m_puiTileCTUNumY;
	u32 *puiTileWidthInCTUs = m_pcImageParam->m_puiTileWidthInCTUs;
	u32 *puiTileHeightInCTUs = m_pcImageParam->m_puiTileHeightInCTUs;
	u32 uiFrameWidthInCTUs = m_pcImageParam->m_uiFrameWidthInCTUs;

	for(u32 i=0;i<m_pcImageParam->m_uiFrameHeightInTiles;i++)
	{
		for(u32 j=0;j<m_pcImageParam->m_uiFrameWidthInTiles;j++)
		{
			uiTileIdx = i*m_pcImageParam->m_uiFrameWidthInTiles + j;
			uiStartCTUNumForTile = puiTileCTUNumX[uiTileIdx]+
				puiTileCTUNumY[uiTileIdx]*uiFrameWidthInCTUs;
			m_pcImageParam->GetCTUStartPel(uiStartCTUNumForTile,
				m_pcTileStartCTUPel[uiTileIdx].x,m_pcTileStartCTUPel[uiTileIdx].y);

			uiEndCTUNumForTile = uiStartCTUNumForTile + (puiTileWidthInCTUs[uiTileIdx]-1)+
				(puiTileHeightInCTUs[uiTileIdx]-1)*uiFrameWidthInCTUs;
			m_pcImageParam->GetCTUStartPel(uiEndCTUNumForTile,
				m_pcTileEndCTUPel[uiTileIdx].x,m_pcTileEndCTUPel[uiTileIdx].y);

			MAKE_SURE(m_pcTileEndCTUPel[uiTileIdx].x > m_pcTileStartCTUPel[uiTileIdx].x,
				"Error: The ending tile x-pixel is smaller or equal to the starting x-pixel");
			MAKE_SURE(m_pcTileEndCTUPel[uiTileIdx].y > m_pcTileStartCTUPel[uiTileIdx].y,
				"Error: The ending tile y-pixel is smaller or equal to the starting y-pixel");
		}
	}
}

void H265SliceCompressor::MakeTileThreadsPool()
{
	// The caller thread will also compress a tile, therefore, the additional
	// threads are shown here
	m_uiTotalTileThreads = m_pcInputParam->m_uiNumTileThreads-1;
	u32 uiTotalTilesQueue = m_uiTotalTiles-1;

	m_pcTileWorkQueue = new WorkQueue(uiTotalTilesQueue);

	m_ppcTileJobArgs = new TileJobArgs_t*[uiTotalTilesQueue];
	for(u32 i=0;i<uiTotalTilesQueue;i++)
		m_ppcTileJobArgs[i] = new TileJobArgs_t;

	m_ppcTileThreadHandler = new ThreadHandler*[m_uiTotalTileThreads];
	for(u32 i=0;i<m_uiTotalTileThreads;i++)
		m_ppcTileThreadHandler[i] = new ThreadHandler(m_pcTileWorkQueue);

	// Start the threads
	// They will wait for jobs inserted in the job queue
	for(u32 i=0;i<m_uiTotalTileThreads;i++)
		m_ppcTileThreadHandler[i]->StartThread();

	m_ppcWorkItem = new WorkItem*[uiTotalTilesQueue];
	for(u32 i=0;i<uiTotalTilesQueue;i++)
		m_ppcWorkItem[i] = new WorkItem();
}

H265SliceCompressor::~H265SliceCompressor()
{
	delete [] m_pcTileStartCTUPel;
	delete [] m_pcTileEndCTUPel;

	for(u32 i=0;i<m_uiTotalTiles;i++)
	{
		delete m_ppcH265TileCompressor[i];
		delete m_ppcCabac[i];
	}
	delete [] m_ppcH265TileCompressor;
	delete [] m_ppcCabac;
	if(m_uiTotalTiles > 1)	// Tiles are present, there must be separate slice header bitstream handler
		delete m_pcSliceHeaderBitStreamHandler;

	delete m_pcH265Headers;
	delete [] m_pcTimePerTile;
	delete [] m_pu64TotalBytesPerTile;

#if(USE_THREADS)
	u32 uiTotalTilesQueue = m_uiTotalTiles-1;
	for(u32 i=0;i<uiTotalTilesQueue;i++)
		delete m_ppcTileJobArgs[i];
	delete [] m_ppcTileJobArgs;

	for(u32 i=0;i<m_uiTotalTileThreads;i++)
		delete m_ppcTileThreadHandler[i];
	delete [] m_ppcTileThreadHandler;

	for(u32 i=0;i<uiTotalTilesQueue;i++)
		delete m_ppcWorkItem[i];
	delete [] m_ppcWorkItem;

	delete m_pcTileWorkQueue;
#endif
}

void H265SliceCompressor::InitialToCompression()
{
	m_u64TotalBytesPerSlice = 0;
	m_pcSliceHeaderBitStreamHandler->InitBitStreamWordLevel(true);	// This is necessary for multiple slices
	for(u32 i=0;i<m_pcImageParam->m_uiFrameSizeInTiles;i++)
	{
		m_ppcBitStreamHandler[i]->InitBitStreamWordLevel(true);
		m_ppcCabac[i]->InitCabac();
	}
}

void H265SliceCompressor::WriteSliceHeader(u32 uiCurrSliceNum)
{
	m_pcH265Headers->GenSliceHeader(m_pcInputParam,
		m_pcImageParam,uiCurrSliceNum,m_pcSliceHeaderBitStreamHandler);
}

void H265SliceCompressor::WriteTilesEntryPointInSliceHeader()
{
	u32 uiNumEntryPointOffsets = m_uiTotalTiles-1;
	u32 *uiEntryPointOffsets = new u32[uiNumEntryPointOffsets];

	for(u32 i=0;i<uiNumEntryPointOffsets;i++)
		uiEntryPointOffsets[i] = u32(m_ppcBitStreamHandler[i]->GetTotalBytesWritten());
	
	m_pcH265Headers->WriteTilesEntryPointsInSliceHeader(uiNumEntryPointOffsets,
		uiEntryPointOffsets,m_pcSliceHeaderBitStreamHandler);
	
	delete [] uiEntryPointOffsets;
}

void H265SliceCompressor::CatTileBitStreamHandlers()
{
	// If there are more than 1 tiles, then slice header information is written at a different
	// bitstream handler. We copy this information to the first tile bitstream handler
	if(m_uiTotalTiles > 1)
	{
		MAKE_SURE(m_ppcBitStreamHandler[0] != m_pcSliceHeaderBitStreamHandler,
			"Error: Something went wrong while assigning the slice header bitstream handler");
		m_pcSliceHeaderBitStreamHandler->SetNextBitStreamHandler(m_ppcBitStreamHandler[0]);
		m_ppcBitStreamHandler[0]->SetPrevBitStreamHandler(m_pcSliceHeaderBitStreamHandler);
	}

	for(u32 i=0;i<m_uiTotalTiles-1;i++)
	{
		m_ppcBitStreamHandler[i]->SetNextBitStreamHandler(m_ppcBitStreamHandler[i+1]);
		m_ppcBitStreamHandler[i+1]->SetPrevBitStreamHandler(m_ppcBitStreamHandler[i]);
	}
}

void H265SliceCompressor::FixZeroTermination()
{
	m_ppcBitStreamHandler[m_uiTotalTiles-1]->FixZeroTermination();
}

/**
*	Compress a Tile using threads
*	Will only be called if USE_THREADS is enabled
*/
static void *CompressTileThread(void *pArgs)
{
	TileJobArgs_t *pcArgs = (TileJobArgs_t *)pArgs;
	H265TileCompressor *pcTileCompressor = pcArgs->pcTileCompressor;
	printf("Trace: Thread for tile %d.\n",pcArgs->iNum);
	pcTileCompressor->CompressTile(pcArgs->pbYBuff, pcArgs->pbCbBuff, pcArgs->pbCrBuff, pcArgs->pcCabac, pcArgs->pcBitStreamHandler);
	return NULL;
}

void H265SliceCompressor::CompressSlice(byte *pbYBuff, byte *pbCbBuff, byte *pbCrBuff,
										u32 uiCurrSliceNum)
{
	m_ctTimeForSlice = GetTimeInMiliSec();
	m_eSliceType = m_pcImageParam->m_eSliceType;
	InitialToCompression();
	WriteSliceHeader(uiCurrSliceNum);
	// Compress each Tile individually
#if(USE_THREADS)
	// Proces all tile except for the last one
	for(u32 i=0;i<m_uiTotalTiles-1;i++)
	{
		PREPARE_WORK_ITEM;
		// Push the current job in the queue
		MAKE_SURE(m_pcTileWorkQueue->AddToJob(m_ppcWorkItem[i]) == 0,
			"Error: No space in the workqueue.");
	}

	// Let the caller process one of the tiles (the last tile)
	printf("Job started for work item number %d\n",m_uiTotalTiles-1);
	m_ppcH265TileCompressor[m_uiTotalTiles-1]->CompressTile(pbYBuff, pbCbBuff, pbCrBuff, 
		m_ppcCabac[m_uiTotalTiles-1],m_ppcBitStreamHandler[m_uiTotalTiles-1]);

	// Let the threads finish their job
	m_pcTileWorkQueue->WaitQueueEmpty();

	// Get time per tile
	for(u32 i=0;i<m_uiTotalTiles;i++)
		m_pcTimePerTile[i] = m_ppcH265TileCompressor[i]->GetTimePerTile();
		
#else
	for(u32 i=0;i<m_uiTotalTiles;i++)
	{
		m_ppcH265TileCompressor[i]->CompressTile(pbYBuff, pbCbBuff, pbCrBuff, m_ppcCabac[i],m_ppcBitStreamHandler[i]);
		m_pcTimePerTile[i] = m_ppcH265TileCompressor[i]->GetTimePerTile();
	}
#endif
	for(u32 i=0;i<m_uiTotalTiles;i++)
	{
		m_pu64TotalBytesPerTile[i] = m_ppcBitStreamHandler[i]->GetTotalBytesWritten();
		MAKE_SURE((m_pu64TotalBytesPerTile[i] < m_ppcBitStreamHandler[i]->GetTotalBytesAllocate()),
			"Error: Bitstream Buffer overflow detected");
		if(m_pcInputParam->m_bVerbose)
			printf("Trace: Tile %u encoded in %u msec.\n",i,m_ppcH265TileCompressor[i]->GetTimePerTile());
		m_u64TotalBytesPerSlice += m_ppcBitStreamHandler[i]->GetTotalBytesWritten();
	}

	if(m_uiTotalTiles > 1)	// If more than 1 tiles per frame
	{
		// Encode the tile entry points in the bitstream
		WriteTilesEntryPointInSliceHeader();
		m_u64TotalBytesPerSlice += m_pcSliceHeaderBitStreamHandler->GetTotalBytesWritten();

		// Concatenate tiles' bitstreams into a linked list
		CatTileBitStreamHandlers();
	}

	FixZeroTermination();
	m_ctTimeForSlice = GetTimeInMiliSec() - m_ctTimeForSlice;
}