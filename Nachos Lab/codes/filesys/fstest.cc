// fstest.cc 
//	Simple test routines for the file system.  
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file 
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"

#define TransferSize 	10 	// make it small, just to be difficult

//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void
Copy(char *from, char *to)
{
	FILE *fp;
	OpenFile* openFile;
	int amountRead, fileLength;
	char *buffer;

// Open UNIX file
	if ((fp = fopen(from, "r")) == NULL) {	 
		printf("Copy: couldn't open input file %s\n", from);
		return;
	}

// Figure out length of UNIX file
	fseek(fp, 0, 2);		
	fileLength = ftell(fp);
	fseek(fp, 0, 0);

// Create a Nachos file of the same length
	DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
	if (!fileSystem->Create(to, fileLength)) {	 // Create Nachos file
		printf("Copy: couldn't create output file %s\n", to);
		fclose(fp);
		return;
	}
	
	
	openFile = fileSystem->Open(to);
	ASSERT(openFile != NULL);
	
// Copy the data in TransferSize chunks
	buffer = new char[TransferSize];
	while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
		openFile->Write(buffer, amountRead);	
	delete [] buffer;

// Close the UNIX and the Nachos files
	delete openFile;
	fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void
Print(char *name)
{
	OpenFile *openFile;    
	int i, amountRead;
	char *buffer;

	if ((openFile = fileSystem->Open(name)) == NULL) {
	printf("Print: unable to open file %s\n", name);
	return;
	}
	
	buffer = new char[TransferSize];
	while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
	for (i = 0; i < amountRead; i++)
		printf("%c", buffer[i]);
	delete [] buffer;

	delete openFile;		// close the Nachos file
	return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName 	"TestFile"
#define FileNameLong 	"TestFileLong"
#define Contents 	"1234567890"
#define ContentSize 	strlen(Contents)
#define FileSize 	((int)(ContentSize * 3))

static void 
FileWrite()
{
	OpenFile *openFile;    
	int i, numBytes;

	printf("Sequential write of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);
	if (!fileSystem->Create(FileName, 0)) 
	{
		printf("Perf test: can't create %s\n", FileName);
		return;
	}
	openFile = fileSystem->Open(FileName);
	if (openFile == NULL) 
	{
		printf("Perf test: unable to open %s\n", FileName);
		return;
	}
	for (i = 0; i < FileSize; i += ContentSize) 
	{
		numBytes = openFile->Write(Contents, ContentSize);
		if (numBytes < ContentSize) 
		{
			printf("Perf test: unable to write %s\n", FileName);
			delete openFile;
			return;
		}
	}
	delete openFile;	// close file
}

static void 
FileRead()
{
	OpenFile *openFile;    
	char *buffer = new char[ContentSize];
	int i, numBytes;

	printf("Sequential read of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);

	if ((openFile = fileSystem->Open(FileName)) == NULL) 
	{
		printf("Perf test: unable to open file %s\n", FileName);
		delete [] buffer;
		return;
	}
	for (i = 0; i < FileSize; i += ContentSize) 
	{
		numBytes = openFile->Read(buffer, ContentSize);
		// printf("%s\n", buffer);
		if ((numBytes < ContentSize) || strncmp(buffer, Contents, ContentSize)) 
		{
			printf("Perf test: unable to read %s\n", FileName);
			delete openFile;
			delete [] buffer;
			return;
		}
	}
	delete [] buffer;
	delete openFile;	// close file
}

char* str = "1234567890";
char str1[10001] = "";

void
LongWrite()
{
	int numBytes;
	OpenFile *openFile;

	if (!fileSystem->Create(FileNameLong, 0)) 
	{
		printf("Perf test: can't create %s\n", FileNameLong);
		return;
	}
	openFile = fileSystem->Open(FileNameLong);
	if (openFile == NULL) 
	{
		printf("Perf test: unable to open %s\n", FileNameLong);
		return;
	}

	for(int i=0; i<(1000); ++i)
		strcat(str1, str);

	numBytes = openFile->Write(str1, 10000);
	if (numBytes < 10000) 
	{
		printf("Perf test: unable to write %s\n", FileNameLong);
		delete openFile;
		return;
	}
	delete openFile;
}

void
LongRead()
{
	int numBytes;
	OpenFile *openFile;
	char *buffer = new char[10001];

	if ((openFile = fileSystem->Open(FileNameLong)) == NULL) 
	{
		printf("Perf test: unable to open file %s\n", FileNameLong);
		delete [] buffer;
		return;
	}

	numBytes = openFile->Read(buffer, 10000);
	if ((numBytes < 10000) || strncmp(buffer, str1, 10000)) 
	{
		printf("%d  %d\n", strlen(buffer), strlen(str1));
		//printf("%s\n\n\n%s", buffer, str1);
		printf("Perf test: unable to read %s\n", FileNameLong);
		delete openFile;
		delete [] buffer;
		return;
	}
	delete [] buffer;
	delete openFile;
}

void
PerformanceTest()
{
	stats->Print();
	printf("\n\n\n");

	printf("----- Starting file system performance test of sequential increasing -----\n");
	FileWrite();
	FileRead();
	if (!fileSystem->Remove(FileName)) {
		printf("Perf test: unable to remove %s\n", FileName);
		return;
	}

	printf("\n\n\n");

	printf("----- Starting file system performance test of jumpin increasing -----\n");
	LongWrite();
	LongRead();
	if (!fileSystem->Remove(FileNameLong)) {
		printf("Perf test: unable to remove %s\n", FileNameLong);
		return;
	}

	printf("\n\n\n");
	stats->Print();
}

void
MakeDir(char *dirname)
{
    DEBUG('D', "Making directory.\n");
    fileSystem->Create(dirname, -1);
}


void
read(int num)
{
	for(int i=0; i<3; ++i)
	{
		printf("Thread %d is reading. Cnt: %d\n", num, i+1);
		FileRead();
	}
}


void
ConcurrencyTest()
{
	printf("Multi-reading test starts!\n");

	Thread* t1 = new Thread("reader1");
	t1->Fork(read, (void*)1);
	Thread* t2 = new Thread("reader2");
	t2->Fork(read, (void*)2);
	Thread* t3 = new Thread("reader3");
	t3->Fork(read, (void*)3);

	printf("Write file\n");
	FileWrite();

	currentThread->Yield();

	printf("try to remove file\n");
	while(!fileSystem->Remove(FileName))
	{
		printf("Fail to delete.\n");
		currentThread->Yield();
	}
	printf("Finish deleting.\n");
}

void
write(int num)
{
	printf("Thread %d is reading.\n", num);
	OpenFile *openFile;    
	int i, numBytes;

	printf("Sequential write of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);
	openFile = fileSystem->Open(FileName);
	if (openFile == NULL) 
	{
		printf("Perf test: unable to open %s\n", FileName);
		return;
	}
	for (i = 0; i < FileSize; i += ContentSize) 
	{
		numBytes = openFile->Write(Contents, ContentSize);
		if (numBytes < ContentSize) 
		{
			printf("Perf test: unable to write %s\n", FileName);
			delete openFile;
			return;
		}
	}
	delete openFile;	// close file
}

void
ConcurrencyTest1()
{
	FileWrite();

	printf("--------------- rewrite ---------------\n");
	Thread* t1 = new Thread("reader1");
	t1->Fork(write, (void*)1);
	Thread* t2 = new Thread("reader2");
	t2->Fork(write, (void*)2);

	currentThread->Yield();

	printf("try to remove file\n");
	while(!fileSystem->Remove(FileName))
	{
		printf("Fail to delete.\n");
		currentThread->Yield();
	}
	printf("Finish deleting.\n");

}


#define NAME "testPipe"
#define MAXLEN 512

void 
ReadPipeTest(OpenFile* file)
{
	printf("Read from Pipe, save into file %s\n", NAME);

	char data[MAXLEN+1];
	int length = fileSystem->ReadPipe(data);
	data[length] = '\0';
	printf("Pipe content is: %s, len is %d\n", data, length);

	file->Write(data, length);
}

void
WritePipeTest()
{
	char data[MAXLEN+1];
	scanf("%s", data);

	printf("Write to Pipe, data is %s\n", data);
	fileSystem->WritePipe(data, strlen(data));
}

void
PipeTest()
{
	fileSystem->Create(NAME, MAXLEN);
	OpenFile* file = fileSystem->Open(NAME);
	
	// fileSystem->Print();

	for(int i=0; i<3; ++i)
	{
		WritePipeTest();
		ReadPipeTest(file);
	}

	fileSystem->Print();
}
