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
* @file H265Headers.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods related to the class H265Headers
*/

#include <H265Headers.h>
#include <InputParameters.h>
#include <ImageParameters.h>
#include <BitStreamHandler.h>

H265Headers::H265Headers()
{

}

H265Headers::~H265Headers()
{

}

/***********************VPS*************************/
void H265Headers::WriteVPSInBitstream(BitStreamHandler *&pcBitStreamHandler)
{
	// See 7.4.3.1 in draft
	// Initialize the bitstream handler
	pcBitStreamHandler->InitBitStreamWordLevel(true);

	// Write the start bytes
	pcBitStreamHandler->PutStartCodeWordLevel();

	// Write the NAL unit type
	// See 7.4.2.2 of draft
	pcBitStreamHandler->PutCodeInBitstream(0x3201,16,true);

	pcBitStreamHandler->PutUNInBitstream(0,3,"vps_max_temporal_layers_minus1");
	pcBitStreamHandler->PutUNInBitstream(0,5,"vps_max_layers_minus1");
	pcBitStreamHandler->PutUVInBitstream(0,"video_parameter_set_id");
	pcBitStreamHandler->PutUNInBitstream(0,1,"vps_temporal_id_nesting_flag");
	pcBitStreamHandler->PutUVInBitstream(1,"vps_max_dec_pic_buffering");
	pcBitStreamHandler->PutUVInBitstream(0,"vps_num_reorder_pics");
	pcBitStreamHandler->PutUVInBitstream(0,"vps_max_latency_increase");
	pcBitStreamHandler->PutUNInBitstream(0,1,"vps_extension_flag");

	// Terminate the bitstream
	// @todo I cannot find any trailing bits in VPS. If this is the case, we need to
	// terminate the bit-stream proper
	pcBitStreamHandler->FlushRemBytes();
}

void H265Headers::GenVPSNALU(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, BitStreamHandler *&pcBitStreamHandler)
{
	// Write the VPS to the bitstream
	WriteVPSInBitstream(pcBitStreamHandler);
}

