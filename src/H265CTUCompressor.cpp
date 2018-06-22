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
* @file H265CTUCompressor.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods that handles image compression functions.
*/

#include <InputParameters.h>
#include <ImageParameters.h>
#include <H265Transform.h>
#include <BitStreamHandler.h>
#include <H265CTUCompressor.h>
#include <Cabac.h>
#include <Utilities.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cassert>

/**
*	See 8.4.3.1.2.
*	1 denotes filtering takes place and 0 denotes filtering doesn't take place.
*	Index:			   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34
*	minDistHorVer:     10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8
*/
const u8 g_pbIntraFilterUsage[5][35] = {
	/*  4x4  @ 10 */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	/*  8x8  @ 7  */ { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	/* 16x16 @ 1  */ { 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
	/* 32x32 @ 0  */ { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
	/* 64x64 @ 10 */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

/**
*	See 8.4.3.1.6 Table 8-5.
*	A 0 is invalid.
*	For mode 0 (planar) and 1 (DC), there is nothing of the sort. For mode 10 (HOR) and 26 (VER), there is also a 0, which means that we
*	need to further process the output.
*/
const i8 g_pbIntraPredAngleFromMode[TOTAL_INTRA_MODES-1] = {
	 0, 0, 32, 26, 21, 17, 13, 9, 5, 2, 0, -2, -5, -9,-13,-17,-21,-26, -32,-26,-21,-17,-13, -9, -5, -2, 0, 2, 5, 9, 13, 17, 21, 26, 32
};

/**
*	See 8.4.3.1.6 Table 8-6.
*	A 0 is invalid.
*/
const i32 g_pbInvAngleFromMode[TOTAL_INTRA_MODES-1] = {
	0, 0, 256, 315, 390, 482, 630, 910, 1638, 4096, 0, 4096, 1638, 910, 630, 482, 390, 315, 256, 315, 390, 482, 630, 910, 1638, 4096, 0, 4096, 1638, 910, 630, 482, 390, 315, 256
};

/**
*	Chroma QP determination from luma QP.
*	See Table 8-11 in 8.6.1 in the draft.
*/
const u8 g_pbChramaQPFromLuma[52] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 
  14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36, 36,
  37, 37, 37, 38, 38, 38, 39, 39, 39, 39
};

// CTU
H265CTUCompressor::H265CTUCompressor(InputParameters const *pcInputParam, ImageParameters const *pcImageParam,
									 pixel cTileStartCTUPelTL, pixel cTileEndCTUPelTL)
{
	m_pcInputParam = pcInputParam;
	m_pcImageParam = pcImageParam;
	m_uiYStride = pcImageParam->m_uiFrameWidth;
	m_uiCStride = pcImageParam->m_uiFrameWidthChroma;
	m_uiQP = pcInputParam->m_uiQP;
	m_pcH265Trans = new H265Transform;

	m_cTileStartCTUPelTL.x = cTileStartCTUPelTL.x;
	m_cTileStartCTUPelTL.y = cTileStartCTUPelTL.y;
	m_cTileEndCTUPelTL.x = cTileEndCTUPelTL.x;
	m_cTileEndCTUPelTL.y = cTileEndCTUPelTL.y;
	
	m_uiTileWidthInPels = cTileEndCTUPelTL.x - cTileStartCTUPelTL.x + CTU_WIDTH;
	m_uiTileHeightInPels = cTileEndCTUPelTL.y - cTileStartCTUPelTL.y + CTU_HEIGHT;

	// CTU_WIDTH+1 added to eliminate invalid reads in reference generation
	m_pbRefTopBuffY = new byte[m_uiTileWidthInPels+CTU_WIDTH+1];
	m_pbRefTopBuffCb = new byte[(m_uiTileWidthInPels>>1)+(CTU_WIDTH>>1)+1];	
	m_pbRefTopBuffCr = new byte[(m_uiTileWidthInPels>>1)+(CTU_WIDTH>>1)+1];
	m_pbTopLineIntraModeInfoL = new u8[m_uiTileWidthInPels/MIN_CU_SIZE+1];

	m_pppbTempPred[0][0] = m_ppbTempPred4[0];
	m_pppbTempPred[0][1] = m_ppbTempPred8[0];
	m_pppbTempPred[0][2] = m_ppbTempPred16[0];
	m_pppbTempPred[0][3] = m_ppbTempPred32[0];
	m_pppbTempPred[0][4] = m_ppbTempPred64[0];

	m_pppbTempPred[1][0] = m_ppbTempPred4[1];
	m_pppbTempPred[1][1] = m_ppbTempPred8[1];
	m_pppbTempPred[1][2] = m_ppbTempPred16[1];
	m_pppbTempPred[1][3] = m_ppbTempPred32[1];
	m_pppbTempPred[1][4] = m_ppbTempPred64[1];

	// Initialize the mode buffer
	InitBuffersNewTile();
}

H265CTUCompressor::~H265CTUCompressor()
{
	delete m_pcH265Trans;
	delete [] m_pbRefTopBuffY;
	delete [] m_pbRefTopBuffCb;
	delete [] m_pbRefTopBuffCr;
	delete [] m_pbTopLineIntraModeInfoL;
}

void H265CTUCompressor::InitBuffersNewTile()
{
	memset(m_pbNeighIntraModeL,INVALID_MODE,sizeof(m_pbNeighIntraModeL));
	memset(m_pbIntraModeInfoL,INVALID_MODE,sizeof(m_pbIntraModeInfoL));
	memset(m_puiIntraModeInfoC,INVALID_MODE,sizeof(m_puiIntraModeInfoC));
	memset(m_pbTopLineIntraModeInfoL,INVALID_MODE,m_uiTileWidthInPels/MIN_CU_SIZE+1);
	m_bSavedLTY = 0;
	m_bSavedLTCb = 0;
	m_bSavedLTCr = 0;
}

void H265CTUCompressor::InitBuffersNewCTULine()
{
	// Clear the left mode buffers
	/*for(u32 i=0;i<TOT_PUS_LINE+1;i++)
	{
		m_pbNeighIntraModeL[i*(TOT_PUS_LINE+2)] = INVALID_MODE;
	}*/
	// Clear all the modes instead of only left modes
	// @todo Need to check this again
	memset(m_pbNeighIntraModeL,INVALID_MODE,sizeof(m_pbNeighIntraModeL));

	// Set the top left pixels
	/*m_pbRefTopBuffY[0] = m_pbRefTopBuffY[1];
	m_pbRefTopBuffCb[0] = m_pbRefTopBuffCb[1];
	m_pbRefTopBuffCr[0] = m_pbRefTopBuffCr[1];*/
}

void H265CTUCompressor::InitCUPointers(byte *& pbCurrIntraModeL, byte *& pbCurrRecY, byte *& pbCurrRecCb, byte *& pbCurrRecCr, 
									 i16 *& piCurrCoeffY, i16 *& piCurrCoeffCb, i16 *& piCurrCoeffCr, 
									 byte *& pbCurrRefTopBuffY, byte *& pbCurrRefTopBuffCb, byte *& pbCurrRefTopBuffCr,
									 byte *& pbCurrIntraModeInfoL, byte *& pbCurrIntraModePredInfoL, u16 *& puiCurrIntraModeInfoC,
									 u32 uiDispCTULeft, u32 uiDispCTUTop, u32 uiCTUOffsetXFromTile)
{
	// In the below, the term Curr pertains to the current location of the CU.
	// E.g. m_pbCurrRefTopBuffY is the pointer to the top reference just above the current CU
	// At start, the current mode should point to correct location of of the neighboring modes array
	// The neighboring modes array contains an additional row and column for the neighboring CTUs. 
	pbCurrIntraModeL = m_pbNeighIntraModeL + (TOT_PUS_LINE+2) + 1;	// Leave the top line and the left sample
	pbCurrIntraModeL += uiDispCTULeft/MIN_CU_SIZE + (uiDispCTUTop/MIN_CU_SIZE)*(TOT_PUS_LINE+2);	// Now we are correctly pointing to the location where we should start filling or from where to pick up the modes

	// Point correctly to the reconstructed Y pixels
	pbCurrRecY = m_pbRecY + 2;	// Leave the first 2 pixels
	pbCurrRecY += uiDispCTULeft + uiDispCTUTop*(CTU_WIDTH+2);	// Now go to the actual location in the CTU

	// Point correctly to the reconstructed Cb pixels
	pbCurrRecCb = m_pbRecCb + 1;	// Leave the first pixel
	pbCurrRecCb += (uiDispCTULeft>>1) + (uiDispCTUTop>>1)*(CTU_WIDTH/2+1);	// Now go to the actual location in the CTU

	// Point correctly to the reconstructed Cr pixels
	pbCurrRecCr = m_pbRecCr + 1;	// Leave the first pixel
	pbCurrRecCr += (uiDispCTULeft>>1) + (uiDispCTUTop>>1)*(CTU_WIDTH/2+1);	// Now go to the actual location in the CTU

	// Adjust to the correct storing coefficients location
	piCurrCoeffY = m_piCoeffY + uiDispCTULeft + uiDispCTUTop*CTU_WIDTH;
	piCurrCoeffCb = m_piCoeffCb + (uiDispCTULeft>>1) + (uiDispCTUTop>>1)*(CTU_WIDTH>>1);
	piCurrCoeffCr = m_piCoeffCr + (uiDispCTULeft>>1) + (uiDispCTUTop>>1)*(CTU_WIDTH>>1);

	// Adjust to the correct top reference array
	pbCurrRefTopBuffY = m_pbRefTopBuffY + 1;	// Leave the first pixel
	pbCurrRefTopBuffY += uiCTUOffsetXFromTile + uiDispCTULeft;	// Enter the offset from the tile start

	pbCurrRefTopBuffCb = m_pbRefTopBuffCb + 1;	// Leave the first pixel
	pbCurrRefTopBuffCb += (uiCTUOffsetXFromTile>>1) + (uiDispCTULeft>>1);	// Enter the offset from the tile start

	pbCurrRefTopBuffCr = m_pbRefTopBuffCr + 1;	// Leave the first pixel
	pbCurrRefTopBuffCr += (uiCTUOffsetXFromTile>>1) + (uiDispCTULeft>>1);	// Enter the offset from the tile start

	// Adjust to the correct mode information location
	pbCurrIntraModeInfoL = m_pbIntraModeInfoL + (TOT_PUS_LINE+1) + 1;
	pbCurrIntraModeInfoL += uiDispCTULeft/MIN_CU_SIZE + (uiDispCTUTop/MIN_CU_SIZE)*(TOT_PUS_LINE+1);
	pbCurrIntraModePredInfoL = m_pbIntraModePredInfoL + (TOT_PUS_LINE+1) + 1;
	pbCurrIntraModePredInfoL += uiDispCTULeft/MIN_CU_SIZE + (uiDispCTUTop/MIN_CU_SIZE)*(TOT_PUS_LINE+1);
	puiCurrIntraModeInfoC = m_puiIntraModeInfoC + uiDispCTULeft/MIN_CU_SIZE + (uiDispCTUTop/MIN_CU_SIZE)*TOT_PUS_LINE;
}

void H265CTUCompressor::InitRefPointers(u32 *puiRefPointArr, u32 uiSize)
{
	// Fill the reference offset address array
	/*   	------------------------
		   | 2  |	3	|	4	| 5 |
			------------------------
		   |	|
		   | 1  |
		   |	|
			----    Current CU
		   |	|
		   | 0	|
		   |	|
			----	*/
	puiRefPointArr[0] = 0;
	puiRefPointArr[1] = uiSize;
	puiRefPointArr[2] = uiSize<<1;
	puiRefPointArr[3] = (uiSize<<1)+1;
	puiRefPointArr[4] = (uiSize<<1)+uiSize+1;
	puiRefPointArr[5] = (uiSize<<2)+1; // For termination only
}

u8 H265CTUCompressor::CheckNeighAvail(u8 *pbTopAndLeftModes, byte *pbCurrIntraModeL, u32 uiSize)
{

	i32 iNeighModeStride = i32(uiSize/MIN_CU_SIZE);		// This must be an integer, as it is added with an integer (which can be negative). @todo Check for other similar sources of errors.

	// Determine the availability and modes of the neighboring PUs from the current PU
	pbTopAndLeftModes[VALID_LB]	=	pbCurrIntraModeL[iNeighModeStride*(TOT_PUS_LINE+2)-1];			// Left bottom
	pbTopAndLeftModes[VALID_L]	=	pbCurrIntraModeL[-1];										// Left 
	pbTopAndLeftModes[VALID_LT]	=	pbCurrIntraModeL[-1*(TOT_PUS_LINE+2)-1];						// Left top
	pbTopAndLeftModes[VALID_T]	=	pbCurrIntraModeL[-1*(TOT_PUS_LINE+2)];							// Top
	pbTopAndLeftModes[VALID_TR]	=	pbCurrIntraModeL[-1*(TOT_PUS_LINE+2)+iNeighModeStride];		// Top right

	// Assign the validity flag using bitwise operations
	u8 bValidNeigFlag = 0;
	bValidNeigFlag |= (pbTopAndLeftModes[VALID_LB]	!= INVALID_MODE ? 0x1	: 0);
	bValidNeigFlag |= (pbTopAndLeftModes[VALID_L]	!= INVALID_MODE ? 0x2	: 0);
	bValidNeigFlag |= (pbTopAndLeftModes[VALID_LT]	!= INVALID_MODE	? 0x4	: 0);
	bValidNeigFlag |= (pbTopAndLeftModes[VALID_T]	!= INVALID_MODE	? 0x8	: 0);
	bValidNeigFlag |= (pbTopAndLeftModes[VALID_TR]	!= INVALID_MODE	? 0x10	: 0);

	return bValidNeigFlag;
}

void H265CTUCompressor::GetCandModeListIntra(u8 *pbTopAndLeftModes, u32 uiDispCTUTop, u8 *pbCandModeListIntra)
{
	// Determine the dominant mode and fill the candidates
	// See 8.4.1 in the draft
	u8 ubTopMode	= (pbTopAndLeftModes[VALID_T] == INVALID_MODE || (uiDispCTUTop == 0) ? DC_MODE_IDX : pbTopAndLeftModes[VALID_T]);
	u8 ubLeftMode	= (pbTopAndLeftModes[VALID_L] == INVALID_MODE ? DC_MODE_IDX : pbTopAndLeftModes[VALID_L]);

	if(ubTopMode == ubLeftMode)
	{
		if(ubLeftMode < 2)	// We have non-angular modes
		{
			pbCandModeListIntra[0] = PLANAR_MODE_IDX;
			pbCandModeListIntra[1] = DC_MODE_IDX;
			pbCandModeListIntra[2] = VER_MODE_IDX;
		}
		else	// We have angular modes
		{
			// @todo Need to check this: Equation 8-18 to 8-20 in Draft 10 and 
			// 8-24 to 8-26 in Draft 7
			pbCandModeListIntra[0] = ubLeftMode;
			pbCandModeListIntra[1] = 2 + ((ubLeftMode+29)%32);	//((ubLeftMode-2-1)%32);
			pbCandModeListIntra[2] = 2 + ((ubLeftMode-1)%32);	//((ubLeftMode-2+1)%32);
		}
	}
	else
	{
		pbCandModeListIntra[0] = ubLeftMode;
		pbCandModeListIntra[1] = ubTopMode;
		if((ubLeftMode != PLANAR_MODE_IDX) && (ubTopMode != PLANAR_MODE_IDX))	// If none of these modes dominant modes are planar modes
			pbCandModeListIntra[2] = PLANAR_MODE_IDX;
		else if((ubLeftMode != DC_MODE_IDX) && (ubTopMode != DC_MODE_IDX))	// If none of these modes dominant modes are DC modes
			pbCandModeListIntra[2] = DC_MODE_IDX;
		else
			pbCandModeListIntra[2] = VER_MODE_IDX;
	}
}

void H265CTUCompressor::SubstituteReference(byte *puiRefArr, u32 *puiRefOffset, byte bValidFlag, u32 uiSize)
{
	u32 uiNeighPredChecked;	// The neighborhoods checked for the intra predictions with total 5 neighborhoods

	// From Bottom left to top right, search the reference samples array and determine where to start substitution
	for(uiNeighPredChecked = 0; uiNeighPredChecked < 5; uiNeighPredChecked++)
	{
		if(bValidFlag & (1 << uiNeighPredChecked))	// If the current neighborhood mode is available
			break;
	}

	// Now for the reference samples pertaining to the neighborhood block which is not available, substitute the pixels
	// @todo Check if this is only for the bottom left and left reference samples
	byte bSubstitutePel = puiRefArr[puiRefOffset[uiNeighPredChecked]];	// Copy first available pixel to the unavailable pixels
	for(u32 i=0;i<puiRefOffset[uiNeighPredChecked];i++)
		puiRefArr[i] = bSubstitutePel;

	// Now check other unavailable neighbors and substitute their reference samples
	// @todo Check if this is only for the top reference samples
	while(uiNeighPredChecked < 5)
	{
		if(!(bValidFlag & (1 << uiNeighPredChecked)))	// Neighbor not available
		{
			u32 uiRefOffset = puiRefOffset[uiNeighPredChecked];	// The location of the reference array from where the substitution should start
			u32 uiTotRefSubs = puiRefOffset[uiNeighPredChecked+1] - puiRefOffset[uiNeighPredChecked];	// Size of the subsitution
			bSubstitutePel = puiRefArr[uiRefOffset-1];		// Copy p[x-1,y] to unavailable pixels
			for(u32 i=0;i<uiTotRefSubs;i++)
				puiRefArr[uiRefOffset+i] = bSubstitutePel;
		}
		uiNeighPredChecked++;
	}
}

void H265CTUCompressor::GenReferenceSamplesIntra(byte *pbCurrRecY, byte *pbCurrRecCb, byte *pbCurrRecCr,
												 byte *pbCurrRefTopBuffY, byte *pbCurrRefTopBuffCb, byte *pbCurrRefTopBuffCr,
												 u32 *puiRefYOffset, u32 *puiRefCOffset,
												 u8 bValidFlag, u32 uiSize, u32 uiDispCTUTop, bit bIsChroma)
{
	if(bIsChroma)
	{
		if(bValidFlag)
		{
			// Copy the bottom left and left pixels
			for(i32 i=0;i<i32(uiSize<<1);i++)
			{
				m_pbRefCb[i] = pbCurrRecCb[-1+(i32(uiSize*2)-1-i)*(CTU_WIDTH/2+1)];
				m_pbRefCr[i] = pbCurrRecCr[-1+(i32(uiSize*2)-1-i)*(CTU_WIDTH/2+1)];
			}
			// Copy the top left, top and top right pixels
			// If this CU is in the top line of the current CTU, then we need to fetch the reference from the top line buffer, else, the reconstructed buffer
			// of the CTU will do.
			memcpy(&m_pbRefCb[uiSize<<1],(uiDispCTUTop == 0 ? pbCurrRefTopBuffCb-1 : &pbCurrRecCb[-1*(CTU_WIDTH/2+1)-1]), (uiSize<<1)+1);
			memcpy(&m_pbRefCr[uiSize<<1],(uiDispCTUTop == 0 ? pbCurrRefTopBuffCr-1 : &pbCurrRecCr[-1*(CTU_WIDTH/2+1)-1]), (uiSize<<1)+1);

			// We now need to substitute the reference samples, if there are some missing
			// See 8.4.3.1.1 in the draft
			SubstituteReference(m_pbRefCb,puiRefCOffset,bValidFlag,uiSize);
			SubstituteReference(m_pbRefCr,puiRefCOffset,bValidFlag,uiSize);
		}
		else
		{
			memset(m_pbRefCb,0x80,4*uiSize+1);
			memset(m_pbRefCr,0x80,4*uiSize+1);
		}
	}	// Chroma generation done
	else	// Luma
	{
		if(bValidFlag)	// There are neighboring modes available
		{
			// Fill the reference pixels in the array, starting from the bottom left to top right

			// Copy the bottom left and left pixels
			// @todo Need to check this. I am reading outside the array
			/*if(bValidFlag & (1<<VALID_LB))	// If the bottom left is available
			{
				for(i32 i=0;i<i32(uiSize);i++)
					m_pbRefYUnfiltered[i] = pbCurrRecY[-1+(uiSize*2-i-1)*(CTU_WIDTH+2)];
			}*/				
			for(i32 i=0;i<i32(uiSize<<1);i++)	// Copy the other prediction pixels
				m_pbRefYUnfiltered[i] = pbCurrRecY[-1+(i32(uiSize*2)-i-1)*(CTU_WIDTH+2)];	

			// Copy the top left, top and top right pixels
			// If this CU is in the top line of the current CTU, then we need to fetch the reference from the top line buffer, else, the reconstructed buffer
			// of the CTU will do.
			// @ todo This will read invalid data for the top-right-most PU of the tile, 
			// but it is made sure it will not impact the output (see SubstituteReference)
			memcpy(&m_pbRefYUnfiltered[uiSize*2],(uiDispCTUTop == 0 ? pbCurrRefTopBuffY-1 : &pbCurrRecY[-1*(CTU_WIDTH+2)-1]), (uiSize<<1)+1);

			// We now need to substitute the reference samples, if there are some missing
			// See 8.4.3.1.1 in the draft
			SubstituteReference(m_pbRefYUnfiltered,puiRefYOffset,bValidFlag,uiSize);

			// Filter the unfiltered samples
			// See 8.4.3.1.2 in the draft
			m_pbRefYFiltered[0] = m_pbRefYUnfiltered[0];				// Bottom left pixel
			m_pbRefYFiltered[4*uiSize] = m_pbRefYUnfiltered[4*uiSize];	// Top right pixel
			for(u32 i=1;i<4*uiSize;i++)
				m_pbRefYFiltered[i] = (m_pbRefYUnfiltered[i-1] + (m_pbRefYUnfiltered[i] << 1) + m_pbRefYUnfiltered[i+1] + 2) >> 2;
		}
		else	// No neighbors available
		{
			memset(m_pbRefYUnfiltered,0x80,sizeof(m_pbRefYUnfiltered));
			memset(m_pbRefYFiltered,0x80,sizeof(m_pbRefYFiltered));
		} 
	}	// Luma generation done
}

void H265CTUCompressor::GenerateReferenceSamplesChromaLM(byte *pbCurrRecY, byte *pbCurrRefTopBuffY, byte *pbLMRefTop,byte *pbLMRefLeft, u8 bValidFlag, u32 uiDispCTUTop, u32 uiSizeChroma)
{
	// See 8.4.3.1 in the draft
	bit bTopAvail = (bValidFlag & (1 << VALID_T)) != 0;
	bit bLeftAvail = (bValidFlag & (1 << VALID_L)) != 0;

	u32 uiT0, uiT1, uiT2;
	u32 uiL0, uiL1;

	// Get the top reconstructed array
	byte *pbTopBuffY = (uiDispCTUTop == 0 ? pbCurrRefTopBuffY : pbCurrRecY-(CTU_WIDTH+2));

	if(bValidFlag)
	{
		if(bTopAvail)
		{
			for(u32 i=0;i<uiSizeChroma;i++)
			{
				uiT0 = pbTopBuffY[(i<<1)-1];
				uiT1 = pbTopBuffY[i<<1];
				uiT2 = pbTopBuffY[(i<<1)+1];
				pbLMRefTop[i] = byte((uiT0 + (uiT1<<1) + uiT2 + 2) >> 2);
			}
		}
		else
			memset(pbLMRefTop,pbCurrRecY[-2],uiSizeChroma);

		if(bLeftAvail)
		{
			for(u32 i=0;i<uiSizeChroma;i++)
			{
				uiL0 = pbCurrRecY[-2 + (i<<1)*(CTU_WIDTH+2)];
				uiL1 = pbCurrRecY[-2 + ((i<<1)+1)*(CTU_WIDTH+2)];
				pbLMRefLeft[i] = byte((uiL0+uiL1)>>1);
			}
		}
		else
			memset(pbLMRefLeft,pbTopBuffY[0],uiSizeChroma);
	}
	else
	{
		memset(pbLMRefLeft,0x80,uiSizeChroma);
		memset(pbLMRefTop,0x80,uiSizeChroma);
	}
}

void H265CTUCompressor::GenIntraPredDC(byte *pbPred, byte *pbRef, u32 uiSize, u32 uiMode, bit bIsChroma)
{
	// See 8.4.3.1.5 in draft
	byte *pbTop = pbRef + (uiSize<<1) + 1;
	byte *pbLeft = pbRef + (uiSize<<1) - 1;	// The left array starts from the top and goes to the bottom. We must have a negative iterator

	// Get DC value
	u32 uiDCVal = 0;
	u32 uiTopSum = 0;
	u32 uiLeftSum = 0;

	for(i32 i=0;i<i32(uiSize);i++)
	{
		uiTopSum += pbTop[i];
		uiLeftSum += pbLeft[-i];
	}

	//u32 uiDiv = m_uiLog2Size + (bIsChroma && m_uiLog2Size != 2? 0 : 1);	// Will only work for 4:2:0
	u32 uiDiv = uiSize<<1;	//uiSize;
	//uiDiv *= (bIsChroma && uiSize != 4? 1 : 2);
	uiDCVal = (uiTopSum + uiLeftSum + uiSize) / uiDiv;// >> uiDiv;

	// Copy DC values to all locations of the predictor
	memset(pbPred,uiDCVal,uiSize*uiSize);	// This requires the prediction buffer to be a full 1-D array, and not a dynamically allocated 2-D array

	if(bIsChroma == 0)	// Luma pixels require DC filtering
	{
		u32 uiDCValx3 = (uiDCVal<<1)+uiDCVal;	// uiDCVal * 3
		pbPred[0] = (pbLeft[0] + (uiDCVal<<1) + pbTop[0] + 2) >> 2;
		for(i32 i=1;i<i32(uiSize);i++)
		{
			pbPred[i]			= (pbTop[i] + uiDCValx3 + 2) >> 2;			// Prediction samples [x,0]
			pbPred[i*uiSize]	= (pbLeft[-i] + uiDCValx3 + 2) >> 2;		// Prediction samples [0,y]
		}
	}
}

void H265CTUCompressor::GenIntraPredPlanar(byte *pbPred, byte *pbRef, u32 uiSize, u32 uiMode, bit bIsChroma)
{
	// See 8.4.3.1.7 of draft
	byte *pbTop = pbRef + (uiSize<<1) + 1;
	byte *pbLeft = pbRef + (uiSize<<1) - 1;	// The left array starts from the top and goes to the bottom. We must have a negative iterator

	byte bTopRightRec = pbTop[uiSize];
	byte bBottomLeftRec = pbLeft[-i32(uiSize)];	// @todo Need to check this

	i32 iTopComp;
	i32 iLeftComp;
	u32 uiLog2Size = LOG2(uiSize-1);
	u32 uiDiv = uiLog2Size + 1;	// Usman: I couldn't find this anywhere (bIsChroma && uiLog2Size != 2? 0 : 1);

	i32 iSize = i32(uiSize);

	for(i32 y=0;y<iSize;y++)
	{
		for(i32 x=0;x<iSize;x++)
		{
			iTopComp = (iSize-1-y)*pbTop[x] + (y+1)*bBottomLeftRec;
			iLeftComp = (iSize-1-x)*pbLeft[-y] + (x+1)*bTopRightRec;
			pbPred[y*iSize+x] = (iTopComp + iLeftComp + iSize) >> uiDiv;
		}
	}
}

void H265CTUCompressor::GenIntraPredAngular(byte *pbPred, byte *pbRef, u32 uiSize, u32 uiMode, bit bIsChroma)
{
	// See 8.4.3.1.6 in draft
	byte *pbTopLeft = pbRef + (uiSize<<1);		// The top left array also starts from the top and goes to the bottom. We must have a negative iterator
	i32 iIntraPredAngle = g_pbIntraPredAngleFromMode[uiMode];
	i32 iInvAngle = g_pbInvAngleFromMode[uiMode];
	bit bModeVer = (uiMode >= 18);
	byte pbRefBuff[2*CTU_WIDTH+1];	// Note that only 2*CTU_WIDTH + 1 elements are used instead of 3*CTU_WIDTH+1 (check 8.4.3.1.6)
	byte *pbMainRef = pbRefBuff + (iIntraPredAngle < 0 ? CTU_WIDTH : 0);

	// Generate reference array
	// Implement equation 8-47 and 8-50
	for(i32 x=0;x<=i32(uiSize);x++)
		pbMainRef[x] = bModeVer ? pbTopLeft[x] : pbTopLeft[-x];

	if(iIntraPredAngle < 0)
	{
		// Implement equation 8-48 or 8-51
		i32 iTmp = 128;
		i32 iMinX = (i32(uiSize)*iIntraPredAngle)>>5;
		i32 iOffset;
		for(i32 x=-1;x>iMinX;x--)
		{
			iTmp += iInvAngle;
			iOffset = bModeVer ? -(iTmp >> 8) : (iTmp >> 8);	// The vertical mode decides the sign of the address index
			pbMainRef[x] = pbTopLeft[iOffset];
		}
	}
	else // Implement equation 8-49 or 8-52
		for(i32 x=uiSize+1;x<=i32(uiSize<<1);x++)
			pbMainRef[x] = bModeVer ? pbTopLeft[x] : pbTopLeft[-x];

	// Now generate the prediction samples
	i32 iDeltaPos = 0;
	i32 iIdx;
	i32 iFact;
	i32 iRefMainIdx;

	for(u32 k=0;k<uiSize;k++)
	{
		iDeltaPos += iIntraPredAngle;
		iIdx = iDeltaPos >> 5;	// Equation 8-53
		iFact = iDeltaPos & 0x1F;	// Equation 8-54

		if(iFact)	// We need to filter
		{
			for(i32 x=0;x<i32(uiSize);x++)
			{
				iRefMainIdx = x+iIdx+1;
				//pbPred[k*uiSize+x] = ( (pbMainRef[iRefMainIdx]<<5) + iFact*(pbMainRef[iRefMainIdx+1]-pbMainRef[iRefMainIdx]) + 16 ) >> 5;
				pbPred[k*uiSize+x] = ( ((32-iFact)*pbMainRef[iRefMainIdx] + iFact*pbMainRef[iRefMainIdx+1] + 16 ) >> 5);
			}
		}
		else
			for(i32 x=0;x<i32(uiSize);x++)
				pbPred[k*uiSize+x] = pbMainRef[x+iIdx+1];
	}

	// Filtering in case of prediction modes equal to HOR or VER
	// See 8.4.3.1.3 and 8.4.3.1.4
	if(bIsChroma == 0 && (uiMode == HOR_MODE_IDX || uiMode == VER_MODE_IDX))
	{
		i32 iOffset = bModeVer ? -1 : 1;
		for(i32 x=0;x<i32(uiSize);x++)
			pbPred[x*uiSize] = Clip1(pbPred[x*uiSize] + ((pbTopLeft[(x+1)*iOffset] - pbTopLeft[0])>>1));
	}

	if(!bModeVer)	// Do the matrix transpose
	{
		byte bTmp;
		for(u32 k=0;k<uiSize-1;k++)
		{
			for(u32 x=k+1;x<uiSize;x++)
			{
				bTmp = pbPred[k*uiSize+x];
				pbPred[k*uiSize+x] = pbPred[x*uiSize+k];
				pbPred[x*uiSize+k] = bTmp;
			}
		}
	}
}

void H265CTUCompressor::GenIntraPrediction(byte *pbPred, byte *pbRef, u32 uiSize, u32 uiMode, bit bIsChroma)
{
	// Depending upon the mode, generate the prediction
	if(uiMode == DC_MODE_IDX)
		GenIntraPredDC(pbPred, pbRef, uiSize, uiMode, bIsChroma);
	else if(uiMode == PLANAR_MODE_IDX)
		GenIntraPredPlanar(pbPred, pbRef, uiSize, uiMode, bIsChroma);
	else
		GenIntraPredAngular(pbPred, pbRef, uiSize, uiMode, bIsChroma);
}

u32 H265CTUCompressor::SAD_MxN(u32 uiM, u32 uiN, byte *pbSrc, u32 uiSrcStride, byte *pbRef, u32 uiRefStride)
{
	u32 uiSAD = 0;

	for(u32 i=0;i<uiN;i++)
		for(u32 j=0;j<uiM;j++)
			uiSAD += ABS(pbSrc[i*uiSrcStride+j] - pbRef[i*uiRefStride+j]);
	return uiSAD;
}

u32 H265CTUCompressor::MAD_MxN(u32 uiM, u32 uiN, byte *pbSrc, u32 uiSrcStride, byte *pbRef, u32 uiRefStride)
{
	u32 uiMAD = SAD_MxN(uiM, uiN, pbSrc, uiSrcStride, pbRef, uiRefStride);
	uiMAD /= (uiM * uiN);	// MAD = MAD / width / height;
	return uiMAD;
}

i32 H265CTUCompressor::MapModeToIndex(u32 uiBestMode, u8 *pbCandModeListIntra)
{
	i32 iPredIdx;
	u8 bTmp;

	if(uiBestMode == pbCandModeListIntra[0])
		iPredIdx = 0;
	else if(uiBestMode == pbCandModeListIntra[1])
		iPredIdx = 1;
	else if(uiBestMode == pbCandModeListIntra[2])
		iPredIdx = 2;
	else
	{
		// Start swapping
		// See 8.4.1 in the draft
		if(pbCandModeListIntra[0] > pbCandModeListIntra[1])
		{
			bTmp = pbCandModeListIntra[0];
			pbCandModeListIntra[0] = pbCandModeListIntra[1];
			pbCandModeListIntra[1] = bTmp;
		}
		if(pbCandModeListIntra[0] > pbCandModeListIntra[2])
		{
			bTmp = pbCandModeListIntra[0];
			pbCandModeListIntra[0] = pbCandModeListIntra[2];
			pbCandModeListIntra[2] = bTmp;
		}
		if(pbCandModeListIntra[1] > pbCandModeListIntra[2])
		{
			bTmp = pbCandModeListIntra[1];
			pbCandModeListIntra[1] = pbCandModeListIntra[2];
			pbCandModeListIntra[2] = bTmp;
		}

		iPredIdx = uiBestMode;
		iPredIdx -= RET_1_IF_TRUE(iPredIdx > pbCandModeListIntra[2]);
		iPredIdx -= RET_1_IF_TRUE(iPredIdx > pbCandModeListIntra[1]);
		iPredIdx -= RET_1_IF_TRUE(iPredIdx > pbCandModeListIntra[0]);
		iPredIdx += 3;
	}

	return iPredIdx;
}

u32 H265CTUCompressor::xCompressLumaCU(u32 uiAddrX, u32 uiAddrY, byte *pbCurrBuff, u32 uiSize, u32 uiDispCTULeft, u32 uiDispCTUTop, bit bIsChroma)
{
	// In the below, the term Curr pertains to the current location of the CU.
	// E.g. m_pbCurrRefTopBuffY is the pointer to the top reference just above the current CU
	byte *pbCurrIntraModeL;	// Current luma mode
	byte *pbCurrRecY, *pbCurrRecCb, *pbCurrRecCr;	// Current reconstructed
	i16 *piCurrCoeffY, *piCurrCoeffCb, *piCurrCoeffCr;	// Current coefficients
	byte *pbCurrRefTopBuffY, *pbCurrRefTopBuffCb, *pbCurrRefTopBuffCr;	// Top reconstructed array
	byte *pbCurrIntraModeInfoL;	// Luma mode information, used by chroma compressor
	byte *pbCurrIntraModePredInfoL;	// Prediciton of the mode
	u16 *puiCurrIntraModeInfoC;
	u32 puiRefYOffset[6];	// Offset in the luma reference array to the next Prediction mode. 4 are for bottom left, left, top and top right and 1 is for the top left and 1 for ending of substitution
	u32 puiRefCOffset[6];	// Offset in the luma reference array to the next Prediction mode. 4 are for bottom left, left, top and top right and 1 is for the top left and 1 for ending of substitution
	u8 pbTopAndLeftModes[5];						// Modes pertaining to the top and left of the current PU
	u8 *pbPtrTopAndLeftModes = pbTopAndLeftModes;
	u8	pbCandModeListIntra[3];					// Candidates for intra coding
	u8 *pbPtrCandModeListIntra = pbCandModeListIntra;

	byte *pbCurrY = pbCurrBuff + (uiAddrY+uiDispCTUTop)*m_uiYStride + (uiAddrX+uiDispCTULeft);
	u32	uiCTUOffsetXFromTile;	// Offset of the current CU from the start of the tile, used for copying the data from the top reference buffer
	uiCTUOffsetXFromTile = uiAddrX - m_cTileStartCTUPelTL.x;
	u32 uiLog2Size = LOG2(uiSize-1);	// m_pcImageParam->GetLog2FromCUSize(uiSize);

	// Initialize the buffer pointers to correctly point to the location within the CTU memory
	InitCUPointers(pbCurrIntraModeL,pbCurrRecY,pbCurrRecCb,pbCurrRecCr, 
					piCurrCoeffY,piCurrCoeffCb,piCurrCoeffCr, 
					pbCurrRefTopBuffY,pbCurrRefTopBuffCb,pbCurrRefTopBuffCr,
					pbCurrIntraModeInfoL,pbCurrIntraModePredInfoL,puiCurrIntraModeInfoC,
					uiDispCTULeft,uiDispCTUTop,uiCTUOffsetXFromTile);

	// Initialize the reference buffer locations for storing the reference pixels
	InitRefPointers(puiRefYOffset,uiSize);

	// Check the availability of the neighborhood and get the corresponding modes
	u8 bValidNeigFlag = CheckNeighAvail(pbPtrTopAndLeftModes,pbCurrIntraModeL,uiSize);

	// Fill the candidate mode list Intra
	GetCandModeListIntra(pbPtrTopAndLeftModes,uiDispCTUTop,pbPtrCandModeListIntra);
	
	// Now make reference samples
	GenReferenceSamplesIntra(pbCurrRecY,pbCurrRecCb,pbCurrRecCr,
								pbCurrRefTopBuffY,pbCurrRefTopBuffCb,pbCurrRefTopBuffCr,
								puiRefYOffset,puiRefCOffset,
								bValidNeigFlag,uiSize,uiDispCTUTop,bIsChroma);

	// Start Prediction
	u32 uiBestSAD = I32_MAX;
	u32 uiBestMode = 0;
	u8 bUseFilter = 0;
	byte *pbCurrRef;
	byte *pbPredPingPong[2] = {m_pppbTempPred[0][uiLog2Size-2], m_pppbTempPred[1][uiLog2Size-2]};	// Prediction buffers for ping-pong
	byte *pbCurrPred;
	u8 bPingPongBuffNum = 0;
	u32 uiSAD;

	// Now test intra modes and decide about the best mode
	// @todo We can have threads here as well
	// @todo Insert the edge predictor here and determine the number of
	// maximum modes being tested
	bit bPlanarTested = false;
	bit bDCTested = false;

	// Test all modes
	u32 uiBestPredMode = 0;
	u32 uiStartPredMode = 0;
	u32 uiEndPredMode = 35;

	for(u32 uiMode=uiStartPredMode;uiMode<uiEndPredMode;uiMode++)
	{
		if(uiMode == PLANAR_MODE_IDX)
			bPlanarTested = true;
		if(uiMode == DC_MODE_IDX)
			bDCTested = true;

		// Determine whether to use filtered or unfiltered reference
		bUseFilter = g_pbIntraFilterUsage[uiLog2Size-2][uiMode];
		pbCurrRef = (bUseFilter == 0 ? m_pbRefYUnfiltered : m_pbRefYFiltered);
		pbCurrPred = pbPredPingPong[bPingPongBuffNum];

		// Generate luma prediction
		GenIntraPrediction(pbCurrPred, pbCurrRef, uiSize, uiMode, bIsChroma);

		// Assign more weight to the most probable modes
		if(uiMode == pbCandModeListIntra[0])
			uiSAD = m_uiQP;
		else if(uiMode == pbCandModeListIntra[1] || uiMode == pbCandModeListIntra[2])
			uiSAD = m_uiQP<<1;
		else
			uiSAD = (m_uiQP<<1)+m_uiQP;

		// Get the SAD value
		uiSAD +=SAD_MxN(uiSize,uiSize,pbCurrY,m_uiYStride,pbCurrPred,uiSize);

		// Test the SAD value
		if(uiSAD < uiBestSAD)
		{
			uiBestSAD = uiSAD;
			uiBestMode = uiMode;
			bPingPongBuffNum = (bPingPongBuffNum+1)%2;	// Change the buffer
		}
	}

	// Now map the modes to the index of candidates and get the prediction index
	i32 iPredIdx = MapModeToIndex(uiBestMode,pbPtrCandModeListIntra);
	MAKE_SURE((iPredIdx < TOTAL_INTRA_MODES+3),"Prediction index is larger than allowed");

	// Split futher and check the outcome
	bit bSplit = (uiSize == MIN_CU_SIZE ? false : true);

	if(uiSize > MIN_CU_SIZE && uiBestSAD > 0)
	{
		u32 uiSubSize = uiSize >> 1;
		u32 uiSubSAD[4];
		uiSAD = 0;
		for(u32 uiPartIdx=0;uiPartIdx<4;uiPartIdx++)
		{
			u32 uiDispX = uiSubSize*(uiPartIdx & 0x1);	// Denoting the shift in x direction for the current CU
			u32 uiDispY = uiSubSize*(uiPartIdx >> 1);	// Denoting the shift in y direction for the current CU
			uiSubSAD[uiPartIdx] = xCompressLumaCU(
									uiAddrX,
									uiAddrY,
									pbCurrBuff,
									uiSubSize, 
									uiDispCTULeft+uiDispX,uiDispCTUTop+uiDispY,
									bIsChroma);
			uiSAD += uiSubSAD[uiPartIdx];
			if(uiSAD >= uiBestSAD)		// Splitting is not good
			{
				bSplit = false;
				break;
			}
		}
	}

	if(bSplit == false)	// No more splitting, compute the DCT and quantize (transform loop)
	{
		bPingPongBuffNum = (bPingPongBuffNum+1)%2;	// Get the prediction buffer with best SAD
		pbCurrPred = pbPredPingPong[bPingPongBuffNum];	// Prediction with the best SAD

		i16 *piTransBuffTmp1 = m_ppiTransBuffTmp[0];	// This is necessary for binding to a reference
		i16 *piTransBuffTmp2 = m_ppiTransBuffTmp[1];	// This is necessary for binding to a reference
		i16 *piQuantCoeff = piCurrCoeffY;	// This must be stored for further processing, therefore, the stride is CTU_WIDTH

		m_pcH265Trans->ResDCT(piTransBuffTmp1,uiSize,uiSize,pbCurrY,m_uiYStride,pbCurrPred,uiSize,piTransBuffTmp1,piTransBuffTmp2,uiBestMode);
		u32 uiQuantSumNonZero = m_pcH265Trans->Quant(piQuantCoeff,CTU_WIDTH,m_uiQP,uiSize,uiSize,piTransBuffTmp1,uiSize,I_SLICE);
		if(uiQuantSumNonZero)
		{
			// Need the inverse loop
			// We do an inplace transformation, i.e. the original image is changed
			m_pcH265Trans->InvQuant(piTransBuffTmp2,uiSize,m_uiQP,uiSize,uiSize,piQuantCoeff,CTU_WIDTH,I_SLICE);
			m_pcH265Trans->IDCTRec(pbCurrRecY,CTU_WIDTH+2,uiSize,uiSize,piTransBuffTmp2,uiSize,pbCurrPred,uiSize,piTransBuffTmp1,piTransBuffTmp2,uiBestMode);
		}
		else // Do not need the inverse loop, as all the coefficients are 0			
			for(u32 i=0;i<uiSize;i++)
				memcpy(&pbCurrRecY[i*(CTU_WIDTH+2)],&pbCurrPred[i*uiSize],uiSize);

		// Now update the information regarding the best modes
		puiCurrIntraModeInfoC[0] = u16(iPredIdx);
		pbCurrIntraModeInfoL[0] = 0;
		pbCurrIntraModeInfoL[0] = (bValidNeigFlag) | (uiQuantSumNonZero != 0 ? 0x20 : 0x00) | ((uiLog2Size-2)<<6);
		pbCurrIntraModePredInfoL[0] = 0;
		pbCurrIntraModePredInfoL[0] = uiBestPredMode;
		u32 uiNeighModeStride = uiSize/MIN_CU_SIZE;
		for(u32 i=0;i<uiNeighModeStride;i++)
		{
			memset(&pbCurrIntraModeL[i*(TOT_PUS_LINE+2)],uiBestMode,uiNeighModeStride);
			memset(&pbCurrIntraModeInfoL[i*(TOT_PUS_LINE+1)],pbCurrIntraModeInfoL[0],uiNeighModeStride);
			memset(&pbCurrIntraModePredInfoL[i*(TOT_PUS_LINE+1)],pbCurrIntraModePredInfoL[0],uiNeighModeStride);
		}
	}
	else
		MAKE_SURE((uiSize >= MIN_CU_SIZE && uiSize <= CTU_WIDTH),"The CU size is not confined to the minimum and maximum CU sizes");

	uiBestSAD = min(uiBestSAD,uiSAD);

	return uiBestSAD;
}

void H265CTUCompressor::GetDispFrom4x4s(u32 uiTot4x4s,u32 &uiDispCTULeft, u32 &uiDispCTUTop)
{
	u32 uiX4x4s = GET_INTERLEAVED_BITS(uiTot4x4s);
	u32 uiY4x4s = GET_INTERLEAVED_BITS(uiTot4x4s>>1);

	uiDispCTULeft = uiX4x4s << 2;
	uiDispCTUTop = uiY4x4s << 2;
}

void H265CTUCompressor::CompressChromaCU(u32 uiAddrX, u32 uiAddrY, byte *pbCbBuff, byte *pbCrBuff)
{
	u32 uiDispCTUTop = 0;	// Displacement of the current "Luma" CU from the top of the CTU
	u32 uiDispCTULeft = 0;	// Displacement of the current "Luma" CU from the left of the CTU

	// Avialable intra modes
	// See table 8-2 and 8-3, last column
	u32 puiChromaMode[5];	// Total chroma modes
	puiChromaMode[0] = PLANAR_MODE_IDX;
	puiChromaMode[1] = VER_MODE_IDX;
	puiChromaMode[2] = HOR_MODE_IDX;
	puiChromaMode[3] = DC_MODE_IDX;
	
	// In the below, the term Curr pertains to the current location of the CU.
	// E.g. m_pbCurrRefTopBuffY is the pointer to the top reference just above the current CU
	byte *pbCurrIntraModeL;	// Current luma mode
	byte *pbCurrRecY, *pbCurrRecCb, *pbCurrRecCr;	// Current reconstructed
	i16 *piCurrCoeffY, *piCurrCoeffCb, *piCurrCoeffCr;	// Current coefficients
	byte *pbCurrRefTopBuffY, *pbCurrRefTopBuffCb, *pbCurrRefTopBuffCr;	// Top reconstructed array
	byte *pbCurrIntraModeInfoL;	// Luma mode information, used by chroma compressor
	byte *pbCurrIntraModePredInfoL;	// Prediciton of the mode
	u16 *puiCurrIntraModeInfoC;
	u32 puiRefYOffset[6];	// Offset in the luma reference array to the next Prediction mode. 4 are for bottom left, left, top and top right and 1 is for the top left and 1 for ending of substitution
	u32 puiRefCOffset[6];	// Offset in the luma reference array to the next Prediction mode. 4 are for bottom left, left, top and top right and 1 is for the top left and 1 for ending of substitution

	byte *pbCurrCb; 
	byte *pbCurrCr;

	u32 uiTot4x4s = 0;	// Denots the total 4x4s scanned in the CTU

	m_pbRefCb = m_pbRefYUnfiltered;	// Assign reference arrays
	m_pbRefCr = m_pbRefYFiltered;	// Assign reference arrays

	u32 uiCTUOffsetXFromTile = uiAddrX - m_cTileStartCTUPelTL.x;

	do
	{
		// From the total 4x4s scanned in the CTU, get the displacement of the current
		// CU from the CTU left and top
		GetDispFrom4x4s(uiTot4x4s,uiDispCTULeft,uiDispCTUTop);

		// Initialize the buffer pointers to correctly point to the location within the CTU memory
		// This will also fit the m_pbCurrLumaModeInfo, m_pbCurrMode etc to the starting location
		InitCUPointers(pbCurrIntraModeL,pbCurrRecY,pbCurrRecCb,pbCurrRecCr, 
					piCurrCoeffY,piCurrCoeffCb,piCurrCoeffCr, 
					pbCurrRefTopBuffY,pbCurrRefTopBuffCb,pbCurrRefTopBuffCr,
					pbCurrIntraModeInfoL,pbCurrIntraModePredInfoL,puiCurrIntraModeInfoC,
					uiDispCTULeft,uiDispCTUTop,uiCTUOffsetXFromTile);
		
		pbCurrCb = pbCbBuff + (uiAddrX>>1) + (uiAddrY>>1)*m_uiCStride;
		pbCurrCb += (uiDispCTULeft>>1) + (uiDispCTUTop>>1)*m_uiCStride;	// Point to the correct location
		pbCurrCr = pbCrBuff + (uiAddrX>>1) + (uiAddrY>>1)*m_uiCStride;
		pbCurrCr += (uiDispCTULeft>>1) + (uiDispCTUTop>>1)*m_uiCStride;	// Point to the correct location

		// Get the information about luma CU
		u32 uiLog2Size = (pbCurrIntraModeInfoL[0]>>6) + 2;
		u32 uiSizeL = 1 << uiLog2Size;	// Get the size of the first luma block (convert it from log2-2 to the actual size)
		u32 uiBestModeL = pbCurrIntraModeL[0];
		u8	ubValidsLuma = pbCurrIntraModeInfoL[0] & 0x1F;

		// Initialize chroma reference
		u32 uiSizeChroma = (uiSizeL == 4 ? 4 : (uiSizeL >> 1));	// Size of chroma block
		/* @todo There is a problem while using the memory like this for a 4x4. Therefore, I just took the largest array for prediction (see below)
		byte *pbPredPingPongCb[2] = {m_pppbTempPred[0][uiLog2Size-2], &m_pppbTempPred[0][uiLog2Size-2][CTU_WIDTH*CTU_WIDTH>>1]};	// Prediction buffers for ping-pong (reuse the same memory)
		byte *pbPredPingPongCr[2] = {m_pppbTempPred[1][uiLog2Size-2], &m_pppbTempPred[1][uiLog2Size-2][CTU_WIDTH*CTU_WIDTH>>1]};	// Prediction buffers for ping-pong	(reuse the same memory)*/
		byte *pbPredPingPongCb[2] = {m_ppbTempPred64[0], &m_ppbTempPred64[0][CTU_WIDTH*CTU_WIDTH>>1]};	// Prediction buffers for ping-pong (reuse the same memory)
		byte *pbPredPingPongCr[2] = {m_ppbTempPred64[1], &m_ppbTempPred64[1][CTU_WIDTH*CTU_WIDTH>>1]};	// Prediction buffers for ping-pong	(reuse the same memory)
		u8 bPingPongBuffNum = 0;
		InitRefPointers(puiRefCOffset,uiSizeChroma);	// Initialize the reference buffer locations for storing the reference pixels
		puiChromaMode[CHROMA_DM_MODE] = uiBestModeL;

		// Change the valids
		// @todo I need to check this. I am not sure what is going on here.
		if(uiSizeL == 4)
		{
			// Fix left bottom and top right
			ubValidsLuma &= ~0x11;	
			ubValidsLuma |= (pbCurrIntraModeInfoL[TOT_PUS_LINE+1]) & 0x01;
			ubValidsLuma |= (pbCurrIntraModeInfoL[1]) & 0x10;
		}

		// Generate the reference samples
		GenReferenceSamplesIntra(pbCurrRecY,pbCurrRecCb,pbCurrRecCr,
								pbCurrRefTopBuffY,pbCurrRefTopBuffCb,pbCurrRefTopBuffCr,
								puiRefYOffset,puiRefCOffset,
								ubValidsLuma,uiSizeChroma,uiDispCTUTop,1);

		// Determine the best mode for chroma
		u32 uiBestModeIdxC = CHROMA_DM_MODE;
		u32 uiBestSAD = I32_MAX;
		u32 uiSAD;
		for(u32 i=0;i<=CHROMA_DM_MODE;i++)
		{
			u32 uiCurrModeC = puiChromaMode[i];
			if(i < CHROMA_DM_MODE && uiCurrModeC == puiChromaMode[CHROMA_DM_MODE])	// Don't repeate the DM mode
				uiCurrModeC = 34;

			// Generate prediction
			GenIntraPrediction(pbPredPingPongCb[bPingPongBuffNum], m_pbRefCb, uiSizeChroma, uiCurrModeC, true);
			GenIntraPrediction(pbPredPingPongCr[bPingPongBuffNum], m_pbRefCr, uiSizeChroma, uiCurrModeC, true);

			// Get the SAD value
			uiSAD = SAD_MxN(uiSizeChroma,uiSizeChroma,pbCurrCb,m_uiCStride,pbPredPingPongCb[bPingPongBuffNum],uiSizeChroma);
			uiSAD += SAD_MxN(uiSizeChroma,uiSizeChroma,pbCurrCr,m_uiCStride,pbPredPingPongCr[bPingPongBuffNum],uiSizeChroma);

			if(uiSAD < uiBestSAD)
			{
				uiBestSAD = uiSAD;
				uiBestModeIdxC = i;
				bPingPongBuffNum = (bPingPongBuffNum+1)%2;	// Change the buffer
			}
		}	// All predictions tested and the best selected

#if(USE_CHROMA_LM_MODE)
		// Now test the chroma from luma mode (LM)
		// See 8.4.3.1.8 in draft
		byte *pbLMRefLeft = new byte[uiSizeChroma];
		byte *pbLMRefTop = new byte[uiSizeChroma];

		// Generate LM prediction
		GenerateReferenceSamplesChromaLM(pbLMRefTop,pbLMRefLeft,ubValidsLuma,uiDispCTUTop,uiSizeChroma);

		delete [] pbLMRefLeft;
		delete [] pbLMRefTop;
#endif
		
		bPingPongBuffNum = (bPingPongBuffNum+1)%2;	// Get the buffer with the best SAD

		i16 *piTransBuffTmp1 = m_ppiTransBuffTmp[0];	// This is necessary for binding to a reference
		i16 *piTransBuffTmp2 = m_ppiTransBuffTmp[1];	// This is necessary for binding to a reference
		i16 *piQuantCoeffCb = piCurrCoeffCb;
		i16 *piQuantCoeffCr = piCurrCoeffCr;
		byte *pbCurrPredCb = pbPredPingPongCb[bPingPongBuffNum];	// Cb prediction with the best SAD
		byte *pbCurrPredCr = pbPredPingPongCr[bPingPongBuffNum];	// Cr prediction with the best SAD

		// Transform loop Cb
		m_pcH265Trans->ResDCT(piTransBuffTmp1,uiSizeChroma,uiSizeChroma,pbCurrCb,m_uiCStride,pbCurrPredCb,uiSizeChroma,piTransBuffTmp1,piTransBuffTmp2,INVALID_MODE);
		u32 uiQPC = g_pbChramaQPFromLuma[m_uiQP];	// Get chroma QP from luma QP
		// @todo Combine the quantization and inverse quantization into one function
		u32 uiQuantSumNonZeroCb = m_pcH265Trans->Quant(piQuantCoeffCb,(CTU_WIDTH>>1),uiQPC,uiSizeChroma,uiSizeChroma,piTransBuffTmp1,uiSizeChroma,I_SLICE);
		if(uiQuantSumNonZeroCb)
		{
			// Need the inverse loop
			// We do an inplace transformation, i.e. the original image is changed
			m_pcH265Trans->InvQuant(piTransBuffTmp2,uiSizeChroma,uiQPC,uiSizeChroma,uiSizeChroma,piQuantCoeffCb,(CTU_WIDTH>>1),I_SLICE);
			m_pcH265Trans->IDCTRec(pbCurrRecCb,CTU_WIDTH/2+1,uiSizeChroma,uiSizeChroma,piTransBuffTmp2,uiSizeChroma,pbCurrPredCb,uiSizeChroma,piTransBuffTmp1,piTransBuffTmp2,INVALID_MODE);
		}
		else // Do not need the inverse loop, as all the coefficients are 0			
			for(u32 i=0;i<uiSizeChroma;i++)
				memcpy(&pbCurrRecCb[i*(CTU_WIDTH/2+1)],&pbCurrPredCb[i*uiSizeChroma],uiSizeChroma);

		// Transform loop Cr
		m_pcH265Trans->ResDCT(piTransBuffTmp1,uiSizeChroma,uiSizeChroma,pbCurrCr,m_uiCStride,pbCurrPredCr,uiSizeChroma,piTransBuffTmp1,piTransBuffTmp2,INVALID_MODE);
		u32 uiQuantSumNonZeroCr = m_pcH265Trans->Quant(piQuantCoeffCr,(CTU_WIDTH>>1),uiQPC,uiSizeChroma,uiSizeChroma,piTransBuffTmp1,uiSizeChroma,I_SLICE);
		if(uiQuantSumNonZeroCr)
		{
			// Need the inverse loop
			// We do an inplace transformation, i.e. the original image is changed
			m_pcH265Trans->InvQuant(piTransBuffTmp2,uiSizeChroma,uiQPC,uiSizeChroma,uiSizeChroma,piQuantCoeffCr,(CTU_WIDTH>>1),I_SLICE);
			m_pcH265Trans->IDCTRec(pbCurrRecCr,CTU_WIDTH/2+1,uiSizeChroma,uiSizeChroma,piTransBuffTmp2,uiSizeChroma,pbCurrPredCr,uiSizeChroma,piTransBuffTmp1,piTransBuffTmp2,INVALID_MODE);
		}
		else // Do not need the inverse loop, as all the coefficients are 0			
			for(u32 i=0;i<uiSizeChroma;i++)
				memcpy(&pbCurrRecCr[i*(CTU_WIDTH/2+1)],&pbCurrPredCr[i*uiSizeChroma],uiSizeChroma);

		u32 uiCbfC = (uiQuantSumNonZeroCr ? 2 : 0) | (uiQuantSumNonZeroCb ? 1 : 0);
		puiCurrIntraModeInfoC[0] |= (uiBestModeIdxC << 13) | (uiCbfC << 8);
		if(uiSizeL == 4)
			uiSizeL <<= 1;

		uiTot4x4s += ((uiSizeL * uiSizeL)>>4);
	}while(uiTot4x4s < (TOT_PUS_LINE*TOT_PUS_LINE));
}

void H265CTUCompressor::PrepareCTU(u32 uiAddrX, u32 uiAddrY)
{
	if(uiAddrX == m_cTileStartCTUPelTL.x)	// New row started, need to initialize the mode buffers
	{
		InitBuffersNewCTULine();
		if(uiAddrY > m_cTileStartCTUPelTL.y)	// Not the first row of CTUs, then the top modes are always available
			memset(m_pbNeighIntraModeL+1,VALID_MODE,TOT_PUS_LINE+1);	// Left mode is not available in the top row
	}
		
	// Determine the mode of top and top right, if they exist for the current CTU
	if(uiAddrX > m_cTileStartCTUPelTL.x && uiAddrY > m_cTileStartCTUPelTL.y)	// Starting CU of a CTU row in a tile, TL unavailable
		m_pbNeighIntraModeL[0] = VALID_MODE;
	else
		m_pbNeighIntraModeL[0] = INVALID_MODE;

	if(uiAddrX < m_cTileEndCTUPelTL.x && uiAddrY > m_cTileStartCTUPelTL.y)	// Ending CU of a CTU row in a tile, TR unavailable
		m_pbNeighIntraModeL[TOT_PUS_LINE+1] = VALID_MODE;
	else
		m_pbNeighIntraModeL[TOT_PUS_LINE+1] = INVALID_MODE;

	// Determine the X offset from the tile boundary
	u32 uiCTUOffsetXFromTile = uiAddrX - m_cTileStartCTUPelTL.x;
	
	// Copy the mode information from the above CTU row
	memcpy(m_pbIntraModeInfoL+1,
		m_pbTopLineIntraModeInfoL+uiCTUOffsetXFromTile/MIN_CU_SIZE,TOT_PUS_LINE);

	// Swaping the top left pixel
	// @todo Need a better approach and also better understanding
	byte tmp;
	byte *pbRefTopBuff;
	pbRefTopBuff = m_pbRefTopBuffY + uiCTUOffsetXFromTile + 1;
	SWAP(pbRefTopBuff[-1],m_bSavedLTY);
	pbRefTopBuff = m_pbRefTopBuffCb + (uiCTUOffsetXFromTile>>1) + 1;
	SWAP(pbRefTopBuff[-1],m_bSavedLTCb);
	pbRefTopBuff = m_pbRefTopBuffCr + (uiCTUOffsetXFromTile>>1) + 1;
	SWAP(pbRefTopBuff[-1],m_bSavedLTCr);

}

void H265CTUCompressor::FinishCTU(u32 uiAddrX, u32 uiAddrY)
{
	// Determine the X offset from the tile boundary
	u32 uiCTUOffsetXFromTile = uiAddrX - m_cTileStartCTUPelTL.x;

	byte *pbRefTopBuff;
	pbRefTopBuff = m_pbRefTopBuffY + uiCTUOffsetXFromTile + 1;
	pbRefTopBuff[-1] = m_bSavedLTY;
	m_bSavedLTY = pbRefTopBuff[CTU_WIDTH-1];

	pbRefTopBuff = m_pbRefTopBuffCb + (uiCTUOffsetXFromTile>>1) + 1;
	pbRefTopBuff[-1] = m_bSavedLTCb;
	m_bSavedLTCb = pbRefTopBuff[(CTU_WIDTH>>1)-1];

	pbRefTopBuff = m_pbRefTopBuffCr + (uiCTUOffsetXFromTile>>1) + 1;
	pbRefTopBuff[-1] = m_bSavedLTCr;
	m_bSavedLTCr = pbRefTopBuff[(CTU_WIDTH>>1)-1];

}

void H265CTUCompressor::CompressCTU(u32 uiAddrX, u32 uiAddrY, byte *pbYBuff, byte *pbCbBuff, byte *pbCrBuff)
{
	m_ctTimeForCTU = GetTimeInMiliSec();

	// Prepare CTU for compression
	PrepareCTU(uiAddrX,uiAddrY);

	// Compress the luma CTU
	xCompressLumaCU(uiAddrX, uiAddrY, pbYBuff, CTU_WIDTH, 0, 0, false);

	// Compress the Chroma CTU
	CompressChromaCU(uiAddrX, uiAddrY, pbCbBuff, pbCrBuff);

	// Finish CTU
	FinishCTU(uiAddrX,uiAddrY);

	m_ctTimeForCTU = GetTimeInMiliSec()-m_ctTimeForCTU;
}

bit H265CTUCompressor::IsLastTileCTU(u32 uiAddrX, u32 uiAddrY)
{
	if(uiAddrX == m_cTileEndCTUPelTL.x && uiAddrY == m_cTileEndCTUPelTL.y)
		return true;
	else
		return false;
}

bit H265CTUCompressor::IsLastSliceCTU(u32 uiAddrX, u32 uiAddrY)
{
	if(uiAddrX == (m_pcImageParam->m_uiFrameWidthInCTUs-1)*CTU_WIDTH &&
		uiAddrY == (m_pcImageParam->m_uiFrameHeightInCTUs-1)*CTU_HEIGHT)
		return true;
	else
		return false;
}

void H265CTUCompressor::EncodeCTU(u32 uiAddrX, u32 uiAddrY, Cabac *& pcCabac, BitStreamHandler *& pcBitStreamHandler)
{
	u32 uiDispCTUTop = 0;	// Displacement of the current "Luma" CU from the top of the CTU
	u32 uiDispCTULeft = 0;	// Displacement of the current "Luma" CU from the left of the CTU

	// Avialable intra modes
	// See table 8-2 and 8-3, last column
	u32 puiChromaMode[5] = {PLANAR_MODE_IDX, VER_MODE_IDX, HOR_MODE_IDX, DC_MODE_IDX, CHROMA_DM_MODE_IDX};	// Total chroma modes

	// In the below, the term Curr pertains to the current location of the CU.
	// E.g. m_pbCurrRefTopBuffY is the pointer to the top reference just above the current CU
	byte *pbCurrIntraModeL;	// Current luma mode
	byte *pbCurrRecY, *pbCurrRecCb, *pbCurrRecCr;	// Current reconstructed
	i16 *piCurrCoeffY, *piCurrCoeffCb, *piCurrCoeffCr;	// Current coefficients
	byte *pbCurrRefTopBuffY, *pbCurrRefTopBuffCb, *pbCurrRefTopBuffCr;	// Top reconstructed array
	byte *pbCurrIntraModeInfoL;	// Luma mode information, used by chroma compressor
	byte *pbCurrIntraModePredInfoL;	// Prediciton of the mode
	u16 *puiCurrIntraModeInfoC;

	u32 uiTot4x4s = 0;	// Denots the total 4x4s scanned in the CTU

	m_pbRefCb = m_pbRefYUnfiltered;	// Assign reference arrays
	m_pbRefCr = m_pbRefYFiltered;	// Assign reference arrays

	u32 uiCTUOffsetXFromTile = uiAddrX - m_cTileStartCTUPelTL.x;

	// This loop will run at 4x4 level. For luma, there is no 4x4 CU, therefore, for the
	// smallest CU of 8x8 and with a split flag, this loop will run 4 times, 1 time per PU.
	// For chroma, and smallest CU of 8x8 with a split flag, this loop will only run for
	// the last of the 4 luma PUs.
	do
	{
		// From the total 4x4s scanned in the CTU, get the displacement of the current
		// CU from the CTU left and top
		GetDispFrom4x4s(uiTot4x4s,uiDispCTULeft,uiDispCTUTop);

		// Initialize the buffer pointers to correctly point to the location within the CTU memory
		// This will also fit the m_pbCurrLumaModeInfo, m_pbCurrMode etc to the starting location
		InitCUPointers(pbCurrIntraModeL,pbCurrRecY,pbCurrRecCb,pbCurrRecCr, 
					piCurrCoeffY,piCurrCoeffCb,piCurrCoeffCr, 
					pbCurrRefTopBuffY,pbCurrRefTopBuffCb,pbCurrRefTopBuffCr,
					pbCurrIntraModeInfoL,pbCurrIntraModePredInfoL,puiCurrIntraModeInfoC,
					uiDispCTULeft,uiDispCTUTop,uiCTUOffsetXFromTile);

		// Get the information about luma CU
		u32 uiLog2Size = (pbCurrIntraModeInfoL[0]>>6) + 2;
		u8	ubValidsLuma = pbCurrIntraModeInfoL[0] & 0x1F;
		u32 uiSizeL = 1 << uiLog2Size;	// Get the size of the first luma block (convert it from log2-2 to the actual size)
		u32 uiSizeC = uiSizeL>>1;

		// Get split flag information
		u32 uiCurrSplitFlag = pbCurrIntraModeInfoL[0]>>6;	// If 0, then no more splitting
		u32 uiLeftSplitFlag = (ubValidsLuma & (1<<VALID_L) ? pbCurrIntraModeInfoL[-1]>>6 : 7);	// If 0, then no more splitting
		u32 uiTopSplitFlag = (ubValidsLuma & (1<<VALID_T) ? pbCurrIntraModeInfoL[-(TOT_PUS_LINE+1)]>>6 : 7);	// If 0, then no more splitting

		// Get the mode information
		u32 uiBestModeL = pbCurrIntraModeL[0];
		u32 uiModeIdxL = puiCurrIntraModeInfoC[0] & 0x3F;
		u32 uiModeIdxC = (puiCurrIntraModeInfoC[0] >> 13) & 0x7;
		u32 uiCbfY = RET_1_IF_TRUE(pbCurrIntraModeInfoL[0] & 0x20);
		u32 uiCbfCb = RET_1_IF_TRUE(puiCurrIntraModeInfoC[0] & 0x0100);
		u32 uiCbfCr = RET_1_IF_TRUE(puiCurrIntraModeInfoC[0] & 0x0200);
		u32 uiCtx;

		if(uiCurrSplitFlag <= 3 && uiTot4x4s == 0)	// Only for the first 4x4
		{
			uiCtx = (uiTopSplitFlag < 3) + (uiLeftSplitFlag < 3);
			MAKE_SURE((uiCtx < 3),"Context is wrong");
			pcCabac->EncodeBin(RET_1_IF_TRUE(uiCurrSplitFlag<3),OFF_SPLIT_FLAG_CTX+uiCtx,pcBitStreamHandler);
		}

		if(uiCurrSplitFlag <= 2 && (uiTot4x4s & 0xF) == 0)	// A 16x16 (or higher) CU
		{
			uiCtx = (uiTopSplitFlag < 2) + (uiLeftSplitFlag < 2);
			MAKE_SURE((uiCtx < 3),"Context is wrong");
			pcCabac->EncodeBin(RET_1_IF_TRUE(uiCurrSplitFlag<2),OFF_SPLIT_FLAG_CTX+uiCtx,pcBitStreamHandler);
		}

		if((uiTot4x4s & 0x3) == 0)	// An 8x8 (or higher) CU
		{
			if(uiCurrSplitFlag <= 1)
				pcCabac->EncodeBin(RET_1_IF_TRUE(uiCurrSplitFlag!=0),OFF_PART_SIZE_CTX,pcBitStreamHandler);

			u32 puiPredIdx[4] = {uiModeIdxL};
			u32 uiTotPUs = 1;
			if(uiCurrSplitFlag == 0)	// If there are 4 4x4 PUs in the 8x8 CU
			{
				// 4 4x4s make a CU here
				uiTotPUs = 4;
				puiPredIdx[1] = puiCurrIntraModeInfoC[1] & 0x3F;	// Right
				puiPredIdx[2] = puiCurrIntraModeInfoC[TOT_PUS_LINE] & 0x3F;	// Bottom
				puiPredIdx[3] = puiCurrIntraModeInfoC[TOT_PUS_LINE+1] & 0x3F;	// Bottom right
			}

			// Encode luma angular direction
			pcCabac->EncodeIntraDirAngGrpL(uiTotPUs,puiPredIdx,pcBitStreamHandler);

			// Encode chroma angular direction
			pcCabac->EncodeIntraDirAngC(uiModeIdxC,pcBitStreamHandler);

			// Encode Cbf for chroma
			// Even if there are 4 4x4 PUs in luma, there will only be one 4x4 for chroma
			pcCabac->EncodeBin(uiCbfCb,OFF_QT_CBF_CTX+NUM_QT_CBF_CTX+GET_CTX_QT_CBF(uiSizeL,false),pcBitStreamHandler);
			pcCabac->EncodeBin(uiCbfCr,OFF_QT_CBF_CTX+NUM_QT_CBF_CTX+GET_CTX_QT_CBF(uiSizeL,false),pcBitStreamHandler);
		}

		// Encode Cbf for luma
		pcCabac->EncodeBin(uiCbfY,OFF_QT_CBF_CTX+GET_CTX_QT_CBF(uiSizeL,true),pcBitStreamHandler);
		
		// Encode the luma coefficients
		if(uiCbfY)
			pcCabac->EncodeCoeffNxN(piCurrCoeffY,uiSizeL,uiBestModeL,true,pcBitStreamHandler);

		// Encode the chroma coefficients
		// For an 8x8 CU with 4 4x4 PUs, chroma coefficients are only encoded for the last
		// of the 4 PUs (4 luma PUs => 1 chroma PU)
		if(uiCurrSplitFlag != 0 || (uiCurrSplitFlag == 0 && (uiTot4x4s & 0x3) == 0x3))
		{
			if(uiCurrSplitFlag == 0 && (uiTot4x4s & 0x3) == 0x3)	// Last of the 4 4x4s
			{
				// Get the first of the 4 4x4s (left-top) and use its information
				u32 uiCurrLTIntraModeInfoC = puiCurrIntraModeInfoC[-TOT_PUS_LINE-1];
				uiModeIdxC = (uiCurrLTIntraModeInfoC >> 13) & 0x7;
				uiBestModeL = pbCurrIntraModeL[-(TOT_PUS_LINE+2)-1];
				uiCbfCb = RET_1_IF_TRUE(uiCurrLTIntraModeInfoC & 0x0100);
				uiCbfCr = RET_1_IF_TRUE(uiCurrLTIntraModeInfoC & 0x0200);
				piCurrCoeffCb -= (((CTU_HEIGHT>>1)<<1) + 2);	// Go 2 lines up and 2 pixels left
				piCurrCoeffCr -= (((CTU_HEIGHT>>1)<<1) + 2);	// Go 2 lines up and 2 pixels left
				uiSizeC = 4;
			}

			MAKE_SURE((uiModeIdxC <= 4),"Chroma mode index is wrong");

			u32 uiNewModeC = uiBestModeL;
			if(uiModeIdxC != CHROMA_DM_MODE)
			{
				uiNewModeC = puiChromaMode[uiModeIdxC];
				if(uiNewModeC == uiBestModeL)
					uiNewModeC = 34;
			}

			if(uiCbfCb)
				pcCabac->EncodeCoeffNxN(piCurrCoeffCb,uiSizeC,uiNewModeC,false,pcBitStreamHandler);
			if(uiCbfCr)
				pcCabac->EncodeCoeffNxN(piCurrCoeffCr,uiSizeC,uiNewModeC,false,pcBitStreamHandler);
		}
		
		uiTot4x4s += (1<<(((2+uiCurrSplitFlag)<<1)-4));
	}while(uiTot4x4s < (TOT_PUS_LINE*TOT_PUS_LINE));

	pcCabac->FinishEncodeCTU(IsLastTileCTU(uiAddrX,uiAddrY),IsLastSliceCTU(uiAddrX,uiAddrY),pcBitStreamHandler);
}


void H265CTUCompressor::UpdateBuffers(u32 uiAddrX, u32 uiAddrY, byte *pbYBuff, byte *pbCbBuff, byte *pbCrBuff)
{
	byte *pbCurrY = pbYBuff + uiAddrY*m_uiYStride + uiAddrX;
	byte *pbCurrCb = pbCbBuff + (uiAddrY>>1)*m_uiCStride + (uiAddrX>>1);
	byte *pbCurrCr = pbCrBuff + (uiAddrY>>1)*m_uiCStride + (uiAddrX>>1);

	byte *pbCurrRecY = m_pbRecY + 2;
	byte *pbCurrRecCb = m_pbRecCb + 1;
	byte *pbCurrRecCr = m_pbRecCr + 1;

	for(u32 i=0;i<(CTU_WIDTH>>1);i++)
	{
		memcpy(pbCurrY,pbCurrRecY,CTU_WIDTH);
		memcpy(pbCurrY+m_uiYStride,pbCurrRecY+CTU_WIDTH+2,CTU_WIDTH);
		memcpy(pbCurrCb,pbCurrRecCb,CTU_WIDTH>>1);
		memcpy(pbCurrCr,pbCurrRecCr,CTU_WIDTH>>1);
		pbCurrY += 2*m_uiYStride;
		pbCurrCb += m_uiCStride;
		pbCurrCr += m_uiCStride;
		pbCurrRecY += ((CTU_WIDTH+2)<<1);
		pbCurrRecCb += (CTU_WIDTH/2+1);
		pbCurrRecCr += (CTU_WIDTH/2+1);
	}

	// Copy the modes to the left reconstructed of the next CTU and clear the available modes
	u8 *pbCurrNeighMode = m_pbNeighIntraModeL + (TOT_PUS_LINE+2) + 1;
	u8 *pbCurrIntraModeInfoL = m_pbIntraModeInfoL + (TOT_PUS_LINE+1) + 1;
	for(i32 i=0;i<TOT_PUS_LINE;i++)
	{
		pbCurrNeighMode[i*(TOT_PUS_LINE+2)-1] = pbCurrNeighMode[i*(TOT_PUS_LINE+2)+TOT_PUS_LINE-1];
		memset(&pbCurrNeighMode[i*(TOT_PUS_LINE+2)],INVALID_MODE,TOT_PUS_LINE);
		pbCurrIntraModeInfoL[i*(TOT_PUS_LINE+1)-1] = pbCurrIntraModeInfoL[i*(TOT_PUS_LINE+1)+TOT_PUS_LINE-1];
	}

	// Now also copy the reconstructed pixels to the top reference frame buffers
	u32 uiCTUOffsetXFromTile = uiAddrX - m_cTileStartCTUPelTL.x;
	memcpy(&m_pbRefTopBuffY[1+uiCTUOffsetXFromTile],
		&m_pbRecY[2+(CTU_WIDTH-1)*(CTU_WIDTH+2)],CTU_WIDTH);
	memcpy(&m_pbRefTopBuffCb[1+uiCTUOffsetXFromTile/2],
		&m_pbRecCb[1+(CTU_WIDTH/2-1)*(CTU_WIDTH/2+1)],CTU_WIDTH/2);
	memcpy(&m_pbRefTopBuffCr[1+uiCTUOffsetXFromTile/2],
		&m_pbRecCr[1+(CTU_WIDTH/2-1)*(CTU_WIDTH/2+1)],CTU_WIDTH/2);


	// Copy the last line PU information to the top line buffer
	// (to be used by the CTU row below)
	memcpy(m_pbTopLineIntraModeInfoL+uiCTUOffsetXFromTile/MIN_CU_SIZE,
		&pbCurrIntraModeInfoL[(TOT_PUS_LINE-1)*(TOT_PUS_LINE+1)],TOT_PUS_LINE);

	// Fill the first two reference columsn for chroma LM from the last 2 columns
	// @todo Need to check whether I need this or not
	for(u32 i=0;i<CTU_HEIGHT;i++)
	{
		// +2 for the shift
		m_pbRecY[i*(CTU_WIDTH+2)-2+2] = m_pbRecY[i*(CTU_WIDTH+2)+CTU_WIDTH-2+2];
		m_pbRecY[i*(CTU_WIDTH+2)-1+2] = m_pbRecY[i*(CTU_WIDTH+2)+CTU_WIDTH-1+2];
	}
	for(u32 i=0;i<CTU_HEIGHT/2;i++)
	{
		// +1 for the shift
		m_pbRecCb[i*(CTU_WIDTH/2+1)-1+1] = m_pbRecCb[i*(CTU_WIDTH/2+1)+CTU_WIDTH/2-1+1];
		m_pbRecCr[i*(CTU_WIDTH/2+1)-1+1] = m_pbRecCr[i*(CTU_WIDTH/2+1)+CTU_WIDTH/2-1+1];
	}
}