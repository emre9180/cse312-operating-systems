
#include "../include/filesystem.h"
#include "../include/directory.h"
#include "../include/file.h"
#include "../include/utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


size_t totalFsSize;
char *fsMemory;

int main(int argc, char *argv[])
{

    char *executableName = argv[0];
    executableName += 2;

    size_t superBlockSize = sizeof(SuperBlock);
    size_t directoryTableSize = sizeof(DirectoryTable);
    size_t freeBlockBitmapSize = sizeof(FreeBlockBitmap);

    size_t fatBits = MAX_BLOCKS * FAT_ENTRY_BITS; // Total bits for FAT12
    size_t fatBytes = (fatBits + 7) / 8;          // Convert bits to bytes, rounding up

    size_t fileNameAreaSize = 128 * sizeof(DirectoryEntry);

    size_t dataBlocksSize = MAX_BLOCKS * BLOCK_SIZE;
    totalFsSize = superBlockSize + directoryTableSize + freeBlockBitmapSize + fatBytes + fileNameAreaSize + dataBlocksSize;


    fsMemory = malloc(totalFsSize);
    if (!fsMemory)
    {
        perror("Failed to allocate memory for file system");
        exit(EXIT_FAILURE);
    }

    // Split the input into separate words
    char *words[6]; // Assuming at most 5 words
    int wordCount = argc;

    if (wordCount > 6 || wordCount < 2)
    {
        printf("Invalid command\n");
        free(fsMemory);
        return 1;
    }

    for (int i = 0; i < wordCount; i++)
    {
        if(i==0)
            words[i] = executableName;
        else
            words[i] = argv[i];
    }

    // remove new line character at the end of the last words
    // words[wordCount - 1][strlen(words[wordCount - 1]) - 1] = '\0';

    // Now you can access the individual words using the words array
    for (int i = 0; i < wordCount; i++)
    {
        // printf("Word %d: %s\n", i + 1, words[i]);
    }

    // Avaliable commands are: dir, mkdir, rmdir, dumpe2fs, write, read, del, chmod, addpw

    if (strcmp(words[0], "makeFileSystem") == 0)
    {
        uint16_t blockSize = (uint16_t)atoi(words[1]);
        if (blockSize == 0 || blockSize > BLOCK_SIZE)
        {
            fprintf(stderr, "Invalid block size. Allowed block size are only 512 Bytes and 1024 Bytes. Give the input as bytes, not KB or MB\n");
            free(fsMemory);
            return 1;
        }

        if(blockSize!= 512 && blockSize != 1024)
        {
            fprintf(stderr, "Invalid block size. Allowed block size are only 512 Bytes and 1024 Bytes. Give the input as bytes, not KB or MB\n");
        }
        initializeFileSystem(blockSize, fsMemory, totalFsSize);
        printf("Total size of superblock: %ld bytes\n", sizeof(SuperBlock));
        printf("Total size of Root Directory Table: %ld bytes\n", sizeof(DirectoryTable));
        printf("Total size of Free Block Bitmap: %ld bytes\n", sizeof(FreeBlockBitmap));
        printf("Total size of FAT: %ld bytes\n", sizeof(FAT));

        long long int totalDataAreaSize = (long long int)superBlock.fileNameArea.size + superBlock.dataArea.size;
        printf("Total size of Data Area (File Name Area + Data Blocks): %lld bytes (%lld MB)\n", totalDataAreaSize, totalDataAreaSize / (1024 * 1024));

        long long int totalFileSystemSize = totalDataAreaSize + sizeof(SuperBlock) + sizeof(DirectoryTable) + sizeof(FreeBlockBitmap) + sizeof(FAT);
        printf("Total size of File System: %lld bytes\n", totalFileSystemSize);

        printf("Available commands are: dir, mkdir, rmdir, dumpe2fs, write, read, del, chmod, addpw\n");
        printf("\n");

        saveFileSystem(words[2]);
        free(fsMemory);
        return 0;
    }

    else if(strcmp(words[0], "fileSystemOper") == 0)
    {
        loadFileSystem(words[1]);
    }

    else if(strcmp(words[0], "fileSystemOper") != 0)
    {
        printf("Invalid Command!\n");
        free(fsMemory);
        return 0;
    }

    if (strcmp(words[2], "dir") == 0)
    {
        if(wordCount != 4)
        {
            printf("Invalid command\n");
            free(fsMemoryBase);
            free(fsMemory);
            return 0;
        }
        printDirectoryDetails(words[3]);
    }

    else if (strcmp(words[2], "mkdir") == 0)
    {
        if(wordCount != 4)
        {
            printf("Invalid command\n");
            free(fsMemory);
            free(fsMemoryBase);
            return 0;
        }
        createDirectory(fsMemory, words[3], PERMISSION_READ | PERMISSION_WRITE, "none");
    }

    else if (strcmp(words[2], "rmdir") == 0)
    {
        if(wordCount != 4)
        {
            printf("Invalid command\n");
            free(fsMemory);
            free(fsMemoryBase);
            return 0;
        }
        int result = deleteDirectory(fsMemory, words[3]);
        if(result==-1)
        {
            printf("Directory not found\n");
        }
    }

    else if (strcmp(words[2], "dumpe2fs") == 0)
    {
        if(wordCount != 3)
        {
            printf("Invalid command\n");
            free(fsMemory);
            free(fsMemoryBase);
            return 0;
        }
        dumpe2fs();
    }

    else if (strcmp(words[2], "write") == 0)
    {
        if (wordCount == 5)
            write(words[3], words[4], "none");
        else if(wordCount == 6)
            write(words[3], words[4], words[5]);
        else
            printf("Invalid command\n");
    }

    else if (strcmp(words[2], "read") == 0)
    {
        if (wordCount == 5)
            read(words[3], words[4], "none");
        else if(wordCount == 6)
            read(words[3], words[4], words[5]);
        else
            printf("Invalid command\n");
    }

    else if (strcmp(words[2], "del") == 0)
    {
        if(wordCount != 4)
        {
            printf("Invalid command\n");
            free(fsMemory);
            free(fsMemoryBase);
            return 0;
        }
        deleteFile(words[3]);
    }

    else if (strcmp(words[2], "chmod") == 0)
    {
        if(wordCount != 5 && wordCount != 6)
        {
            printf("Invalid command\n");
            free(fsMemory);
            free(fsMemoryBase);
            return 0;
        }

        int removeOrAdd = 0;
        int permissions = 0;

        // Check if read permission is present
        if (strchr(words[4], 'r') != NULL)
        {
            permissions |= PERMISSION_READ;
        }

        // Check if write permission is present
        if (strchr(words[4], 'w') != NULL)
        {
            permissions |= PERMISSION_WRITE;
        }

        // Check if write permission is present
        if (strchr(words[4], '+') != NULL)
        {
            removeOrAdd = 0;
        }

        // Check if write permission is present
        if (strchr(words[4], '-') != NULL)
        {
            removeOrAdd = 1;
        }

        if(wordCount == 5)
            chmodFile(words[3], permissions, removeOrAdd, "none");
        if(wordCount == 6)
            chmodFile(words[3], permissions, removeOrAdd, words[5]);
    }

    else if (strcmp(words[2], "addpw") == 0)
    {
        if(wordCount != 5 && wordCount != 6)
        {
            printf("Invalid command\n");
            free(fsMemory);
            free(fsMemoryBase);
            return 0;
        }
        if(wordCount == 5)
            addPassword(words[3], words[4], "none");
        if(wordCount == 6)
            addPassword(words[3], words[4], words[5]);
    }

    else if (strcmp(words[2], "exit") == 0)
    {
        free(fsMemory);
        free(fsMemoryBase);
        return 0;
    }

    else
    {
        printf("Invalid command\n");
    }
    
    saveFileSystem(words[1]);
    free(fsMemory);
    free(fsMemoryBase);
    return 0;
}
