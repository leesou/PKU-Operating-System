// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    //printf("%s %d\n",name,  value);
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    //printf("%s %d\n",name,  value);
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

/* ----------------------------------------- changed by Li cong 1800012826 ----------------------------------------- */

/*
// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) {}
Lock::~Lock() {}
void Lock::Acquire() {}
void Lock::Release() {}

Condition::Condition(char* debugName) { }
Condition::~Condition() { }
void Condition::Wait(Lock* conditionLock) { ASSERT(FALSE); }
void Condition::Signal(Lock* conditionLock) { }
void Condition::Broadcast(Lock* conditionLock) { }
*/

//----------------------------------------------------------------------
//  Lock::Lock
// 	Initialize a lock, so that it can be used for synchronization.
//  Use semaphore to implement lock.
//	"debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------

Lock::Lock(char* debugName) 
{
    name = debugName;
    Owner = NULL;
    lockSemaphore = new Semaphore("Lock semaphore", 1); // Only allow one thread to access lock at ont time.
}

//----------------------------------------------------------------------
// Lock::~Lock
// 	De-allocate this lock, when no longer needed.  Assume no one
//	is still holding or waiting for  the lock!
//----------------------------------------------------------------------

Lock::~Lock() 
{
    delete lockSemaphore;
}

//----------------------------------------------------------------------
// Lock::Acquire
// 	Wait until lockSemaphore value > 0, then decrement and update Owner of this lock.  Checking the
//	value and decrementing and Update Owner must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void Lock::Acquire() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff); 

    DEBUG('s', "Lock \"%s\"  is Acquired by Thread \"%s\"\n", name, currentThread->getName());

    lockSemaphore->P(); // try to get semaphore, if failed, currentThread will sleep.
    Owner = currentThread;

    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Lock::Release
// 	Increment lockSemaphore value, clear Owner, and waking up a waiter if necessary.
//	As with Acquire(), these operations must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void Lock::Release()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    DEBUG('s', "Lock \"%s\" is Released by Thread \"%s\"\n", name, currentThread->getName());
    ASSERT(isHeldByCurrentThread()); //  By convention, only the thread that acquired the lock may release it.
    Owner = NULL;
    lockSemaphore->V();

    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
//  Lock::isHeldByCurrentThread
// 	Check whether current thread really holds this lock.
//  If not, use ASSERT to notify user.
//----------------------------------------------------------------------

bool Lock::isHeldByCurrentThread()
{
    return Owner == currentThread;
}

//----------------------------------------------------------------------
//  Condition::Condition
// 	Initialize a condition variable, so that it can be used for synchronization.
//  waitQueue is used for recording threads pending on it.
//	"debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------

Condition::Condition(char* debugName) 
{
    name = debugName;
    waitQueue = new List();
}

//----------------------------------------------------------------------
// Condition::~Condition
// 	De-allocate this condition variable, when no longer needed.  Assume no one
//	is still holding or waiting for  it!
//----------------------------------------------------------------------

Condition::~Condition() 
{
    delete waitQueue;
}

//----------------------------------------------------------------------
// Condition::Wait
// 	Release the lock, relinquish the CPU until signaled, 
//  then re-acquire the lock.
//----------------------------------------------------------------------

