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
* @file EncTop.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods used in the EncTop class.
*/

#include <H265GOPCompressor.h>
#include <H265SliceCompressor.h>
#include <Defines.h>
#include <EncTop.h>
#include <InputParameters.h>
#include <ImageParameters.h>
#include <BitStreamHandler.h>
#include <H265Headers.h>
#include <TypeDefs.h>
#include <Utilities.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <iostream>
#include <fstream>

using namespace std;

EncTop::EncTop(int argc, char *argv[])
{
	m_pcInputParam = new InputParameters;
	m_pcImageParam = new ImageParameters;

	m_iNumInputArgs = argc;
	m_ppcInputArgs = argv;

	// Configure the encoder
	ConfigureEncoder();

	// Initialize image properties and store them
	m_pcImageParam->InitImgProp(m_pcInputParam);
	
	// Allocate memory to the buffers
	InitEncoder();

	// Open the IO files
	OpenIOFiles();

	return;
}

EncTop::~EncTop()
{
	// Close the files
	CloseIOFiles();

	// Free the memories
	FreeAllocBuff();
}

void EncTop::ConfigureEncoder()
{
	m_u64CurrFrameNum = 0;
	i32 gopsize = 0;
	i32 inputqp = 0;
	i32 gopthreads = 0;
	i32 slicethreads = 0;
	i32 totaltiles = 0;
	i32 totaltilecols = 0;
	i32 totaltilerows = 0;
	i32 framerate = 0;
	i32 tilethreads = 1;
	m_bOutputRec = false;
	m_bStats = false;
	m_uiTotalCores = 1;
	bool verbose = false;

	for(i32 i=1;i<m_iNumInputArgs;i++)
	{
		if(!(strcmp(m_ppcInputArgs[i], "-i")))
		{
			// File names
			strcpy(m_pcInputParam->m_cInputYuvName, m_ppcInputArgs[++i]);
			i32 filenamelen = strlen(m_pcInputParam->m_cInputYuvName);
			strcpy(m_pcInputParam->m_cRecYuvName,"");
			strncat(m_pcInputParam->m_cRecYuvName, m_pcInputParam->m_cInputYuvName, filenamelen - 4);
			strcat(m_pcInputParam->m_cRecYuvName, "_HEVCRecon.yuv");	// Output reconstructed image
		}
	
		else if(!(strcmp(m_ppcInputArgs[i], "-w")))
		{
			// Image resolution
			m_pcInputParam->m_uiFrameWidth = atoi(m_ppcInputArgs[++i]);  //	image uiWidth  (must be a multiple of 16 pels)
		}

		else if(!(strcmp(m_ppcInputArgs[i], "-h")))
		{
			// Image resolution
			m_pcInputParam->m_uiFrameHeight = atoi(m_ppcInputArgs[++i]); //	image height (must be a multiple of 16 pels)
		}

		else if(!(strcmp(m_ppcInputArgs[i], "-gop")))
		{
			gopsize = atoi(m_ppcInputArgs[++i]);	// GOP size
		}

		else if(!(strcmp(m_ppcInputArgs[i], "-Nframes")))
		{
			m_pcInputParam->m_uiNumFrames = atoi(m_ppcInputArgs[++i]);  //	number of frames to be encoded
		}

		else if(!(strcmp(m_ppcInputArgs[i], "-fps")))
		{
			framerate = atoi(m_ppcInputArgs[++i]); //	frame rate of the m_pcInputParam
		}

		else if(!(strcmp(m_ppcInputArgs[i], "-QP")))
		{
			inputqp = atoi(m_ppcInputArgs[++i]);
		}

		else if(!(strcmp(m_ppcInputArgs[i], "-Ngopth")))
		{
			gopthreads = atoi(m_ppcInputArgs[++i]);
		}

		else if(!(strcmp(m_ppcInputArgs[i], "-Nsliceth")))
		{
			slicethreads = atoi(m_ppcInputArgs[++i]);
		}

		else if(!(strcmp(m_ppcInputArgs[i], "-Ntiles")))
		{
			totaltiles = atoi(m_ppcInputArgs[++i]);
			totaltilecols = atoi(m_ppcInputArgs[++i]);
			totaltilerows = atoi(m_ppcInputArgs[++i]);
		}

		else if(!(strcmp(m_ppcInputArgs[i], "-Ntileth")))
		{
			tilethreads = atoi(m_ppcInputArgs[++i]);
		}

		else if(!(strcmp(m_ppcInputArgs[i], "--ver")))
		{
			verbose = true;
		}

		else if(!(strcmp(m_ppcInputArgs[i], "--rec")))
		{
			m_bOutputRec = true;
		}

		else if(!(strcmp(m_ppcInputArgs[i], "--stat")))
		{
			m_bStats = true;
		}

		else
		{
			printf("Error: Unknown input argument %s.\n",m_ppcInputArgs[i]);
			exit(EXIT_FAILURE);
		}
	}

	MAKE_SURE((m_pcInputParam->m_uiFrameWidth % CTU_WIDTH == 0) && (m_pcInputParam->m_uiFrameHeight % CTU_HEIGHT == 0),
		"Input picture dimensions must be a multiple of CTU size");

	MAKE_SURE(CTU_HEIGHT == CTU_WIDTH,"The CTU must be a square. Check definitions of CTU_HEIGHT and CTU_WIDTH");

	m_pcInputParam->m_bVerbose = verbose;
	if(verbose)
	{
		printf("Trace: CES_H265 Version [%s].\n",CES_H265_VER);
		printf("Trace: Build date [%s].\n",__DATE__);
		printf("Trace: [%s]-bits on [%s].\n",sizeof(void*)==8?"64":"32",CES_H265_OS);
		printf("Trace: Input file %s.\n",m_pcInputParam->m_cInputYuvName);
		if(m_bOutputRec) printf("Trace: Reconstructed file %s.\n",m_pcInputParam->m_cRecYuvName);
		printf("Trace: Image resolution %d x %d.\n",m_pcInputParam->m_uiFrameWidth,m_pcInputParam->m_uiFrameHeight);
		printf("Trace: CTU resolution %d x %d.\n",CTU_WIDTH,CTU_HEIGHT);
		printf("Trace: Number of frames %d.\n",m_pcInputParam->m_uiNumFrames);
	}

	// Gop size
	m_pcInputParam->m_uiGopSize = gopsize < 1 || gopsize > 7 ? INIT_GOP_SIZE : gopsize;
	if(gopsize < 1 || gopsize > 7) printf("Warning: GOP size being set to %d.\n",m_pcInputParam->m_uiGopSize);
	else if(verbose) printf("Trace: GOP size %d.\n",m_pcInputParam->m_uiGopSize);
	MAKE_SURE((m_pcInputParam->m_uiNumFrames >= m_pcInputParam->m_uiGopSize),
		"Total number of frames should be at least equal to the GOP size");

	MAKE_SURE(m_pcInputParam->m_uiGopSize == 1, "Error: Currently, GOP size of only 1 is acceptable.");

	// Frame rate
	m_pcInputParam->m_iFrameRate = framerate < 1 ? INIT_FRAME_RATE : framerate;
	if(framerate < 1) printf("Warning: Frame rate being set to %d.\n",m_pcInputParam->m_iFrameRate);
	else if(verbose) printf("Trace: Frame rate %d.\n",m_pcInputParam->m_iFrameRate);

	// QP
	m_pcInputParam->m_uiQP = inputqp > 0 && inputqp < 52 ? inputqp : INIT_QP; //	QP of first frame
	if(inputqp <= 0 || inputqp >= 52)	printf("Warning: QP value being set to %d.\n",INIT_QP);
	else if(verbose) printf("Trace: QP value %u.\n",m_pcInputParam->m_uiQP);

	// Threads
	// GOP threads
	m_pcInputParam->m_uiNumGOPThreads = gopthreads < 1 ? 1 : gopthreads;
	m_pcInputParam->m_uiNumGOPThreads = gopthreads > MAX_GOP_THREADS ? MAX_GOP_THREADS : m_pcInputParam->m_uiNumGOPThreads;
	if(gopthreads < 1 || gopthreads > MAX_GOP_THREADS) printf("Warning: Total GOP threads being set to %d.\n",m_pcInputParam->m_uiNumGOPThreads);
	else if(verbose) printf("Trace: Total GOP threads %d.\n",m_pcInputParam->m_uiNumGOPThreads);
	// Slice threads
	m_pcInputParam->m_uiNumSliceThreads = slicethreads < 1 ? 1 : slicethreads;
	m_pcInputParam->m_uiNumSliceThreads = slicethreads > MAX_SLICE_THREADS ? MAX_SLICE_THREADS : m_pcInputParam->m_uiNumSliceThreads;
	MAKE_SURE((m_pcInputParam->m_uiGopSize % m_pcInputParam->m_uiNumSliceThreads) == 0,
		"The GOP size must be completely divisble by the number of slice threads");
	if(slicethreads < 1 || slicethreads > MAX_SLICE_THREADS) printf("Warning: Total Slice threads being set to %d.\n",m_pcInputParam->m_uiNumSliceThreads);
	else if(verbose) printf("Trace: Total Slice threads %d.\n",m_pcInputParam->m_uiNumSliceThreads);

	MAKE_SURE(m_pcInputParam->m_uiNumSliceThreads == 1, "Error: Currently, only 1 slice thread is acceptable.");

	// Tiles
	m_pcInputParam->m_uiTilesPerFrame = totaltiles < 1 ? 1 : totaltiles;
	m_pcInputParam->m_uiTilesPerFrame = totaltiles > MAX_TILES ? MAX_TILES : m_pcInputParam->m_uiTilesPerFrame;
	m_pcInputParam->m_uiFrameWidthInTiles = totaltilecols;
	m_pcInputParam->m_uiFrameHeightInTiles = totaltilerows;
	if(totaltiles < 1 || totaltiles > MAX_TILES) 
	{
		if(m_pcInputParam->m_uiTilesPerFrame == 1)
		{
			m_pcInputParam->m_uiFrameWidthInTiles = 1;
			m_pcInputParam->m_uiFrameHeightInTiles = 1;
		}
		else
		{
			m_pcInputParam->m_uiFrameWidthInTiles = m_pcInputParam->m_uiTilesPerFrame/2;
			m_pcInputParam->m_uiFrameHeightInTiles = m_pcInputParam->m_uiTilesPerFrame/2;
		}
		printf("Warning: Total Tiles per frame being set to %d.\n",m_pcInputParam->m_uiTilesPerFrame);
	}
	else if(verbose) printf("Trace: Tiles_per_frame Frame_width_tiles Frame_height_tiles %d %d %d.\n",m_pcInputParam->m_uiTilesPerFrame,
		m_pcInputParam->m_uiFrameWidthInTiles, m_pcInputParam->m_uiFrameHeightInTiles);
	MAKE_SURE(m_pcInputParam->m_uiTilesPerFrame == (m_pcInputParam->m_uiFrameWidthInTiles*m_pcInputParam->m_uiFrameHeightInTiles),
		"Error: The total tiles do not match the frame width in tiles and frame height in tiles");

	// Tile threads
	m_pcInputParam->m_uiNumTileThreads = tilethreads < 1 ? 1 : tilethreads;
	m_pcInputParam->m_uiNumTileThreads = tilethreads > MAX_TILE_THREADS ? MAX_TILE_THREADS : m_pcInputParam->m_uiNumTileThreads;
	if(tilethreads < 1 || tilethreads > MAX_TILE_THREADS) printf("Warning: Total Tile threads being set to %d.\n",m_pcInputParam->m_uiNumTileThreads);
	else if(verbose) printf("Trace: Total Tile threads %d.\n",m_pcInputParam->m_uiNumTileThreads);

	m_pfPSNRPerFrame[0] = new f32[m_pcInputParam->m_uiNumFrames];	// Y PSNR
	m_pfPSNRPerFrame[1] = new f32[m_pcInputParam->m_uiNumFrames];	// Cb PSNR
	m_pfPSNRPerFrame[2] = new f32[m_pcInputParam->m_uiNumFrames];	// Cr PSNR
	m_pu64BytesPerFrame = new u64[m_pcInputParam->m_uiNumFrames];
}

