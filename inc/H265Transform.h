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
* @file H265Transform.h
* @author Muhammad Usman Karim Khan
* Group: Prof. Joerg Henkel at Chair for Embedded Systems (CES), KIT
* @brief This file contains the class that handles H265 Transform related functions
*/

#ifndef __H265TRANSFORM_H__
#define	__H265TRANSFORM_H__

#include <TypeDefs.h>

/**
*	HEVC Transform.
*	Implements the transforms for HEVC, from 32x32 to 4x4 DCT and 4x4 DST.
*/
class H265Transform
{
private:
	typedef void	(H265Transform::*DCT)(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);		//!< Function pointer type definition
	typedef void	(H265Transform::*IDCT)(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);		//!< Function pointer type definition
	DCT		DCTN[5];																								//!< Function pointers to DCT 
	IDCT	IDCTN[5];																								//!< Function pointers to IDCT

	/**
	*	4x4 DST.
	*	@param piDest Destination pointer.
	*	@param piSrc Sournce data.
	*	@param uiStride Stride for the next line of source.
	*	@param uiTrLines Total size of the transform.
	*	@param uiShift Shift of the transform.
	*/
	void	DST4(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);

	/**
	*	4x4 DCT.
	*	@param piDest Destination pointer.
	*	@param piSrc Sournce data.
	*	@param uiStride Stride for the next line of source.
	*	@param uiTrLines Total size of the transform.
	*	@param uiShift Shift of the transform.
	*/
	void	DCT4(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);

	/**
	*	8x8 DCT.
	*	@param piDest Destination pointer.
	*	@param piSrc Sournce data.
	*	@param uiStride Stride for the next line of source.
	*	@param uiTrLines Total size of the transform.
	*	@param uiShift Shift of the transform.
	*/
	void	DCT8(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);

	/**
	*	16x16 DCT.
	*	@param piDest Destination pointer.
	*	@param piSrc Sournce data.
	*	@param uiStride Stride for the next line of source.
	*	@param uiTrLines Total size of the transform.
	*	@param uiShift Shift of the transform.
	*/
	void	DCT16(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);

	/**
	*	32x32 DCT.
	*	@param piDest Destination pointer.
	*	@param piSrc Sournce data.
	*	@param uiStride Stride for the next line of source.
	*	@param uiTrLines Total size of the transform.
	*	@param uiShift Shift of the transform.
	*/
	void	DCT32(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);

	/**
	*	4x4 IDST.
	*	@param piDest Destination pointer.
	*	@param piSrc Sournce data.
	*	@param uiStride Stride for the next line of source.
	*	@param uiTrLines Total size of the transform.
	*	@param uiShift Shift of the transform.
	*/
	void	IDST4(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);

	/**
	*	4x4 IDCT.
	*	@param piDest Destination pointer.
	*	@param piSrc Sournce data.
	*	@param uiStride Stride for the next line of source.
	*	@param uiTrLines Total size of the transform.
	*	@param uiShift Shift of the transform.
	*/
	void	IDCT4(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);

	/**
	*	8x8 IDCT.
	*	@param piDest Destination pointer.
	*	@param piSrc Sournce data.
	*	@param uiStride Stride for the next line of source.
	*	@param uiTrLines Total size of the transform.
	*	@param uiShift Shift of the transform.
	*/
	void	IDCT8(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);

	/**
	*	16x16 IDCT.
	*	@param piDest Destination pointer.
	*	@param piSrc Sournce data.
	*	@param uiStride Stride for the next line of source.
	*	@param uiTrLines Total size of the transform.
	*	@param uiShift Shift of the transform.
	*/
	void	IDCT16(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);

	/**
	*	32x32 IDCT.
	*	@param piDest Destination pointer.
	*	@param piSrc Sournce data.
	*	@param uiStride Stride for the next line of source.
	*	@param uiTrLines Total size of the transform.
	*	@param uiShift Shift of the transform.
	*/
	void	IDCT32(i16 *piDest, i16 *piSrc, u32 uiStride, u32 uiTrLines, u32 uiShift);
public:

