#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

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
    for (int i = 0; i < (FAT_SIZE_BLOCKS * BLOCK_SIZE / sizeof(uint16_t)); i++)
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
    for (int i = 0; i < (superBlock.dataArea.offset / blockSize); i++)
    {
        setBlockUsed(i);
    }
}

void setBlockFree(uint16_t blockNumber)
{
    if (blockNumber < MAX_BLOCKS)
    {
        superBlock.freeBlocksBitmap.bitmap[blockNumber / 8] |= (1 << (blockNumber % 8));
        superBlock.freeBlocks++;
    }
}

void setBlockUsed(uint16_t blockNumber)
{
    if (blockNumber < MAX_BLOCKS)
    {
        superBlock.freeBlocksBitmap.bitmap[blockNumber / 8] &= ~(1 << (blockNumber % 8));
        superBlock.freeBlocks--;
    }
}

uint16_t findFreeBlock(char *fsBase)
{
    for (int i = 0; i < MAX_BLOCKS; ++i)
    {
        if (superBlock.freeBlocksBitmap.bitmap[i / 8] & (1 << (i % 8)))
        {
            return i;
        }
    }
    return (uint16_t)-1; // No free block found
}

void printFileDetails(const char *directoryName, const char *fileName)
{
    DirectoryTable *dir = findDirectory(directoryName);
    if (dir == NULL)
    {
        printf("Directory not found: %s\n", directoryName);
        return;
    }

    for (int i = 0; i < dir->fileCount; i++)
    {
        DirectoryEntry *entry = &dir->entries[i];
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        if (entry->entryType == FILE_TYPE && strncmp(storedFileName, fileName, entry->fileNameLength - 1) == 0) // Use entry->fileNameLength - 1 to exclude null terminator
        {
            printf("File Name: %s\n", storedFileName);
            printf("Size: %u bytes\n", entry->size);
            printf("Permissions: %s%s\n",
                   (entry->permissions & PERMISSION_READ) ? "R" : "",
                   (entry->permissions & PERMISSION_WRITE) ? "W" : "");
            printf("Creation Date: %s", ctime(&entry->creationDate));
            printf("Modification Date: %s", ctime(&entry->modificationDate));
            printf("First Block: %u\n", entry->firstBlock);
            return;
        }
    }
    printf("File not found: %s\n", fileName);
}

int createFile(char *fsBase, const char *directoryName, const char *fileName, uint16_t permissions, const char *password)
{
    DirectoryTable *dir = findDirectory(directoryName);
    if (dir == NULL)
    {
        fprintf(stderr, "Directory not found: %s\n", directoryName);
        return -1;
    }

    if (dir->fileCount >= MAX_FILES)
    {
        fprintf(stderr, "Directory full\n");
        return -1;
    }

    uint16_t fileNameLength = strlen(fileName) + 1; // Include null terminator
    if (superBlock.fileNameArea.used + fileNameLength > superBlock.fileNameArea.size)
    {
        fprintf(stderr, "Not enough space for file name\n");
        return -1;
    }

    uint16_t freeBlock = findFreeBlock(fsBase);
    if (freeBlock == (uint16_t)-1)
    {
        fprintf(stderr, "No free blocks available\n");
        return -1;
    }

    printf("Creating file %s in directory %s at block %u\n", fileName, directoryName, freeBlock);

    DirectoryEntry *newEntry = &dir->entries[dir->fileCount++];
    newEntry->entryType = FILE_TYPE;
    newEntry->fileNameOffset = superBlock.fileNameArea.used;
    newEntry->fileNameLength = fileNameLength;
    newEntry->size = 0;
    newEntry->permissions = permissions;
    newEntry->firstBlock = freeBlock;
    newEntry->creationDate = time(NULL);
    newEntry->modificationDate = newEntry->creationDate;
    strncpy(newEntry->password, password, sizeof(newEntry->password) - 1);

    char *fileNamePtr = fsMemoryBase + superBlock.fileNameArea.offset + superBlock.fileNameArea.used;
    memcpy(fileNamePtr, fileName, fileNameLength - 1); // Copy the filename without null terminator
    fileNamePtr[fileNameLength - 1] = '\0';            // Ensure null termination
    superBlock.fileNameArea.used += fileNameLength;

    setBlockUsed(freeBlock);
    superBlock.fat.fat[freeBlock] = 0xFFFF; // Initialize the FAT entry

    return 0; // Success
}

