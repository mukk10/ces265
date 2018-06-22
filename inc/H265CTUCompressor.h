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
* @file H265CTUCompressor.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the class that handles image compression functions.
*/

#ifndef __H265COMPRESSOR_H__
#define	__H265COMPRESSOR_H__

#include <Defines.h>
#include <TypeDefs.h>

class InputParameters;
class ImageParameters;
class H265Transform;
class BitStreamHandler;
class Cabac;

/**
*	CTU compressor.
*	Compress the current CTU.
*/
class H265CTUCompressor
{
private:
	InputParameters const	*m_pcInputParam;							//!< Input parameters
	ImageParameters const	*m_pcImageParam;							//!< Image parameters
	H265Transform			*m_pcH265Trans;								//!< Transform and quantization
	byte					*m_pbRefTopBuffY;							//!< Top pixels of luma
	byte					*m_pbRefTopBuffCb;							//!< Top pixels of CB
	byte					*m_pbRefTopBuffCr;							//!< Top pixels of CR
	byte					m_ppbTempPred4[2][4*4];						//!< Prediction buffer for 4x4
	byte					m_ppbTempPred8[2][8*8];						//!< Prediction buffer for 8x8
	byte					m_ppbTempPred16[2][16*16];					//!< Prediction buffer for 16x16
	byte					m_ppbTempPred32[2][32*32];					//!< Prediction buffer for 32x32
	byte					m_ppbTempPred64[2][64*64];					//!< Prediction buffer for 64x64
	byte					*m_pppbTempPred[2][5];						//!< Pointer storing all the temporaray predictors
	i16						m_ppiTransBuffTmp[2][CTU_HEIGHT*CTU_WIDTH];	//!< Temporary buffer to hold the transformed data
	i16						m_piCoeffY[CTU_WIDTH*CTU_WIDTH];			//!< Transformed luma coefficients
	i16						m_piCoeffCb[CTU_WIDTH*CTU_WIDTH>>2];		//!< Transformed Cb coefficients
	i16						m_piCoeffCr[CTU_WIDTH*CTU_WIDTH>>2];		//!< Transformed Cr coefficients
	u32						m_uiYStride;								//!< Width of luma frame
	u32						m_uiCStride;								//!< Width of the chroma frame
	u32						m_uiQP;										//!< Quantization parameter
	u32						m_uiTileWidthInPels;						//!< Width of the tile under process
	u32						m_uiTileHeightInPels;						//!< Height of the tile under process
	pixel					m_cTileStartCTUPelTL;						//!< Tile starting pixel
	pixel					m_cTileEndCTUPelTL;							//!< Tile ending pixel
	u8						m_pbNeighIntraModeL[(TOT_PUS_LINE+2)*(TOT_PUS_LINE+2)];		//!< Modes of the neighboring blocks. There is a mode for every PU (+2 for the top and left/right and bottom and left/right)
	byte					m_pbRecY[(CTU_WIDTH+2)*CTU_WIDTH];			//!< Stores the reconstructed Y samples (+2 the left of the CTU for chroma from luma prediction (LM chroma mode))
	byte					m_pbRecCb[(CTU_WIDTH/2+1)*CTU_WIDTH/2];		//!< Stores the reconstructed Cb samples
	byte					m_pbRecCr[(CTU_WIDTH/2+1)*CTU_WIDTH/2];		//!< Stores the reconstructed Cb samples
	byte					m_pbRefYUnfiltered[4*CTU_WIDTH + 1];		//!< Unfiltered reference samples for bottom left, left, top and top right.
	byte					m_pbRefYFiltered[4*CTU_WIDTH + 1];			//!< Filtered reference samples for bottom left, left, top and top right.
	byte					*m_pbRefCb;									//!< Holds the reference Cb samples
	byte					*m_pbRefCr;									//!< Holds the reference Cr samples
	u16						m_puiIntraModeInfoC[TOT_PUS_LINE*TOT_PUS_LINE];		//!< Stores information about chroma mode
	u8						m_pbIntraModeInfoL[(TOT_PUS_LINE+1)*(TOT_PUS_LINE+1)];	//!< Stores information about luma mode
	u8						m_pbIntraModePredInfoL[(TOT_PUS_LINE+1)*(TOT_PUS_LINE+1)];	//!< Keeps the information about the predicted angular mode (@todo Delete this)
	u8						*m_pbTopLineIntraModeInfoL;					//!< Holds the top line mode info from the above CTU row
	byte					m_bSavedLTY;								//!< Left top pixel for luma
	byte					m_bSavedLTCb;								//!< Left top pixel for Cb
	byte					m_bSavedLTCr;								//!< Left top pixel for Cr
	u32						m_ctTimeForCTU;								//!< Time consumed for processing the current CTU

