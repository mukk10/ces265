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
* @file ThreadHandler.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods contained in the ThreadHandler class.
* Basic idea taken from: http://vichargrave.com/java-style-thread-class-in-c/
*/

#include "ThreadHandler.h"
#include "WorkQueue.h"
#include "WorkItem.h"
#include <iostream>
#include <cassert>

using namespace std;

ThreadHandler::ThreadHandler(WorkQueue *pcWorkQueue)
{
	m_pcWorkQueue = pcWorkQueue;
	m_iStatus = 0;
	m_iDetached = 0;
}

ThreadHandler::~ThreadHandler()
{
	if(m_iStatus == 1 && m_iDetached == 0)
		pthread_detach(m_TID);
	//if(m_iStatus == 1)
		pthread_cancel(m_TID);
}

static void* runThread(void* arg)
{
	return ((ThreadHandler*)arg)->RunThread();
}

void *ThreadHandler::RunThread()
{
	while(1)
	{
		// Remove an item from the queue
		WorkItem *pcWorkItem = m_pcWorkQueue->GetNextJob();
		m_pcWorkQueue->IncNumJobsUnderProcess();
		cout << "Job started for work item number " << pcWorkItem->m_iItemNum << endl;
		pcWorkItem->m_pfPtrToFunc(pcWorkItem->m_pArgs);
		m_pcWorkQueue->JobDone();
	}
	pthread_exit((void*) 0);
}

int ThreadHandler::StartThread()
{
	int result = pthread_create(&m_TID, NULL, runThread, this);
	assert(result==0);
	if (result == 0)
		m_iStatus = 1;

	return result;
}

int ThreadHandler::WaitTillThreadFinish()
{
	int iRet = -1;
	if(m_iStatus == 1)	// Still running
	{
		iRet = pthread_join(m_TID, NULL);	// Wait for the thread to finish
		if(iRet == 0)
			m_iDetached = 1;
	}
	return iRet;
}

int ThreadHandler::DetachThread()
{
	int iRet = -1;
	if (m_iStatus == 1 && m_iDetached == 0) 
	{
		iRet = pthread_detach(m_TID);
		if (iRet == 0) 
			m_iDetached = 1;
	}
	return iRet;
}