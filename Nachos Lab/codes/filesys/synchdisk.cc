// synchdisk.cc 
//	Routines to synchronously access the disk.  The physical disk 
//	is an asynchronous device (disk requests return immediately, and
//	an interrupt happens later on).  This is a layer on top of
//	the disk providing a synchronous interface (requests wait until
//	the request completes).
//
//	Use a semaphore to synchronize the interrupt handlers with the
//	pending requests.  And, because the physical disk can only
//	handle one operation at a time, use a lock to enforce mutual
//	exclusion.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchdisk.h"

//----------------------------------------------------------------------
// DiskRequestDone
// 	Disk interrupt handler.  Need this to be a C routine, because 
//	C++ can't handle pointers to member functions.
//----------------------------------------------------------------------

static void
DiskRequestDone (int arg)
{
    SynchDisk* disk = (SynchDisk *)arg;

    disk->RequestDone();
}

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	Initialize the synchronous interface to the physical disk, in turn
//	initializing the physical disk.
//
//	"name" -- UNIX file name to be used as storage for the disk data
//	   (usually, "DISK")
//----------------------------------------------------------------------

SynchDisk::SynchDisk(char* name)
{
    semaphore = new Semaphore("synch disk", 0);
    lock = new Lock("synch disk lock");
    disk = new Disk(name, DiskRequestDone, (int) this);

    readerLock = new Lock("reader cnt lock");
    numberLock = new Lock("visitor cnt lock");
    for(int i=0; i<NumSectors; ++i)
    {
        readerCnt[i] = 0;
        visitorCnt[i] = 0;
        mutex[i] = new Semaphore("sector lock", 1);
    }
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	De-allocate data structures needed for the synchronous disk
//	abstraction.
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{
    delete disk;
    delete lock;
    delete semaphore;

    delete readerLock;
    for(int i=0; i<NumSectors; ++i)
    {
        delete mutex[i];
    }
    delete numberLock;
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	Read the contents of a disk sector into a buffer.  Return only
//	after the data has been read.
//
//	"sectorNumber" -- the disk sector to read
//	"data" -- the buffer to hold the contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::ReadSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
    disk->ReadRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	Write the contents of a buffer into a disk sector.  Return only
//	after the data has been written.
//
//	"sectorNumber" -- the disk sector to be written
//	"data" -- the new contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::WriteSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
    disk->WriteRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::RequestDone
// 	Disk interrupt handler.  Wake up any thread waiting for the disk
//	request to finish.
//----------------------------------------------------------------------

void
SynchDisk::RequestDone()
{ 
    semaphore->V();
}









void
SynchDisk::PlusReader(int sector)
{
    DEBUG('C', "-> reader try to get lock\n");
    // printf("===> reader try to get lock %d\n", sector);
    readerLock->Acquire();
    readerCnt[sector]++;
    // printf("===> reader cnt is %d\n", readerCnt[sector]);
    if(readerCnt[sector]==1)
    {
        DEBUG('C', "-> the first reader need to acquire mutex semaphore.\n");
        //printf("===> the first reader need to acquire mutex semaphore %d.\n", sector);
        mutex[sector]->P();
    }
    readerLock->Release();  
}

void
SynchDisk::MinusReader(int sector)
{
    DEBUG('C', "-> reader release lock\n");
    readerLock->Acquire();
    readerCnt[sector]--;
    // printf("===> reader cnt is %d\n", readerCnt[sector]);
    if(readerCnt[sector]==0)
    {
        DEBUG('C', "-> the last reader need to release mutex semaphore.\n");
        // printf("===> the last reader need to release mutex semaphore %d.\n", sector);
        mutex[sector]->V();
    }
    readerLock->Release(); 
}

void
SynchDisk::PlusWriter(int sector)
{
    DEBUG('C', "-> writer try to get lock\n");
    // printf("===> writer try to get lock %d\n", sector);
    mutex[sector]->P();
    // printf("===> Now the writer start to write %d.\n", sector);
}

void
SynchDisk::MinusWriter(int sector)
{
    // printf("===> Now the writer finish writing %d.\n", sector);
    DEBUG('C', "-> writer release lock\n");
    mutex[sector]->V();
}