/***********************SPS*************************/
void H265Headers::WriteSPSInBitstream(ImageParameters const *pcImageParam, BitStreamHandler *&pcBitStreamHandler)
{
	u32 uiMinCUSize = pcImageParam->m_uiMaxCUSize >> (pcImageParam->m_uiMaxCUDepth-1);	//The expression LOG2(MIN_CU_SIZE)-1; gives wrong results
	u32 uiLog2MinCUSize = LOG2(uiMinCUSize-1);
	u32 uiSplitCUSize = (CTU_WIDTH >> pcImageParam->m_uiMaxCUDepth);
	u32 uiAMVPNum = pcImageParam->m_uiMaxCUDepth;

	// Initialize the bitstream handler
	pcBitStreamHandler->InitBitStreamWordLevel(true);

	// Write the start bytes
	pcBitStreamHandler->PutStartCodeWordLevel();

	// Write the NAL unit type
	pcBitStreamHandler->PutCodeInBitstream(0x3401,16,true);

	pcBitStreamHandler->PutUNInBitstream(0,3,"profile_space");
	pcBitStreamHandler->PutUNInBitstream(0,5,"profile_idc");
	pcBitStreamHandler->PutUNInBitstream(0,16,"reserved_indicator_flags");
	pcBitStreamHandler->PutUNInBitstream(0,8,"level_idc");
	pcBitStreamHandler->PutUNInBitstream(0,32,"profile_compatibility");
	pcBitStreamHandler->PutUVInBitstream(0,"seq_parameter_set_id");
	pcBitStreamHandler->PutUVInBitstream(0,"video_parameter_set_id");
	pcBitStreamHandler->PutUVInBitstream(1,"chroma_format_idc");
	pcBitStreamHandler->PutUNInBitstream(0,3,"max_temporal_layers_minus1");
	pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiFrameWidth,"pic_width_in_luma_samples");
	pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiFrameHeight,"pic_height_in_luma_samples");
	pcBitStreamHandler->PutUNInBitstream(0,1,"pic_cropping_flag");
	pcBitStreamHandler->PutUVInBitstream(0,"bit_depth_luma_minus8");
	pcBitStreamHandler->PutUVInBitstream(0,"bit_depth_chroma_minus8");
	pcBitStreamHandler->PutUNInBitstream(0,1,"pcm_enabled_flag");
	pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiBitsForPOC-4,"log2_max_pic_order_cnt_lsb_minus4" );
	pcBitStreamHandler->PutUVInBitstream(1,"max_dec_pic_buffering[i]");
	pcBitStreamHandler->PutUVInBitstream(0,"num_reorder_pics[i]");
	pcBitStreamHandler->PutUVInBitstream(0,"max_latency_increase[i]");
	pcBitStreamHandler->PutUNInBitstream(1,1,"restricted_ref_pic_lists_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"lists_modification_present_flag");
	pcBitStreamHandler->PutUVInBitstream(uiLog2MinCUSize-3,"log2_min_coding_block_size_minus3" );
	pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiMaxCUDepth-1,"log2_diff_max_min_coding_block_size" );
	pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiQuadTreeTULog2MinSize-2,"log2_min_transform_block_size_minus2" );
	pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiQuadTreeTULog2MaxSize-2,"log2_diff_max_min_transform_block_size" );
	pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiQuadTreeTUMaxDepthInter-1,"max_transform_hierarchy_depth_inter" );
	pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiQuadTreeTUMaxDepthIntra-1,"max_transform_hierarchy_depth_intra" );
	pcBitStreamHandler->PutUNInBitstream(0,1,"scaling_list_enabled_flag");
	pcBitStreamHandler->PutUNInBitstream(1,1,"asymmetric_motion_partitions_enabled_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"sample_adaptive_offset_enabled_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"temporal_id_nesting_flag");
	pcBitStreamHandler->PutUVInBitstream(1,"num_short_term_ref_pic_sets");
	pcBitStreamHandler->PutUNInBitstream(0,1,"inter_ref_pic_set_prediction_flag");
	pcBitStreamHandler->PutUVInBitstream(1,"num_negative_pics");
	pcBitStreamHandler->PutUVInBitstream(0,"num_positive_pics");
	pcBitStreamHandler->PutUVInBitstream(0,"delta_poc_s0_minus1");
	pcBitStreamHandler->PutUNInBitstream(1,1,"used_by_curr_pic_s0_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"long_term_ref_pics_present_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"sps_temporal_mvp_enable_flag");
	
	// @todo Need to check this
	if(uiSplitCUSize > 2) 
	{
		u32 uiDiff = LOG2(uiSplitCUSize-1) - LOG2(pcImageParam->m_uiQuadTreeTULog2MinSize);
		uiAMVPNum += uiDiff;
	}
	// There is AMVP mode at each depth
	for(u32 i = 0; i < uiAMVPNum; i++) 
		pcBitStreamHandler->PutUNInBitstream(1,1,"AMVPMode");

	// Usman: Didn't find it in HM-8.0
	// pcBitStreamHandler->PutUNInBitstream(0,2,"tiles_or_entropy_coding_sync_idc");
	pcBitStreamHandler->PutUNInBitstream(0,1,"sps_extension_flag");

	pcBitStreamHandler->WriteRBSPTrailingBits();
	pcBitStreamHandler->FlushRemBytes();
}

void H265Headers::GenSPSNALU(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, BitStreamHandler *&pcBitStreamHandler)
{
	// Write the SPS to the bitstream
	WriteSPSInBitstream(pcImageParam, pcBitStreamHandler);
}

