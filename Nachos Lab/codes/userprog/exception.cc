// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

#ifdef USE_FIFO
void
TLBFIFO(TranslationEntry page)
{
	DEBUG('m', "===> Now use FIFO algotithm to exchange TLB item.\n");
	int idx = -1;
	
	for(int i=0; i<TLBSize; ++i)
	{
		if(machine->tlb[i].valid == FALSE) // find a n unused item
		{
			idx = i;
			DEBUG('m', "===> Find a free item in TLB.\n");
			break;
		}
	}
	
	if(idx == -1) // all items are used
	{
		DEBUG('m', "===> We need to remove the first page in TLB.\n");
		for(int i=0; i<TLBSize-1; ++i)
		{
			machine->tlb[i] = machine->tlb[i+1];
		}
		idx = TLBSize - 1;
	}
	
	machine->tlb[idx] = page;
}
#endif

int Index = 0;

#ifdef USE_CLOCK
void
TLBClock(TranslationEntry page)
{
	DEBUG('m', "===> Now use CLOCK algotithm to exchange TLB item.\n");
	while(true)
	{
		if(machine->tlb[Index].valid == FALSE) // there is a free item
		{
			DEBUG('m', "===> Find a free item in TLB.\n");
			break;
		}
		else // this item is occupied
		{
			if(machine->tlb[Index].use) // just used once
			{
				DEBUG('m', "===> Item %d can be given another chance.\n", Index);
				machine->tlb[Index].use = FALSE;
				Index = (Index +1) % TLBSize;
			}
			else // have given one chance
			{
				DEBUG('m', "===> Item %d have been given extra chance.\n", Index);
				break;
			}
		}
	}
	
	// initialize new entry
	machine->tlb[Index] = page;
	machine->tlb[Index].use = TRUE;
}
#endif

int
ReplacePage()
{
	DEBUG('P', "===> Try to find a physical page in current pagetable.\n");
	int physicalPage = -1;
	for(int i=0; i<machine->pageTableSize; ++i)
	{
		if(machine->pageTable[i].valid)
		{
			if(!machine->pageTable[i].dirty)
			{
				DEBUG('P', "===> Find an unmodified physical page.\n");
				machine->pageTable[i].valid = FALSE;
				physicalPage = machine->pageTable[i].physicalPage;
				break;
			}
		}
	}
	if(physicalPage == -1) // all has been modified
	{
		for(int i=0; i<machine->pageTableSize; ++i)
		{
			if(machine->pageTable[i].valid)
			{
				DEBUG('P', "===> Find a modified physical page.\n");
				machine->pageTable[i].valid = FALSE;
				physicalPage = machine->pageTable[i].physicalPage;
				
				OpenFile *vm = fileSystem->Open(currentThread->space->VMName);
				ASSERT(vm!=NULL);
				vm->WriteAt(&(machine->mainMemory[PageSize*machine->pageTable[i].physicalPage]), PageSize, i*PageSize);
				delete vm;
				break;
			}
		}
	}
	return physicalPage;
}


void
PageFaultHandler(int vpn)
{
	int physicalPage = -1;
#ifdef USE_BITMAP
	physicalPage = machine->allocateMem();
#else
	ASSERT(FALSE);
#endif
	if(physicalPage == -1)
	{
		physicalPage = ReplacePage();
	}
	machine->pageTable[vpn].physicalPage = physicalPage;
	
	DEBUG('P', "Load page from virtual memory %s\n", currentThread->space->VMName);
	OpenFile *vm = fileSystem->Open(currentThread->space->VMName);
	ASSERT(vm!=NULL);
	vm->ReadAt(&(machine->mainMemory[PageSize*physicalPage]), PageSize, vpn*PageSize);
	delete vm;
	
	machine->pageTable[vpn].valid = TRUE;
	machine->pageTable[vpn].use = FALSE;
	machine->pageTable[vpn].dirty = FALSE;
	machine->pageTable[vpn].readOnly = FALSE;
	
	//currentThread->space->PrintAddrState();
}

