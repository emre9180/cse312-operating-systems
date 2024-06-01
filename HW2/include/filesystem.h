#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "directory.h"

// Constants for file system configuration
#define BLOCK_SIZE 1024 // 1 KB blocks
#define SUPER_BLOCK_SIZE 1
#define FAT_ENTRY_BITS 12 // 12 bits per entry in FAT12
#define BITS_PER_BYTE 8

// Calculate the total number of bits needed for the FAT
#define FAT_TOTAL_BITS (MAX_BLOCKS * FAT_ENTRY_BITS)

// Convert bits to bytes, and then calculate the number of blocks needed to store the FAT
#define FAT_TOTAL_BYTES ((FAT_TOTAL_BITS + BITS_PER_BYTE - 1) / BITS_PER_BYTE) // Round up to nearest byte
#define FAT_SIZE_BLOCKS ((FAT_TOTAL_BYTES + BLOCK_SIZE - 1) / BLOCK_SIZE)      // Round up to nearest block


// Permission flags for files and directories
#define PERMISSION_READ 0x01
#define PERMISSION_WRITE 0x02

// File Allocation Table (FAT) structure
typedef struct {
    uint16_t fat[MAX_BLOCKS]; // Size the FAT in terms of 16-bit entries
} FAT;

// Free Block Bitmap structure
typedef struct {
    uint8_t bitmap[MAX_BLOCKS / 8]; // Bitmap to track free blocks
} FreeBlockBitmap;

// File Name Area structure
typedef struct {
    uint32_t offset; // Offset where file names start
    uint32_t size;   // Size of the file name storage area
    uint32_t used;   // Amount of used space in the file name storage area
} FileNameArea;

// Data Area structure
typedef struct {
    uint32_t offset;     // Offset to the start of the data segment
    uint32_t size;       // Total size of the data segment
    uint32_t blockCount; // Number of blocks in the data segment
} DataArea;

// SuperBlock structure
typedef struct {
    uint16_t blockSize;            // Size of each block
    uint16_t totalBlocks;          // Total number of blocks
    uint16_t freeBlocks;           // Number of free blocks
    uint16_t fatBlocks;            // Number of FAT blocks
    DirectoryTable rootDirectory;  // Root directory table
    FreeBlockBitmap freeBlocksBitmap; // Bitmap for free blocks
    FAT fat;                       // File Allocation Table
    FileNameArea fileNameArea;     // File name area structure
    DataArea dataArea;             // Data area structure
} SuperBlock;

extern SuperBlock superBlock;


// Function declarations

/**
 * Initializes the file system.
 *
 * @param blockSize The size of each block.
 * @param fsBase The base pointer for the file system memory.
 * @param totalFsSize The total size of the file system.
 */
void initializeFileSystem(uint16_t blockSize, char *fsBase, int totalFsSize);

/**
 * Saves the file system to a file.
 *
 * @param fileName The name of the file to save the file system to.
 * @return 0 on success, -1 on failure.
 */
int saveFileSystem(const char *fileName);

/**
 * Loads the file system from a file.
 *
 * @param fileName The name of the file to load the file system from.
 * @return 0 on success, -1 on failure.
 */
int loadFileSystem(const char *fileName);

/**
 * Dumps the file system information.
 */
void dumpe2fs();

#endif // FILESYSTEM_H
