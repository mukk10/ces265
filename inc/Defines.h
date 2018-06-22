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
* @file Defines.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the definitions and preprocessor directives used in this project
*/

#ifndef __DEFINES_H__
#define __DEFINES_H__

// Version information
#define			CES_H265_VER					"0"				//!<	Current version
#if defined _WIN32 || defined _WIN64
#define			CES_H265_OS						"Win"			//!<	Windows platform
#elif defined __linux
#define			CES_H265_OS						"Linux"			//!<	Linux platform
#endif

// For Debugging
#define			DEBUG_APP										//!<	Define this to debug the application
#define			TEST_MEMORY_LEAK								//!<	Define this to test memory leaks in the program (for windows only, use valgrind instead)

#if defined DEBUG_APP && defined _MSC_VER && defined TEST_MEMORY_LEAK
#define			TEST_MEMORY_LEAKS
#endif

#ifdef _MSC_VER
#ifndef			_CRT_SECURE_NO_WARNINGS
#define			_CRT_SECURE_NO_WARNINGS
#endif
#endif

// General
#define			FILE_NAME_LEN						100			//!<	The file names can be maximum of this length

// Encoding
#define			CTU_WIDTH							32			//!<	CTU/CU width (make sure it completely divides the frame width)
#define			CTU_HEIGHT							32			//!<	CTU/CU height (make sure it completely divides the frame height)
#define			MIN_CU_SIZE							4			//!<	Minimum CU width or height
#define			TOT_PUS_LINE						CTU_WIDTH/MIN_CU_SIZE	//!< Total PUs possible in a row/col of a CTU
#define			MAX_TILES							24			//!<	Maximum tiles allowed per slice
#define			BYTES_PER_CTU						800			//!<	Total bytes an encoded CTU will presumably take 

// Threads
#define			USE_THREADS							1			//!<	Multithreading using pthreads will be used
#define			MAX_GOP_THREADS						2			//!<	Maximum number of GOP threads. Minimum is 1.
#define			MAX_SLICE_THREADS					4			//!<	Maximum number of Slice threads. Minimum is 1.
#define			MAX_TILE_THREADS					24			//!<	Maximum number of Tile threads. Minimum is 1.

// Intra modes
#define			TOTAL_INTRA_MODES					36			//!<	Total number of intra modes available
#define			INVALID_MODE						255			//!<	Denotes unavailability
#define			VALID_MODE							0			//!<	Modes are available
#define			PLANAR_MODE_IDX						0			//!<	Index for the Planar mode
#define			DC_MODE_IDX							1			//!<	Index for the DC mode
#define			HOR_MODE_IDX						10			//!<	Index for the Horizontal mode
#define			VER_MODE_IDX						26			//!<	Index for the Vertical mode
#define			CHROMA_DM_MODE						4			//!<	Location of the chroma DM mode
#define			CHROMA_LM_MODE						5			//!<	Index for the intra chroam LM mode
#define			CHROMA_DM_MODE_IDX					36			//!<	Index for the intra chroma DM mode
#define			USE_CHROMA_LM_MODE					0			//!<	To use chroma LM mode or not (this should be set to 0 if the main profile is used, see A.3.2 in the draft). DON'T USE THIS AS IT IS INCOMPLETE

// Quantization
#define			MAX_TR_DYN_RANGE					15			//!<	Total dynamic range of the transformed coefficients
#define			QUANT_SHIFT							14			//!<	Total qunatization shift
#define			IQUANT_SHIFT						20			//!<	Total inverse quantization shift

// Transform
#define			SHIFT_INV_1							7			//!<	Shift after 1st butterfly of IDCT
#define			SHIFT_INV_2							12			//!<	Shift after 2nd butterfly of IDCT