void EncTop::InitEncoder()
{
	// This depends upon the total GOP and slice threads
	// Each GOP thread has separate slice threads
	// In this project, a slice equals a full frame
	m_pppcYBuff = new byte**[m_pcInputParam->m_uiNumGOPThreads];
	m_pppcCbBuff = new byte**[m_pcInputParam->m_uiNumGOPThreads];
	m_pppcCrBuff = new byte**[m_pcInputParam->m_uiNumGOPThreads];
	m_ppppcStreamHandler = new BitStreamHandler***[m_pcInputParam->m_uiNumGOPThreads];
	m_ppcH265GOPCompressor = new H265GOPCompressor*[m_pcInputParam->m_uiNumGOPThreads];
	for(u32 i=0;i<m_pcInputParam->m_uiNumGOPThreads;i++)
	{
		m_pppcYBuff[i] = new byte*[m_pcInputParam->m_uiGopSize];
		m_pppcCbBuff[i] = new byte*[m_pcInputParam->m_uiGopSize];
		m_pppcCrBuff[i] = new byte*[m_pcInputParam->m_uiGopSize];
		m_ppppcStreamHandler[i] = new BitStreamHandler**[m_pcInputParam->m_uiGopSize];		
		for(u32 j=0;j<m_pcInputParam->m_uiGopSize;j++)
		{
			m_pppcYBuff[i][j] = new byte[m_pcImageParam->m_uiFramePelsLuma];
			m_pppcCbBuff[i][j] = new byte[m_pcImageParam->m_uiFramePelsChroma];
			m_pppcCrBuff[i][j] = new byte[m_pcImageParam->m_uiFramePelsChroma];	
			m_ppppcStreamHandler[i][j] = new BitStreamHandler*[m_pcInputParam->m_uiTilesPerFrame];//(m_pcImageParam->m_uiFrameSizeInCTUs * 4096);	// Assume for the moment that a CTU will not take more than 4096 bytes
			for(u32 k=0;k<m_pcInputParam->m_uiTilesPerFrame;k++)
				m_ppppcStreamHandler[i][j][k] = new BitStreamHandler(m_pcImageParam->m_u64TotalBytesPerTile);
		}
		m_ppcH265GOPCompressor[i] = new H265GOPCompressor(m_pcInputParam,m_pcImageParam,m_ppppcStreamHandler[i]);	// This will create the whole chain of slice, tile and CTU encoders
	}
}