	/**
	*	Prepare the CTU for prediction.
	*	Adjust the neighboring modes and other stuff.
	*/
	void					PrepareCTU(u32 uiAddrX, u32 uiAddrY);

	/**	
	*	CTU processing finisher.
	*	Re-adjust the changes that were made via PrepareCTU() and other stuff.
	*/
	void					FinishCTU(u32 uiAddrX, u32 uiAddrY);

	/**
	*	Get displacement from CTU boundaries, given the total 4x4s traversed.
	*	The left and top discplacements are for the luma CU from the current CTU.
	*	This is used in compressing the chroma CU.
	*/
	void					GetDispFrom4x4s(u32 uiTot4x4s,u32 &uiDispCTULeft, u32 &uiDispCTUTop);

	/**
	*	Initialize the pointers.
	*	Initialization is done via the displacement of the current CU from the left and top of the CTU.
	*/
	void					InitCUPointers(byte *& pbCurrIntraModeL, byte *& pbCurrRecY, byte *& pbCurrRecCb, byte *& pbCurrRecCr, 
									 i16 *& piCurrCoeffY, i16 *& piCurrCoeffCb, i16 *& piCurrCoeffCr, 
									 byte *& pbCurrRefTopBuffY, byte *& pbCurrRefTopBuffCb, byte *& pbCurrRefTopBuffCr,
									 byte *& pbCurrIntraModeInfoL, byte *& pbCurrIntraModePredInfoL, u16 *& puiCurrIntraModeInfoC,
									 u32 uiDispCTULeft, u32 uiDispCTUTop, u32 uiCTUOffsetXFromTile);

	/**
	*	Set the reference array pointers for luma.
	*/
	void					InitRefPointers(u32 *puiRefPointArr, u32 uiSize);

	/**
	*	Check the neighborhood availability.
	*/
	u8						CheckNeighAvail(u8 *pbTopAndLeftModes, byte *pbCurrIntraModeL, u32 uiSize);

	/**
	*	Generate the candidate mode list for intra.
	*/
	void					GetCandModeListIntra(u8 *pbTopAndLeftModes, u32 uiDispCTUTop, u8 *pbCandModeListIntra);

	/**
	*	Generate reference samples for intra prediction.
	*	Note that the uiSize parameter is not the size of the luma CU, but the size of the current CU (which can be both for luma and chroma).
	*/
	void					GenReferenceSamplesIntra(byte *pbCurrRecY, byte *pbCurrRecCb, byte *pbCurrRecCr,
												 byte *pbCurrRefTopBuffY, byte *pbCurrRefTopBuffCb, byte *pbCurrRefTopBuffCr,
												 u32 *puiRefYOffset, u32 *puiRefCOffset,
												 u8 bValidFlag, u32 uiSize, u32 uiDispCTUTop, bit bIsChroma);

	/**
	*	Generate chroma LM mode reference.
	*/
	void					GenerateReferenceSamplesChromaLM(byte *pbCurrRecY, byte *pbCurrRefTopBuffY, byte *pbLMRefTop,byte *pbLMRefLeft, u8 bValidFlag, u32 uiDispCTUTop, u32 uiSizeChroma);

	/**
	*	Substitute the reference samples.
	*	Depending upon the availability, some of the reference samples are unavailable which are generated on the go.
	*	Note that the uiSize parameter is not the size of the luma CU, but the size of the current CU (which can be both for luma and chroma).
	*/
	void					SubstituteReference(byte *puiRefArr, u32 *puiRefOffset, byte bValidFlag, u32 uiSize);

	/**
	*	Generate Prediciton samples.
	*/
	void					GenIntraPrediction(byte *pbPred, byte *pbRef, u32 uiSize, u32 uiMode, bit bIsChroma);

	/**
	*	Generate Intra DC Prediction.
	*/
	void					GenIntraPredDC(byte *pbPred, byte *pbRef, u32 uiSize, u32 uiMode, bit bIsChroma);

	/**
	*	Generate Intra Planar Prediction.
	*/
	void					GenIntraPredPlanar(byte *pbPred, byte *pbRef, u32 uiSize, u32 uiMode, bit bIsChroma);

