// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, (void*)1);
    SimpleThread(0);
}











/*******************************   added by Li Cong 1800012826   *******************************/

/*******************************************************************************************   LAB 1   *******************************************************************************************/

//----------------------------------------------------------------------
// CheckIDs
// 	used for fork
//----------------------------------------------------------------------

void 
CheckIDs(int which)
{
    int num;

    for (num = 0; num < 5; num++) {
	printf("*** thread %d with TID %d and UID %d looped %d times\n", which, currentThread->getTID(), currentThread->getUID(), num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// Excercise3Test
// 	test function for question 3
//----------------------------------------------------------------------

void 
Exercise3Test()
{
    DEBUG('t', "Entering Excercise3Test");

    const int n_threads = 5;
    const int tid = 233;

    for(int i=0; i<n_threads; ++i)
    {
        Thread *t = new Thread("forked thread");
        t->setUID(tid+i);
        t->Fork(CheckIDs, (void *)t->getTID());
    }
    CheckIDs(0);
}

//----------------------------------------------------------------------
// Excercise4_1Test
// 	test function for question 4_1
//----------------------------------------------------------------------

void 
Exercise4_1Test()
{
    DEBUG('t', "Entering Excercise4_1Test");

    const int n_threads = 514;
    const int tid = 233;

    for(int i=0; i<n_threads; ++i)
    {
        Thread *t = new Thread("forked thread");
        t->setUID(tid+i);
        t->Fork(CheckIDs, (void *)t->getTID());
    }
    CheckIDs(0);
}

//----------------------------------------------------------------------
// TS
// 	PS-like print function
//----------------------------------------------------------------------

void 
TS()
{

    DEBUG('t', "Entering TS.");

    char *State2String[] = {"JUST_CREATED", "RUNNING", "READY", "BLOCKED"};

    printf("UID\tTID\tPRI\tNAME\tSTAT\n");
    for(int i=0; i<MAX_THREAD_NUM; ++i)
    {
        if(used_TID[i])
        {
            printf("%d\t%d\t%d\t%s\t%s\n", Thread_Pointer[i]->getUID(), Thread_Pointer[i]->getTID(), Thread_Pointer[i]->getPriority(), Thread_Pointer[i]->getName(), State2String[Thread_Pointer[i]->getStatus()]);
        }
    }
}

//----------------------------------------------------------------------
// SetStatus
// 	Set the status of thread, used for test TS
//----------------------------------------------------------------------

void
SetStatus(int which)
{
    printf("***** current thread (uid=%d, tid=%d, priority=%d, name=%s) => \n", currentThread->getUID(), currentThread->getTID(), currentThread->getPriority(), currentThread->getName());
    switch(which)
    {
        case 0:
            scheduler->Print();
            printf("\n\n");
            currentThread->Yield();
            break;
        case 1:
            IntStatus oldlevel = interrupt->SetLevel(IntOff);
            currentThread->Sleep();
            (void) interrupt->SetLevel(oldlevel);
            break;
        case 2:
            currentThread->Finish();
            break;
        default:
            currentThread->Yield();
            break;
    }
    printf("thread %d finished\n\n", currentThread->getTID());
}

//----------------------------------------------------------------------
// Excercise4_2Test
// 	test function for question 4_2
//----------------------------------------------------------------------

void
Exercise4_2Test()
{
    DEBUG('t', "Entering Excercise4_2Test");

    Thread *t1 = new Thread("fork0");
    t1->Fork(SetStatus, (void *)0);
    Thread *t2 = new Thread("fork1");
    t2->Fork(SetStatus, (void *)1);
    Thread *t3 = new Thread("fork2");
    t3->Fork(SetStatus, (void *)2);

    Thread *t4 = new Thread("forked");

    SetStatus(0);

    printf("\n\n-------------------this is TS result-------------------\n");
    TS();
    printf("------------------- finish TS test  -------------------\n\n");
    
}

/*******************************************************************************************   LAB 1   *******************************************************************************************/

/*******************************************************************************************   LAB 2   *******************************************************************************************/

//----------------------------------------------------------------------
// Lab2_3_1_Test
// 	test function for 3_1 in lab2
//----------------------------------------------------------------------

void
Lab2_3_1_Test()
{
    DEBUG('t', "Entering Lab2_3_1_Test");

    Thread *t1 = new Thread("withP", 514);
    t1->Fork(SetStatus, (void *)1);
    Thread *t2 = new Thread("setP");
    t2->setPriority(114);
    t2->Fork(SetStatus, (void *)1);
    Thread *t3 = new Thread("noP");
    t3->Fork(SetStatus, (void *)1);

    Thread *t4 = new Thread("forked");

    SetStatus(0);

    printf("\n\n-------------------this is TS result-------------------\n");
    TS();
    printf("------------------- finish TS test  -------------------\n\n");
    
}

//----------------------------------------------------------------------
// Lab2_3_2_Test
// 	test function for 3_2 in lab2
//----------------------------------------------------------------------

void
Lab2_3_2_Test()
{
    DEBUG('t', "Entering Lab2_3_2_Test");

    Thread *t1 = new Thread("low", 810);
    Thread *t2 = new Thread("mid", 514);
    Thread *t3 = new Thread("high", 114);


    t1->Fork(SetStatus, (void *)0);
    t2->Fork(SetStatus, (void *)0);
    t3->Fork(SetStatus, (void *)0);

    SetStatus(0);

    printf("\n\n-------------------this is TS result-------------------\n");
    TS();
    printf("------------------- finish TS test  -------------------\n\n");

}

//----------------------------------------------------------------------
// p3
// 	test function for 3_2 in lab2, just loop
//----------------------------------------------------------------------

void
p3(int which)
{
    for(int i=0; i<5; ++i)
    {
        printf("***** thread name %s priority %d looped %d times\n", currentThread->getName(), currentThread->getPriority(), i);
    }
}

//----------------------------------------------------------------------
// p2
// 	test function for 3_2 in lab2, 
//      loop and create a middle priority thread
//----------------------------------------------------------------------

void
p2(int which)
{
    for(int i=0; i<5; ++i)
    {
        printf("***** thread name %s priority %d looped %d times\n", currentThread->getName(), currentThread->getPriority(), i);
        if(i==0)
        {
            Thread *t3 = new Thread("mid", 514);
            t3->Fork(p3, (void *)0);
        }
    }
}

//----------------------------------------------------------------------
// p1
// 	test function for 3_2 in lab2, 
//      loop and create a high priority thread
//----------------------------------------------------------------------

void
p1(int which)
{
    for(int i=0; i<5; ++i)
    {
        printf("***** thread name %s priority %d looped %d times\n", currentThread->getName(), currentThread->getPriority(), i);
        if(i==0)
        {
            Thread *t2 = new Thread("high", 114);
            t2->Fork(p2, (void *)0);
        }
    }
}

//----------------------------------------------------------------------
// Lab2_3_2_Test1
// 	test function for 3_2 in lab2,
//      for the correctness of creating a higher thread
//----------------------------------------------------------------------

void
Lab2_3_2_Test1()
{
    DEBUG('t', "Entering Lab2_3_2_Test1");

    Thread *t1 = new Thread("low", 810);

    t1->Fork(p1, (void *)0);

    printf("\n\n-------------------this is TS result-------------------\n");
    TS();
    printf("------------------- finish TS test  -------------------\n\n");
}

//----------------------------------------------------------------------
// ClockSimulator1
// 	simulate clock interrupt to move on the time
//----------------------------------------------------------------------

void
ClockSimulator1(int loops)
{   
    for (int i=0; i<loops*SystemTick; i++) {
        printf("*** thread with priority %d looped %d times (stats->totalTicks: %d)\n", currentThread->getPriority(), i+1, stats->totalTicks);
        scheduler->Print();
        printf("\n");
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
    }
    currentThread->Finish();
}

//----------------------------------------------------------------------
// Lab2_3_2_Test2
// 	test function for 3_2 in lab2,
//      for the correctness of timer interrupt
//----------------------------------------------------------------------

void
Lab2_3_2_Test2()
{
    DEBUG('t', "Entering Lab2_3_2_Test2");

    Thread *t1 = new Thread("low", 810);
    Thread *t2 = new Thread("mid", 514);
    Thread *t3 = new Thread("high", 114);


    t1->Fork(ClockSimulator1, (void *)1);
    t2->Fork(ClockSimulator1, (void *)1);
    t3->Fork(ClockSimulator1, (void *)1);

    printf("\n\n-------------------this is TS result-------------------\n");
    TS();
    printf("------------------- finish TS test  -------------------\n\n");

    ClockSimulator1(1);
}

//----------------------------------------------------------------------
// ClockSimulator
// 	simulate clock interrupt to move on the time
//----------------------------------------------------------------------

void
ClockSimulator(int loops)
{   
    for (int i=0; i<loops*SystemTick; i++) {
        printf("*** thread with running time %d looped %d times (stats->totalTicks: %d)\n", loops, i+1, stats->totalTicks);
        scheduler->Print();
        printf("\n");
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
    }
    currentThread->Finish();
}

//----------------------------------------------------------------------
// Lab2_3_ch_Test
// 	test function for challenge in lab2
//----------------------------------------------------------------------

void
Lab2_3_ch_Test()
{
    DEBUG('t', "Entering Lab2_3_ch_Test");

    Thread *t1 = new Thread("low", 810);
    Thread *t2 = new Thread("mid", 514);
    Thread *t3 = new Thread("high", 114);


    t1->Fork(ClockSimulator, (void *)3);
    t2->Fork(ClockSimulator, (void *)2);
    t3->Fork(ClockSimulator, (void *)4);

    printf("\n\n-------------------this is TS result-------------------\n");
    TS();
    printf("------------------- finish TS test  -------------------\n\n");

    ClockSimulator(1);

}

/*******************************************************************************************   LAB 2   *******************************************************************************************/

/*******************************************************************************************   LAB 3   *******************************************************************************************/

//----------------------------------------------------------------------
// Element
//  Elements saved in Buffer's list.
//----------------------------------------------------------------------

class Element
{
    public:
        int value;
};

//----------------------------------------------------------------------
// Buffer
//  Use Semaphore to solve Prodecer-Consumer Problem.
//  When list is full, no Prodecer can write it.
//  And when list is empty, no Consumer can read it.
//----------------------------------------------------------------------

# define BUF_LEN 5

class Buffer
{
    private:
        Semaphore* Empty;
        Semaphore* Full;
        Lock* Mutex;
        int count;
        Element list[BUF_LEN];
    
    public:
        Buffer()
        {
            Empty = new Semaphore("Empty Count", BUF_LEN);
            Full = new Semaphore("Full Count", 0);
            Mutex = new Lock("Buffer Mutex");
            count = 0;
        }

        ~Buffer()
        {
            delete list;
        }

        void WriteBuffer(Element* product)
        {
            //printf("0\n");
            Empty->P();
            //printf("1\n");
            Mutex->Acquire();
           // printf("pointer is %ld\n", product);
            
            /*  Critical Rigion  */
            list[count++] = *product;
            /*  Critical Region  */
            
            Mutex->Release();
            Full->V();

        }

        Element* ReadBuffer()
        {
            Element* item;

            Full->P();
            Mutex->Acquire();

            /*  Critical Rigion  */
            item = &list[count-1];
            count--;
            /*  Critical Region  */

            Mutex->Release();
            Empty->V();

            return item;
        }

        void printBuffer()
        {
            printf("Buffer(size: %d, count: %d): [", BUF_LEN, count);
            for(int i=0; i<count; ++i)
            {
                printf("%d, ", list[i].value);
            }
            for(int i=count; i<BUF_LEN; ++i)
            {
                printf("__, ");
            }
            printf("]\n");
        }
}*globalBuf;

//----------------------------------------------------------------------
// ProduceItem
//  Produce element and print it.
//----------------------------------------------------------------------

Element
ProduceItem(int value)
{
    Element item;
    //printf("0");
    item.value = value;
    return  item;
}

//----------------------------------------------------------------------
// ComsmeItem
//  Consume element and print it.
//----------------------------------------------------------------------

void 
ConsumeItem(Element* item)
{
    printf("Consume item with value %d\n", item->value);
    // delete item;
}

//----------------------------------------------------------------------
// ProducerThread
//  Used to create Producers.
//----------------------------------------------------------------------

void
ProducerThread(int iters)
{
    for(int i=0; i<iters; ++i)
    {
        int TID = currentThread->getTID();
        printf("## Thread %s produces!!!\n", currentThread->getName());
        Element item = ProduceItem(i+TID);
        //printf("tmp ptr is %ld\n", & item);
        globalBuf->WriteBuffer(& item);
        printf("Produce Element with value %d\n", item.value);
        globalBuf->printBuffer();

        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
    }
}

//----------------------------------------------------------------------
// ProducerThread
//  Used to create Consumers.
//----------------------------------------------------------------------

void
ConsumerThread(int iters)
{
    for(int i=0; i<iters; ++i)
    {
        int TID = currentThread->getTID();
        printf("## Thread %s consumes!!!\n", currentThread->getName());
        Element* item = globalBuf->ReadBuffer();
        ConsumeItem(item);
        globalBuf->printBuffer();

        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
    }
}

//----------------------------------------------------------------------
// Lab3Test4_1
//  Test function.
//----------------------------------------------------------------------

void
Lab3Test4_1()
{
    DEBUG('t', "Entering Lab3Test4_1.");

    globalBuf = new Buffer();

    Thread *producer1 = new Thread("Producer 1");
    Thread *producer2 = new Thread("Producer 2");
    Thread *consumer1 = new Thread("Consumer 1");
    Thread *consumer2 = new Thread("Consumer 2");

    producer1->Fork(ProducerThread, (void*)7);
    consumer1->Fork(ConsumerThread, (void*)2);
    consumer2->Fork(ConsumerThread, (void*)8);
    producer2->Fork(ProducerThread, (void*)3);
}


//----------------------------------------------------------------------
// BufferCon
//  Use Condition variables and lock to solve Prodecer-Consumer Problem.
//  When list is full, no Prodecer can write it.
//  And when list is empty, no Consumer can read it.
//----------------------------------------------------------------------

class BufferCnd
{
    private:
        Condition* Producer;
        Condition* Consumer;
        Lock* Mutex;
        int count;
        Element list[BUF_LEN];
    
    public:
        BufferCnd()
        {
            Producer = new Condition("Producer variable");
            Consumer = new Condition("Consumer variable");
            Mutex = new Lock("Buffer Mutex");
            count = 0;
        }

        ~BufferCnd()
        {
            delete list;
        }

        int getCnt(){return count;}

        void WriteBuffer(Element* product)
        {
            Mutex->Acquire();

            while(count==BUF_LEN)
            {
                printf("##### FULL!! #####\n");
                Producer->Wait(Mutex);
            }

            /*  Critical Rigion  */
            list[count++] = *product;
            /*  Critical Region  */
            
            Consumer->Signal(Mutex);

            Mutex->Release();
        }

        Element* ReadBuffer()
        {
            Element* item;

            Mutex->Acquire();

            while(count==0)
            {
                printf("##### EMPTY!! #####\n");
                Consumer->Wait(Mutex);
            }

            /*  Critical Rigion  */
            item = &list[count-1];
            count--;
            /*  Critical Region  */

            Producer->Signal(Mutex);

            Mutex->Release();

            return item;
        }

        void printBuffer()
        {
            printf("Buffer(size: %d, count: %d): [", BUF_LEN, count);
            for(int i=0; i<count; ++i)
            {
                printf("%d, ", list[i].value);
            }
            for(int i=count; i<BUF_LEN; ++i)
            {
                printf("__, ");
            }
            printf("]\n");
        }
}*globalBufCnd;

//----------------------------------------------------------------------
// ProducerThreadCnd
//  Used to create Producers.
//----------------------------------------------------------------------

void
ProducerThreadCnd(int iters)
{
    for(int i=0; i<iters; ++i)
    {
        int TID = currentThread->getTID();
        printf("## Thread %s produces!!!\n", currentThread->getName());
        Element item = ProduceItem(i+TID);
        //printf("tmp ptr is %ld\n", & item);
        printf("Produce Element with value %d\n", item.value);
        globalBufCnd->WriteBuffer(& item);
        globalBufCnd->printBuffer();

        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
    }
}

//----------------------------------------------------------------------
// ProducerThread
//  Used to create Consumers.
//----------------------------------------------------------------------

void
ConsumerThreadCnd(int iters)
{
    for(int i=0; i<iters; ++i)
    {
        int TID = currentThread->getTID();
        printf("## Thread %s consumes!!!\n", currentThread->getName());
        Element* item = globalBufCnd->ReadBuffer();
        ConsumeItem(item);
        globalBufCnd->printBuffer();

        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
    }
}

//----------------------------------------------------------------------
// Lab3Test4_2
//  Test function.
//----------------------------------------------------------------------

void
Lab3Test4_2()
{
    DEBUG('t', "Entering Lab3Test4_2.");

    globalBufCnd = new BufferCnd();

    Thread *producer1 = new Thread("Producer 1");
    Thread *producer2 = new Thread("Producer 2");
    Thread *consumer1 = new Thread("Consumer 1");
    Thread *consumer2 = new Thread("Consumer 2");

    producer1->Fork(ProducerThreadCnd, (void*)7);
    consumer1->Fork(ConsumerThreadCnd, (void*)2);
    consumer2->Fork(ConsumerThreadCnd, (void*)8);
    producer2->Fork(ProducerThreadCnd, (void*)3);
}

Barrier* Bar;

//----------------------------------------------------------------------
// BarrierThread
//  Call ArriveAndWait for 3 times to test the correceness
//----------------------------------------------------------------------

void
BarrierThread(int num)
{
    printf("## Thread %s with num [%d] reached 1st barrier!\n", currentThread->getName(), num);
    Bar->ArriveAndWait();
    printf("## Thread %s with num [%d]  crossed over barrier 1 and reached barrier 2!\n", currentThread->getName(), num);
    Bar->ArriveAndWait();
    printf("## Thread %s with num [%d] crossed over barrier 2 and reached barrier 3!\n", currentThread->getName(), num);
    Bar->ArriveAndWait();
    printf("## Thread %s with num [%d] crossed all barriers and reached deadline!\n");
}

//----------------------------------------------------------------------
// Lab3Challenge1
//  Test function.
//----------------------------------------------------------------------

void
Lab3Challenge1()
{
    DEBUG('t', "Entering Lab3Challenge1.");

    int num_threads = 10;
    Bar = new Barrier("Lab3 Test", num_threads+1); // main thread should also be included

    for(int i=0; i<num_threads; ++i)
    {
        Thread* t = new Thread("[Test]");
        t->Fork(BarrierThread, (void*)i);
    }

    TS();

    printf("main starts to cross barrier 1!\n");
    //currentThread->Yield();
    Bar->ArriveAndWait();
    printf("main crossed over barrier 1 and reached barrier 2!\n");
    //currentThread->Yield();
    Bar->ArriveAndWait();
    printf("main crossed over barrier 1 and reached barrier 2!\n");
    //currentThread->Yield();
    Bar->ArriveAndWait();
    printf("main crossed over all barriers and reached DDL!\n");
}

ReadWriteLock* RWLock;
int Buff = 114514;

//----------------------------------------------------------------------
// ReaderThread
//  Used to create a reader.
//----------------------------------------------------------------------

void
ReaderThread(int num)
{
    for(int i=0; i<10; ++i)
    {
        printf("Reader %d comes!\n", num);
        RWLock->AcquireReaderLock();

        printf("Reader %d is reading the buffer!\n", num);
        currentThread->Yield();
        printf("Reader %d says The Buffer is %d\n",num,  Buff);

        RWLock->ReleaseReaderLock();
    }
}

//----------------------------------------------------------------------
// WriterThread
//  Used to create a writer.
//----------------------------------------------------------------------

void
WriterThread(int num)
{
        for(int i=0; i<10; ++i)
    {
        printf("Writer %d comes!\n", num);
        RWLock->AcquireWriterLock();

        printf("Writer %d is writing the buffer!\n", num);
        currentThread->Yield();
        Buff+=num+i;

        RWLock->ReleaseWriterLock();
    }
}

//----------------------------------------------------------------------
// Lab3Challenge2
//  Test function.
//----------------------------------------------------------------------

void
Lab3Challenge2()
{
    DEBUG('t', "Entering Lab3Challenge2.");

    RWLock = new ReadWriteLock("Lab 3 Challenge 2");
    int numReader = 2;

    Thread* t1 = new Thread("Writer");
    t1->Fork(WriterThread, (void*)0);

    for(int i=0; i<numReader; ++i)
    {
        Thread* t = new Thread("Reader");
        t->Fork(ReaderThread, (void*)i);
    }

    Thread* t2 = new Thread("Writer");
    t2->Fork(WriterThread, (void*)1);

    TS();
    printf("########## Test starts! ##########\n");
}

void
Lab3Challenge2_1()
{
    DEBUG('t', "Entering Lab3Challenge2_1.");

    RWLock = new ReadWriteLock("Lab 3 Challenge 2_1");
    int numReader = 2;

    for(int i=0; i<numReader; ++i)
    {
        Thread* t = new Thread("Reader");
        t->Fork(ReaderThread, (void*)i);
    }

    Thread* t1 = new Thread("Writer");
    t1->Fork(WriterThread, (void*)0);

    Thread* t2 = new Thread("Writer");
    t2->Fork(WriterThread, (void*)1);

    TS();
    printf("########## Test starts! ##########\n");
}


/*******************************************************************************************   LAB 3   *******************************************************************************************/

/*******************************   added by Li Cong 1800012826   *******************************/

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
/*******************************   added by Li Cong 1800012826   *******************************/
    case 2:
        printf("Just a test for make!\n");
    case 3:
        Exercise3Test();
        break;
    case 4:
        Exercise4_1Test();
        break;
    case 5:
        Exercise4_2Test();
        break;
    case 6:
        Lab2_3_1_Test();
        break;
    case 7:
        Lab2_3_2_Test();
        break;
    case 8:
        Lab2_3_2_Test1();
        break;
    case 9:
        Lab2_3_ch_Test();
        break;
    case 10:
        Lab2_3_2_Test2();
        break;
    case 11:
        Lab3Test4_1();
        break;
    case 12:
        Lab3Test4_2();
        break;
    case 13:
        Lab3Challenge1();
        break;
    case 14:
        Lab3Challenge2();
        break;
    case 15:
        Lab3Challenge2_1();
        break;
/*******************************   added by Li Cong 1800012826   *******************************/
    default:
	printf("No test specified.\n");
	break;
    }
}

