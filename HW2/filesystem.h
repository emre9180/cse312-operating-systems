#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 1024 // 1 KB blocks
#define MAX_BLOCKS 4096 // 4 MB file system
#define SUPER_BLOCK_SIZE 1
#define FAT_ENTRY_BITS 12 // 12 bits per entry in FAT12
#define BITS_PER_BYTE 8

// Calculate the total number of bits needed for the FAT
#define FAT_TOTAL_BITS (MAX_BLOCKS * FAT_ENTRY_BITS)

// Convert bits to bytes, and then calculate the number of blocks needed to store the FAT
#define FAT_TOTAL_BYTES ((FAT_TOTAL_BITS + BITS_PER_BYTE - 1) / BITS_PER_BYTE) // Round up to nearest byte
#define FAT_SIZE_BLOCKS ((FAT_TOTAL_BYTES + BLOCK_SIZE - 1) / BLOCK_SIZE)      // Round up to nearest block

#define MAX_FILES 128

#define PERMISSION_READ 0x01
#define PERMISSION_WRITE 0x02

typedef enum
{
    FILE_TYPE,
    DIRECTORY_TYPE
} EntryType;

typedef struct
{
    uint16_t fat[FAT_SIZE_BLOCKS * BLOCK_SIZE / sizeof(uint16_t)]; // Size the FAT in terms of 16-bit entries
} FAT;

typedef struct
{
    EntryType entryType;       // Type of entry (file or directory)
    uint32_t fileNameOffset;   // Offset of the file name within the file system
    uint16_t fileNameLength;   // Length of the file name
    uint32_t size;             // File size (0 for directories)
    uint16_t permissions;      // Owner permissions (R and W)
    uint32_t creationDate;     // File creation date
    uint32_t modificationDate; // Last modification date
    char password[32];         // Password protection
    uint16_t firstBlock;       // First block of the file or directory
} DirectoryEntry;

typedef struct
{
    DirectoryEntry entries[MAX_FILES];
    uint16_t fileCount;
} DirectoryTable;

typedef struct
{
    uint8_t bitmap[MAX_BLOCKS / 8]; // Bitmap to track free blocks
} FreeBlockBitmap;

typedef struct
{
    uint32_t offset; // Offset where file names start
    uint32_t size;   // Size of the file name storage area
    uint32_t used;   // Amount of used space in the file name storage area
} FileNameArea;

typedef struct
{
    uint32_t offset;     // Offset to the start of the data segment
    uint32_t size;       // Total size of the data segment
    uint32_t blockCount; // Number of blocks in the data segment
} DataArea;

typedef struct
{
    uint16_t blockSize;
    uint16_t totalBlocks;
    uint16_t freeBlocks;
    uint16_t fatBlocks;
    DirectoryTable rootDirectory;
    FreeBlockBitmap freeBlocksBitmap;
    FAT fat;
    FileNameArea fileNameArea; // Struct for file name area
    DataArea dataArea;         // Struct for data area
} SuperBlock;

extern SuperBlock superBlock;
extern char *fsMemoryBase; // Base pointer for file system memory
extern size_t totalFsSize;
extern char *fsMemory;

// Function declarations
void initializeFileSystem(uint16_t blockSize, char *fsBase, int totalFsSize);
void setBlockFree(uint16_t blockNumber);
void setBlockUsed(uint16_t blockNumber);
uint16_t findFreeBlock();

int createFile(char *fsBase, const char *directoryName, const char *fileName, uint16_t permissions, const char *password);
int deleteFile(const char *fileName);
void printFileDetails(const char *directoryName, const char *fileName);

int saveFileSystem(const char *fileName);
int loadFileSystem(const char *fileName);

int createDirectory(char *fsBase, const char *dirName, uint16_t permissions, const char *password);
int deleteDirectory(char *fsBase, const char *dirName);
void printDirectoryDetails(const char *dirName);
DirectoryTable *findDirectory(DirectoryTable *dir, const char *directoryName);

int write(const char *directoryName, char *linuxFileName, char *password);
int read(const char *filePath, const char *linuxFileName, char *password);

DirectoryEntry *findFileInDirectory(DirectoryTable *dir, const char *fileName);

void dumpe2fs();
void printDirectoryContents(DirectoryTable *dir, const char *path);
void countFilesAndDirectories(DirectoryTable *dir, uint16_t *fileCount, uint16_t *dirCount);
int chmodFile(char *fsBase, const char *filePath, uint16_t newPermissions);

void addPassword(char *fsBase, const char *filePath, const char *password);

#endif // FILESYSTEM_H