void EncTop::OpenIOFiles()
{
	m_ifsYUVFile.open(m_pcInputParam->m_cInputYuvName, ios::binary | ios::in);
	MAKE_SURE(m_ifsYUVFile.is_open(),"Error: Cannot open input YUV file.");

	if(m_bOutputRec)
	{
		m_fsYUVFileRec.open(m_pcInputParam->m_cRecYuvName, ios::binary | ios::in | ios::out);
		if(!(m_fsYUVFileRec.is_open()))	// File not available
		{
			// Make the file first
			m_fsYUVFileRec.open(m_pcInputParam->m_cRecYuvName, ios::binary | ios::trunc | ios::out);
			m_fsYUVFileRec.close();
			// Open the file again in read/write mode
			m_fsYUVFileRec.open(m_pcInputParam->m_cRecYuvName, ios::binary | ios::in | ios::out);
		}
		MAKE_SURE(m_fsYUVFileRec.is_open(),"Error: Cannot open Reconstructed YUV file.");
	}

	m_ofsBitStream.open("Video.h265", ios::binary | ios::out);
	MAKE_SURE(m_ofsBitStream.is_open(),"Error: Cannot open Bitstream file.");

	if(m_bStats)
	{
		m_ofsStats.open("Statistics.txt", ios::out);
		MAKE_SURE(m_ofsStats.is_open(),"Error: Cannot open Statistics file.");
		// Write initial
		m_ofsStats<<"CES_H265 Encoder (by Muhammad Usman Karim Khan @ CES, KIT)" << endl;
		m_ofsStats<<"CES_H265 Version "<< CES_H265_VER << endl;
		m_ofsStats<<"Build date " << __DATE__ << endl;
		m_ofsStats<< (sizeof(void*)==8?64:32) << "-bits on " << CES_H265_OS << endl;
		i8 piBuff[80];
		GetCurrentDateTime(piBuff,80);
		m_ofsStats<<"File generated on "<< piBuff << endl;
		m_ofsStats<<"Input file: "<< m_pcInputParam->m_cInputYuvName << endl;
		m_ofsStats<<"Image resolution: " << m_pcInputParam->m_uiFrameWidth << "x" << m_pcInputParam->m_uiFrameHeight << endl;
		m_ofsStats<<"Number of frames: " << m_pcInputParam->m_uiNumFrames << endl;
		m_ofsStats<<"GOP size: " << m_pcInputParam->m_uiGopSize << endl;
		m_ofsStats<<"Frame rate: " << m_pcInputParam->m_iFrameRate << endl;
		m_ofsStats<<"QP: " << m_pcInputParam->m_uiQP << endl;
		m_ofsStats<<"Total GOP threads: " << m_pcInputParam->m_uiNumGOPThreads << endl;
		m_ofsStats<<"Total slice threads: " << m_pcInputParam->m_uiNumSliceThreads << endl;
		m_ofsStats<<"Total tiles per frame: " << m_pcInputParam->m_uiTilesPerFrame << endl;
		m_ofsStats<<"Frame width in tiles: " << m_pcInputParam->m_uiFrameWidthInTiles << endl;
		m_ofsStats<<"Frame height in tiles: " << m_pcInputParam->m_uiFrameHeightInTiles << endl;
		m_ofsStats<<"Data is written in the following format" << endl;
		m_ofsStats<<"GOP_Number GOP_Bytes Frame_Number Frame_Bytes Frame_Time Tile_Bytes Tile_Time"<< endl;
	}
}

