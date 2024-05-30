
#include "../include/filesystem.h"
#include "../include/directory.h"
#include "../include/file.h"
#include "../include/utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


size_t totalFsSize;
char *fsMemory;

int main()
{
    printf("Hello, World!\n");
    char data[100];
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

    int loaded = 0;
    // Check if the file exists
    // FILE *file = fopen(argv[2], "rb");
    // if (file)
    // {
    //     fclose(file);
    //     // Load the existing file system
    //     if (loadFileSystem(argv[2]) != 0)
    //     {
    //         fprintf(stderr, "Failed to load file system from %s\n", argv[2]);
    //         free(fsMemory);
    //         return 1;
    //     }
    //     printf("File system loaded from %s\n", argv[2]);
    // }
    // else
    // {

    //     // Initialize a new file system
    //     initializeFileSystem(blockSize, fsMemory, totalFsSize);
    //     createDirectory(fsMemory, "/documents", PERMISSION_READ | PERMISSION_WRITE, "none");
    //     createDirectory(fsMemory, "/documents/images", PERMISSION_READ | PERMISSION_WRITE, "none");
    //     // createDirectory(fsMemory, "/info", PERMISSION_READ | PERMISSION_WRITE, "none");
    //     // createDirectory(fsMemory, "/asdf", PERMISSION_READ | PERMISSION_WRITE, "none");
    //     createDirectory(fsMemory, "/documents/audio", PERMISSION_READ | PERMISSION_WRITE, "none");
    //     // // printDirectoryDetails("/documents/images");
    //     createFile(fsMemory, "/documents", "example.txt", PERMISSION_WRITE, "none");
    //     createFile(fsMemory, "/documents/images", "example2.txt", PERMISSION_WRITE, "none");
    //     printDirectoryDetails("/documents");
    //     // // // createFile(fsMemory, "documents", "test2.txt", PERMISSION_READ | PERMISSION_WRITE, "none");
    //     // // // Save the newly created file system

    //     // if (write("/documents/images", "test2.txt") == 0)
    //     // {
    //     //     printf("File 'test2.txt' successfully written to 'documents' directory in the file system.\n");
    //     // }

    //     // if (read("/documents/images/test2.txt", "copied_linux_file.txt") == 0)
    //     // {
    //     //     printf("File 'documents/example.txt' successfully read to 'copied_linux_file.txt' in the Linux file system.\n");
    //     // }

    //     // deleteDirectory(fsMemory, "/documents/images");

    //     // dumpe2fs();
    //     // createFile(fsMemory, "documents", "example2.txt", PERMISSION_READ | PERMISSION_WRITE, "password123");

    //     if (saveFileSystem(argv[2]) != 0)
    //     {
    //         fprintf(stderr, "Failed to save file system to %s\n", argv[2]);
    //         free(fsMemory);
    //         return 1;
    //     }

    //     // printf("New file system created and saved to %s\n", argv[2]);
    // }

    // // Print the details of "documents"
    // // printDirectoryDetails("documents");
    // // printf("\n");

    // // // Use the write function to copy a Linux file to the file system

    // // chmodFile(fsMemory, "documents/test2.txt", PERMISSION_WRITE);
    // // printFileDetails("documents", "test2.txt");
    // // printFileDetails("documents", "example.txt");

    // // printFileDetails("documents", "example2.txt");

    // // printf("\n");
    // // if (read("documents/test2.txt", "copied_linux_file.txt") == 0)
    // // {
    // //     printf("File 'documents/example.txt' successfully read to 'copied_linux_file.txt' in the Linux file system.\n");
    // // }

    // // // deleteFile(fsMemory, "documents/example.txt");
    // // // Clean up
    // // dumpe2fs();

    // // Get command from user

    while (1)
    {
        char command[100];
        printf("\nEnter command: ");
        fgets(command, sizeof(command), stdin);

        // Split the input into separate words
        char *token = strtok(command, " ");
        char *words[6]; // Assuming at most 5 words
        int wordCount = 0;

        while (token != NULL && wordCount < 6)
        {
            words[wordCount++] = token;
            token = strtok(NULL, " ");
        }

        if (wordCount > 6 || wordCount < 3)
        {
            printf("Invalid command\n");
            continue;
        }

        // remove new line character at the end of the last words
        words[wordCount - 1][strlen(words[wordCount - 1]) - 1] = '\0';

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
                fprintf(stderr, "Invalid block size\n");
                return 1;
            }
            initializeFileSystem(blockSize, fsMemory, totalFsSize);
            strcpy(data, words[2]);
            printf("File system created and saved to %s\n", words[2]);
            printf("You can continue to use the file system\n");
            printf("Available commands are: dir, mkdir, rmdir, dumpe2fs, write, read, del, chmod, addpw\n");
            printf("Enter 'exit' to exit the program\n");
            printf("\n");
            loaded = 0;
            createDirectory(fsMemory, "/documents", PERMISSION_READ | PERMISSION_WRITE, "none");
            createDirectory(fsMemory, "/documents/images", PERMISSION_READ | PERMISSION_WRITE, "none");
            createDirectory(fsMemory, "/documents/audio", PERMISSION_READ | PERMISSION_WRITE, "none");
            createFile(fsMemory, "/documents", "example.txt", PERMISSION_WRITE, "none");
            // createFile(fsMemory, "/documents/images", "example2.txt", PERMISSION_WRITE, "none");
            chmodFile("/documents/example.txt", PERMISSION_READ | PERMISSION_WRITE, 0);
            
            read("/documents/example.txt", "test.txt", "none");


            write("/documents/images", "test2.txt", "none");
            deleteFile("/documents/images/test2.txt");
            // createDirectory(fsMemory, "/root", PERMISSION_READ | PERMISSION_WRITE, "none");
            dumpe2fs();
            loaded = 1;
            saveFileSystem(words[2]);
            continue;
        }

        else if(strcmp(words[0], "fileSystemOper") == 0)
        {
            if (!loaded)
            {
                loadFileSystem(data);
                loaded = 1;
            }

            else if(strcmp(words[1], data)!=0)
            {
                printf("Loading new file system\n");
                loadFileSystem(words[0]);
                strcpy(data, words[0]);
                loaded = 1;
            }
        }

        else if(strcmp(words[0], "fileSystemOper") != 0)
        {
            printf("Invalid Command!\n");
            continue;
        }



        if (strcmp(words[2], "dir") == 0)
        {
            if(wordCount != 4)
            {
                printf("Invalid command\n");
                continue;
            }

            printDirectoryDetails(words[3]);
        }

        else if (strcmp(words[2], "mkdir") == 0)
        {
            if(wordCount != 4)
            {
                printf("Invalid command\n");
                continue;
            }

            createDirectory(fsMemory, words[3], PERMISSION_READ | PERMISSION_WRITE, "none");
        }

        else if (strcmp(words[2], "rmdir") == 0)
        {
            if(wordCount != 4)
            {
                printf("Invalid command\n");
                continue;
            }

            deleteDirectory(fsMemory, words[3]);
        }

        else if (strcmp(words[2], "dumpe2fs") == 0)
        {
            if(wordCount != 3)
            {
                printf("Invalid command\n");
                continue;
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
                continue;
            }

            deleteFile(words[3]);
        }

        else if (strcmp(words[2], "chmod") == 0)
        {
            if(wordCount != 5)
            {
                printf("Invalid command\n");
                continue;
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
                continue;
            }

            addPassword(words[3], words[4]);
        }

        else if (strcmp(words[2], "exit") == 0)
        {
            break;
        }

        else
        {
            printf("Invalid command\n");
        }

        // Clean words
        for (int i = 0; i < wordCount; i++)
        {
            words[i] = NULL;
        }
    }

    free(fsMemory);
    return 0;
}