void Condition::Wait(Lock* conditionLock) 
{
     IntStatus oldLevel = interrupt->SetLevel(IntOff); // Ensure *atomic* operation.

     // Step 0: Check whether current thread is holding this lock.
      ASSERT(conditionLock->isHeldByCurrentThread()); 

      // Step 1: release the lock
      conditionLock->Release();

      // Step 2: append thread to waitQueue and block this thread
      waitQueue->Append(currentThread);
      currentThread->Sleep();

      /*  Signal came */

      // Step 3: re-Acquire this lock after Awaken by a signal operation.
      conditionLock->Acquire();

      (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Condition::Signal
// 	Wake up a thread, if there are any waiting on 
//  the condition.
//----------------------------------------------------------------------
 
void Condition::Signal(Lock* conditionLock)
{
     IntStatus oldLevel = interrupt->SetLevel(IntOff);

     // Step 0: Check whether current thread is holding this lock.
     ASSERT(conditionLock->isHeldByCurrentThread());

    // Step 1: Wake up the first thread in waitQueue.
     if(!waitQueue->IsEmpty())
     {
         Thread* t = (Thread*) waitQueue->Remove();
         scheduler->ReadyToRun(t);
     }

     (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Condition::Broadcast
//  Wake up all threads waiting on the condition.
// Similar to Signal.
//----------------------------------------------------------------------

void Condition::Broadcast(Lock* conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    // Step 0: Check whether current thread is holding this lock.
    ASSERT(conditionLock->isHeldByCurrentThread());

    DEBUG('b', "##### Start to broadcast! #####\n");
    // Step 1: Wake up all threads in waitQueue.
    while(!waitQueue->IsEmpty())
    {
        Thread* t = (Thread*) waitQueue->Remove();
        scheduler->ReadyToRun(t);
    }

    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
//  Barrier::Barrier
// 	Initialize a barrier, so that it can be used for synchronization.
//  num_threads is used to record total number of threads/
//	"debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------

Barrier::Barrier(char*debugName, int thread_number)
{
    name = debugName;
    num_threads = thread_number;
    num_left = num_threads;
    Mutex = new Lock("Lock in Barrier");
    barrier = new Condition("Condition in Barrier");
}

//----------------------------------------------------------------------
// Barrier:~Barrier
// 	De-allocate this Barrier, when no longer needed.  Assume no one
//	is still holding or waiting for  it!
//----------------------------------------------------------------------

Barrier::~Barrier()
{
    delete Mutex;
    delete barrier;
}

//----------------------------------------------------------------------
// Barrier:ArriveAndWait
// 	Wait until all threads arrive at this point.
//----------------------------------------------------------------------

void Barrier::ArriveAndWait()
{
    IntStatus oldStatus = interrupt->SetLevel(IntOff);

    Mutex->Acquire();

    num_left--;
    if(num_left==0) // all threads reach this point
    {
        barrier->Broadcast(Mutex);
        num_left = num_threads; // make this barrier reuseable
    }
    else
    {
        barrier->Wait(Mutex); // wait for other threads
    }
    
    Mutex->Release();

    (void) interrupt->SetLevel(oldStatus);
}

//----------------------------------------------------------------------
//  ReadWriteLock::ReadWriteLock
// 	Initialize a ReadWriteLock, so that it can be used for synchronization.
//	"debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------

ReadWriteLock::ReadWriteLock(char* debugName)
{
    name = debugName;
    ReaderCnt = 0;
    ReaderLock = new Lock("Reader lock");
    WriterLock = new Semaphore("Writer Lock", 1);
}

//----------------------------------------------------------------------
// ReadWriteLock::~ReadWriteLock
// 	De-allocate this ReadWriteLock, when no longer needed. Assume no one
//	is still holding or waiting for  it!
//----------------------------------------------------------------------

ReadWriteLock::~ReadWriteLock()
{
    delete ReaderLock;
    delete WriterLock;
}

//----------------------------------------------------------------------
// ReadWriteLock::AcquireReaderLock
// 	Acquire the reader lock. This step not only change ReaderCnt, 
//      but also acquire WriterLock when the first reader comes. 
//----------------------------------------------------------------------

void
ReadWriteLock::AcquireReaderLock()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ReaderLock->Acquire();
    ReaderCnt++;
    if(ReaderCnt==1)
    {
        WriterLock->P();
    }
    ReaderLock->Release();

    interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// ReadWriteLock::ReleaseReaderLock
// 	Release the reader lock. This step not only change ReaderCnt, 
//      but also release WriterLock when the last reader leaves. 
//----------------------------------------------------------------------

void
ReadWriteLock::ReleaseReaderLock()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ReaderLock->Acquire();
    ReaderCnt--;
    if(ReaderCnt==0)
    {
        WriterLock->V();
    }
    ReaderLock->Release();

    interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// ReadWriteLock::AcquireWriterLock
// 	Acquire the writer lock. 
//----------------------------------------------------------------------

void
ReadWriteLock::AcquireWriterLock()
{
    WriterLock->P();
}

//----------------------------------------------------------------------
// ReadWriteLock::ReleaseWriterLock
// 	Release the writer lock. 
//----------------------------------------------------------------------

void
ReadWriteLock::ReleaseWriterLock()
{
    WriterLock->V();
}
