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
* @file WorkQueue.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the WorkQueue class.
* Basic idea taken from: http://vichargrave.com/multithreaded-work-queue-in-c/
*/

#ifndef __WORKQUEUE_H__
#define __WORKQUEUE_H__

#include <pthread.h>

class WorkItem;

/**
*	A queue of work items.
*	A work queue is for all the threads/jobs.
*/
class WorkQueue
{
private:
	WorkItem			**m_ppcWorkItemQueue;	//!< A queue for work items
	int					m_iSize;				//!< Size of the queue
	int					m_iCurrSize;			//!< Current size of the queue
	int					m_iCurrReadLoc;			//!< Read location in the work queue
	int					m_iCurrWriteLoc;		//!< Write location in the work queue
	int					m_iPendingJobs;			//!< Total Pending jobs
	pthread_mutex_t		m_ptMutex;				//!< Mutex for inserting and removing the job
	pthread_cond_t		m_ptJobAvailCond;		//!< Condition variable if a job is available in the queue
	pthread_cond_t		m_ptQueueEmptyCond;		//!< Condition variable if the job queue is empty
public:
	/**
	*	Constructor.
	*/
	WorkQueue(int iSize = 1);

	~WorkQueue();

	/**
	*	Add a job to the back of the job queue.
	*	Before writing, always check if there is space in the queue.
	*	@param pcWorkItem The work item added to the queue.
	*	@return 0 means successful and otherwise is unsuccessful.
	*/
	int					AddToJob(WorkItem *pcWorkItem);

	/**
	*	Extract from the front of the job queue.
	*	If no job is available, the function will be suspended in wait state.
	*	I.e. this is a blocking function.
	*/
	WorkItem			*GetNextJob();

	/**
	*	Get current work items written.
	*/
	int					GetNumJobsInQueue();

	/**
	*	Increment job pending signal for the job.
	*	Should be set by the working thread.
	*/	
	void				IncNumJobsUnderProcess();

	/**
	*	Job done signal.
	*	Should be called by the working thread.
	*/
	void				JobDone();

	/**
	*	Wait until the job queue is empty.
	*	It is better to sleep a little before calling this function.
	*/
	void				WaitQueueEmpty();
};

#endif // __WORKQUEUE_H__