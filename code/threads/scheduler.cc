// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"
#include <stdio.h>
#include <algorithm>

bool cmpL2(Thread *th1, Thread *th2) {
    return th1->checkPriority() > th2->checkPriority();
}

bool cmpL1(Thread *th1, Thread *th2) {
    return th1->checkT() < th2->checkT();
}

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    L3Queue = new std::list<Thread *>; 
    L2Queue = new std::list<Thread *>;
    L1Queue = new std::list<Thread *>;
    toBeDestroyed = NULL;
    enablePreemptOnce = false;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete L3Queue; 
    delete L2Queue;
    delete L1Queue;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
    
    thread->setStatus(READY);
    // kernel->currentThread == thread...yielding
    if (kernel->currentThread != thread) kernel->currentThread->setT(kernel->currentThread->checkTempTick() / 2 + kernel->currentThread->checkT() / 2);
    
    if (thread->checkPriority() < 50) {
        // L3
        printf("Tick %d: Thread %d is inserted into queue L3\n", kernel->stats->totalTicks, thread->getID());
        L3Queue->push_back(thread);
    } else if (thread->checkPriority() < 100) {
        // L2
        printf("Tick %d: Thread %d is inserted into queue L2\n", kernel->stats->totalTicks, thread->getID());
        L2Queue->push_back(thread);
        L2Queue->sort(cmpL2);
        if (kernel->currentThread != thread) enablePreemptOnce = true;
    } else {
        // L1
        printf("Tick %d: Thread %d is inserted into queue L1\n", kernel->stats->totalTicks, thread->getID());
        L1Queue->push_back(thread);
        L1Queue->sort(cmpL1);
        if (kernel->currentThread != thread) enablePreemptOnce = true;
    }
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    Thread *thread;

    if (L1Queue->empty() && L2Queue->empty() && L3Queue->empty()) {
        return NULL;
    } else if (L1Queue->empty() && L2Queue->empty()) {
        // L3
        kernel->alarm->setStat(true); // turn on alarm
        thread = L3Queue->front();
        L3Queue->pop_front();
        printf("Tick %d: Thread %d is removed from queue L3\n", kernel->stats->totalTicks, thread->getID());
        return thread;
    } else if (L1Queue->empty()) {
        // L2
        kernel->alarm->setStat(false); // turn off alarm
        thread = L2Queue->front();
        L2Queue->pop_front();
        printf("Tick %d: Thread %d is removed from queue L2\n", kernel->stats->totalTicks, thread->getID());
        return thread;
    } else {
        // L1
        kernel->alarm->setStat(false); // turn off alarm
        thread = L1Queue->front();
        L1Queue->pop_front();
        printf("Tick %d: Thread %d is removed from queue L1\n", kernel->stats->totalTicks, thread->getID());
        return thread;
    }
    
    /*if (L3Queue->IsEmpty()) {
		return NULL;
    } else {
    	return L3Queue->RemoveFront();
    }*/
}

Thread* Scheduler::PureFindNext() {
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    Thread *thread;

    if (L1Queue->empty() && L2Queue->empty() && L3Queue->empty()) {
        return NULL;
    } else if (L1Queue->empty() && L2Queue->empty()) {
        // L3
        thread = L3Queue->front();
        return thread;
    } else if (L1Queue->empty()) {
        // L2
        thread = L2Queue->front();
        return thread;
    } else {
        // L1
        thread = L1Queue->front();
        return thread;
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
        ASSERT(toBeDestroyed == NULL);
        toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
        oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    printf("Tick %d: Thread %d is now selected for execution\n", kernel->stats->totalTicks, nextThread->getID());
    printf("Tick %d: Thread %d is replaced, and it has executed %d ticks\n", kernel->stats->totalTicks, oldThread->getID(), oldThread->checkTempTick());
    oldThread->setLastExecTick(kernel->stats->totalTicks);
    oldThread->setTempTick(0);
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
        oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    //L3Queue->Apply(ThreadPrint);
}
