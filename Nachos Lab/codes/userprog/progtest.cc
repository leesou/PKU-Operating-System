// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"


void
UserThread(int which)
{
    printf("User Program %d starts to run!\n", which);
    
    // initialization
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    //currentThread->space->PrintAddrState();
    
    // run program
    machine->Run();
    
    //should not reach
    printf("error occurs\n");
    ASSERT(FALSE);
}


Thread*
CreateThread(OpenFile* executable, int num)
{
    printf("Thread %d is being created\n", num);
    
    // allocate structrue of new thread
    char ThreadName[20];
    sprintf(ThreadName, "User Program %d", num);
    Thread *thread = new Thread(strdup(ThreadName), 0);
    
    // generate address space for new thread
    AddrSpace *space = new AddrSpace(executable);
    thread->space = space;
    
    return thread;
}


#define N 100
void
StartNProcesses(char *filename)
{
    // open executable file
    OpenFile *executable = fileSystem->Open(filename);

    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }

    // create N threads
    Thread *threads[N];
    for(int i=0; i<N; ++i)
    {
        threads[i] = CreateThread(executable, i+1);
    }
    
    // close file
    delete executable;			
    
    // fork new threads
    for(int i=0; i<N; ++i)
    {
        threads[i]->Fork(UserThread, (void*)(i+1));
    }
    
    // main thread give up CPU for new User Programs
    currentThread->Yield();
}


//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
	    printf("Unable to open file %s\n", filename);
	    return;
    }
    space = new AddrSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	    readAvail->P();		// wait for character to arrive
	    ch = console->GetChar();
	    console->PutChar(ch);	// echo it!
	    writeDone->P() ;        // wait for write to finish
	    if (ch == 'q') return;  // if q, quit
    }
}

static SynchConsole* synchConsole;

void
SynchConsoleTest(char* in, char* out)
{
    char ch;
    synchConsole = new SynchConsole(in, out);

    for(;;)
    {
        ch = synchConsole->SynchGetChar();
        
        if(ch == '!')
        {
            synchConsole->SynchPutChar(']');
            synchConsole->SynchPutChar('\n');
            return;
        }
        if(ch == '~')
        {
            synchConsole->SynchPutChar('[');
            synchConsole->SynchPutChar('\n');
            break;
        }
        else
        {
            synchConsole->SynchPutChar(ch);
        } 
    }
    printf("You've put '~' in synch console!\n");
}
