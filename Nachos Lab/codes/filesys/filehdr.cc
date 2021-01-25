// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
extern "C" {
#include <time.h>
#include <libgen.h>
}

char*
GetFileExtension(char* filename)
{
	char* last = strrchr(filename, '.');
	if(last == NULL || last == filename)
		return "";
	return last+1;
}


char*
GetTime()
{
	time_t RawTime = time(NULL);
	struct tm* CurrentTime = localtime(&RawTime);

	char* t =  asctime(CurrentTime);
	char* result = new char[strlen(t)];

	strncpy(result, t, strlen(t)-1);
	result[strlen(t)-1] = '\0';

	return result; 
}

char*
printChar(char oriChar)
{
    if ('\040' <= oriChar && oriChar <= '\176') // isprint(oriChar)
        printf("%c", oriChar); // Character content
    else
        printf("\\%x", (unsigned char)oriChar); // Unreadable binary content
}


FilePath
PathParser(char* path)
{
	FilePath filepath;

	if(path[0] == '/')
		path = &path[1]; // skip root dir
	char* ts1 = strdup(path);
	char* ts2 = strdup(path);

	// note that this two functions will directly change the string
	char* curDir = dirname(ts1);
	filepath.Base = strdup(basename(ts2));

	int depth = 0;
	for(depth=0; path[depth]; path[depth]=='/' ? depth++: *path++); // calculate depth
	filepath.DirDep = depth;
	ASSERT(depth <= MAX_DIR_DEP);

	while(strcmp(curDir, "."))
	{
		filepath.DirArray[--depth] = strdup(basename(curDir));
		curDir = dirname(curDir);
	}

	return filepath;
}

//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
	numBytes = fileSize;
	numSectors  = divRoundUp(fileSize, SectorSize);
	int numClear = freeMap->NumClear();
	if (numClear < numSectors)
		return FALSE;		// not enough space

#ifndef USE_INDIRECT
	DEBUG('f', "Now use direct mapping.\n");
	for (int i = 0; i < numSectors; i++)
		dataSectors[i] = freeMap->Find();
#else

	DEBUG('f', "Now use indirect mapping.\n");
	if(numSectors <= NumDirect)
	{
		DEBUG('f', "Just need to use direct mapping.\n");
		for (int i = 0; i < numSectors; i++)
			dataSectors[i] = freeMap->Find();
	}
	else
	{
		//printf("233\n");
		if(numSectors <= LevelNum + NumDirect)
		{
			DEBUG('f', "Need to use one-level indirect mapping.\n");
			if(numClear < numSectors + 1) // cannot hold single index and data
				return FALSE;

			// direct
			for(int i=0; i<NumDirect; ++i)
			{
				dataSectors[i] = freeMap->Find();
			}

			// one-level
			dataSectors[OneLevelIdx] = freeMap->Find();
			int OneLevelIndexes[LevelNum];
			for(int i=0; i<numSectors - NumDirect; ++i)
			{
				OneLevelIndexes[i] = freeMap->Find();
			}
			synchDisk->WriteSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);
		}
		else if(numSectors <= LevelNum*LevelNum + LevelNum + NumDirect)
		{
			DEBUG('f', "Need to use two-level indirect mapping.\n");

			int LeftSectors = numSectors - NumDirect - LevelNum;
			int SecondLevelNum = divRoundUp(LeftSectors, LevelNum);
			if(numClear < numSectors + 1 + 1 + SecondLevelNum)  // cannot hold double index and data
				return FALSE;

			// direct
			for(int i=0; i<NumDirect; ++i)
			{
				dataSectors[i] = freeMap->Find();
			}

			// one-level
			dataSectors[OneLevelIdx] = freeMap->Find();
			int OneLevelIndexes[LevelNum];
			for(int i=0; i<LevelNum; ++i)
			{
				OneLevelIndexes[i] = freeMap->Find();
			}
			synchDisk->WriteSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);

			// two-level
			dataSectors[TwoLevelIdx] = freeMap->Find();
			int TwoLevelIndexes[LevelNum];
			for(int i=0; i<SecondLevelNum; ++i)
			{
				TwoLevelIndexes[i] = freeMap->Find();
				int SingleLevelIndexes[LevelNum];
				for(int j=0; j<LevelNum && (j+i*LevelNum)<LeftSectors; ++j)
				{
					SingleLevelIndexes[j] = freeMap->Find();
				}
				synchDisk->WriteSector(TwoLevelIndexes[i], (char*)SingleLevelIndexes);
			}
			synchDisk->WriteSector(dataSectors[TwoLevelIdx], (char*)TwoLevelIndexes);
		}
		else
		{
			printf("Allocation error occurs!\n");
			ASSERT(FALSE);
		}
	}