/***********************PPS*************************/
void H265Headers::WritePPSInBitstream(ImageParameters const *pcImageParam, BitStreamHandler *&pcBitStreamHandler)
{
	// Initialize the bitstream handler
	pcBitStreamHandler->InitBitStreamWordLevel(true);

	// Write the start bytes
	pcBitStreamHandler->PutStartCodeWordLevel();

	// Write the NAL unit type
	pcBitStreamHandler->PutCodeInBitstream(0x3601,16,true);

	pcBitStreamHandler->PutUVInBitstream(0,"pic_parameter_set_id");
	pcBitStreamHandler->PutUVInBitstream(0,"seq_parameter_set_id");
	pcBitStreamHandler->PutUNInBitstream(0,1,"sign_data_hiding_flag");
	pcBitStreamHandler->PutUNInBitstream(1,1,"cabac_init_present_flag");
	pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiMaxNumRefFrames-1,"num_ref_idx_l0_default_active_minus1");
	pcBitStreamHandler->PutUVInBitstream(0,"num_ref_idx_l1_default_active_minus1");
	pcBitStreamHandler->PutSVInBitstream(0,"pic_init_qp_minus26");
	pcBitStreamHandler->PutUNInBitstream(0,1,"constrained_intra_pred_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"transform_skip_enabled_flag"); 
	pcBitStreamHandler->PutUNInBitstream(0,1,"cu_qp_delta_enabled_flag");
	pcBitStreamHandler->PutSVInBitstream(0,"cb_qp_offset");
	pcBitStreamHandler->PutSVInBitstream(0,"cr_qp_offset");
	pcBitStreamHandler->PutUNInBitstream(0,1,"slicelevel_chroma_qp_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"weighted_pred_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"weighted_bipred_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"output_flag_present_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"dependent_slices_enabled_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"transquant_bypass_enable_flag");

	// Tile information
	pcBitStreamHandler->PutUNInBitstream(pcImageParam->m_uiTileCodingSync,2,"tiles_or_entropy_coding_sync_idc");
	if(pcImageParam->m_uiTileCodingSync == 1)	// Tiles are present
	{
		pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiFrameWidthInTiles-1,"num_tile_columns_minus1");
		pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiFrameHeightInTiles-1,"num_tile_columns_minus1");
		pcBitStreamHandler->PutUNInBitstream(1,1,"uniform_spacing_flag");	// Always uniform spacing
		pcBitStreamHandler->PutUNInBitstream(0,1,"loop_filter_across_tiles_enabled_flag");
	}

	pcBitStreamHandler->PutUNInBitstream(0,1,"seq_loop_filter_across_slices_enabled_flag");
	pcBitStreamHandler->PutUNInBitstream(1,1,"deblocking_filter_control_present_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"pps_deblocking_filter_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"pps_scaling_list_data_present_flag"); 
	pcBitStreamHandler->PutUVInBitstream(0,"log2_parallel_merge_level_minus2");
	pcBitStreamHandler->PutUNInBitstream(0,1,"slice_header_extension_present_flag");
	pcBitStreamHandler->PutUNInBitstream(0,1,"pps_extension_flag");

	pcBitStreamHandler->WriteRBSPTrailingBits();
	pcBitStreamHandler->FlushRemBytes();
}

void H265Headers::GenPPSNALU(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, BitStreamHandler *&pcBitStreamHandler)
{
	// Write the PPS to the bitstream
	WritePPSInBitstream(pcImageParam, pcBitStreamHandler);
}