void
TLBMissHandler(int addr)
{
	DEBUG('m', "===> Handle TLB miss caused by VAddr %d!\n", addr);

	unsigned int vpn = (unsigned) addr / PageSize;
	//unsigned int offset = (unsigned) addr % PageSize;  // Maybe we'll not use this.
	TranslationEntry page = machine->pageTable[vpn];
#ifndef USE_DISK
	ASSERT(page.valid);
#else
	if(!page.valid)
	{
		DEBUG('P', "===> Page Miss Found, vpn is %d 0x%x!\n", vpn, vpn);
		PageFaultHandler(vpn);
	}
#endif

#ifdef USE_FIFO
	TLBFIFO(page);
#elif USE_CLOCK
	TLBClock(page);
#else
	machine->tlb[Index] = page;
	Index = (Index + 1) % TLBSize;
#endif
}

void
TLBMissRate()
{
#ifdef USE_TLB
	printf("Total translation count is %d, translation missing count is %d, miss rate is %.6f.\n", 
	totalcnt-misscnt, misscnt, misscnt*1.0 / (totalcnt-misscnt));
#endif
}


/********************************* about system call *********************************/

#define FileNameMaxLen 		85

char*
GetFileName(int address)
{
	int pos = 0;
	int data;
	char* fileName = new char[FileNameMaxLen+1];

	do
	{
		// the first time might meet TLB miss
		if(!machine->ReadMem(address+pos, 1, &data)) 
			ASSERT(machine->ReadMem(address+pos, 1, &data));
		fileName[pos++] = (char)data;
		ASSERT(pos <= FileNameMaxLen); // should not be too long
	} while (data != '\0');
	fileName[pos] = '\0';

	// printf("name is %s \n", fileName);
	return fileName;	
}

void
IncreasePC()
{
	int PC = machine->ReadRegister(PCReg);
	int NextPC = machine->ReadRegister(NextPCReg);

	machine->WriteRegister(PrevPCReg, PC);
	machine->WriteRegister(PCReg, NextPC);
	machine->WriteRegister(NextPCReg, NextPC+4);
}

/***************************
 *     thread operation    *
 **************************/

void
ExecFunc(int addr)
{
	DEBUG('S', "initialize thread created by Exec\n");
	char* fileName = GetFileName(addr);

	// open executable file
	OpenFile* file = fileSystem->Open(fileName);
	ASSERT(file!=NULL);

	// create thread and address space
	AddrSpace* space = new AddrSpace(file);
	space->InitRegisters();
	space->RestoreState();
	currentThread->space = space;

	delete file;
	DEBUG('S', "---start to run---\n");
	machine->Run();
}

void
ExecHandler()
{
	DEBUG('S', "Exec system call\n");
	// get file name
	int addr = machine->ReadRegister(4);

	// create new thread and set it
	Thread* thread = new Thread("Exec by other thread");
	thread->Fork(ExecFunc, (void*)addr);

	// fill return value, increase PC
	machine->WriteRegister(2, thread->getTID());
	IncreasePC();
}

void
JoinHandler()
{
	// get Thread id
	DEBUG('S', "System call Join\n");
	int TID = machine->ReadRegister(4);
	DEBUG('S', "\nwait for thread with tid %d\n", TID);

	// check whether the thread ends, yield if still running
	while(TRUE)
	{
		threadLock->Acquire();
		if(!used_TID[TID])
		{
			threadLock->Release();
			break;
		}
		// run for too many times
		// DEBUG('S', "wait for thread\n"); 
		threadLock->Release();
		currentThread->Yield();
	}

	// after waiting, write return value and increase PC
	DEBUG('S', "Finish waiting\n");
	threadLock->Acquire();
	machine->WriteRegister(2, end_state[TID]);
	threadLock->Release();
	IncreasePC();
}

void
ForkFunc(int infoAddr)
{
	DEBUG('S', "initialize thread created by Fork\n");
	// get info
	ForkInfo* info = (ForkInfo*) infoAddr;

	// initialize addr space
	DEBUG('S', "set addr space\n");
	currentThread->space = info->addrSpace;
	currentThread->space->InitRegisters();
	currentThread->space->RestoreState();

	// change PC
	DEBUG('S', "set PC\n");
	machine->WriteRegister(PCReg, info->PC);
	machine->WriteRegister(NextPCReg, info->PC+4);
	machine->Run();
}