int deleteFile(char *fsBase, const char *filePath)
{
    // Duplicate the file path to avoid modifying the original
    char *dirPath = strdup(filePath);
    if (!dirPath)
    {
        perror("Failed to allocate memory");
        return -1;
    }

    // Find the last occurrence of '/' in the file path
    char *fileName = strrchr(dirPath, '/');
    if (!fileName)
    {
        fprintf(stderr, "Invalid file path: %s\n", filePath);
        free(dirPath);
        return -1;
    }

    // Split the path into directory and file name
    *fileName = '\0'; // Terminate the directory path
    fileName++;       // Move to the file name part

    if (strlen(dirPath) == 0)
    {
        fprintf(stderr, "Invalid directory path: %s\n", filePath);
        free(dirPath);
        return -1;
    }

    DirectoryTable *dir = findDirectory(dirPath);
    free(dirPath);

    if (dir == NULL)
    {
        fprintf(stderr, "Directory not found: %s\n", filePath);
        return -1;
    }

    // Find the file in the directory
    for (int i = 0; i < dir->fileCount; i++)
    {
        DirectoryEntry *entry = &dir->entries[i];
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        if (strncmp(storedFileName, fileName, entry->fileNameLength) == 0)
        {
            // Free the blocks used by the file
            uint16_t currentBlock = entry->firstBlock;
            while (currentBlock != 0xFFFF)
            {
                uint16_t nextBlock = superBlock.fat.fat[currentBlock];
                setBlockFree(currentBlock);
                currentBlock = nextBlock;
            }

            // Reclaim space (simplified)
            superBlock.fileNameArea.used -= entry->fileNameLength;
            memmove(entry, entry + 1, (dir->fileCount - i - 1) * sizeof(DirectoryEntry));
            dir->fileCount--;
            return 0; // Success
        }
    }
    return -1; // File not found
}

