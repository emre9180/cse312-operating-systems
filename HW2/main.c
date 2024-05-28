#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t totalFsSize;
char *fsMemory;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <blockSize> <fileName>\n", argv[0]);
        return 1;
    }

    uint16_t blockSize = (uint16_t)atoi(argv[1]);
    if (blockSize == 0 || blockSize > BLOCK_SIZE)
    {
        fprintf(stderr, "Invalid block size\n");
        return 1;
    }

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

    // Check if the file exists
    FILE *file = fopen(argv[2], "rb");
    if (file)
    {
        fclose(file);
        // Load the existing file system
        if (loadFileSystem(argv[2]) != 0)
        {
            fprintf(stderr, "Failed to load file system from %s\n", argv[2]);
            free(fsMemory);
            return 1;
        }
        printf("File system loaded from %s\n", argv[2]);
    }
    else
    {
        // Initialize a new file system
        initializeFileSystem(blockSize, fsMemory, totalFsSize);
        createDirectory(fsMemory, "documents", PERMISSION_READ | PERMISSION_WRITE, "password123");
        createFile(fsMemory, "documents", "example.txt", PERMISSION_WRITE, "password123");
        // createFile(fsMemory, "documents", "test2.txt", PERMISSION_READ | PERMISSION_WRITE, "password123");
        // Save the newly created file system

        if (write("documents", "test2.txt") == 0)
        {
            printf("File 'test.txt' successfully written to 'documents' directory in the file system.\n");
        }

        createFile(fsMemory, "documents", "example2.txt", PERMISSION_READ | PERMISSION_WRITE, "password123");

        if (saveFileSystem(argv[2]) != 0)
        {
            fprintf(stderr, "Failed to save file system to %s\n", argv[2]);
            free(fsMemory);
            return 1;
        }

        printf("New file system created and saved to %s\n", argv[2]);
    }

    // Print the details of "documents"
    printDirectoryDetails("documents");
    printf("\n");

    // Use the write function to copy a Linux file to the file system

    chmodFile(fsMemory, "documents/example.txt", PERMISSION_READ);
    printFileDetails("documents", "test2.txt");
    printFileDetails("documents", "example.txt");

    printFileDetails("documents", "example2.txt");

    printf("\n");
    if (read("documents/test2.txt", "copied_linux_file.txt") == 0)
    {
        printf("File 'documents/example.txt' successfully read to 'copied_linux_file.txt' in the Linux file system.\n");
    }

    // deleteFile(fsMemory, "documents/example.txt");
    // Clean up
    dumpe2fs();
    free(fsMemory);
    return 0;
}