#endif
	return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
#ifndef USE_INDIRECT
	for (int i = 0; i < numSectors; i++) {
		ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
		freeMap->Clear((int) dataSectors[i]);
	}
#else

	int direct_idx, single_idx, double_idx;	
	DEBUG("f", "Deallocating index table.\n");

	// direct mapping
	for(direct_idx=0; direct_idx<numSectors && direct_idx<NumDirect; ++direct_idx)
	{
		ASSERT(freeMap->Test((int) dataSectors[direct_idx]));  // ought to be marked!
		freeMap->Clear((int) dataSectors[direct_idx]);		
	}
	
	// one level
	if(numSectors > NumDirect)
	{
		DEBUG("f", "Deallocating one-level index table.\n");
		int OneLevelIndexes[LevelNum];
		synchDisk->ReadSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);
		for(direct_idx=NumDirect, single_idx=0; direct_idx<numSectors && single_idx<LevelNum; ++direct_idx, ++single_idx)
		{
			ASSERT(freeMap->Test((int) OneLevelIndexes[single_idx]));  // ought to be marked!
			freeMap->Clear((int) OneLevelIndexes[single_idx]);
		}
		ASSERT(freeMap->Test((int) dataSectors[OneLevelIdx]));  // ought to be marked!
		freeMap->Clear((int) dataSectors[OneLevelIdx]);

		if(numSectors > NumDirect + LevelNum)
		{
			DEBUG("f", "Deallocating two-level index table.\n");
			int TwoLevelIndexes[LevelNum];
			synchDisk->ReadSector(dataSectors[TwoLevelIdx], (char*)TwoLevelIndexes);
			for(direct_idx = NumDirect+LevelNum, single_idx=0; direct_idx<numSectors && single_idx<LevelNum; ++single_idx)
			{
				synchDisk->ReadSector(TwoLevelIndexes[single_idx], (char*)OneLevelIndexes);
				for(double_idx=0; direct_idx<numSectors && double_idx<LevelNum; ++direct_idx, ++double_idx)
				{
					ASSERT(freeMap->Test((int)OneLevelIndexes[double_idx]));
					freeMap->Clear((int)OneLevelIndexes[double_idx]);
				}

				ASSERT(freeMap->Test((int)TwoLevelIndexes[single_idx]));
				freeMap->Clear((int)TwoLevelIndexes[single_idx]);
			}
			
			ASSERT(freeMap->Test((int)dataSectors[TwoLevelIdx]));
			freeMap->Clear((int)dataSectors[TwoLevelIdx]);

		}

	}
#endif
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
	synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
	synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
#ifndef USE_INDIRECT
	return(dataSectors[offset / SectorSize]);
#else

	int DirectSize = NumDirect*SectorSize;
	int SingleSize = DirectSize + LevelNum*SectorSize;
	int DoubleSize = SingleSize + LevelNum*LevelNum*SectorSize;

	if(offset < DirectSize)
		return(dataSectors[offset/SectorSize]);
	
	if(offset < SingleSize)
	{
		int idx = (offset - DirectSize) / SectorSize;
		int OneLevelIndexes[LevelNum];
		synchDisk->ReadSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);
		//printf("idx %d, data %d\n", idx, OneLevelIndexes[idx]);
		return OneLevelIndexes[idx];
	}

	int TwoLevelNum = (offset - SingleSize) / (LevelNum*SectorSize);
	int TwoLevelOneNum = ((offset - SingleSize) % (LevelNum*SectorSize)) / SectorSize;
	int TwoLevelIndexes[LevelNum];
	synchDisk->ReadSector(dataSectors[TwoLevelIdx], (char*)TwoLevelIndexes);
	int OneLevelIndexes[LevelNum];
	synchDisk->ReadSector(TwoLevelIndexes[TwoLevelNum], (char*) OneLevelIndexes);
	return OneLevelIndexes[TwoLevelOneNum];


#endif
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
	return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
	int i, j, k;
	char *data = new char[SectorSize];

	printf("------------------------ %s ----------------------------\n", "FileHeader contents");
	printf("File Extension: %s\n", fileExtension);
	printf("Create Time: %s\n", createTime);
	printf("Last Visit Time: %s\n", lastVisitTime);
	printf("Last Modify Time: %s\n", modifyTime);