int saveFileSystem(const char *fileName)
{
    FILE *file = fopen(fileName, "wb");
    if (!file)
    {
        perror("Failed to open file for writing");
        return -1;
    }

    printf("Total file system size: %zu bytes\n", totalFsSize);

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
    printf("block size is: %d\n", superBlock.blockSize);

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

void printDirectoryDetails(const char *dirName)
{
    DirectoryTable *dir = findDirectory(dirName);
    if (dir == NULL)
    {
        printf("Directory not found: %s\n", dirName);
        return;
    }

    printf("Directory Name: %s\n", dirName);
    for (int i = 0; i < dir->fileCount; i++)
    {
        DirectoryEntry *entry = &dir->entries[i];
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        if (entry->entryType == FILE_TYPE)
        {
            printf("  File Name: %s\n", storedFileName);
        }
        else
        {
            printf("  Directory Name: %s\n", storedFileName);
            // Recursively print the subdirectory contents
            printDirectoryDetails(storedFileName);
        }
    }
}

int createDirectory(char *fsBase, const char *dirName, uint16_t permissions, const char *password)
{
    if (superBlock.rootDirectory.fileCount >= MAX_FILES)
    {
        fprintf(stderr, "Directory full\n");
        return -1;
    }

    uint16_t dirNameLength = strlen(dirName) + 1; // Include null terminator
    if (superBlock.fileNameArea.used + dirNameLength > superBlock.fileNameArea.size)
    {
        fprintf(stderr, "Not enough space for directory name\n");
        return -1;
    }

    uint16_t freeBlock = findFreeBlock(fsBase);
    if (freeBlock == (uint16_t)-1)
    {
        fprintf(stderr, "No free blocks available\n");
        return -1;
    }

    DirectoryEntry *newEntry = &superBlock.rootDirectory.entries[superBlock.rootDirectory.fileCount++];
    newEntry->entryType = DIRECTORY_TYPE;
    newEntry->fileNameOffset = superBlock.fileNameArea.used;
    newEntry->fileNameLength = dirNameLength;
    newEntry->size = 0; // Directories have size 0
    newEntry->permissions = permissions;
    newEntry->firstBlock = freeBlock;
    newEntry->creationDate = time(NULL);
    newEntry->modificationDate = newEntry->creationDate;
    strncpy(newEntry->password, password, sizeof(newEntry->password) - 1);

    char *dirNamePtr = fsMemoryBase + superBlock.fileNameArea.offset + superBlock.fileNameArea.used;
    memcpy(dirNamePtr, dirName, dirNameLength - 1); // Copy the directory name without null terminator
    dirNamePtr[dirNameLength - 1] = '\0';           // Ensure null termination
    superBlock.fileNameArea.used += dirNameLength;

    setBlockUsed(freeBlock);

    // Initialize the new directory table in the allocated block
    DirectoryTable *newDirTable = (DirectoryTable *)(fsMemoryBase + freeBlock * superBlock.blockSize);
    memset(newDirTable, 0, sizeof(DirectoryTable));

    return 0; // Success
}

int deleteDirectory(char *fsBase, const char *dirName)
{
    for (int i = 0; i < superBlock.rootDirectory.fileCount; i++)
    {
        DirectoryEntry *entry = &superBlock.rootDirectory.entries[i];
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        if (entry->entryType == DIRECTORY_TYPE && strncmp(storedFileName, dirName, entry->fileNameLength) == 0)
        {
            // Recursively delete contents of the directory
            uint16_t blockNumber = entry->firstBlock;
            while (blockNumber != 0xFFFF)
            {
                DirectoryTable *dirTable = (DirectoryTable *)(fsMemoryBase + blockNumber * superBlock.blockSize);
                for (int j = 0; j < dirTable->fileCount; j++)
                {
                    DirectoryEntry *childEntry = &dirTable->entries[j];
                    char *childFileName = fsMemoryBase + superBlock.fileNameArea.offset + childEntry->fileNameOffset;
                    if (childEntry->entryType == FILE_TYPE)
                    {
                        deleteFile(fsBase, childFileName);
                    }
                    else
                    {
                        deleteDirectory(fsBase, childFileName);
                    }
                }
                blockNumber = superBlock.fat.fat[blockNumber];
            }

            setBlockFree(entry->firstBlock);
            superBlock.fileNameArea.used -= entry->fileNameLength; // Reclaim space (simplified)
            memmove(entry, entry + 1, (superBlock.rootDirectory.fileCount - i - 1) * sizeof(DirectoryEntry));
            superBlock.rootDirectory.fileCount--;
            return 0; // Success
        }
    }
    return -1; // Directory not found
}

DirectoryTable *findDirectory(const char *directoryName)
{
    if (strcmp(directoryName, "/") == 0) // Root directory
    {
        return &superBlock.rootDirectory;
    }

    for (int i = 0; i < superBlock.rootDirectory.fileCount; i++)
    {
        DirectoryEntry *entry = &superBlock.rootDirectory.entries[i];
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        if (entry->entryType == DIRECTORY_TYPE && strncmp(storedFileName, directoryName, entry->fileNameLength - 1) == 0) // Use entry->fileNameLength - 1 to exclude null terminator
        {
            uint16_t blockNumber = entry->firstBlock;
            return (DirectoryTable *)(fsMemoryBase + blockNumber * superBlock.blockSize);
        }
    }

    return NULL; // Directory not found
}

int write(const char *directoryName, const char *linuxFileName)
{
    // Get Linux file permissions
    struct stat fileStat;
    if (stat(linuxFileName, &fileStat) != 0)
    {
        perror("Failed to get Linux file permissions");
        return -1;
    }
    uint16_t permissions = 0;
    if (fileStat.st_mode & S_IRUSR)
        permissions |= PERMISSION_READ;
    if (fileStat.st_mode & S_IWUSR)
        permissions |= PERMISSION_WRITE;

    // Open the Linux file for reading
    FILE *linuxFile = fopen(linuxFileName, "rb");
    if (!linuxFile)
    {
        perror("Failed to open Linux file for reading");
        return -1;
    }

    // Get the size of the Linux file
    fseek(linuxFile, 0, SEEK_END);
    size_t linuxFileSize = ftell(linuxFile);
    fseek(linuxFile, 0, SEEK_SET);

    // Read the content of the Linux file
    char *linuxFileContent = malloc(linuxFileSize);
    if (!linuxFileContent)
    {
        perror("Failed to allocate memory for Linux file content");
        fclose(linuxFile);
        return -1;
    }

    size_t read = fread(linuxFileContent, 1, linuxFileSize, linuxFile);
    if (read != linuxFileSize)
    {
        perror("Failed to read complete Linux file");
        free(linuxFileContent);
        fclose(linuxFile);
        return -1;
    }
    fclose(linuxFile);

    // Create a new file in the target directory in our file system
    char *fileName = strrchr(linuxFileName, '/');
    fileName = fileName ? fileName + 1 : (char *)linuxFileName;

    if (createFile(fsMemoryBase, directoryName, fileName, permissions, "password123") != 0)
    {
        fprintf(stderr, "Failed to create file in directory: %s\n", directoryName);
        free(linuxFileContent);
        return -1;
    }

    printf("File created successfully\n");

    // Find the directory and file entry
    DirectoryTable *dir = findDirectory(directoryName);
    if (dir == NULL)
    {
        fprintf(stderr, "Directory not found: %s\n", directoryName);
        free(linuxFileContent);
        return -1;
    }

    DirectoryEntry *newEntry = findFileInDirectory(dir, fileName);
    if (!newEntry)
    {
        fprintf(stderr, "Failed to find new file entry in directory: %s\n", directoryName);
        free(linuxFileContent);
        return -1;
    }

    // Write the content to the new file in our file system
    size_t remainingSize = linuxFileSize;
    uint16_t currentBlock = newEntry->firstBlock;
    while (remainingSize > 0)
    {
        size_t writeSize = remainingSize > superBlock.blockSize ? superBlock.blockSize : remainingSize;
        memcpy(fsMemoryBase + currentBlock * superBlock.blockSize, linuxFileContent + (linuxFileSize - remainingSize), writeSize);
        remainingSize -= writeSize;

        if (remainingSize > 0)
        {
            uint16_t nextBlock = findFreeBlock(fsMemoryBase);
            if (nextBlock == (uint16_t)-1)
            {
                fprintf(stderr, "No free blocks available to complete file write\n");
                free(linuxFileContent);
                return -1;
            }
            printf("Allocating block %u -> %u\n", currentBlock, nextBlock);
            superBlock.fat.fat[currentBlock] = nextBlock;
            setBlockUsed(nextBlock);
            currentBlock = nextBlock;
        }
        else
        {
            printf("Marking block %u as end of chain\n", currentBlock);
            superBlock.fat.fat[currentBlock] = 0xFFFF; // Mark as end of the chain
        }
    }

    newEntry->size = linuxFileSize;
    newEntry->modificationDate = time(NULL);

    free(linuxFileContent);
    return 0;
}

int read(const char *filePath, const char *linuxFileName)
{
    // Duplicate the file path to avoid modifying the original
    char *dirPath = strdup(filePath);
    if (!dirPath)
    {
        perror("Failed to allocate memory");
        return -1;
    }

    // Find the last occurrence of '/' in the file path
    char *fileName = strrchr(dirPath, '/');
    if (!fileName)
    {
        fprintf(stderr, "Invalid file path: %s\n", filePath);
        free(dirPath);
        return -1;
    }

    // Split the path into directory and file name
    *fileName = '\0'; // Terminate the directory path
    fileName++;       // Move to the file name part

    if (strlen(dirPath) == 0)
    {
        fprintf(stderr, "Invalid directory path: %s\n", filePath);
        free(dirPath);
        return -1;
    }

    printf("fileName: %s\n", fileName);
    printf("dirPath: %s\n", dirPath);
    printf("linuxPath: %s\n", linuxFileName);

    DirectoryTable *dir = findDirectory(dirPath);
    // free(dirPath);

    if (dir == NULL)
    {
        fprintf(stderr, "Directory not found: %s\n", filePath);
        return -1;
    }

    // Find the file in the directory
    printf("dir: %s\n", dir);
    printf("fileName: %s\n", fileName);
    DirectoryEntry *fileEntry = findFileInDirectory(dir, fileName);
    if (!fileEntry)
    {
        fprintf(stderr, "File not found: %s\n", fileName);
        return -1;
    }

    // Open the Linux file for writing
    FILE *linuxFile = fopen(linuxFileName, "wb");
    if (!linuxFile)
    {
        perror("Failed to open Linux file for writing");
        return -1;
    }

    // Read the content from the file system and write it to the Linux file
    size_t remainingSize = fileEntry->size;
    uint16_t currentBlock = fileEntry->firstBlock;
    while (remainingSize > 0)
    {
        size_t readSize = remainingSize > superBlock.blockSize ? superBlock.blockSize : remainingSize;
        fwrite(fsMemoryBase + currentBlock * superBlock.blockSize, 1, readSize, linuxFile);
        remainingSize -= readSize;
        currentBlock = superBlock.fat.fat[currentBlock];
        if (currentBlock == 0xFFFF)
        {
            break;
        }
    }

    fclose(linuxFile);

    // Set Linux file permissions
    struct stat fileStat;
    if (stat(linuxFileName, &fileStat) != 0)
    {
        perror("Failed to get Linux file status");
        return -1;
    }

    uint16_t permissions = fileEntry->permissions;
    mode_t linuxPermissions = 0;
    if (permissions & PERMISSION_READ)
        linuxPermissions |= S_IRUSR;
    if (permissions & PERMISSION_WRITE)
        linuxPermissions |= S_IWUSR;

    if (chmod(linuxFileName, linuxPermissions) != 0)
    {
        perror("Failed to set Linux file permissions");
        return -1;
    }

    return 0;
}

// Find the DirectoryEntry in a given DirectoryTable by file name
DirectoryEntry *findFileInDirectory(DirectoryTable *dir, const char *fileName)
{
    for (int i = 0; i < dir->fileCount; i++)
    {
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + dir->entries[i].fileNameOffset;
        if (strncmp(storedFileName, fileName, dir->entries[i].fileNameLength - 1) == 0)
        {
            return &dir->entries[i];
        }
    }
    return NULL;
}

void printDirectoryContents(DirectoryTable *dir, const char *path)
{
    for (int i = 0; i < dir->fileCount; i++)
    {
        DirectoryEntry *entry = &dir->entries[i];
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        char fullPath[256];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, storedFileName);

        if (entry->entryType == FILE_TYPE)
        {
            printf("File: %s (Block: %u)\n", fullPath, entry->firstBlock);
            uint16_t currentBlock = entry->firstBlock;
            while (currentBlock != 0xFFFF && currentBlock < MAX_BLOCKS)
            {
                printf("  Block %u: %s (Next: %u)\n", currentBlock, fullPath, superBlock.fat.fat[currentBlock]);
                currentBlock = superBlock.fat.fat[currentBlock];
            }
        }
        else if (entry->entryType == DIRECTORY_TYPE)
        {
            printf("Directory: %s (Block: %u)\n", fullPath, entry->firstBlock);
            uint16_t blockNumber = entry->firstBlock;
            while (blockNumber != 0xFFFF && blockNumber < MAX_BLOCKS)
            {
                DirectoryTable *subDir = (DirectoryTable *)(fsMemoryBase + blockNumber * superBlock.blockSize);
                printDirectoryContents(subDir, fullPath);
                blockNumber = superBlock.fat.fat[blockNumber];
            }
        }
    }
}

