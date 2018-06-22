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
* @file WorkItem.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the WorkItem class.
* Basic idea taken from: http://vichargrave.com/multithreaded-work-queue-in-c/
*/

#ifndef __WORKITEM_H__
#define __WORKITEM_H__

/**
*	Stores a work item.
*	Stores the items in the queue, which can be poped by a thread.
*/
class WorkItem
{
public:
	/**
	*	Thread calling function.
	*	The function which must be called when the thread is run.
	*	@param p The parameters of the function.
	*/
	void	*(*m_pfPtrToFunc)(void *p);

	int		m_iItemNum;					//!< Current item number
	void	*m_pArgs;					//!< Arguments array
	int		m_iTotArg;					//!< Total arguments

	/**
	*	Default Constructor.
	*/
	WorkItem(){}

	/**
	*	Constructor.
	*	@param PtrToFunc Function to be called by the thread when the thread pops up this work item.
	*	@param iItemNum The current work item identification number.
	*	@param pArgs Pointer to an arguments. This can be an array or structure etc.
	*	@param iTotArgs The total number of arguments.
	*/
	WorkItem(void * (*PtrToFunc)(void*), int iItemNum, void *pArgs, int iTotArgs):
		m_pfPtrToFunc(PtrToFunc), m_iItemNum(iItemNum), m_pArgs(pArgs), m_iTotArg(iTotArgs){}
	~WorkItem(){}
};

#endif	// __WORKITEM_H__