void EncTop::Encode()
{
	u32 uiCurrTime = GetTimeInMiliSec();
	// Write the VPS, SPS and PPS
	u64 uiTotalPSBytes = WritePS();
	if(m_pcInputParam->m_bVerbose)
		printf("Trace: Headers encoded with total %llu bytes.\n",uiTotalPSBytes);

	// Start encoding the rest of the frames
	for(u32 i=0;i<m_pcInputParam->m_uiNumFrames/m_pcInputParam->m_uiNumGOPThreads/m_pcInputParam->m_uiGopSize;i++)
	{
		// Compress one GOP at a time
		for(u32 j=0;j<m_pcInputParam->m_uiNumGOPThreads;j++)
		{
			// For every GOP, read exactly Num Slice threads frames and compress these frames
			// Read input files
			FillGOPBuffFromYUV(j);
			m_pcImageParam->m_eSliceType = I_SLICE;

			// Start compression
			m_ppcH265GOPCompressor[j]->CompressGOP(m_pppcYBuff[j], m_pppcCbBuff[j], m_pppcCrBuff[j],
				i*m_pcInputParam->m_uiNumGOPThreads+j);

			// Write the bitstream
			// For each slice of the GOP, there is one or more Tiles
			// and we write per-tile
			m_u64CurrGOPBytes = 0;
			m_pu64BytesPerFrame[i*m_pcInputParam->m_uiNumGOPThreads+j] = 0;
			for(u32 k=0;k<m_pcInputParam->m_uiGopSize;k++)
			{
				BitStreamHandler *pcBitStreamHandler = m_ppcH265GOPCompressor[j]->GetSliceBitStreamHandler(k);
				u64 u64TotalSliceBytes = 0;
				// Loop over slice headers and tiles (if present)
				do
				{
					u64TotalSliceBytes += WriteBitstreamFile(pcBitStreamHandler);
					pcBitStreamHandler = pcBitStreamHandler->GetNextBitStreamHandler();
				}while(pcBitStreamHandler);	// If there is a next bitstream allocated for the tile
				m_pu64BytesPerFrame[i*m_pcInputParam->m_uiNumGOPThreads+j] += u64TotalSliceBytes;
				m_u64CurrGOPBytes += u64TotalSliceBytes;
			}
			if(m_pcInputParam->m_bVerbose)
				printf("Trace: GOP %d encoded with total %llu bytes.\n",i,m_u64CurrGOPBytes);

			// Dump the stats
			if(m_bStats)
				DumpStats(j,i*m_pcInputParam->m_uiNumGOPThreads+j);

			// Write reconstructed GOP
			if(m_bOutputRec)
				WriteGOPBuffToYUV(j);
		}
	}
	uiCurrTime = GetTimeInMiliSec() - uiCurrTime;
	printf("Trace: Total encoding time is %u msec.\n",uiCurrTime);
}