	/**
	*	Generate Intra Angular Prediction.
	*	The prediction must be a single dimensional array. It should not be dynamically allocated 2-D array.
	*/
	void					GenIntraPredAngular(byte *pbPred, byte *pbRef, u32 uiSize, u32 uiMode, bit bIsChroma);

	/**
	*	Compute the MAD.
	*/
	u32						MAD_MxN(u32 uiM, u32 uiN, byte *pbSrc, u32 uiSrcStride, byte *pbRef, u32 uiRefStride);

	/**
	*	Compute the SAD.
	*/
	u32						SAD_MxN(u32 uiM, u32 uiN, byte *pbSrc, u32 uiSrcStride, byte *pbRef, u32 uiRefStride);
	
	/**
	*	Map the final best mode to an index.
	*	Changes the candidate mode list and also the prediciton index.
	*/
	i32						MapModeToIndex(u32 uiBestMode, u8 *pbCandModeListIntra);

	/**
	*	Recursively compress a CU.
	*/
	u32						xCompressLumaCU(u32 uiAddrX, u32 uiAddrY, byte *pbBuff, u32 uiSize, u32 uiDispCTULeft, u32 uiDispCTUTop, bit bIsChroma);

	/**
	*	Compress chroma components.
	*	Note that the addresses provided are for luma.
	*/
	void					CompressChromaCU(u32 uiAddrX, u32 uiAddrY, byte *pbCbBuff, byte *pbCrBuff);

	/**	
	*	Check if last CTU of the tile.
	*/
	bit						IsLastTileCTU(u32 uiAddrX, u32 uiAddrY);

	/**
	*	Check if last CTU of the slice.
	*	This is important for the end_of_slice_segment_flag.
	*/
	bit						IsLastSliceCTU(u32 uiAddrX, u32 uiAddrY);

public:

	/**
	*	Constructor.
	*	@param pcInputParam Input parameters to the program.
	*	@param pcImageParam Image parameters of a video frame.
	*	@param cTileStartCTUPelTL Top left (x,y) pixel locations of the the top left CTU of the tile which contains this CTU compressor.
	*	@param cTileEndCTUPelTL Top left (x,y) pixel location of the bottom right CTU of the tile which contains this CTU compressor.
	*/
	H265CTUCompressor(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, pixel cTileStartCTUPelTL, pixel cTileEndCTUPelTL);
	~H265CTUCompressor();
	/**
	*	Compress a CTU.
	*	@param uiAddrX Absolute displacement of the CTU from the left of the picture.
	*	@param uiAddrY Absolute displacement of the CTU from the top of the picture.
	*	@param pbYBuff Contains the luma pixels.
	*	@param pbCbBuff Contains the Cb pixels.
	*	@param pbCrBuff Contains the Cr pixels.
	*/
	void					CompressCTU(u32 uiAddrX, u32 uiAddrY, byte *pbYBuff, byte *pbCbBuff, byte *pbCrBuff);

	/**
	*	Encode a CTU.
	*	Use CABAC after compressing the CTU. 
	*	@see CompressCTU()
	*	@param uiAddrX Absolute displacement of the CTU from the left of the picture.
	*	@param uiAddrY Absolute displacement of the CTU from the top of the picture.
	*	@param pcCabac CABAC encoder.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void					EncodeCTU(u32 uiAddrX, u32 uiAddrY, Cabac *& pcCabac, BitStreamHandler *& pcBitStreamHandler);

	/**
	*	Initialize the buffers on new CTU line.
	*	If a new CTU line within a tile starts, some of the internal buffers of the CTU compressor must be initialized again.
	*/
	void					InitBuffersNewCTULine();

	/**
	*	Initialize the buffers on a new frame.
	*	Initialize the internal buffers after a full frame is being processed.
	*/
	void					InitBuffersNewTile();

	/**
	*	Update the reconstructed pixels by replacing the current ones.
	*	@param uiAddrX Absolute displacement of the CTU from the left of the picture.
	*	@param uiAddrY Absolute displacement of the CTU from the top of the picture.
	*	@param pbYBuff Contains the luma pixels.
	*	@param pbCbBuff Contains the Cb pixels.
	*	@param pbCrBuff Contains the Cr pixels.
	*/
	void					UpdateBuffers(u32 uiAddrX, u32 uiAddrY, byte *pbYBuff, byte *pbCbBuff, byte *pbCrBuff);

	/**
	*	Get the time consumed for the CTU.
	*	@return Time in msecs consumed for encoding the CTU.
	*/
	u32						GetTimePerCTU(){return m_ctTimeForCTU;}
};

#endif	// __H265COMPRESSOR_H__