void countFilesAndDirectories(DirectoryTable *dir, uint16_t *fileCount, uint16_t *dirCount)
{
    for (int i = 0; i < dir->fileCount; i++)
    {
        DirectoryEntry *entry = &dir->entries[i];
        if (entry->entryType == FILE_TYPE)
        {
            (*fileCount)++;
        }
        else if (entry->entryType == DIRECTORY_TYPE)
        {
            (*dirCount)++;
            uint16_t blockNumber = entry->firstBlock;
            while (blockNumber != 0xFFFF && blockNumber < MAX_BLOCKS)
            {
                DirectoryTable *subDir = (DirectoryTable *)(fsMemoryBase + blockNumber * superBlock.blockSize);
                countFilesAndDirectories(subDir, fileCount, dirCount);
                blockNumber = superBlock.fat.fat[blockNumber];
            }
        }
    }
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

int chmodFile(char *fsBase, const char *filePath, uint16_t newPermissions)
{
    // Duplicate the file path to avoid modifying the original
    char *dirPath = strdup(filePath);
    if (!dirPath)
    {
        perror("Failed to allocate memory");
        return -1;
    }

    // Find the last occurrence of '/' in the file path
    char *fileName = strrchr(dirPath, '/');
    if (!fileName)
    {
        fprintf(stderr, "Invalid file path: %s\n", filePath);
        free(dirPath);
        return -1;
    }

    // Split the path into directory and file name
    *fileName = '\0'; // Terminate the directory path
    fileName++;       // Move to the file name part

    if (strlen(dirPath) == 0)
    {
        fprintf(stderr, "Invalid directory path: %s\n", filePath);
        free(dirPath);
        return -1;
    }

    DirectoryTable *dir = findDirectory(dirPath);
    free(dirPath);

    if (dir == NULL)
    {
        fprintf(stderr, "Directory not found: %s\n", filePath);
        return -1;
    }

    // Find the file in the directory
    for (int i = 0; i < dir->fileCount; i++)
    {
        DirectoryEntry *entry = &dir->entries[i];
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        if (strncmp(storedFileName, fileName, entry->fileNameLength) == 0)
        {
            // Update the file permissions
            entry->permissions = newPermissions;
            entry->modificationDate = time(NULL); // Update the modification date
            return 0;                             // Success
        }
    }
    return -1; // File not found
}