u64 EncTop::WritePS()
{
	u64 u64TotalBytes = 0;
	H265Headers *pcHeader = new H265Headers;
	// We write the SPS and PPS headers using the first buffers
	BitStreamHandler *pcBitStreamHandler = m_ppppcStreamHandler[0][0][0];
	pcHeader->GenVPSNALU(m_pcInputParam,m_pcImageParam,pcBitStreamHandler);
	u64TotalBytes += WriteBitstreamFile(pcBitStreamHandler);
	pcHeader->GenSPSNALU(m_pcInputParam,m_pcImageParam,pcBitStreamHandler);
	u64TotalBytes += WriteBitstreamFile(pcBitStreamHandler);
	pcHeader->GenPPSNALU(m_pcInputParam,m_pcImageParam,pcBitStreamHandler);
	u64TotalBytes += WriteBitstreamFile(pcBitStreamHandler);
	delete pcHeader;
	return u64TotalBytes;
}

u64 EncTop::WriteBitstreamFile(BitStreamHandler *pcBitStreamHandler)
{
	u64 u64TotalBytes = pcBitStreamHandler->GetTotalBytesWritten();
	if(m_ofsBitStream.good())
		m_ofsBitStream.write((i8 *)pcBitStreamHandler->GetBitStreamBuffer(),u64TotalBytes);
	return u64TotalBytes;
}

