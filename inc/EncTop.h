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
* @file EncTop.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the class that Encoder's top.
* It is to be used for 8-bit, YUV 420p sequences.
*/

#ifndef __ENCTOP_H__
#define	__ENCTOP_H__

#include <TypeDefs.h>
#include <iostream>
#include <fstream>

class InputParameters;
class ImageParameters;
class BitStreamHandler;
class H265GOPCompressor;
class WorkQueue;

using namespace std;

/**
*	Encoder's Top module.
*	For encoding an input 8-bit, YUV 420p sequence and writing an output bitstream.
*/
class EncTop
{
private:
	i32					m_iNumInputArgs;								//!<	 Total input arguments
	InputParameters		*m_pcInputParam;								//!<	 Input parameters class
	ImageParameters		*m_pcImageParam;								//!<	 Image parameters class
	i8					**m_ppcInputArgs;								//!<	 Input arguments to the encoder
	byte				***m_pppcYBuff;									//!<	 Contains luma pixels [GOP number][Slice number][ptr]
	byte				***m_pppcCbBuff;								//!<	 Contains CB pixels [GOP number][Slice number][ptr]
	byte				***m_pppcCrBuff;								//!<	 Contains CR pixels [GOP number][Slice number][ptr]
	BitStreamHandler	****m_ppppcStreamHandler;						//!<	 Handles the full bitstream and its related variables [GOP number][Slice number][Tile][ptr]
	H265GOPCompressor	**m_ppcH265GOPCompressor;						//!<	 Compressor functions [GOP number][ptr]
	ifstream			m_ifsYUVFile;									//!<	 Input YUV file
	fstream				m_fsYUVFileRec;									//!<	 Ouput reconstructed file 
	ofstream			m_ofsBitStream;									//!<	 Output bitstream file
	ofstream			m_ofsStats;										//!<	 Output file for storing statistics
	u64					m_u64CurrFrameNum;								//!<	 Current frame number under process
	bit					m_bOutputRec;									//!<	 Output reconstructed frames
	u32					m_uiTotalCores;									//!<	 Total cores available for processing
	bit					m_bStats;										//!<	 Output statistics are generated
	f32					*m_pfPSNRPerFrame[3];							//!<	 PSNR per frame for Y, Cb, Cr
	u64					*m_pu64BytesPerFrame;							//!<	 Keeps the total bytes per frame
	u64					m_u64CurrGOPBytes;								//!<	 Keeps the total bytes for the current GOP

	void				ConfigureEncoder();								//!<	 Configure the encoder
	void				InitEncoder();									//!<	 Allocate memory to the buffers
	void				FreeAllocBuff();								//!<	 Free the allocated memory
	void				OpenIOFiles();									//!<	 Open the input and output files
	void				CloseIOFiles();									//!<	 Close the input and output files

	/*
	*	Write Parameter Set.
	*	i.e. VPS, SPS and PPS headers.
	*/
	u64					WritePS();
	
	/**
	*	Fill buffers by reading the YUV file.
	*	Fills the buffer by considering the position of the GOP.
	*	@param iGopNum GOP number.
	*/
	void				FillGOPBuffFromYUV(i32 iGopNum);

	/**
	*	Write the reconstructed output.
	*	@param iGopNum GOP number.
	*/
	void				WriteGOPBuffToYUV(i32 iGopNum);

	/**
	*	Write output bitstream.
	*	@param pcBitStreamHandler The bitstream where the output will be written.
	*/
	u64					WriteBitstreamFile(BitStreamHandler *pcBitStreamHandler);

	/**
	*	Compute PSNR of one frame.
	*	@param currframe Current frame.
	*	@param recframe Reconstructed frame.
	*	@param size Size of the frame.
	*/
	f32					PSNROneFrame(byte *currframe, byte *recframe, u32 size);

	/**
	*	Dump the statistics.
	*	File name will correspond to the current date.
	*	@param iGopNum GOP number.
	*	@param uiGopStartFrameNum Starting frame number of the GOP.
	*/
	void				DumpStats(i32 iGopNum, u32 uiGopStartFrameNum);

	/**
	*	Make threads pool.
	*/
	void				MakeThreadsPool();

public:

	/**
	*	Constructor.
	*	The command line arguments are fed directly to this entity.
	*/
	EncTop(int argc, char* argv[]);	
	~EncTop();

	/**
	*	Encode the whole sequence.
	*/
	void Encode();													

	/**
	*	Get PSNR of the whole sequence.
	*	This will also write a file to print the PSNR [dB] and bitrate [Kbps] for all the frames.
	*	@param bLumaOnly If 1, the PSNR only considers the luma frames. 
	*/
	void PrintPSNR(bool bLumaOnly);									
};

#endif	// __ENCTOP_H__