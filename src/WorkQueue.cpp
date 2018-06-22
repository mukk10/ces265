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
* @file WorkQueue.cpp
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the methods in WorkQueue class.
* Basic idea taken from: http://vichargrave.com/multithreaded-work-queue-in-c/
*/

#include "WorkQueue.h"
#include "WorkItem.h"
#include "TypeDefs.h"

WorkQueue::WorkQueue(int iSize)
{
	m_iSize = iSize;
	m_iCurrSize = 0;
	m_iCurrWriteLoc = 0;
	m_iCurrReadLoc = 0;
	m_iPendingJobs = 0;
	m_ppcWorkItemQueue = new WorkItem*[iSize];

	pthread_mutex_init(&m_ptMutex, NULL);
	pthread_cond_init(&m_ptJobAvailCond, NULL);
	pthread_cond_init(&m_ptQueueEmptyCond, NULL);
}

WorkQueue::~WorkQueue()
{
	delete [] m_ppcWorkItemQueue;

	pthread_mutex_destroy(&m_ptMutex);
	pthread_cond_destroy(&m_ptJobAvailCond);
	pthread_cond_destroy(&m_ptQueueEmptyCond);
}

int WorkQueue::AddToJob(WorkItem *pcWorkItem)
{
	pthread_mutex_lock(&m_ptMutex);
	if(m_iCurrSize == m_iSize)	// No space in the queue
	{
		pthread_mutex_unlock(&m_ptMutex);
		return 1;
	}
	else	// Space in the queue available
	{
		m_ppcWorkItemQueue[m_iCurrWriteLoc] = pcWorkItem;
		m_iCurrWriteLoc = (m_iCurrWriteLoc+1) % m_iSize;	// Circular buffer
		m_iCurrSize++;
		pthread_cond_signal(&m_ptJobAvailCond);
		pthread_mutex_unlock(&m_ptMutex);
		return 0;
	}
}

WorkItem *WorkQueue::GetNextJob()
{
	pthread_mutex_lock(&m_ptMutex);
	WorkItem *pcWorkItem = NULL;
	while(m_iCurrSize == 0)	// Wait until a job gets available
		pthread_cond_wait(&m_ptJobAvailCond, &m_ptMutex);

	pcWorkItem = m_ppcWorkItemQueue[m_iCurrReadLoc];
	m_iCurrReadLoc = (m_iCurrReadLoc+1) % m_iSize;
	m_iCurrSize--;

	MAKE_SURE(m_iCurrSize >= 0, "Error: The thread is spuriously woken-up."); 

	pthread_mutex_unlock(&m_ptMutex);
	return pcWorkItem;
}

int WorkQueue::GetNumJobsInQueue()
{
	pthread_mutex_lock(&m_ptMutex);
	int iWrittenItems = m_iCurrSize;
	pthread_mutex_unlock(&m_ptMutex);
	return iWrittenItems;
}

void WorkQueue::IncNumJobsUnderProcess()
{
	pthread_mutex_lock(&m_ptMutex);
	m_iPendingJobs++;
	pthread_mutex_unlock(&m_ptMutex);
}

void WorkQueue::JobDone()
{
	pthread_mutex_lock(&m_ptMutex);
	m_iPendingJobs--;
	if(m_iPendingJobs == 0 && m_iCurrSize == 0)
	{
		i32 iRetVal = pthread_cond_signal(&m_ptQueueEmptyCond);
		MAKE_SURE(iRetVal == 0, "Error: The conditional variable not set properly");
	}
	pthread_mutex_unlock(&m_ptMutex);
}

void WorkQueue::WaitQueueEmpty()
{
	// \todo Maybe I can insert a conditional wait statement here and get rid of too much testing of this function.
	pthread_mutex_lock(&m_ptMutex);
	while(m_iPendingJobs > 0 || m_iCurrSize > 0)	// Wait for job to finish
		pthread_cond_wait(&m_ptQueueEmptyCond,&m_ptMutex);	// Wait should come before signal pthread
	pthread_mutex_unlock(&m_ptMutex);
}