void EncTop::FillGOPBuffFromYUV(i32 iGopNum)
{
	for(u32 i=0;i<m_pcInputParam->m_uiGopSize;i++)
	{
		if(m_ifsYUVFile.good())
		{
			m_ifsYUVFile.read((i8 *)m_pppcYBuff[iGopNum][i],m_pcImageParam->m_uiFramePelsLuma);
			m_ifsYUVFile.read((i8 *)m_pppcCbBuff[iGopNum][i],m_pcImageParam->m_uiFramePelsChroma);
			m_ifsYUVFile.read((i8 *)m_pppcCrBuff[iGopNum][i],m_pcImageParam->m_uiFramePelsChroma);
		}
	}
}

void EncTop::WriteGOPBuffToYUV(i32 iGopNum)
{
	for(u32 i=0;i<m_pcInputParam->m_uiGopSize;i++)
	{
		if(m_fsYUVFileRec.good())
		{
			m_fsYUVFileRec.write((i8 *)m_pppcYBuff[iGopNum][i],m_pcImageParam->m_uiFramePelsLuma);
			m_fsYUVFileRec.write((i8 *)m_pppcCbBuff[iGopNum][i],m_pcImageParam->m_uiFramePelsChroma);
			m_fsYUVFileRec.write((i8 *)m_pppcCrBuff[iGopNum][i],m_pcImageParam->m_uiFramePelsChroma);
		}
	}
}

void EncTop::DumpStats(i32 iGopNum, u32 uiGopStartFrameNum)
{
	m_ofsStats << uiGopStartFrameNum;
	m_ofsStats << "\t" << m_u64CurrGOPBytes;
	for(u32 i=0;i<m_pcInputParam->m_uiGopSize;i++)
		m_ofsStats << "\t" << uiGopStartFrameNum+i << "\t" <<m_ppcH265GOPCompressor[iGopNum]->GetSliceCompressor(i)->GetTotalBytes();
	for(u32 i=0;i<m_pcInputParam->m_uiGopSize;i++)
		m_ofsStats << "\t" << m_ppcH265GOPCompressor[iGopNum]->GetSliceCompressor(i)->GetTimePerSlice();
	for(u32 i=0;i<m_pcInputParam->m_uiGopSize;i++)
	{
		u64 *pcBytesPerTile = m_ppcH265GOPCompressor[iGopNum]->GetSliceCompressor(i)->GetBytesPerTile();
		for(u32 j=0;j<m_pcInputParam->m_uiTilesPerFrame;j++)
			m_ofsStats << "\t" << pcBytesPerTile[j];
	}
	for(u32 i=0;i<m_pcInputParam->m_uiGopSize;i++)
	{
		u32 *pcTimePerTile = m_ppcH265GOPCompressor[iGopNum]->GetSliceCompressor(i)->GetTimePerTile();
		for(u32 j=0;j<m_pcInputParam->m_uiTilesPerFrame;j++)
			m_ofsStats << "\t" << pcTimePerTile[j];
	}
	m_ofsStats << endl;
}

