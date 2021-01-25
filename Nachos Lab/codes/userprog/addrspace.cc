// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

int SpaceCnt = 0;

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    exeSector = executable->GetHeaderSector();

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

#ifndef INVERTED_PAGETABLE

#ifndef USE_DISK
    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory
#else // USE_DISK
    char str[20];
    sprintf(str, "VirtualMemory%d", SpaceCnt++);
    VMName = strdup(str);
    bool succeed_creating_file = fileSystem->Create(VMName, MemorySize);
    ASSERT(succeed_creating_file);
#endif

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) 
    {
	    pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
#ifndef USE_DISK
#if USE_BITMAP
        pageTable[i].physicalPage = machine->allocateMem();
        ASSERT(pageTable[i].physicalPage!=-1);
#else // USE_BITMAP
	    pageTable[i].physicalPage = i;
#endif // USE_BITMAP
	    pageTable[i].valid = TRUE;
#else // USE_DISK
        pageTable[i].valid = FALSE;
#endif // USE_DISK
	    pageTable[i].use = FALSE;
	    pageTable[i].dirty = FALSE;
	    pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    }

    //printf("%x %x %x %x %x %x\n", noffH.code.size, noffH.code.inFileAddr, noffH.initData.inFileAddr, noffH.initData.size, noffH.code.virtualAddr, noffH.initData.virtualAddr);
#ifndef USE_DISK

#if USE_BITMAP
    // clear main memory used by current program
    for(int i=0; i<numPages; ++i)
    {
        for(int j=0; j<PageSize; ++j)
        {
            machine->mainMemory[pageTable[i].physicalPage*PageSize+j] = 0;
        }
    }
    
    // allocate data and code sections, we need to translate sections' virtualAddr to physicalAddr
    // one byte per loop
    for(int VA=noffH.code.virtualAddr, cnt=0; VA<noffH.code.size; ++VA, ++cnt)
    {
        unsigned int VPN = VA / PageSize;
        unsigned int PPN = pageTable[VPN].physicalPage;
        unsigned int PPO = VA % PageSize;
        unsigned int PA = PPN*PageSize + PPO;
        //printf("%d 0x%x %d 0x%x\n", PA, PA, VA, VA);
        executable->ReadAt(&(machine->mainMemory[PA]),
			1, noffH.code.inFileAddr+cnt);
	//printf("%d 0x%x\n", machine->mainMemory[PA], machine->mainMemory[PA]);
    }
    
    for(int VA=noffH.initData.virtualAddr, cnt=0; VA<noffH.initData.size; ++VA, ++cnt)
    {
        unsigned int VPN = VA / PageSize;
        unsigned int PPN = pageTable[VPN].physicalPage;
        unsigned int PPO = VA % PageSize;
        unsigned int PA = PPN*PageSize + PPO;
        //printf("%d 0x%x %d 0x%x\n", PA, PA, VA, VA);
        executable->ReadAt(&(machine->mainMemory[PA]),
			1, noffH.initData.inFileAddr+cnt);
	//printf("%d 0x%x\n", machine->mainMemory[PA], machine->mainMemory[PA]);
    }

#else // USE_BITMAP
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
#endif // USE_BITMAP

