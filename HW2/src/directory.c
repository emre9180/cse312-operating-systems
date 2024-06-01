#include "../include/directory.h"
#include "../include/filesystem.h"
#include "../include/utility.h"
#include "../include/file.h"

void printDirectoryDetails(char *dirName)
{
    DirectoryTable *dir;

    if(strcmp(dirName, "/") == 0)
    {
        dir = &superBlock.rootDirectory;
    }
    else
        dir = findDirectory(&superBlock.rootDirectory, dirName);


    if (dir == NULL || (strlen(dirName) != 1 && dir == &superBlock.rootDirectory))
    {
        printf("Directory not found: %s\n", dirName);
        return;
    }

    printf("Content of %s:\n", dirName);
    // printf("Dir File Count: %d\n", dir->fileCount);
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
            // printDirectoryDetails(storedFileName);
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

    // Count '/' characters in the directory name to determine the depth
    int depth = 0;
    for (int i = 0; i < dirNameLength; i++)
    {
        if (dirName[i] == '/')
        {
            depth++;
        }
    }

    DirectoryEntry *newEntry;
    DirectoryTable *targetDirectory;

    if (depth == 1)
    {
        newEntry = &superBlock.rootDirectory.entries[superBlock.rootDirectory.fileCount++];
        targetDirectory = &superBlock.rootDirectory;
    }
    else
    {
        // Find the parent directory
        char *parentDirName = strdup(dirName);
        char *lastSlash = strrchr(parentDirName, '/');
        *lastSlash = '\0'; // Terminate the parent directory name
        targetDirectory = findDirectory(&superBlock.rootDirectory, parentDirName);

        if (targetDirectory == NULL || targetDirectory==&superBlock.rootDirectory)
        {
            fprintf(stderr, "Parent directory not found: %s\n", parentDirName);
            free(parentDirName);
            return -1;
        }
        free(parentDirName);

        if (targetDirectory->fileCount >= MAX_FILES)
        {
            fprintf(stderr, "Parent directory full\n");
            return -1;
        }

        newEntry = &targetDirectory->entries[targetDirectory->fileCount++];
    }

    newEntry->entryType = DIRECTORY_TYPE;
    newEntry->fileNameOffset = superBlock.fileNameArea.used;
    newEntry->fileNameLength = dirNameLength;
    newEntry->size = 0; // Directories have size 0
    newEntry->permissions = permissions;
    newEntry->firstBlock = freeBlock;
    newEntry->creationDate = time(NULL);
    newEntry->modificationDate = newEntry->creationDate;
    strncpy(newEntry->password, password, sizeof(newEntry->password) - 1);
    newEntry->password[sizeof(newEntry->password) - 1] = '\0'; // Ensure null-termination

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

int deleteDirectoryHelper(char *fsBase, const char *dirName, DirectoryTable *dir)
{
    for (int i = 0; i < dir->fileCount; i++)
    {
        DirectoryEntry *entry = dir->entries + i;
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;

        if (entry->entryType == DIRECTORY_TYPE && strcmp(storedFileName, dirName) == 0)
        {

            // Recursively delete contents of the directory
            uint16_t blockNumber = entry->firstBlock;
            int isEmpty = 1;
            while (blockNumber != 0xFFFF)
            {
                DirectoryTable *dirTable = (DirectoryTable *)(fsMemoryBase + blockNumber * superBlock.blockSize);
                for (int j = 0; j < dirTable->fileCount; j++)
                {
                    DirectoryEntry *childEntry = &dirTable->entries[j];
                    // char *childFileName = fsMemoryBase + superBlock.fileNameArea.offset + childEntry->fileNameOffset;
                    if (childEntry->entryType == FILE_TYPE)
                    {
                        isEmpty = 0;
                    }
                    else if (childEntry->entryType == DIRECTORY_TYPE)
                    {
                        isEmpty = 0;
                    }
                }
                blockNumber = superBlock.fat.fat[blockNumber];
            }

            if(isEmpty == 0)
            {
                printf("Error while deleting directory: Directory is not empty\n");
                return 0;
            }

            setBlockFree(entry->firstBlock);
            superBlock.fileNameArea.used -= entry->fileNameLength; // Reclaim space (simplified)
            memmove(entry, entry + 1, (dir->fileCount - i - 1) * sizeof(DirectoryEntry));
            dir->fileCount--;
            return 0; // Success
        }

        else
        {
            uint16_t blockNumber = entry->firstBlock;
            DirectoryTable *dirTable = (DirectoryTable *)(fsMemoryBase + blockNumber * superBlock.blockSize);
            return deleteDirectoryHelper(fsBase, dirName, dirTable);
        }
    }

    return -1; // Directory not found
}

int deleteDirectory(char *fsBase, const char *dirName)
{
    return deleteDirectoryHelper(fsBase, dirName, &superBlock.rootDirectory);
}

DirectoryTable *findDirectoryHelper(DirectoryTable *dirTable, char *directoryName, DirectoryTable *result)
{
    if (strcmp(directoryName, "/") == 0) // Root directory
    {
        return &superBlock.rootDirectory;
    }

    for (int i = 0; i < dirTable->fileCount; i++)
    {
        DirectoryEntry *entry = dirTable->entries + i;
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        if (entry->entryType == DIRECTORY_TYPE && strncmp(storedFileName, directoryName, entry->fileNameLength - 1) == 0) // Use entry->fileNameLength - 1 to exclude null terminator
        {
            uint16_t blockNumber = entry->firstBlock;

            // get the last [entry->fileNameLength-1] characters from directoryName, and make a recursive call
            char *nextDirName = directoryName + entry->fileNameLength - 1;
            return findDirectoryHelper((DirectoryTable *)(fsMemoryBase + blockNumber * superBlock.blockSize), nextDirName, result);
        }
    }

    return NULL; // Directory not found
}

DirectoryTable *findDirectory(DirectoryTable *dirTable, char *directoryName)
{
    if (strcmp(directoryName, "/") == 0) // Root directory
    {
        return &superBlock.rootDirectory;
    }

    for (int i = 0; i < dirTable->fileCount; i++)
    {
        DirectoryEntry *entry = dirTable->entries + i;
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        if (entry->entryType == DIRECTORY_TYPE &&
            (strncmp(storedFileName, directoryName, entry->fileNameLength - 1) == 0 || strcmp(storedFileName, directoryName) == 0)) // Use entry->fileNameLength - 1 to exclude null terminator
        {
            uint16_t blockNumber = entry->firstBlock;

            // // get the last [entry->fileNameLength-1] characters from directoryName, and make a recursive call
            // char *nextDirName = directoryName + entry->fileNameLength - 1;
            // printf("next dir name: %s\n", nextDirName);
            return findDirectory((DirectoryTable *)(fsMemoryBase + blockNumber * superBlock.blockSize), directoryName);
        }
    }

    return dirTable; // Directory not found
}


void printDirectoryContents(DirectoryTable *dir, const char *path)
{
    for (int i = 0; i < dir->fileCount; i++)
    {
        DirectoryEntry *entry = &dir->entries[i];
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        char fullPath[256];

        if (entry->entryType == FILE_TYPE)
        {
            if(entry->firstBlock == 0) continue;
            snprintf(fullPath, sizeof(fullPath), "%s/%s", path, storedFileName);
            printf("File: %s (Block: %u)\n", fullPath, entry->firstBlock);
            uint16_t currentBlock = entry->firstBlock;
            while (currentBlock != 0xFFFF && currentBlock < MAX_BLOCKS && currentBlock != 0)
            {
                printf("  Block %u: %s (Next: %u)\n", currentBlock, fullPath, superBlock.fat.fat[currentBlock]);
                currentBlock = superBlock.fat.fat[currentBlock];
            }
        }
        else if (entry->entryType == DIRECTORY_TYPE)
        {
            snprintf(fullPath, sizeof(fullPath), "%s", storedFileName);
            printf("Directory: %s (Block: %u)\n", fullPath, entry->firstBlock);
            uint16_t blockNumber = entry->firstBlock;
            while (blockNumber != 0xFFFF && blockNumber < MAX_BLOCKS && blockNumber != 0)
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