#ifndef USE_INDIRECT 
	printf("File size: %d.  File blocks:\n", numBytes);
	for (i = 0; i < numSectors; i++)
		printf("%d ", dataSectors[i]);
	printf("\nFile contents:\n");
	for (i = k = 0; i < numSectors; i++) {
		synchDisk->ReadSector(dataSectors[i], data);
		for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
			printChar(data[j]);
		}
		printf("\n"); 
	}
#else
	printf("File size: %d.  File blocks:\n", numBytes);
    int ii, iii; // For direct / single / double indirect indexing
    int OneLevelIndexes[LevelNum]; // used to restore the indexing map
    int TwoLevelIndexes[LevelNum]; // used to restore the indexing map
    printf("  Direct indexing:\n    ");
    for (i = 0; (i < numSectors) && (i < NumDirect); ++i)
        printf("%d ", dataSectors[i]);
    if (numSectors > NumDirect) {
        printf("\n  Indirect indexing: (mapping table sector: %d)\n    ", dataSectors[OneLevelIdx]);
        synchDisk->ReadSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);
        for (i = NumDirect, ii = 0; (i < numSectors) && (ii < LevelNum); ++i, ++ii)
            printf("%d ", OneLevelIndexes[ii]);
        if (numSectors > NumDirect + LevelNum) {
            printf("\n  Double indirect indexing: (mapping table sector: %d)", dataSectors[TwoLevelIdx]);
            synchDisk->ReadSector(dataSectors[TwoLevelIdx], (char*)TwoLevelIndexes);
            for (i = NumDirect + LevelNum, ii = 0; (i < numSectors) && (ii < LevelNum); ++ii) {
                printf("\n    single indirect indexing: (mapping table sector: %d)\n      ", TwoLevelIndexes[ii]);
                synchDisk->ReadSector(TwoLevelIndexes[ii], (char*)OneLevelIndexes);
                for (iii = 0;  (i < numSectors) && (iii < LevelNum); ++i, ++iii)
                    printf("%d ", OneLevelIndexes[iii]);
            }
        }
    }
    printf("\nFile contents:\n");
    for (i = k = 0; (i < numSectors) && (i < NumDirect); ++i)
    {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); ++j, ++k)
            printChar(data[j]);
        printf("\n");
    }
    if (numSectors > NumDirect) {
        synchDisk->ReadSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);
        for (i = NumDirect, ii = 0; (i < numSectors) && (ii < LevelNum); ++i, ++ii) {
            synchDisk->ReadSector(OneLevelIndexes[ii], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); ++j, k++)
                printChar(data[j]);
            printf("\n");
        }
        if (numSectors > NumDirect + LevelNum) {
            synchDisk->ReadSector(dataSectors[TwoLevelIdx], (char*)TwoLevelIndexes);
            for (i = NumDirect + LevelNum, ii = 0; (i < numSectors) && (ii < LevelNum); ++ii) {
                synchDisk->ReadSector(TwoLevelIndexes[ii], (char*)OneLevelIndexes);
                for (iii = 0; (i < numSectors) && (iii < LevelNum); ++i, ++iii) {
                    synchDisk->ReadSector(OneLevelIndexes[iii], data);
                    for (j = 0; (j < SectorSize) && (k < numBytes); ++j, ++k)
                        printChar(data[j]);
                    printf("\n");
                }
            }
        }
    }

#endif
	printf("---------------------------------------------------------------------\n\n\n");
	delete [] data;
}

void
FileHeader::CreateInit(char* ext)
{
	SetFileExtension(ext);

	char* curTime = GetTime();
	SetCreateTime(curTime);
	SetModifyTime(curTime);
	SetLastVisitTime(curTime);
}