void
ForkHandler()
{
	DEBUG('S', "System call Fork\n");
	// get func ptr, also regarded as new PC for child thread
	int addr = machine->ReadRegister(4);

	// update vm and initialize new space
	currentThread->space->WriteBackAll();
	OpenFile* executable = new OpenFile(currentThread->space->exeSector);
	AddrSpace* addrSpace = new AddrSpace(executable);
	delete executable;
	// addrSpace->CopyTable(currentThread->space);

	DEBUG('S', "---flush virtual memory from %s to %s---\n", currentThread->space->VMName, addrSpace->VMName);
	OpenFile* vm = fileSystem->Open(currentThread->space->VMName);
	OpenFile* nextvm = fileSystem->Open(addrSpace->VMName);
	ASSERT(vm != NULL && nextvm != NULL);
	int size = currentThread->space->GetNumPages()*PageSize;
	char* buf = new char[size];
	vm->Read(buf, size);
	nextvm->Write(buf, size);
	delete vm; delete nextvm;
	DEBUG('S', "---finish flush virtual memory---\n");

	// initialize info
	ForkInfo* info = new ForkInfo;
	info->addrSpace = addrSpace;
	info->PC = addr;

	// execute fork function
	Thread* thread = new Thread("created by Fork Syscall");
	thread->Fork(ForkFunc, (void*)info);

	// copy open-file table
	DEBUG('S', "---copy---\n");
	tableLock->Acquire();
	ASSERT(fileSystem->CopyFile(thread->getTID()));
	tableLock->Release();
	DEBUG('S', "---finish copy---\n");
	// increase PC
	IncreasePC();
}

void
YieldHandler()
{
	DEBUG('S', "System call Yield\n");

	// or we will loop infinitely 
	IncreasePC(); 
	currentThread->Yield();
	DEBUG('S', "Back from Yield\n");
}

void
ExitHandler()
{
	// deal with exit status
	int status = machine->ReadRegister(4); // get the first argument ------ exit status
	if(status == 0)
		DEBUG('S', "******User program exit normally******\n"); 
	else
		DEBUG('S', "******User program exit with status %d******\n", status);
	// record exit status
	threadLock->Acquire();
	end_state[currentThread->getTID()] = status;
	threadLock->Release(); 

	// release space
#ifdef USER_PROGRAM
	if(currentThread->space != NULL)
	{
#if USE_BITMAP || INVERTED_PAGETABLE
		machine->freeMem();
#endif
		currentThread->space->SaveState(); // clear tlb when current thread exits

		// should delete virtual memory file
		// printf("---%s---\n", currentThread->space->VMName);
		fileSystem->Remove(currentThread->space->VMName);

		delete currentThread->space;
		currentThread->space = NULL;
	}
	
#ifdef INVERTED_PAGETABLE
	printf("================PAGE TABLE================\n");
	for(int i=0; i<NumPhysPages; ++i)
	{
		printf("%d\t%d\t%d\t%d\n", machine->pageTable[i].virtualPage, machine->pageTable[i].physicalPage, machine->pageTable[i].valid, machine->pageTable[i].TID);
	}
	printf("==========================================\n");
#endif

#endif

	// should clear unclosed files
	tableLock->Acquire();
	fileSystem->CloseAll();
	fileSystem->PrintTable();
	tableLock->Release();

	// finish this user thread
	currentThread->Finish();
}

/**************************
 *     file operation     *
 **************************/

void
CreateHandler()
{
	int startAddr = machine->ReadRegister(4);
	DEBUG('S', "Start to create new file, addr %x\n", startAddr);
	// printf("\naddr %x\n\n", startAddr);
	char* fileName = GetFileName(startAddr);
	DEBUG('S', "file name is %s \n", fileName);
	// printf("\n%s  filename\n\n", fileName);
	ASSERT(fileSystem->Create(fileName, 0));
	IncreasePC();
}

void 
OpenHandler()
{
	DEBUG('S', "Try to open file\n");
	// get file name
	int startAddr = machine->ReadRegister(4);
	// printf("\naddr %x\n\n", startAddr);
	char* fileName = GetFileName(startAddr);

	// get file
	OpenFile* openFile = fileSystem->Open(fileName);
	ASSERT(openFile != NULL);

	// add to list
	tableLock->Acquire();
	int id = fileSystem->AddNewFile(openFile);
	tableLock->Release();
	ASSERT(id>=2);

	// write return value
	machine->WriteRegister(2, (OpenFileId)id);
	IncreasePC();
}

