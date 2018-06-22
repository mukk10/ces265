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
* @file EncMain.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the H.265 encoder that uses an H.265 intra.
*/

#ifdef			TEST_MEMORY_LEAKS
#define			_CRTDBG_MAP_ALLOC
#include		<stdlib.h>
#include		<crtdbg.h>
#include		<vld.h>
#else
#include		<stdlib.h>
#endif
#include 		<stdio.h>
#include		<EncTop.h>

int main (int argc, char* argv[])
{
#ifdef TEST_MEMORY_LEAKS
	// Send all reports to STDOUT
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );
	//_crtBreakAlloc = 103;	// To conditionally set a break point at the memory allcoation number
#endif
	EncTop *pcEnc = new EncTop(argc, argv);
	pcEnc->Encode();
	pcEnc->PrintPSNR(false);
	delete pcEnc;
	
#ifdef TEST_MEMORY_LEAKS
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
