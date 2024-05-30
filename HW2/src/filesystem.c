#include "../include/filesystem.h"
#include "../include/directory.h"
#include "../include/file.h"
#include "../include/utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

SuperBlock superBlock;
char *fsMemoryBase; // Base pointer for file system memory

void initializeFileSystem(uint16_t blockSize, char *fsBase, int totalFsSize)
{
    fsMemoryBase = fsBase;                // Set the global file system base pointer
    memset(fsMemoryBase, 0, totalFsSize); // Initialize all memory to zero

    superBlock.blockSize = blockSize;
    superBlock.totalBlocks = MAX_BLOCKS;
    superBlock.freeBlocks = MAX_BLOCKS; // Start with all blocks as free
    superBlock.fatBlocks = FAT_SIZE_BLOCKS;

    // Initialize the bitmap to all free
    memset(superBlock.freeBlocksBitmap.bitmap, 0xFF, sizeof(superBlock.freeBlocksBitmap.bitmap));

    // Set up the FAT
    memset(&superBlock.fat, 0, sizeof(FAT));

    // Set up the FAT
    for (long unsigned int i = 0; i < (FAT_SIZE_BLOCKS * BLOCK_SIZE / sizeof(uint16_t)); i++)
    {
        superBlock.fat.fat[i] = 0xFFFF; // Initialize FAT entries to 0xFFFF
    }

    // Set up the directory table
    memset(&superBlock.rootDirectory, 0, sizeof(DirectoryTable));

    // Setup filename area
    superBlock.fileNameArea.offset = sizeof(SuperBlock) + sizeof(FAT) + sizeof(DirectoryTable) + sizeof(FreeBlockBitmap);
    superBlock.fileNameArea.size = MAX_FILES * sizeof(DirectoryEntry);
    superBlock.fileNameArea.used = 0;

    // Setup data area
    superBlock.dataArea.offset = superBlock.fileNameArea.offset + superBlock.fileNameArea.size;
    superBlock.dataArea.size = (MAX_BLOCKS * blockSize) - superBlock.dataArea.offset;
    superBlock.dataArea.blockCount = superBlock.dataArea.size / blockSize;

    // Mark system-reserved blocks as used
    for (long unsigned int  i = 0; i < (superBlock.dataArea.offset / blockSize); i++)
    {
        setBlockUsed(i);
    }
}

int saveFileSystem(const char *fileName)
{
    FILE *file = fopen(fileName, "wb");
    if (!file)
    {
        perror("Failed to open file for writing");
        return -1;
    }

    // write superblock
    size_t written = fwrite(&superBlock, 1, sizeof(SuperBlock), file);

    // write FAT
    written += fwrite(&superBlock.fat, 1, sizeof(FAT), file);

    // write Directory Table
    written += fwrite(&superBlock.rootDirectory, 1, sizeof(DirectoryTable), file);

    // write Free Block Bitmap
    written += fwrite(&superBlock.freeBlocksBitmap, 1, sizeof(FreeBlockBitmap), file);

    // write file name area
    written += fwrite(fsMemoryBase + superBlock.fileNameArea.offset, 1, superBlock.fileNameArea.size, file);

    // write data area
    written += fwrite(fsMemoryBase + superBlock.dataArea.offset, 1, superBlock.dataArea.size, file);

    // if (written != totalFsSize)
    // {
    //     perror("Failed to write complete file system to file");
    //     fclose(file);
    //     return -1;
    // }

    fclose(file);
    return 0;
}

int loadFileSystem(const char *fileName)
{
    FILE *file = fopen(fileName, "rb");
    if (!file)
    {
        perror("Failed to open file for reading");
        return -1;
    }

    // Get the size of the file
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the file system
    fsMemoryBase = malloc(fileSize);
    if (!fsMemoryBase)
    {
        perror("Failed to allocate memory for file system");
        fclose(file);
        return -1;
    }

    // Read the entire file into memory
    size_t read = fread(fsMemoryBase, 1, fileSize, file);
    if (read != fileSize)
    {
        perror("Failed to read complete file system from file");
        free(fsMemoryBase);
        fclose(file);
        return -1;
    }

    // Correctly initialize the super block and other structures
    memcpy(&superBlock, fsMemoryBase, sizeof(SuperBlock));

    // Set pointers to the rest of the file system components
    char *currentPtr = fsMemoryBase + sizeof(SuperBlock);

    // Update FAT pointer
    memcpy(&superBlock.fat, currentPtr, sizeof(FAT));
    currentPtr += sizeof(FAT);

    // Update Directory Table pointer
    memcpy(&superBlock.rootDirectory, currentPtr, sizeof(DirectoryTable));
    currentPtr += sizeof(DirectoryTable);

    // Update Free Block Bitmap pointer
    memcpy(&superBlock.freeBlocksBitmap, currentPtr, sizeof(FreeBlockBitmap));
    currentPtr += sizeof(FreeBlockBitmap);

    // Update FileName Area offset
    superBlock.fileNameArea.offset = currentPtr - fsMemoryBase;
    currentPtr += superBlock.fileNameArea.size;

    // Update Data Area offset
    superBlock.dataArea.offset = currentPtr - fsMemoryBase;

    fclose(file);
    return 0;
}

void dumpe2fs()
{
    printf("Filesystem status:\n");
    printf("Block count: %u\n", superBlock.totalBlocks);
    printf("Free blocks: %u\n", superBlock.freeBlocks);
    printf("Block size: %u bytes\n", superBlock.blockSize);
    printf("First %d blocks are reserved for file system\n", superBlock.dataArea.offset / superBlock.blockSize);

    uint16_t fileCount = 0;
    uint16_t dirCount = 0;

    // Count files and directories starting from the root directory
    countFilesAndDirectories(&superBlock.rootDirectory, &fileCount, &dirCount);

    printf("Number of files: %u\n", fileCount);
    printf("Number of directories: %u\n", dirCount);

    printf("Occupied blocks:\n");
    printDirectoryContents(&superBlock.rootDirectory, "/");
}