void EncTop::FreeAllocBuff()
{
	for(u32 i=0;i<m_pcInputParam->m_uiNumGOPThreads;i++)
	{
		for(u32 j=0;j<m_pcInputParam->m_uiGopSize;j++)
		{
			delete [] m_pppcYBuff[i][j] ;
			delete [] m_pppcCbBuff[i][j];
			delete [] m_pppcCrBuff[i][j];
			for(u32 k=0;k<m_pcInputParam->m_uiTilesPerFrame;k++)
				delete m_ppppcStreamHandler[i][j][k];
			delete [] m_ppppcStreamHandler[i][j];
		}
		delete [] m_pppcYBuff[i];
		delete [] m_pppcCbBuff[i];
		delete [] m_pppcCrBuff[i];
		delete [] m_ppppcStreamHandler[i];
		delete m_ppcH265GOPCompressor[i];
	}

	delete [] m_pppcYBuff;
	delete [] m_pppcCbBuff;
	delete [] m_pppcCrBuff;
	delete [] m_ppppcStreamHandler;
	delete [] m_ppcH265GOPCompressor;
	delete [] m_pfPSNRPerFrame[0];
	delete [] m_pfPSNRPerFrame[1];
	delete [] m_pfPSNRPerFrame[2];
	delete [] m_pu64BytesPerFrame;

	delete m_pcInputParam;
	delete m_pcImageParam;
}

void EncTop::CloseIOFiles()
{
	if(m_ifsYUVFile.is_open()) m_ifsYUVFile.close();
	if(m_bOutputRec && m_fsYUVFileRec.is_open()) m_fsYUVFileRec.close();
	if(m_ofsBitStream.is_open()) m_ofsBitStream.close();
	if(m_ofsStats.is_open()) m_ofsStats.close();
}

