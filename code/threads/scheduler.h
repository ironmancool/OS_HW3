// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "thread.h"
#include <list>

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class Scheduler {
  public:
    Scheduler();		// Initialize list of ready threads 
    ~Scheduler();		// De-allocate ready list

    void ReadyToRun(Thread* thread);	
    				// Thread can be dispatched.
    Thread* FindNextToRun();	// Dequeue first thread on the ready 
				// list, if any, and return thread.
    void Run(Thread* nextThread, bool finishing);
    				// Cause nextThread to start running
    void CheckToBeDestroyed();// Check if thread that had been
    				// running needs to be deleted
    void Print();		// Print contents of ready list
    
    std::list<Thread *> *getL1Queue() { return L1Queue; }
    std::list<Thread *> *getL2Queue() { return L2Queue; }
    std::list<Thread *> *getL3Queue() { return L3Queue; }
    
    // SelfTest for scheduler is implemented in class Thread
    
  private:
    std::list<Thread *> *L3Queue;  // queue of threads that are ready to run,
    std::list<Thread *> *L2Queue;
    std::list<Thread *> *L1Queue;
				// but not running
    Thread *toBeDestroyed;	// finishing thread to be destroyed
    				// by the next thread that runs
};

#endif // SCHEDULER_H