#else // USE_DISK
    DEBUG('P', "Demand paging: copy executable to virtual memory %s!\n", VMName);
    OpenFile *vm = fileSystem->Open(VMName);
    char *VM_tmp = new char[size];
    bzero(VM_tmp, size);
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(VM_tmp[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(VM_tmp[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
    vm->WriteAt(VM_tmp, size, 0);
    delete vm;
#endif



#else // INVERTED_PAGETABLE
    ASSERT(numPages <= NumPhysPages);
    for(int i=0; i<numPages; ++i)
    {
        int physicalPage = machine->allocateMem();
        //printf("%d\n", physicalPage);
        machine->pageTable[physicalPage].virtualPage = i;
        machine->pageTable[physicalPage].physicalPage = physicalPage;
        machine->pageTable[physicalPage].valid = TRUE;
        machine->pageTable[physicalPage].use = FALSE;
        machine->pageTable[physicalPage].readOnly = FALSE;
        machine->pageTable[physicalPage].dirty = FALSE;
        machine->pageTable[physicalPage].TID = currentThread->getTID();
        for(int j=0; j<PageSize; ++j)
        {
            machine->mainMemory[physicalPage*PageSize+j] = 0;
        }
    }
    
    printf("================PAGE TABLE================\n");
    for(int i=0; i<NumPhysPages; ++i)
    {
        printf("%d\t%d\t%d\t%d\n", machine->pageTable[i].virtualPage, machine->pageTable[i].physicalPage, 
                machine->pageTable[i].valid, machine->pageTable[i].TID);
    }
    printf("==========================================\n");
    // allocate data and code sections, we need to translate sections' virtualAddr to physicalAddr
    // one byte per loop
    for(int VA=noffH.code.virtualAddr, cnt=0; VA<noffH.code.size; ++VA, ++cnt)
    {
        unsigned int VPN = VA / PageSize;
        unsigned int PPN = 0;
        for(PPN=0; PPN<NumPhysPages; ++PPN)
        {
            if(machine->pageTable[PPN].valid && machine->pageTable[PPN].virtualPage==VPN 
                && machine->pageTable[PPN].TID == currentThread->getTID())
                break;
        }
        unsigned int PPO = VA % PageSize;
        unsigned int PA = PPN*PageSize + PPO;
        //printf("%d 0x%x %d 0x%x\n", PA, PA, VA, VA);
        executable->ReadAt(&(machine->mainMemory[PA]),
			1, noffH.code.inFileAddr+cnt);
	//printf("%d 0x%x\n", machine->mainMemory[PA], machine->mainMemory[PA]);
    }
    
    for(int VA=noffH.initData.virtualAddr, cnt=0; VA<noffH.initData.size; ++VA, ++cnt)
    {
        unsigned int VPN = VA / PageSize;
        unsigned int PPN = 0;
        for(PPN=0; PPN<NumPhysPages; ++PPN)
        {
            if(machine->pageTable[PPN].valid && machine->pageTable[PPN].virtualPage==VPN 
                && machine->pageTable[PPN].TID == currentThread->getTID())
                break;
        }
        unsigned int PPO = VA % PageSize;
        unsigned int PA = PPN*PageSize + PPO;
        //printf("%d 0x%x %d 0x%x\n", PA, PA, VA, VA);
        executable->ReadAt(&(machine->mainMemory[PA]),
			1, noffH.initData.inFileAddr+cnt);
	//printf("%d 0x%x\n", machine->mainMemory[PA], machine->mainMemory[PA]);
    }
#endif
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
#ifdef USE_TLB
    DEBUG('T', "Clean up TLB when context switch occurs!\n");
    for(int i=0; i<TLBSize; ++i)
    {
        machine->tlb[i].valid = FALSE;
    }
#endif
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
#ifndef INVERTED_PAGETABLE
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
#endif
}


void
AddrSpace::PrintAddrState() 
{
    printf("======= PageTable Info ==========\n");
    printf("numpages = %d\n", numPages);
    printf("VPN\tPPN\tvalid\tRD\tUsed\tDirty\n");
    for(int i=0; i<numPages; ++i)
    {
        printf("%d\t%d\t%d\t", pageTable[i].virtualPage, pageTable[i].physicalPage, pageTable[i].valid);
        printf("%d\t%d\t%d\n", pageTable[i].readOnly, pageTable[i].use, pageTable[i].dirty);
    }
    printf("=================================\n");
}


void
AddrSpace::CopyTable(AddrSpace* space)
{
    TranslationEntry* table = space->GetPageTable();
    for(int i=0; i<numPages; ++i)
    {
        pageTable[i].virtualPage = table[i].virtualPage;
        pageTable[i].physicalPage = table[i].physicalPage;
        pageTable[i].valid = table[i].valid;
        pageTable[i].readOnly = table[i].readOnly;
        pageTable[i].use = table[i].use;
        pageTable[i].dirty = table[i].dirty;
    }
}

void
AddrSpace::WriteBackAll()
{
    OpenFile *vm = fileSystem->Open(currentThread->space->VMName);
	ASSERT(vm!=NULL);
    for(int i=0; i<numPages; ++i)
    {
        if(pageTable[i].valid)
            vm->WriteAt(&(machine->mainMemory[PageSize*pageTable[i].physicalPage]), PageSize, i*PageSize);
    }
    delete vm;
}