	/**
	*	Constructor.
	*/
	H265Transform();
	~H265Transform(){};

	/**
	*	Generate residue and DCT transform.
	*	The residue array needs to be a 1D array of size uiWidth x uiHeight.
	*	After the DCT, the output will contain 2Mx2M data in a linear order. I.e. a 4x4 will start from array location 0 and end at array location 16.
	*	@param piOutput Output buffer pointer.
	*	@param uiWidth Width of the transform.
	*	@param uiHeight Height of the transform.
	*	@param pbSrc Source pointer.
	*	@param uiSrcStride Stride within the source for the next line.
	*	@param pbRef Reference pointer.
	*	@param uiRefStride Stride within the reference for the next line.
	*	@param piRes Output residual pointer.
	*	@param piResHorTrans A temporary buffer of size uiWidthxuiHeight.
	*	@param uiMode Mode of the encoding.
	*/
	void	ResDCT(i16 *piOutput, u32 uiWidth, u32 uiHeight, byte *pbSrc, u32 uiSrcStride, byte *pbRef, u32 uiRefStride, i16 *piRes, i16 *piResHorTrans, u32 uiMode);	

	/**
	*	Generate the quantized coefficients.
	*	After Quantization, the output will contain 2Mx2M data in a linear order. I.e. a 4x4 will start from array location 0 and end at array location 16.
	*	@param piOutput Output pointer.
	*	@param uiOutputStride Stride within the output for the next line.
	*	@param uiQP QP value.
	*	@param uiWidth Width of the block.
	*	@param uiHeight Height of the block.
	*	@param piSrc Source pointer.
	*	@param uiSrcStride Stride within the source for the next line.
	*	@param eST Type of slice (I or P).
	*/
	u32		Quant(i16 *piOutput, u32 uiOutputStride, u32 uiQP, u32 uiWidth, u32 uiHeight, i16 *piSrc, u32 uiSrcStride, eSliceType eST);

	/**
	*	Generate inverse qunatized coefficients.
	*	After Inverse Quantization, the output will contain 2Mx2M data in a linear order. I.e. a 4x4 will start from array location 0 and end at array location 16.
	*	@param piOutput Output pointer.
	*	@param uiOutputStride Stride within the output for the next line.
	*	@param uiQP QP value.
	*	@param uiWidth Width of the block.
	*	@param uiHeight Height of the block.
	*	@param piSrc Source pointer.
	*	@param uiSrcStride Stride within the source for the next line.
	*	@param eST Type of slice (I or P).
	*/
	void	InvQuant(i16 *piOutput, u32 uiOutputStride, u32 uiQP, u32 uiWidth, u32 uiHeight, i16 *piSrc, u32 uiSrcStride, eSliceType eST);

	/**
	*	Recontruct after IDCT.
	*	The output of the butterflies have the same stride as the source. Therefore, allocate the same amount of memory to the butterflies as is allocated (or accessed) in the source.
	*	@param pbOutput Output pointer.
	*	@param uiOutputStride Stride within the output for the next line.
	*	@param uiWidth Width of the block.
	*	@param uiHeight Height of the block.
	*	@param piSrc Source pointer.
	*	@param uiSrcStride Stride within the source for the next line.
	*	@param pbRef Reference pointer.
	*	@param uiRefStride Stride within the reference for the next line.
	*	@param uiBufferflyOut1 Temporary buffer of size at least uiWidthxuiHeight.
	*	@param uiMode Mode of the encoding.
	*/
	void	IDCTRec(byte *pbOutput, u32 uiOutputStride, u32 uiWidth, u32 uiHeight, i16 *piSrc, u32 uiSrcStride, byte *pbRef, u32 uiRefStride, i16 *piButterflyOut1, i16 *piButterflyOut2, u32 uiMode);	
};

#endif	// __H265TRANSFORM_H__