// when we want to write and there is no enpugh space, call this function first
bool
FileHeader::ExpandFileSize(BitMap* freeMap, int ExpandBytes)
{
	// ensure extension
	if(ExpandBytes < 0)
	{
		DEBUG('f', "===> Expand size should be a non-negative number!\n");
		return FALSE;
	}

	int afterBytes = numBytes+ExpandBytes;
	int afterSectors = divRoundUp(afterBytes, SectorSize);
	int clearSectors = freeMap->NumClear();

	// don't need to get new sector
	if(afterSectors == numSectors)
	{
		DEBUG('f', "===> Previous file size is large enough!\n");
		numBytes = afterBytes;
		return TRUE;
	}
	
	// too large to hold the whole file
	int deltaSectors = afterSectors-numSectors;
	if(deltaSectors > clearSectors)
	{
		DEBUG('f', "===> Empty size is not large enough to hold the file!\n");
		return FALSE;
	}

	DEBUG('f', "===> Start to expand file, expand size is %d, need %d sectors.\n", ExpandBytes, deltaSectors);
	if(afterSectors <= NumDirect) // just need direct index
	{
		for(int i=numSectors; i<afterSectors; ++i)
		{
			dataSectors[i] = freeMap->Find();
		}
	}
	else
	{
#ifndef USE_INDIRECT
		DEBUG('E', "===> There is no enough space in Sector Table.\n");
		return FALSE;
#else
		if(numSectors <= NumDirect && afterSectors <= NumDirect + LevelNum)
		{
			DEBUG('E', "===> Need to use one-level indirect index **from 0 to 1**.\n");
			if(deltaSectors+1 > clearSectors)
			{
				DEBUG('f', "===> There is no enough space for additional file space and space table.\n");
				return FALSE;
			}

			// fill direct index
			for(int i=numSectors; i<NumDirect; ++i)
			{
				dataSectors[i] = freeMap->Find();
			}

			// one-level
			dataSectors[OneLevelIdx] = freeMap->Find();
			int OneLevelIndexes[LevelNum];
			for(int i=0; i<afterSectors - NumDirect; ++i)
			{
				OneLevelIndexes[i] = freeMap->Find();
			}
			synchDisk->WriteSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);
		}
		else if(afterSectors <= NumDirect + LevelNum)
		{
			DEBUG('f', "===> Need to use one-level indirect index **from 1 to 1**.\n");

			int OneLevelIndexes[LevelNum];
			synchDisk->ReadSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);
			for(int i=numSectors - NumDirect; i<afterSectors - NumDirect; ++i)
			{
				OneLevelIndexes[i] = freeMap->Find();
			}
			synchDisk->WriteSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);
		}
		else if(numSectors <= NumDirect && afterSectors <= NumDirect + LevelNum + LevelNum*LevelNum)
		{
			DEBUG('E', "===> Need to use two-level indirect index **from 0 to 2**.\n");
			int LeftSectors = afterSectors - NumDirect - LevelNum;
			int SecondLevelNum = divRoundUp(LeftSectors, LevelNum);
			if(clearSectors < numSectors + 1 + 1 + SecondLevelNum)  // cannot hold double index and data
			{
				DEBUG('f', "===> There is no enough space for additional file space and space table.\n");
				return FALSE;
			}

			// direct
			for(int i=numSectors; i<NumDirect; ++i)
			{
				dataSectors[i] = freeMap->Find();
			}

			// one-level
			dataSectors[OneLevelIdx] = freeMap->Find();
			int OneLevelIndexes[LevelNum];
			for(int i=0; i<LevelNum; ++i)
			{
				OneLevelIndexes[i] = freeMap->Find();
			}
			synchDisk->WriteSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);

			// two-level
			dataSectors[TwoLevelIdx] = freeMap->Find();
			int TwoLevelIndexes[LevelNum];
			for(int i=0; i<SecondLevelNum; ++i)
			{
				TwoLevelIndexes[i] = freeMap->Find();
				int SingleLevelIndexes[LevelNum];
				for(int j=0; j<LevelNum && (j+i*LevelNum)<LeftSectors; ++j)
				{
					SingleLevelIndexes[j] = freeMap->Find();
				}
				synchDisk->WriteSector(TwoLevelIndexes[i], (char*)SingleLevelIndexes);
			}
			synchDisk->WriteSector(dataSectors[TwoLevelIdx], (char*)TwoLevelIndexes);
		}
		else if(numSectors <= NumDirect + LevelNum && afterSectors <= NumDirect + LevelNum + LevelNum*LevelNum)
		{
			DEBUG('E', "===> Need to use two-level indirect index **from 1 to 2**.\n");

			int LeftSectors = afterSectors - NumDirect - LevelNum;
			int SecondLevelNum = divRoundUp(LeftSectors, LevelNum);
			if(clearSectors < numSectors + 1 + SecondLevelNum)  // cannot hold double index and data
			{
				DEBUG('f', "===> There is no enough space for additional file space and space table.\n");
				return FALSE;
			}

			// fill one level
			int OneLevelIndexes[LevelNum];
			synchDisk->ReadSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);
			for(int i=numSectors - NumDirect; i<LevelNum; ++i)
			{
				OneLevelIndexes[i] = freeMap->Find();
			}
			synchDisk->WriteSector(dataSectors[OneLevelIdx], (char*)OneLevelIndexes);

			// two-level
			dataSectors[TwoLevelIdx] = freeMap->Find();
			int TwoLevelIndexes[LevelNum];
			for(int i=0; i<SecondLevelNum; ++i)
			{
				TwoLevelIndexes[i] = freeMap->Find();
				int SingleLevelIndexes[LevelNum];
				for(int j=0; j<LevelNum && (j+i*LevelNum)<LeftSectors; ++j)
				{
					SingleLevelIndexes[j] = freeMap->Find();
				}
				synchDisk->WriteSector(TwoLevelIndexes[i], (char*)SingleLevelIndexes);
			}
			synchDisk->WriteSector(dataSectors[TwoLevelIdx], (char*)TwoLevelIndexes);
		}
		else if(afterSectors <= NumDirect + LevelNum + LevelNum*LevelNum)
		{
			DEBUG('f', "===> Need to use two-level indirect index **from 2 to 2**.\n");

			int LeftSectors = afterSectors - NumDirect - LevelNum;
			int SecondLevelNum = divRoundUp(LeftSectors, LevelNum);

			int prevLeftSectors = numSectors - NumDirect - LevelNum;
			int prevSecondLevelNum = divRoundUp(prevLeftSectors, LevelNum);

			int TwoLevelIndexes[LevelNum];
			synchDisk->ReadSector(dataSectors[TwoLevelIdx], (char*)TwoLevelIndexes);

			if(SecondLevelNum == prevSecondLevelNum)
			{
				int SingleLevelIndexes[LevelNum];
				synchDisk->ReadSector(TwoLevelIndexes[prevSecondLevelNum-1], (char*)SingleLevelIndexes);
				//printf("---%d %d %d %d---\n", SecondLevelNum, LeftSectors, prevSecondLevelNum , prevLeftSectors);
				for(int j=(prevLeftSectors%LevelNum); (j+(SecondLevelNum-1)*LevelNum)<LeftSectors; ++j)
				{
					SingleLevelIndexes[j] = freeMap->Find();
				}
				//printf("--- %d %d ---\n", SingleLevelIndexes[(prevLeftSectors%LevelNum)-1], SingleLevelIndexes[prevLeftSectors%LevelNum]);
				synchDisk->WriteSector(TwoLevelIndexes[prevSecondLevelNum-1], (char*)SingleLevelIndexes);
			}
			else
			{
				if(deltaSectors+1 > clearSectors)
				{
					DEBUG('f', "===> There is no enough space for additional file space and space table.\n");
					return FALSE;
				}
				// fill prev second-level index first
				int SingleLevelIndexes[LevelNum];
				synchDisk->ReadSector(TwoLevelIndexes[prevSecondLevelNum-1], (char*)SingleLevelIndexes);
				for(int j=(prevLeftSectors%LevelNum); j>0 && j<LevelNum; ++j)
				{
					SingleLevelIndexes[j] = freeMap->Find();
				}
				synchDisk->WriteSector(TwoLevelIndexes[prevSecondLevelNum-1], (char*)SingleLevelIndexes);

				// fill new second-level indexes
				for(int i=prevSecondLevelNum; i<SecondLevelNum; ++i)
				{
					TwoLevelIndexes[i] = freeMap->Find();
					int SingleLevelIndexes[LevelNum];
					for(int j=0; j<LevelNum && (j+i*LevelNum)<LeftSectors; ++j)
					{
						SingleLevelIndexes[j] = freeMap->Find();
					}
					synchDisk->WriteSector(TwoLevelIndexes[i], (char*)SingleLevelIndexes);
				}
				synchDisk->WriteSector(dataSectors[TwoLevelIdx], (char*)TwoLevelIndexes);
			}
		}
		else
		{
			// should not occur, since we have judged whether all empty sectors can hold this extension.
			DEBUG('f', "===> There is no enough space in Sector Table.\n");
			ASSERT(FALSE);
		}
#endif
	}
	DEBUG('f', "===> Finish allocating new Sectors.\n");
	numBytes = afterBytes;
	numSectors = afterSectors;
	return TRUE;
}