/***********************Slice*************************/
void H265Headers::WriteSliceHdrInBitstream(ImageParameters const *pcImageParam, u32 uiCurrSliceNum, BitStreamHandler *&pcBitStreamHandler)
{
	eSliceType eCurrSliceType = pcImageParam->m_eSliceType;
	if(eCurrSliceType == I_SLICE)
	{
		if(uiCurrSliceNum == 0)	// First frame of the sequence
			pcBitStreamHandler->PutCodeInBitstream(0x00000110,32,false);
		else
			pcBitStreamHandler->PutCodeInBitstream(0x00000102,32,false);	// Usman: changed from 0x00000108 to 0x00000102
	}
	else
		pcBitStreamHandler->PutCodeInBitstream(0x00000102,32,false);

	pcBitStreamHandler->PutCodeInBitstream(1,8,true);

	pcBitStreamHandler->PutUNInBitstream(1,1,"first_slice_in_pic_flag");	// One slice per frame
	if(eCurrSliceType == I_SLICE && uiCurrSliceNum == 0)	// Usman: Added the uiCurrSliceNum condition
		pcBitStreamHandler->PutUNInBitstream(0,1,"no_output_of_prior_pics_flag");

	pcBitStreamHandler->PutUVInBitstream(0,"pic_parameter_set_id");

	pcBitStreamHandler->PutUVInBitstream(eCurrSliceType,"slice_type");
	pcBitStreamHandler->PutUNInBitstream(0,1,"dependent_slice_flag");

	if( (eCurrSliceType != I_SLICE) || ((eCurrSliceType == I_SLICE) && (uiCurrSliceNum != 0)) ) 
	{
		pcBitStreamHandler->PutUNInBitstream(uiCurrSliceNum % (1<<pcImageParam->m_uiBitsForPOC),pcImageParam->m_uiBitsForPOC,"pic_order_cnt_lsb");
		pcBitStreamHandler->PutUNInBitstream(1,1,"short_term_ref_pic_set_sps_flag");	// Usman: Changed the trace string from pps to sps
		pcBitStreamHandler->PutUVInBitstream(0,"short_term_ref_pic_set_idx");
	}

	// Setting num_ref_idx_active_override_flag to 1
	if(eCurrSliceType != I_SLICE) 
	{
		pcBitStreamHandler->PutUNInBitstream(1,1,"num_ref_idx_active_override_flag");
		pcBitStreamHandler->PutUVInBitstream(pcImageParam->m_uiMaxNumRefFrames-1,"num_ref_idx_l0_active_minus1");
	}

	if (eCurrSliceType != I_SLICE) 
		pcBitStreamHandler->PutUNInBitstream(0,1,"cabac_init_flag");

	pcBitStreamHandler->PutSVInBitstream(pcImageParam->m_uiQP-26,"slice_qp_delta");
	pcBitStreamHandler->PutUNInBitstream(1,1,"loop_filter_disable");
	pcBitStreamHandler->PutUVInBitstream(0,"maxNumMergeCand");

	if(pcImageParam->m_uiFrameSizeInTiles == 1)	// There is only one tile, so byte align here
		pcBitStreamHandler->WriteRBSPTrailingBits();  // @todo Check if this is required for multiple tiles
}

void H265Headers::GenSliceHeader(InputParameters const *pcInputParam, ImageParameters const *pcImageParam, u32 uiCurrSliceNum, BitStreamHandler *&pcBitStreamHandler)
{
	// Write the slice header to the bitstream
	WriteSliceHdrInBitstream(pcImageParam,uiCurrSliceNum,pcBitStreamHandler);
}

void H265Headers::WriteTilesEntryPointsInSliceHeader(u32 uiNumEntryPointOffsets, u32 const *uiEntryPointOffsets, BitStreamHandler *&pcBitStreamHandler)
{
	pcBitStreamHandler->PutUVInBitstream(uiNumEntryPointOffsets,"num_entry_point_offsets");
	u32 uiMaxOffsetLengthMinus1;
	if(uiNumEntryPointOffsets > 0)
	{
		// Determine the total number of bits required to present maximum offset
		// This is required for encoding in the slice header
		u32 uiMaxOffset = 0;
		for(u32 i=0;i<uiNumEntryPointOffsets;i++)
		{
			if(uiEntryPointOffsets[i] > uiMaxOffset)
				uiMaxOffset = uiEntryPointOffsets[i];
		}
		uiMaxOffsetLengthMinus1 = 0;
		while(uiMaxOffset >= (1u << (uiMaxOffsetLengthMinus1+1)))
		{
			uiMaxOffsetLengthMinus1++;
			MAKE_SURE(uiMaxOffsetLengthMinus1+1 < 32, 
				"Error: The total bits requires to present the tile offset is more than 32");
		}
		pcBitStreamHandler->PutUVInBitstream(uiMaxOffsetLengthMinus1,"offset_len_minus1");
	}

	for(u32 i=0;i<uiNumEntryPointOffsets;i++)
		pcBitStreamHandler->PutUNInBitstream(uiEntryPointOffsets[i],uiMaxOffsetLengthMinus1+1,"entry_point_offset");

	pcBitStreamHandler->WriteRBSPTrailingBits();
	pcBitStreamHandler->FlushRemBytes();
}