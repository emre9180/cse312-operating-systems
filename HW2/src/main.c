
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

    size_t fileNameAreaSize = MAX_FILES * sizeof(DirectoryEntry);

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
            return 1;
        }

        if(blockSize!= 512 && blockSize != 1024)
        {
            fprintf(stderr, "Invalid block size. Allowed block size are only 512 Bytes and 1024 Bytes. Give the input as bytes, not KB or MB\n");
        }
        initializeFileSystem(blockSize, fsMemory, totalFsSize);
        // strcpy(data, words[2]);
        printf("File system created and saved to %s\n", words[2]);
        printf("Available commands are: dir, mkdir, rmdir, dumpe2fs, write, read, del, chmod, addpw\n");
        printf("\n");
        // loaded = 0;
        // createDirectory(fsMemory, "/documents", PERMISSION_READ | PERMISSION_WRITE, "none");
        // createDirectory(fsMemory, "/documents/images", PERMISSION_READ | PERMISSION_WRITE, "none");
        // createDirectory(fsMemory, "/documents/audio", PERMISSION_READ | PERMISSION_WRITE, "none");
        // createFile(fsMemory, "/documents", "example.txt", PERMISSION_WRITE, "none");
        // // createFile(fsMemory, "/documents/images", "example2.txt", PERMISSION_WRITE, "none");
        // chmodFile("/documents/example.txt", PERMISSION_READ | PERMISSION_WRITE, 0);
        
        // read("/documents/example.txt", "test.txt", "none");


        // write("/documents/images/newwrite.txt", "test2.txt", "none");
        // addPassword("/documents/images/newwrite.txt", "password123");
        // read("/documents/images/newwrite.txt", "test3.txt", "password1223");
        // // createDirectory(fsMemory, "/root", PERMISSION_READ | PERMISSION_WRITE, "none");
        // dumpe2fs();
        // loaded = 1;
        saveFileSystem(words[2]);
        return 0;
    }

    else if(strcmp(words[0], "fileSystemOper") == 0)
    {
        loadFileSystem(words[1]);
    }

    else if(strcmp(words[0], "fileSystemOper") != 0)
    {
        printf("Invalid Command!\n");
        return 0;
    }

    if (strcmp(words[2], "dir") == 0)
    {
        if(wordCount != 4)
        {
            printf("Invalid command\n");
            return 0;
        }
        printDirectoryDetails(words[3]);
    }

    else if (strcmp(words[2], "mkdir") == 0)
    {
        if(wordCount != 4)
        {
            printf("Invalid command\n");
            return 0;
        }
        createDirectory(fsMemory, words[3], PERMISSION_READ | PERMISSION_WRITE, "none");
    }

    else if (strcmp(words[2], "rmdir") == 0)
    {
        if(wordCount != 4)
        {
            printf("Invalid command\n");
            return 0;
        }
        deleteDirectory(fsMemory, words[3]);
    }

    else if (strcmp(words[2], "dumpe2fs") == 0)
    {
        if(wordCount != 3)
        {
            printf("Invalid command\n");
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
            return 0;
        }
        deleteFile(words[3]);
    }

    else if (strcmp(words[2], "chmod") == 0)
    {
        if(wordCount != 5)
        {
            printf("Invalid command\n");
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
        chmodFile(words[3], permissions, removeOrAdd);
    }

    else if (strcmp(words[2], "addpw") == 0)
    {
        if(wordCount != 5)
        {
            printf("Invalid command\n");
            return 0;
        }
        addPassword(words[3], words[4]);
    }

    else if (strcmp(words[2], "exit") == 0)
    {
        return 0;
    }

    else
    {
        printf("Invalid command\n");
    }
    
    saveFileSystem(words[1]);
    free(fsMemory);
    return 0;
}
