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
* @file BitStreamHandler.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the class that handles output bitstream. 
* Note that any function in this class will not write to an output file.
*/

#ifndef __BITSTREAMHANDLER_H__
#define __BITSTREAMHANDLER_H__

#include <TypeDefs.h>

/**	
*	Bitstream handling.
*	Writes the output bitstream. This bitstream can then be written to a file.
*/
class BitStreamHandler
{
private:
	u32					m_uiCurrWord;						//!< Current Word
	i32					m_iBitLocPtr;						//!< Global Byte Write Pointer
	u64					m_u64AllocBytes;					//!< Total bytes allocated to the RBSP buffer
	byte				*m_pbByteBuffer;					//!< Buffer to hold the data
	byte				*m_pbRBSPBuffer;					//!< Holds the compressed slice data. Emulation prevention is performed for this.
	byte				*m_pbCurrRBSPBuffer;				//!< Points to the current location in the buffer
	u64					m_u64BytesPerFrame;					//!< Number of Bytes per Frame
	u64					m_u64BytesRBSP;						//!< Number of bytes in the payload (without emulation prevention)
	BitStreamHandler	*m_pcNextBitStreamHandler;			//!< Next bitstream handler (useful for tiles and parallel encoding)
	BitStreamHandler	*m_pcPrevBitStreamHandler;			//!< Previous bitstream handler (only allocated for slice header encoding with tiling)
public:

	/**
	*	Constructor.
	*	@param uiTotalBytes The total number of bytes that are allocated to the bitstream buffer.
	*/
	BitStreamHandler(u64 uiTotalBytes=1);
	~BitStreamHandler();

	/**
	*	Reset RBSP Buffer size.
	*	Usually, you should not use this function.
	*	@param uiTotalBytes The total number of bytes.
	*/
	void				ResetRBSPBufferSize(u64 uiTotalBytes);	

	/**
	*	Initialize the Bitstream Handler.
	*	Initialize the bitstream handler at the word level (32-bit) boundary.
	*	@param isNewBuffer If a new buffer is started, then the pointers are reset. Usually, this should be true.
	*/
	void				InitBitStreamWordLevel(bit isNewBuffer);	

	/**
	*	Put the starting code in the stream.
	*	Puts 0x00000001 in the bitstream.
	*/
	void				PutStartCodeWordLevel();

	/**
	*	Insert a code in the bitstream.
	*	@param uiCode Input code.
	*	@param iLength Input length of the code.
	*	@param bEmuPrevActivate If true, then emulation prevention is performed, else not.
	*/
	void				PutCodeInBitstream(u32 uiCode, i32 iLength, bit bEmuPrevActivate);

	/**
	*	Insert u(n) in bitstream.
	*	@param uiCode Input code.
	*	@param iLength Input length of the code.
	*	@param pbTraceString Trace string for information.
	*/
	void				PutUNInBitstream(u32 uiCode, i32 iLength, char *pbTraceString);

	/**
	*	Insert ue(v) in bitstream.
	*	@param uiCode Input code.
	*	@param pbTraceString Trace string for information.
	*/
	void				PutUVInBitstream(u32 uiCode, char *pbTraceString);

	/**
	*	Insert se(v) in bitstream.
	*	@param uiCode Input code.
	*	@param pbTraceString Trace string for information.
	*/
	void				PutSVInBitstream(i32 iCode, char *pbTraceString);

	/**
	*	Finish/terminate the bitstream.
	*	Flush the remaining bytes in the word-level cache to the bitstream.
	*	This doesn't mean that no further writing to bitstream and output file write. It is
	*	just called after e.g. VPS writing to flush the bitstream buffer.
	*/
	void				FlushRemBytes();

	/**
	*	Write the trailing bits.
	*	After e.g. header writing is finished, the remaining bits in the cache must also be
	*	pushed into the bitstream, which is handled by this function.
	*/
	void				WriteRBSPTrailingBits();

	/**
	*	Write 0 bits for byte-alignment.
	*	If the total bits written to the bitstream are not byte algined, write 0s to byte-align.
	*/
	void				WriteAlignZeroBits();


	/**
	*	Get bitstream buffer.
	*	The buffer address starts at address 0.
	*	@return Starting address of the bitstream buffer.
	*/
	byte				*GetBitStreamBuffer(){return m_pbRBSPBuffer;}

	/**
	*	Get Current bitstream buffer location.
	*	@return Address of the bitstream buffer where the next value will be written.
	*/
	byte				*GetCurrBitStreamBuffer(){return m_pbCurrRBSPBuffer;}

	/**
	*	Get total number of bytes in the bitstream.
	*	@return Total bytes written to the bitstream buffer.
	*/
	u64					GetTotalBytesWritten(){return (m_pbCurrRBSPBuffer-m_pbRBSPBuffer);}

	/**
	*	Get total allocated bytes to the RBSP buffer.
	*	@return Total bytes allocated to the bitstream buffer.
	*/
	u64					GetTotalBytesAllocate(){return m_u64AllocBytes;}

	/**
	*	Add another bitstream handler to the current handler.
	*	A reference to the other bitstream handlers are added to make a linked list.
	*	@param pcBitStreamHandler Next bitstream handler to the current handler
	*/
	void				SetNextBitStreamHandler(BitStreamHandler *& pcBitStreamHandler){m_pcNextBitStreamHandler=pcBitStreamHandler;}

	/**
	*	Get the next bitstream handler in the linked list.
	*	@return Get the next bitstream handler in the linked list.
	*/
	BitStreamHandler	*GetNextBitStreamHandler(){return m_pcNextBitStreamHandler;}

	/**
	*	Add another bitstream handler to the current handler.
	*	A reference to the other bitstream handlers are added to make a linked list.
	*	@param pcBitStreamHanlder Previous bitstream handler to the current handler.
	*/
	void				SetPrevBitStreamHandler(BitStreamHandler *& pcBitStreamHandler){m_pcPrevBitStreamHandler=pcBitStreamHandler;}

	/**
	*	Get the next bitstream handler in the linked list.
	*	@return Get Previous bitstream handler in the linked list.
	*/
	BitStreamHandler	*GetPrevBitStreamHandler(){return m_pcPrevBitStreamHandler;}


	/**
	*	Fix the zero termination.
	*	If the last byte of the bitstream is 0, append it with 0x03.
	*/
	void				FixZeroTermination();

};

#endif // __BITSTREAMHANDLER_H__