void EncTop::PrintPSNR(bool bLumaOnly)
{
	if(m_bOutputRec)
	{
		// We read one frame at a time and compute its PSNR

		byte *pbCurrFrameY = m_pppcYBuff[0][0];
		byte *pbCurrFrameCb = m_pppcCbBuff[0][0];
		byte *pbCurrFrameCr = m_pppcCrBuff[0][0];
		byte *pbRecFrameY = new byte[m_pcImageParam->m_uiFramePelsLuma];
		byte *pbRecFrameCb = new byte[m_pcImageParam->m_uiFramePelsChroma];
		byte *pbRecFrameCr = new byte[m_pcImageParam->m_uiFramePelsChroma];

		f32 fAvgPSNR;
		f32 fAvgBitrate;
		f32 fAvgPSNRY = 0.0;
		f32 fAvgPSNRCb = 0.0;
		f32 fAvgPSNRCr = 0.0;
		f32 fAvgBytesPerFrame = 0.0;
		f32 fPSNRPerFrameY = 0.0;
		f32 fPSNRPerFrameCb = 0.0;
		f32 fPSNRPerFrameCr = 0.0;

		if(m_pcInputParam->m_bVerbose)
			printf("Frame\t\tBytes\t\tPSNR_Y\t\tPSNR_Cb\t\tPSNR_Cr\n");

		// Restart the original YUV file from the start
		if(m_ifsYUVFile.good())
			m_ifsYUVFile.seekg(0,ios::beg);

		// Restart the reconstructed YUV file from the start
		if(m_fsYUVFileRec.good())
			m_fsYUVFileRec.seekg(0,ios::beg);

		for(u32 i=0;i<m_pcInputParam->m_uiNumFrames;i++)
		{
			// Read actual
			if(m_ifsYUVFile.good())
			{
				m_ifsYUVFile.read((i8 *)pbCurrFrameY ,m_pcImageParam->m_uiFramePelsLuma);
				m_ifsYUVFile.read((i8 *)pbCurrFrameCb,m_pcImageParam->m_uiFramePelsChroma);
				m_ifsYUVFile.read((i8 *)pbCurrFrameCr,m_pcImageParam->m_uiFramePelsChroma);
			}

			// Read reconstructed
			if(m_fsYUVFileRec.good())
			{
				m_fsYUVFileRec.read((i8 *)pbRecFrameY ,m_pcImageParam->m_uiFramePelsLuma);
				m_fsYUVFileRec.read((i8 *)pbRecFrameCb,m_pcImageParam->m_uiFramePelsChroma);
				m_fsYUVFileRec.read((i8 *)pbRecFrameCr,m_pcImageParam->m_uiFramePelsChroma);
			}

			// Generate PSNR
			fPSNRPerFrameY = PSNROneFrame(pbCurrFrameY, pbRecFrameY, m_pcImageParam->m_uiFramePelsLuma);
			fAvgPSNRY += fPSNRPerFrameY;

			if(!bLumaOnly)	// Also use chroma components for computing PSNR
			{
				fPSNRPerFrameCb = PSNROneFrame(pbCurrFrameCb, pbRecFrameCb, m_pcImageParam->m_uiFramePelsChroma);
				fAvgPSNRCb += fPSNRPerFrameCb;

				fPSNRPerFrameCr = PSNROneFrame(pbCurrFrameCr, pbRecFrameCr, m_pcImageParam->m_uiFramePelsChroma);
				fAvgPSNRCr += fPSNRPerFrameCr;
			}

			fAvgBytesPerFrame += f32(m_pu64BytesPerFrame[i]);
			m_pfPSNRPerFrame[0][i] = fPSNRPerFrameY;
			m_pfPSNRPerFrame[1][i] = fPSNRPerFrameCb;
			m_pfPSNRPerFrame[2][i] = fPSNRPerFrameCr;

			if(m_pcInputParam->m_bVerbose)
				printf("%u\t\t%llu\t\t%f\t%f\t%f\n",i,m_pu64BytesPerFrame[i],fPSNRPerFrameY,fPSNRPerFrameCb,fPSNRPerFrameCr);
		}

		if(!bLumaOnly)
			fAvgPSNR = f32((4*fAvgPSNRY + fAvgPSNRCb + fAvgPSNRCr)/(m_pcInputParam->m_uiNumFrames*6.0));
		else
			fAvgPSNR = f32(fAvgPSNRY/(m_pcInputParam->m_uiNumFrames*1.0));

		fAvgBitrate = f32(fAvgBytesPerFrame*m_pcInputParam->m_iFrameRate/f32(m_pcInputParam->m_uiNumFrames)/1024.0);

		printf("Average PSNR [dB] = %f \t Average Byte-rate [KBps] = %f.\n",fAvgPSNR,fAvgBitrate);

		delete [] pbRecFrameY;
		delete [] pbRecFrameCb;
		delete [] pbRecFrameCr;

		// Print PSNRs to a file
		ofstream ofsPSNR;
		ofsPSNR.open("RD.txt",ios::out);
		ofsPSNR << "Frame\tKbytes\tY_PSNR\tCb_PSNR\tCr_PSNR" << endl;
		if(ofsPSNR.good())
		{
			for(u32 i=0;i<m_pcInputParam->m_uiNumFrames;i++)
				ofsPSNR << i << "\t" 
						<< f64(m_pu64BytesPerFrame[i])/1024.0 << "\t"
						<< m_pfPSNRPerFrame[0][i] << "\t"
						<< m_pfPSNRPerFrame[1][i] << "\t"
						<< m_pfPSNRPerFrame[2][i] << endl;
			ofsPSNR << "Avg\t" 
				<< fAvgBitrate << "\t" 
				<< fAvgPSNRY/f32(m_pcInputParam->m_uiNumFrames) << "\t" 
				<< fAvgPSNRCb/f32(m_pcInputParam->m_uiNumFrames) << "\t" 
				<< fAvgPSNRCr/f32(m_pcInputParam->m_uiNumFrames) << endl;
		}
		ofsPSNR.close();
	}
	else
		printf("Warning: Enable the reconstructed output generation for PSNR.\n");
}

f32 EncTop::PSNROneFrame(byte *pbCurrFrame, byte *pbRecFrame, u32 uiSize)
{
	f32 fPsnr = 0.0;
	f32 fDiff = 0.0;
	f32 fMeanDiff = 0.0;

	for(u32 i=0;i<uiSize;i++)
		fDiff += (pbCurrFrame[i]-pbRecFrame[i])*(pbCurrFrame[i]-pbRecFrame[i]);

	fMeanDiff = f32(fDiff/(1.0*uiSize));

	fPsnr = f32(10*log10(PSNR_NUMERATOR/fMeanDiff));

	return fPsnr;
}
