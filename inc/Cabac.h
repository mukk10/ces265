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
* @file Cabac.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the class that Cabac's implementation.
*/

#ifndef __CABAC_H__
#define __CABAC_H__

class ImageParameters;
class BitStreamHandler;

/**
*	Context-Adaptive Binary Arithmetic Coding.
*	Populates the bitstream with CABAC.
*/
class Cabac
{
private:
	u32						m_uiLow;							//!< Cabac engine variable
	u32						m_uiRange;							//!< Cabac engine variable
	i32						m_iBitsLeft;						//!< Cabac engine variable
	u8						m_bBuffer;							//!< Cabac engine variable
	u32						m_uiNumByte;						//!< Total bytes
	u8						m_pbContextModels[MAX_NUM_CTX_MOD];	//!< Context models for the CABAC for storing neighborhood
	ImageParameters const	*m_pcImageParameters;				//!< Image parameters

	/**
	*	Initialize CABAC enteries.
	*	@param uiTotalInputs Total inputs to the CABAC engine.
	*	@param uiQP QP value.
	*	@param pbContextModels Context mode table for update.
	*	@param pbInitVals Initital values used.
	*/
	void	InitCabacEntry(u32 uiTotalInputs, u32 uiQP, u8 *pbContextModels, const u8 *pbInitVals);

	/**
	*	Write Cabac output to the bitstream.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	WriteOutputBitstream(BitStreamHandler *& pcBitStreamHandler);

	/**
	*	Get coefficient scanning mode.
	*	@param uiSize Size of the block.
	*	@param uiMode Mode of the block.
	*	@param bIsLuma If 1, denotes that current block is luma.
	*/
	u32		GetCoeffScanIdx(u32 uiSize, u32 uiMode, bit bIsLuma);

	/**
	*	Code the last significant location.
	*	@param uiPosX Displacement from left.
	*	@param uiPosY Displacement from top.
	*	@param uiSize Size of the block.
	*	@param uiScanIdx Scanning index.
	*	@param bIsLuma If 1, denotes that current block is luma.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	CodeLastSignifXY(u32 uiPosX, u32 uiPosY, u32 uiSize, u32 uiScanIdx, bit bIsLuma, BitStreamHandler *& pcBitStreamHandler);

	/**
	*	Get the context of the group.
	*	@param ubSigCoeffGroupFlag Signal coefficients group flag.
	*	@param iCGPosX Displacement from left.
	*	@param iCGPosY Displacement from Top.
	*	@param uiScanIdx Scanning index.
	*	@param uiSize Size of the block.
	*/
	u32		GetSigCoeffGroupCtxInc(const u8 *ubSigCoeffGroupFlag, const i32 iCGPosX, const i32 iCGPosY, const u32 uiScanIdx, u32 uiSize);

	/**
	* Pattern decision for context derivation process of significant_coeff_flag.
	* @param sigCoeffGroupFlag pointer to prior coded significant coeff group.
	* @param nPosXCG column of current coefficient group.
	* @param nPosYCG row of current coefficient group.
	* @param nSize width/height of the block.
	* @returns pattern for current coefficient group.
	*/
	i32		CalcPatternSigCtx(const u8 *ubSigCoeffGroupFlag, i32 iCGPosX, i32 iCGPosY, u32 uiSize);

	/**
	* Context derivation process of coeff_abs_significant_flag.
	* @param patternSigCtx pattern for current coefficient group.
	* @param posX column of current scan position.
	* @param posY row of current scan position.
	* @param blockType log2 value of block size if square block, or 4 otherwise.
	* @param nSize width/height of the block.
	* @param bIsLuma texture type (TEXT_LUMA...).
	* @returns ctxInc for current scan position.
	*/
	i32		GetSigCtxInc(i32 iPatternSigCtx, u32 uiScanIdx, i32 iPosX, i32 iPosY, i32 iBlkType, u32 uiSize, bit bIsLuma);

	/**
	*	Write the remaining coefficients using Exponential Golumb.
	*	@param iSymbol Input symbol.
	*	@param uiParam Parameter.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	WriteCoeffRemainExGolomb(i32 iSymbol, u32 uiParam, BitStreamHandler *& pcBitStreamHanlder);

public:

	/**
	*	Constructor.
	*	@param pcImageParameters Image parameters for a video frame.
	*/
	Cabac(ImageParameters const *pcImageParameters);
	~Cabac(){};

	/**
	*	Initialize the CABAC generator.
	*/
	void	InitCabac();

	/**
	*	Reset CABAC.
	*/
	void	ResetCabac();

	/**
	*	Encode a value.
	*	@param uiBinVal Binary value to be encoded.
	*	@param uiCtxState Context of the value.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	EncodeBin(u32 uiBinVal, u32 uiCtxState, BitStreamHandler *& pcBitStreamHandler);

	/**
	*	Encode Binary values.
	*	@param uiBinValues Values to be encoded.
	*	@param uiNumBins Number of binaraies.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	EncodeBinsEP(u32 uiBinValues, u32 uiNumBins, BitStreamHandler *& pcBitStreamHandler);

	/**
	*	Encode quantized coefficients.
	*	@param piCoeff Input coefficients.
	*	@param uiSize Size of the block.
	*	@param uiMode Current encoding mode.
	*	@param bIsLuma If 1, denotes that current block is luma.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	EncodeCoeffNxN(i16 *piCoeff, u32 uiSize, u32 uiMode, bit bIsLuma, BitStreamHandler *& pcBitStreamHandler);

	/**
	*	Encode luma intra angular group.
	*	The puiPredIdx array has 4 enteries.
	*	@param uiTotPUs Total number of PUs (it is 4 for 4x4s).
	*	@param puiPredIdx Prediction indices of the PUs.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	EncodeIntraDirAngGrpL(u32 uiTotPUs, u32 *puiPredIdx, BitStreamHandler *& pcBitStreamHandler);

	/**
	*	Encode chroma intra angular direction.
	*	@param uiModeIdxC Mode index for chroma component.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	EncodeIntraDirAngC(u32 uiModeIdxC, BitStreamHandler *& pcBitStreamHandler);

	/**
	*	Encode the terminating bit.
	*	@param uiBinValue Binary value to be encoded.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	EncodeTerminatingBit(u32 uiBinValue, BitStreamHandler *& pcBitStreamHanlder);

	/**
	*	Flush Cabac stream.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	Flush(BitStreamHandler *& pcBitStreamHanlder);

	/**
	*	Finishing CTU encoding.
	*	@param bIsLastTileCTU If 1, then this is the bottom right CTU of the full tile.
	*	@param bisLastSliceCTU If 1, then this is the bottom right CTU of the full slice.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	void	FinishEncodeCTU(bit bIsLastTileCTU, bit bIsLastSliceCTU, BitStreamHandler *& pcBitStreamHandler);
};

#endif // __CABAC_H__