void 
CloseHandler()
{
	DEBUG('S', "Try to close file\n");
	// tableLock->Acquire();
	// fileSystem->PrintTable();
	// tableLock->Release();
	// get file id
	int id = machine->ReadRegister(4);
	// printf("\nid is %d\n\n", id);

	// remove file
	tableLock->Acquire();
	ASSERT(fileSystem->RemoveFile(id));
	// fileSystem->PrintTable();
	tableLock->Release();
	IncreasePC();
}

void 
ReadHandler()
{
	DEBUG('S', "Try to read file\n");
	// get parameters
	int addr = (char*) machine->ReadRegister(4);
	int size = machine->ReadRegister(5);
	int id = machine->ReadRegister(6);

	// read file
	char* buffer = new char[size+1];
	int numBytes = 0;
	if(id == 0)
	{
		scanf("%s", buffer);
		numBytes = strlen(buffer);
	}
	else
	{
		tableLock->Acquire();
		OpenFile* file = fileSystem->GetFile(id);
		tableLock->Release();
		ASSERT(file!=NULL);
		numBytes = file->Read(buffer, size);
	}
	buffer[numBytes] = '\0'; // ensure content
	
	// write to memory
	// printf("\nread content %s\n\n", buffer);
	for(int i=0; i<numBytes; ++i)
	{
		if(!machine->WriteMem(addr+i, 1, (int)buffer[i]))
			machine->WriteMem(addr+i, 1, (int)buffer[i]);
	}

	// set return value and PC
	machine->WriteRegister(2, numBytes);
	IncreasePC();
}

void 
WriteHandler()
{
	DEBUG('S', "Try to write file\n");
	// get parameters
	int addr = (char*) machine->ReadRegister(4);
	int size = machine->ReadRegister(5);
	int id = machine->ReadRegister(6);

	// get content(read memory)
	char* buffer = new char[size+1];
	for(int i=0; i<size; ++i)
	{
		if(!machine->ReadMem(addr+i, 1, (int*)&buffer[i]));
			machine->ReadMem(addr+i, 1, (int*)&buffer[i]);
	}
	buffer[size] = '\0'; // ensure content

	// write to file
	// printf("\nwrite content %s\n\n", buffer);
	int numBytes = 0;
	if(id == 1)
	{
		numBytes = strlen(buffer);
		printf("%s", buffer);
	}
	else
	{
		tableLock->Acquire();
		OpenFile* file = fileSystem->GetFile(id);
		tableLock->Release();
		ASSERT(file!=NULL);
		numBytes = file->Write(buffer, size);
	}
	
	// incerase PC
	IncreasePC();
}


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
	int type = machine->ReadRegister(2);

	/* added by leesou 1800012826 Lab4 Exercise 2 */
	//printf("User mode exception %d %d\n", which, type);

	if (which == PageFaultException)
	{
#ifndef INVERTED_PAGETABLE
		if(machine->tlb == NULL)  // PageTable error
		{
			DEBUG('m', "===> Page Table Fault.\n");
			ASSERT(FALSE);
		}
		else  // TLB miss
		{
			DEBUG('m', "===> TLB miss arises!\n");
			int badVAddr = machine->ReadRegister(BadVAddrReg);
			TLBMissHandler(badVAddr);
		}
		return;
#else
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
#endif
	}
	
	/* added by leesou 1800012826 Lab4 Exercise 2 */

	if (which == SyscallException) 
	{
		if(type == SC_Halt)
		{
			DEBUG('a', "Shutdown, initiated by user program.\n");
			TLBMissRate();
#ifdef USER_PROGRAM
			if(currentThread->space != NULL)
			{
#ifdef USE_BITMAP
				machine->freeMem();
#endif
				delete currentThread->space;
				currentThread->space = NULL;
			}
#endif
   			interrupt->Halt();
		} 
		else if(type == SC_Exit)
			ExitHandler();
		else if(type == SC_Create)
			CreateHandler();
		else if(type == SC_Open)
			OpenHandler();
		else if(type == SC_Read)
			ReadHandler();
		else if(type == SC_Write)
			WriteHandler();
		else if(type == SC_Close)
			CloseHandler();
		else if(type == SC_Exec)
			ExecHandler();
		else if(type == SC_Join)
			JoinHandler();
		else if(type == SC_Fork)
			ForkHandler();
		else if(type == SC_Yield)
			YieldHandler();
	}
	else 
	{
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
	}
}
