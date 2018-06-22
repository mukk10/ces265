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
* @file ThreadHandler.h
* @author Muhammad Usman Karim Khan, Muhammad Shafique, Joerg Henkel (CES, KIT)
* @brief This file contains the ThreadHandler class.
* Basic idea taken from: http://vichargrave.com/java-style-thread-class-in-c/
*/

#ifndef __THREADHANDLER_H__
#define __THREADHANDLER_H__

#include <pthread.h>

class WorkQueue;

/**
*	Thread handling class.
*	Handles the threads created by some source.
*/
class ThreadHandler
{
private:
	WorkQueue	*m_pcWorkQueue;		//!< Working queue with jobs
	int			m_iStatus;			//!< 1 -> Running, 0 -> Idle
	int			m_iDetached;		//!< 1 -> Detached, 0 -> Not detached (default)
	pthread_t	m_TID;				//!< Thread ID

public:
	/**
	*	Constructor.
	*	@param pcWorkQueue Working queue which holds the jobs for the thread.
	*	@see WorkQueue()
	*/
	ThreadHandler(WorkQueue *pcWorkQueue);
	
	~ThreadHandler();
	
	/**
	*	Actual runing function of the thread.
	*	When this function is called, the thread will start running continously and will look for a job in the queue. 
	*/
	void		*RunThread();

	/**
	*	Start the thread.
	*	This will start the pthread creation. It will wait for the jobs and then start processing.
	*/
	int			StartThread();

	/**
	*	Wait for thread to finish.
	*	This is actually pthread's join() function.
	*/
	int			WaitTillThreadFinish();

	/**
	*	Detach the thread.
	*/
	int			DetachThread();

};

#endif // __THREADHANDLER_H__