// CABAC
#define			MAX_NUM_CTX_MOD						256			//!<	maximum number of supported contexts
#define			NUM_SPLIT_FLAG_CTX					3			//!<	Number of context models for split flag
#define			NUM_SKIP_FLAG_CTX					3			//!<	Number of context models for skip flag
#define			NUM_MERGE_FLAG_EXT_CTX				1			//!<	Number of context models for merge flag of merge extended
#define			NUM_MERGE_IDX_EXT_CTX				1			//!<	Number of context models for merge index of merge extended
#define			NUM_ALF_CTRL_FLAG_CTX				1			//!<	Number of context models for ALF control flag
#define			NUM_PART_SIZE_CTX					4			//!<	Number of context models for partition size
#define			NUM_CU_AMP_CTX						1			//!<	Number of context models for partition size (AMP)
#define			NUM_PRED_MODE_CTX					1			//!<	Number of context models for prediction mode
#define			NUM_ADI_CTX							1			//!<	Number of context models for intra prediction
#define			NUM_CHROMA_PRED_CTX					2			//!<	Number of context models for intra prediction (chroma)
#define			NUM_INTER_DIR_CTX					4			//!<	Number of context models for inter prediction direction
#define			NUM_MV_RES_CTX						2			//!<	Number of context models for motion vector difference
#define			NUM_REF_NO_CTX						2			//!<	Number of context models for reference index
#define			NUM_TRANS_SUBDIV_FLAG_CTX			3			//!<	Number of context models for transform subdivision flags
#define			NUM_QT_CBF_CTX						5			//!<	Number of context models for QT CBF
#define			NUM_QT_ROOT_CBF_CTX					1			//!<	Number of context models for QT ROOT CBF
#define			NUM_DELTA_QP_CTX					3			//!<	Number of context models for dQP
#define			NUM_SIG_CG_FLAG_CTX					2			//!<	Number of context models for MULTI_LEVEL_SIGNIFICANCE
#define			NUM_SIG_FLAG_CTX					42			//!<	Number of context models for sig flag
#define			NUM_SIG_FLAG_CTX_LUMA				27			//!<	Number of context models for luma sig flag
#define			NUM_SIG_FLAG_CTX_CHROMA				15			//!<	Number of context models for chroma sig flag
#define			NUM_LAST_FLAG_XY_CTX				15			//!<	Number of context models for last coefficient position
#define			NUM_ONE_FLAG_CTX					24			//!<	Number of context models for greater than 1 flag
#define			NUM_ONE_FLAG_CTX_LUMA				16			//!<	Number of context models for greater than 1 flag of luma
#define			NUM_ONE_FLAG_CTX_CHROMA				8			//!<	Number of context models for greater than 1 flag of chroma
#define			NUM_ABS_FLAG_CTX					6			//!<	Number of context models for greater than 2 flag
#define			NUM_ABS_FLAG_CTX_LUMA				4			//!<	Number of context models for greater than 2 flag of luma
#define			NUM_ABS_FLAG_CTX_CHROMA				2			//!<	Number of context models for greater than 2 flag of chroma
#define			NUM_MVP_IDX_CTX						2			//!<	Number of context models for MVP index
#define			NUM_TRANSFORMSKIP_FLAG_CTX			1			//!<	Number of context models for transform skipping
#define			CNU									154			//!<	Dummy initialization value for unused context models 'Context model Not Used'
#define			OFF_SPLIT_FLAG_CTX					( 0 )
#define			OFF_SKIP_FLAG_CTX					( OFF_SPLIT_FLAG_CTX        +   NUM_SPLIT_FLAG_CTX      )
#define			OFF_ALF_CTRL_FLAG_CTX				( OFF_SKIP_FLAG_CTX         +   NUM_SKIP_FLAG_CTX       )
#define			OFF_MERGE_FLAG_EXT_CTX				( OFF_ALF_CTRL_FLAG_CTX     +   NUM_ALF_CTRL_FLAG_CTX   )
#define			OFF_MERGE_IDX_EXT_CTX				( OFF_MERGE_FLAG_EXT_CTX    +   NUM_MERGE_FLAG_EXT_CTX  )
#define			OFF_PART_SIZE_CTX					( OFF_MERGE_IDX_EXT_CTX     +   NUM_MERGE_IDX_EXT_CTX   )
#define			OFF_CU_AMP_CTX						( OFF_PART_SIZE_CTX         +   NUM_PART_SIZE_CTX       )
#define			OFF_PRED_MODE_CTX					( OFF_CU_AMP_CTX            +   NUM_CU_AMP_CTX          )
#define			OFF_INTRA_PRED_CTX					( OFF_PRED_MODE_CTX         +   NUM_PRED_MODE_CTX       )
#define			OFF_CHROMA_PRED_CTX					( OFF_INTRA_PRED_CTX        +   NUM_ADI_CTX             )
#define			OFF_INTER_DIR_CTX					( OFF_CHROMA_PRED_CTX       +   NUM_CHROMA_PRED_CTX     )
#define			OFF_MVD_CTX							( OFF_INTER_DIR_CTX         +   NUM_INTER_DIR_CTX       )
#define			OFF_REF_PIC_CTX						( OFF_MVD_CTX               +   NUM_MV_RES_CTX          )
#define			OFF_DELTA_QP_CTX					( OFF_REF_PIC_CTX           +   NUM_REF_NO_CTX          )
#define			OFF_QT_CBF_CTX						( OFF_DELTA_QP_CTX          +   NUM_DELTA_QP_CTX        )
#define			OFF_QT_ROOT_CBF_CTX					( OFF_QT_CBF_CTX            + 2*NUM_QT_CBF_CTX          )
#define			OFF_SIG_CG_FLAG_CTX					( OFF_QT_ROOT_CBF_CTX       +   NUM_QT_ROOT_CBF_CTX     )
#define			OFF_SIG_FLAG_CTX					( OFF_SIG_CG_FLAG_CTX       + 2*NUM_SIG_CG_FLAG_CTX     )
#define			OFF_LAST_X_CTX						( OFF_SIG_FLAG_CTX          +   NUM_SIG_FLAG_CTX        )
#define			OFF_LAST_Y_CTX						( OFF_LAST_X_CTX            + 2*NUM_LAST_FLAG_XY_CTX    )
#define			OFF_ONE_FLAG_CTX					( OFF_LAST_Y_CTX            + 2*NUM_LAST_FLAG_XY_CTX    )
#define			OFF_ABS_FLAG_CTX					( OFF_ONE_FLAG_CTX          +   NUM_ONE_FLAG_CTX        )
#define			OFF_MVP_IDX_CTX						( OFF_ABS_FLAG_CTX          +   NUM_ABS_FLAG_CTX        )
#define			OFF_TRANS_SUBDIV_FLAG_CTX			( OFF_MVP_IDX_CTX           +   NUM_MVP_IDX_CTX         )
#define			OFF_TS_FLAG_CTX						( OFF_TRANS_SUBDIV_FLAG_CTX +   NUM_TRANS_SUBDIV_FLAG_CTX)
#define			MLS_CG_SIZE							4			//!<	G644: Coefficient group size of 4x4
#define			MLS_GRP_NUM							64			//!<	G644: Max number of coefficient groups, max(16, 64)
#define			LOG2_SCAN_SET_SIZE					4
#define			C1FLAG_NUMBER						8			//!<	maximum number of largerThan1 flag coded in one chunk :  16 in HM5
#define			C2FLAG_NUMBER						1			//!<	maximum number of largerThan2 flag coded in one chunk:  16 in HM5
#define			COEF_REMAIN_BIN_REDUCTION			3			//!<	J0142: Maximum codeword length of coeff_abs_level_remaining reduced to 32.
																//!<	COEF_REMAIN_BIN_REDUCTION is also used to indicate the level at which the VLC 
																//!<	transitions from Golomb-Rice to TU+EG(k)

// Default inputs
#define 		INIT_FRAME_RATE 					30
#define			INIT_QP								32
#define			INIT_GOP_SIZE						1
#define			PSNR_NUMERATOR						65025.0		//!<	For image values between [0,255] inclusive

#endif
