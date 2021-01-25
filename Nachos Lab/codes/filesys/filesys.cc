// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"
#include "system.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1
#define PipeSector          2

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{ 
	DEBUG('f', "Initializing the file system.\n");
	if (format) 
	{
		BitMap *freeMap = new BitMap(NumSectors);
		Directory *directory = new Directory(NumDirEntries);
		FileHeader *mapHdr = new FileHeader;
		mapHdr->CreateInit("BHdr");
		FileHeader *dirHdr = new FileHeader;
		dirHdr->CreateInit("DHdr");

		// pipe
		FileHeader* pipHdr = new FileHeader;
		pipHdr->CreateInit("PHdr");

		DEBUG('f', "Formatting the file system.\n");

		// First, allocate space for FileHeaders for the directory and bitmap
		// (make sure no one else grabs these!)
		freeMap->Mark(FreeMapSector);	    
		freeMap->Mark(DirectorySector);

		// pipe
		freeMap->Mark(PipeSector);

		// Second, allocate space for the data blocks containing the contents
		// of the directory and bitmap files.  There better be enough space!

		ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
		ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

		ASSERT(pipHdr->Allocate(freeMap, 0));

		// Flush the bitmap and directory FileHeaders back to disk
		// We need to do this before we can "Open" the file, since open
		// reads the file header off of disk (and currently the disk has garbage
		// on it!).

		DEBUG('f', "Writing headers back to disk.\n");
		mapHdr->WriteBack(FreeMapSector);    
		dirHdr->WriteBack(DirectorySector);

		// pipe
		pipHdr->WriteBack(PipeSector);

		// OK to open the bitmap and directory files now
		// The file system operations assume these two files are left open
		// while Nachos is running.

		freeMapFile = new OpenFile(FreeMapSector);
		directoryFile = new OpenFile(DirectorySector);
	 
		// Once we have the files "open", we can write the initial version
		// of each file back to disk.  The directory at this point is completely
		// empty; but the bitmap has been changed to reflect the fact that
		// sectors on the disk have been allocated for the file headers and
		// to hold the file data for the directory and bitmap.

		DEBUG('f', "Writing bitmap and directory back to disk.\n");
		freeMap->WriteBack(freeMapFile);	 // flush changes to disk
		directory->WriteBack(directoryFile);

		if (DebugIsEnabled('f')) 
		{
			freeMap->Print();
			directory->Print();

			delete freeMap; 
			delete directory; 
			delete mapHdr; 
			delete dirHdr;
		}
	} 
	else 
	{
		// if we are not formatting the disk, just open the files representing
		// the bitmap and directory; these are left open while Nachos is running
		freeMapFile = new OpenFile(FreeMapSector);
		directoryFile = new OpenFile(DirectorySector);
	}

	for(int i=0; i<MAX_FILE_NUMBER; ++i)
	{
		OpenedFiles[i] = new FileRecord;
		OpenedFiles[i]->file = NULL;
		OpenedFiles[i]->TID = -1;
		OpenedFiles[i]->ForkFileID = -1;
		// 0 and 1 is used for console
		OpenedFiles[i]->valid = (i>=2 ? FALSE : TRUE);
	}
	tableLock = new Lock("file system table lock");
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool
FileSystem::Create(char *name, int initialSize)
{
	Directory *directory;
	BitMap *freeMap;
	FileHeader *hdr;
	int sector;
	bool success;

#ifndef MULTI_LEVEL_DIR
	DEBUG('f', "Creating file %s, size %d\n", name, initialSize);
#else
	bool isDir = FALSE;
	if(initialSize==-1) // this means that we will create directory file
	{
		isDir = TRUE;
		initialSize = DirectoryFileSize;
		DEBUG('D', "===> Creating directory %s, size is %d.\n", name, initialSize);
	}
	else
		DEBUG('f', "Creating file %s, size %d\n", name, initialSize);
#endif

	directory = new Directory(NumDirEntries);

#ifndef MULTI_LEVEL_DIR
	directory->FetchFrom(directoryFile);
#else
	int DirSector = FindDirSector(name);
//	printf("---> dir sector %d\n", DirSector);
	ASSERT(DirSector!=-1); // if we want to create a file, the directory must exist
	OpenFile* DirFile = new OpenFile(DirSector);
	directory->FetchFrom(DirFile);
	FilePath filepath = PathParser(name);
	if(filepath.DirDep > 0)
	{
		name = filepath.Base; // delete directory
	}
#endif


	if (directory->Find(name) != -1)
		success = FALSE;			// file is already in directory
	else 
	{	
		freeMap = new BitMap(NumSectors);
		freeMap->FetchFrom(freeMapFile);
		sector = freeMap->Find();	// find a sector to hold the file header
		if (sector == -1) 		
			success = FALSE;		// no free block for file header 
		else if (!directory->Add(name, sector))
			success = FALSE;	// no space in directory
		else 
		{
			hdr = new FileHeader;
			if (!hdr->Allocate(freeMap, initialSize))
			success = FALSE;	// no space on disk for data
			else 
			{	
				success = TRUE;
				// everthing worked, flush all changes back to disk

#ifdef MULTI_LEVEL_DIR
				if(isDir)
					hdr->CreateInit(Dir_Ext);
				else
					hdr->CreateInit(GetFileExtension(name));
				hdr->WriteBack(sector);
				//hdr->Print();

				if(isDir) // initialize directory
				{
					Directory* dir = new Directory(NumDirEntries);
					OpenFile* subDir = new OpenFile(sector);
					dir->WriteBack(subDir);
					//dir->Print();
					delete dir;
					delete subDir;
				}

	 			directory->WriteBack(DirFile);
				freeMap->WriteBack(freeMapFile);
				delete DirFile;

#else
				hdr->CreateInit(GetFileExtension(name));
				hdr->WriteBack(sector); 
	 			directory->WriteBack(directoryFile);
				freeMap->WriteBack(freeMapFile);

				char* curTime = GetTime();
				FileHeader *mapHdr = new FileHeader;
				FileHeader *dirHdr = new FileHeader;
				mapHdr->FetchFrom(FreeMapSector);
				dirHdr->FetchFrom(DirectorySector);
				mapHdr->SetLastVisitTime(curTime);
				mapHdr->SetModifyTime(curTime);
				dirHdr->SetLastVisitTime(curTime);
				dirHdr->SetModifyTime(curTime);
				mapHdr->WriteBack(FreeMapSector);
				dirHdr->WriteBack(DirectorySector);
				delete mapHdr;
				delete dirHdr;
#endif
		}
		delete hdr;
	}
	delete freeMap;
	}
	delete directory;
	return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{ 
	Directory *directory;
	OpenFile *openFile = NULL;
	int sector;

	DEBUG('f', "Opening file %s\n", name);

#ifndef MULTI_LEVEL_DIR
	directory = = new Directory(NumDirEntries);
	directory->FetchFrom(directoryFile);
#else
	directory = (Directory*)FindDir(name);
	FilePath filepath = PathParser(name);
	if(filepath.DirDep > 0)
	{
		name = filepath.Base;
	}
#endif
	sector = directory->Find(name); 
	if (sector >= 0) 		
		openFile = new OpenFile(sector);	// name was found in directory 
	delete directory;
	return openFile;				// return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool
FileSystem::Remove(char *name)
{ 
	Directory *directory;
	BitMap *freeMap;
	FileHeader *fileHdr;
	int sector;
	
#ifndef MULTI_LEVEL_DIR
	directory = new Directory(NumDirEntries);
	directory->FetchFrom(directoryFile);
#else
	directory = (Directory*)FindDir(name);
	FilePath filepath = PathParser(name);
	if(filepath.DirDep>0)
	{
		DEBUG('D', "===> '-r' can only be used to delete file/dir in root directory!\n");
		delete directory;
		return FALSE;
	}
#endif


	sector = directory->Find(name);
	if (sector == -1) {
	   delete directory;
	   return FALSE;			 // file not found 
	}
	fileHdr = new FileHeader;
	fileHdr->FetchFrom(sector);

#ifdef MULTI_LEVEL_DIR
	if(!strcmp(fileHdr->GetFileExtension(), Dir_Ext))
	{
		DEBUG('D', "===> You are trying to delete a directory in root directory!\n");
		OpenFile* SubDirFile = new OpenFile(sector);
		Directory* SubDirectory = new Directory(NumDirEntries);
		SubDirectory->FetchFrom(SubDirFile);
//		SubDirectory->Print();
		
		if(!SubDirectory->IsEmpty())
		{
			DEBUG('D', "===> This directory isn't empty. Operation failed!\n");
			delete fileHdr;
    		delete directory;
			delete SubDirFile;
			delete SubDirectory;
			return FALSE;
		}
	}
#endif

	synchDisk->numberLock->Acquire();
	if(synchDisk->visitorCnt[sector]!=0)
	{
		DEBUG('C', "===> This file is still being used by %d visitors!\n", synchDisk->visitorCnt[sector]);
		synchDisk->numberLock->Release();
		delete fileHdr;
		return FALSE;
	}
	synchDisk->numberLock->Release();

	freeMap = new BitMap(NumSectors);
	freeMap->FetchFrom(freeMapFile);

	fileHdr->Deallocate(freeMap);  		// remove data blocks
	freeMap->Clear(sector);			// remove header block
	directory->Remove(name);

	freeMap->WriteBack(freeMapFile);		// flush to disk
	directory->WriteBack(directoryFile);        // flush to disk

#ifndef MULTI_LEVEL_DIR
	char* curTime = GetTime();
	FileHeader *mapHdr = new FileHeader;
	FileHeader *dirHdr = new FileHeader;
	mapHdr->FetchFrom(FreeMapSector);
	dirHdr->FetchFrom(DirectorySector);
	mapHdr->SetLastVisitTime(curTime);
	mapHdr->SetModifyTime(curTime);
	dirHdr->SetLastVisitTime(curTime);
	dirHdr->SetModifyTime(curTime);
	mapHdr->WriteBack(FreeMapSector);
	dirHdr->WriteBack(DirectorySector);
	delete mapHdr;
	delete dirHdr;
#endif

	delete fileHdr;
	delete directory;
	delete freeMap;
	return TRUE;
} 

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List()
{
	Directory *directory = new Directory(NumDirEntries);

	directory->FetchFrom(directoryFile);
	directory->List();
	delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
	FileHeader *bitHdr = new FileHeader;
	FileHeader *dirHdr = new FileHeader;
	BitMap *freeMap = new BitMap(NumSectors);
	Directory *directory = new Directory(NumDirEntries);

	printf("Bit map file header:\n");
	bitHdr->FetchFrom(FreeMapSector);
	bitHdr->Print();

	printf("Directory file header:\n");
	dirHdr->FetchFrom(DirectorySector);
	dirHdr->Print();

	directory->FetchFrom(directoryFile);
	directory->Print();

	freeMap->FetchFrom(freeMapFile);
	freeMap->Print();

	delete bitHdr;
	delete dirHdr;
	delete freeMap;
	delete directory;
} 

/**********************************************
 *       used for multi-level directory       *
 * ********************************************/
#ifdef MULTI_LEVEL_DIR

int
FileSystem::FindDirSector(char* path)
{
	FilePath filepath = PathParser(path);

//	printf("---> depth is %d\n", filepath.DirDep);
//	for(int i=0; i<filepath.DirDep; ++i)
//		printf("---> %s\n", filepath.DirArray[i]);

	int sector = DirectorySector;
	if(filepath.DirDep != 0) // not root dirFindDirSector
	{
		OpenFile* dirFile;
		Directory* tmpDir;
		for(int i=0; i<filepath.DirDep; ++i)
		{
			DEBUG('D', "===> Finding directory %s in sector %d.\n", filepath.DirArray[i], sector);
			dirFile = new OpenFile(sector);
			tmpDir = new Directory(NumDirEntries);
			tmpDir->FetchFrom(dirFile);
			//tmpDir->Print();
			sector = tmpDir->Find(filepath.DirArray[i]);
			if(sector==-1)
			{
				DEBUG('D', "===> Fail to find directory %s in sector %d.\n", filepath.DirArray[i], sector);
				break;
			}
			delete dirFile;
			delete tmpDir;
		}
	}
	return sector;
}


void*
FileSystem::FindDir(char* path)
{
	Directory* returnDir = new Directory(NumDirEntries);
	int sector = FindDirSector(path);

	if(sector == DirectorySector) // in root
	{
		returnDir->FetchFrom(directoryFile);
	}
	else if(sector != -1)
	{
		OpenFile* dirFile = new OpenFile(sector);
		returnDir->FetchFrom(dirFile);
		delete dirFile;
	}
	else
	{
		DEBUG('D', "===> No such directory. Maybe it has been deleted.\n");
	}

	// Directly return Directory* will raise error, I don't know why.
	return (void*)returnDir; 
}

bool
FileSystem::RemoveDir(char* name)
{
	int DirSector;
	OpenFile* DirFile;
    Directory* DirDirectory = new Directory(NumDirEntries);

    BitMap *FreeMap;

    FileHeader *FileHdr;
    int FileSector;

	// find directory first
	DirSector = FindDirSector(name);
//	printf("---> dirsector %d\n", DirSector);
	if(DirSector == -1) // not exists
	{
		DEBUG('D', "===> Dir of %s doesn't exists!\n", name);
    	return FALSE; 
	}
	DirFile = new OpenFile(DirSector);
    DirDirectory->FetchFrom(DirFile); // read directory content
//	DirDirectory->Print();

	// find the sector of object file
    FilePath filepath = PathParser(name);
    if (filepath.DirDep > 0) 
	{
        name = filepath.Base;
    }
    FileSector = DirDirectory->Find(name);
//	printf("---> %s\n", name);
//	printf("---> sector %d\n", FileSector);
    if (FileSector == -1)  // this file/directory doesn't exist
	{
       delete DirDirectory;
	   DEBUG('D', "===> Object File/Dir %s doesn't exists!\n", name);
       return FALSE;             
    }

	// read file header
    FileHdr = new FileHeader;
    FileHdr->FetchFrom(FileSector);

	// we must ensure that there are no file in subdirectory
	if(!strcmp(FileHdr->GetFileExtension(), Dir_Ext)) 
	{
		DEBUG('D', "===> You are Trying to delete a directory!\n");
		OpenFile* SubDirFile = new OpenFile(FileSector);
		Directory* SubDirectory = new Directory(NumDirEntries);
		SubDirectory->FetchFrom(SubDirFile);
//		SubDirectory->Print();
		
		if(!SubDirectory->IsEmpty())
		{
			DEBUG('D', "===> This directory isn't empty. Operation failed!\n");
			delete SubDirFile;
			delete SubDirectory;
			delete FileHdr;
    		delete DirDirectory;
			delete DirFile;
			return FALSE;
		}	
	}

	// free the file and its sector
    FreeMap = new BitMap(NumSectors);
    FreeMap->FetchFrom(freeMapFile);
    FileHdr->Deallocate(FreeMap);       // delete data
    FreeMap->Clear(FileSector);         // delete header
    DirDirectory->Remove(name);

	// flush change to disk
    FreeMap->WriteBack(freeMapFile); 
	// *** note that we need to flush to correct directory       
    DirDirectory->WriteBack(DirFile);   

	DEBUG('D', "===> Successfully remove a multi-level file!\n");        
    delete FileHdr;
    delete DirDirectory;
	delete DirFile;
    delete FreeMap;
    return TRUE;
}

void
FileSystem::ListDir(char* directory)
{
	printf("Now List Directory: %s.\n", directory);
	Directory* dir = (Directory*)FindDir(strcat(directory, "/anything"));
	dir->List();
	delete dir;
}

#endif

void
FileSystem::WritePipe(char* data, int length)
{
	data[length] = '\0';

	OpenFile* pipe = new OpenFile(PipeSector);
	pipe->Write(data, length+1);

	DEBUG('P', "Write into pipe file\n");
	delete pipe;
}

int
FileSystem::ReadPipe(char* dst)
{
	FileHeader* pipHdr = new FileHeader;
	pipHdr->FetchFrom(PipeSector);

	OpenFile* pipe = new OpenFile(PipeSector);
	pipe->Read(dst, pipHdr->FileLength());
	dst[pipHdr->FileLength()] = '\0';

	// printf("len is %d\n", pipHdr->FileLength());
	DEBUG('P', "Read from pipe file\n");
	delete pipHdr;
	delete pipe;
	return strlen(dst);
}






int 
FileSystem::AddNewFile(OpenFile* file)
{
	int pos = 0;
	for(pos=2; pos<MAX_FILE_NUMBER; ++pos)
	{
		if(!OpenedFiles[pos]->valid)
		{
			OpenedFiles[pos]->valid = TRUE;
			OpenedFiles[pos]->file = file;
			OpenedFiles[pos]->TID = currentThread->getTID();
			return pos;
		}
	}
	return -1;
}

bool 
FileSystem::RemoveFile(int openFileID)
{
	if(openFileID < 2)
	{
		DEBUG('S', "You cannot remove console input & output!\n");
		return FALSE;
	}

	if(OpenedFiles[openFileID]->valid && OpenedFiles[openFileID]->TID == currentThread->getTID())
	{
		OpenedFiles[openFileID]->valid = FALSE;
		delete OpenedFiles[openFileID]->file;
		OpenedFiles[openFileID]->file = NULL;
		OpenedFiles[openFileID]->TID = -1;
		OpenedFiles[openFileID]->ForkFileID = -1;
		return TRUE;
	}

	for(int i=2; i<MAX_FILE_NUMBER; ++i)
	{
		if(OpenedFiles[i]->valid && OpenedFiles[i]->TID == currentThread->getTID() && OpenedFiles[i]->ForkFileID == openFileID)
		{
			OpenedFiles[i]->valid = FALSE;
			delete OpenedFiles[i]->file;
			OpenedFiles[i]->file = NULL;
			OpenedFiles[i]->TID = -1;
			OpenedFiles[i]->ForkFileID = -1;
			return TRUE;
		}
	}

	return FALSE;
}

OpenFile* 
FileSystem::GetFile(int openFileID)
{
	if(openFileID<2)
	{
		DEBUG('S', "console input and output should not be get.\n");
		return NULL;
	}

	if(OpenedFiles[openFileID]->valid && OpenedFiles[openFileID]->TID == currentThread->getTID())
	{
		return OpenedFiles[openFileID]->file;
	}

	for(int i=2; i<MAX_FILE_NUMBER; ++i)
	{
		if((OpenedFiles[i]->valid) && (OpenedFiles[i]->TID = currentThread->getTID()) && (OpenedFiles[i]->ForkFileID == openFileID))
			return OpenedFiles[i]->file;
	}

	DEBUG('S', "This file doesn't belong to you or invalid ID.\n");
	return NULL;
}

void
FileSystem::PrintTable()
{
	printf("\n\n------open file table------\n\n\n");
	printf("current TID is %d \n", currentThread->getTID());
	for(int i=2; i<MAX_FILE_NUMBER; ++i)
	{
		if(OpenedFiles[i]->valid)
		{
			printf("file ID: %d, owner's TID is %d, fork id is %d\n", i, OpenedFiles[i]->TID, OpenedFiles[i]->ForkFileID);
		}
	}
	printf("\n\n---------------------------\n\n\n");
}

void
FileSystem::CloseAll()
{
	DEBUG('S', "close opened files before user program exit\n");
	for(int i=2; i<MAX_FILE_NUMBER; ++i)
	{
		if(OpenedFiles[i]->valid && OpenedFiles[i]->TID == currentThread->getTID())
		{
			OpenedFiles[i]->valid = FALSE;
			delete OpenedFiles[i]->file;
			OpenedFiles[i]->file = NULL;
			OpenedFiles[i]->ForkFileID = -1;
			OpenedFiles[i]->TID = -1;
		}
	}
}

bool
FileSystem::CopyFile(int newTID)
{
	DEBUG('S', "copy opened files for Fork syscall\n");
	// check whether table can hold all files
	int leftNum = 0;
	int openNum = 0;
	for(int i=2; i<MAX_FILE_NUMBER; ++i)
	{
		if(!OpenedFiles[i]->valid)
			leftNum++;
		else
		{
			if(OpenedFiles[i]->TID == currentThread->getTID())
				openNum++;
		}	
	}
	if(leftNum<openNum)
		return FALSE;
	// copy files
	for(int i=2; i<MAX_FILE_NUMBER; ++i)
	{
		if(OpenedFiles[i]->valid && OpenedFiles[i]->TID == currentThread->getTID())
		{
			// do deep copy
			OpenFile* file = new OpenFile(OpenedFiles[i]->file->GetHeaderSector());
			file->Seek(OpenedFiles[i]->file->GetPos());
			// add to table
			int fd = AddNewFile(file);
			ASSERT(fd != -1);
			OpenedFiles[fd]->ForkFileID = i;
			OpenedFiles[fd]->TID = newTID;
		}
	}

	PrintTable();
	return TRUE;
}
