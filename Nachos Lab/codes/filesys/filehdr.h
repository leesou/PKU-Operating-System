// filehdr.h 
//	Data structures for managing a disk file header.  
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"

// #define NumDirect 	((SectorSize - 2 * sizeof(int)) / sizeof(int))
#define NumTimeString 3
#define NumIntProperty 2
#define TimeStringLength 25 // 24 chars + '\0'
#define ExtensionLength 5 // 4 chars + '\0'

#define AllStringLength (NumTimeString*TimeStringLength+ExtensionLength)

# ifndef USE_INDIRECT

#define NumDirect ((SectorSize - NumIntProperty*sizeof(int) - AllStringLength*sizeof(char)) / sizeof(int))
#define MaxFileSize 	(NumDirect * SectorSize)

#else

#define NumDataSectors ((SectorSize - NumIntProperty*sizeof(int) - AllStringLength*sizeof(char)) / sizeof(int))
#define NumDirect (NumDataSectors - 2)
#define OneLevelIdx (NumDataSectors - 2) // index = number+1
#define TwoLevelIdx (NumDataSectors - 1)
#define LevelNum (SectorSize / sizeof(int))
#define MaxFileSize (NumDirect * SectorSize) + (LevelNum * SectorSize) + (LevelNum * LevelNum * SectorSize)

#endif

// The following class defines the Nachos "file header" (in UNIX terms,  
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks. 
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader {
  public:
    bool Allocate(BitMap *bitMap, int fileSize);// Initialize a file header, 
						//  including allocating space 
						//  on disk for the file data
    void Deallocate(BitMap *bitMap);  		// De-allocate this file's 
						//  data blocks

    void FetchFrom(int sectorNumber); 	// Initialize file header from disk
    void WriteBack(int sectorNumber); 	// Write modifications to file header
					//  back to disk

    int ByteToSector(int offset);	// Convert a byte offset into the file
					// to the disk sector containing
					// the byte

    int FileLength();			// Return the length of the file 
					// in bytes

    void Print();			// Print the contents of the file.

    // Lab5: methods for operating additional file attributes
    void CreateInit(char* ext); // used for initialization

    void SetFileExtension(char* ext){ strcpy(fileExtension, ext); } // "" if no extension
    char* GetFileExtension(){return fileExtension;}
    void SetCreateTime(char* t){ strcpy(createTime, t); }
    void SetModifyTime(char* t){ strcpy(modifyTime, t); }
    void SetLastVisitTime(char* t){ strcpy(lastVisitTime, t); }
    
    void SetHeaderSector(int s){ headerSector = s; }
    int GetHeaderSector(){ return headerSector; }

    // Lab5 Exercise5: expand file size
    bool ExpandFileSize(BitMap* freeMap, int ExpandBytes);

  private:
    int numBytes;			// Number of bytes in the file
    int numSectors;			// Number of data sectors in the file
    
    /* Lab5 Exercise2: additional file attributes */
    char fileExtension[ExtensionLength];
    char createTime[TimeStringLength];
    char modifyTime[TimeStringLength];
    char lastVisitTime[TimeStringLength];

#ifndef USE_INDIRECT
    int dataSectors[NumDirect];		// Disk sector numbers for each data 
					// block in the file
#else
    int dataSectors[NumDataSectors];
#endif

    // Don't save to disk, in order to simplify file header location.
    int headerSector;

};


#define MAX_DIR_DEP 5

typedef struct 
{
	 /* data */
	char* DirArray[MAX_DIR_DEP];
	int DirDep; // dep of root dir = 0
	char* Base;

} FilePath;



char* GetFileExtension(char* filename);
char* GetTime();
extern FilePath PathParser(char* path);

#endif // FILEHDR_H
