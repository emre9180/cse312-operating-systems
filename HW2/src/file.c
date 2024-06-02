#include "../include/filesystem.h"
#include "../include/directory.h"
#include "../include/file.h"
#include "../include/utility.h"

void printFileDetails(char *directoryName, const char *fileName)
{
    DirectoryTable *dir = findDirectory(&superBlock.rootDirectory, directoryName);
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
            printf("Creation Date: %s", ctime((time_t *)&entry->creationDate));
            printf("Modification Date: %s", ctime((time_t *)&entry->modificationDate));
            printf("First Block: %u\n", entry->firstBlock);
            return;
        }
    }
    printf("File not found: %s\n", fileName);
}

int createFile(char *fsBase, char *directoryName, const char *fileName, uint16_t permissions, const char *password)
{
    DirectoryTable *dir = findDirectory(&superBlock.rootDirectory, directoryName);
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

    DirectoryEntry *newEntry = &dir->entries[dir->fileCount++];
    newEntry->entryType = FILE_TYPE;
    newEntry->fileNameOffset = superBlock.fileNameArea.used;
    newEntry->fileNameLength = fileNameLength;
    newEntry->size = 0;
    newEntry->permissions = permissions;
    newEntry->firstBlock = freeBlock;

    time_t currentTime = time(NULL);
    if (currentTime == (time_t)-1)
    {
        newEntry->creationDate = 0; // or some error value
    }
    else
    {
        // Safely cast to int32_t
        newEntry->creationDate = (int32_t)currentTime;
    }

    newEntry->modificationDate = newEntry->creationDate;
    strcpy(newEntry->password, password);

    char *fileNamePtr = fsMemoryBase + superBlock.fileNameArea.offset + superBlock.fileNameArea.used;
    memcpy(fileNamePtr, fileName, fileNameLength - 1); // Copy the filename without null terminator
    fileNamePtr[fileNameLength - 1] = '\0';            // Ensure null termination
    superBlock.fileNameArea.used += fileNameLength;

    setBlockUsed(freeBlock);
    superBlock.fat.fat[freeBlock] = 0xFFFF; // Initialize the FAT entry

    return 0; // Success
}

int deleteFile(const char *filePath)
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

    DirectoryTable *dir = findDirectory(&superBlock.rootDirectory, dirPath);

    if (dir == NULL)
    {
        fprintf(stderr, "Directory not found: %s\n", filePath);
        free(dirPath);
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
            free(dirPath);
            return 0; // Success
        }
    }
    printf("File not found: %s\n", fileName);
    free(dirPath);
    return -1; // File not found
}

int write(char *directoryName, char *linuxFileName, char *password)
{
    // Duplicate the file path to avoid modifying the original
    char *dirPath = strdup(directoryName);
    if (!dirPath)
    {
        perror("Failed to allocate memory");
        return -1;
    }

    // Count how many '/' characters are there in input
    int count = 0;
    for (long unsigned int i = 0; i < strlen(dirPath); i++)
    {
        if (dirPath[i] == '/')
        {
            count++;
        }
    }

    // Find the last occurrence of '/' in the file path
    char *fileName = strrchr(dirPath, '/');
    if (!fileName)
    {
        fprintf(stderr, "Invalid file path: %s\n", directoryName);
        free(dirPath);
        return -1;
    }

    // Split the path into directory and file name
    *fileName = '\0'; // Terminate the directory path
    fileName++;       // Move to the file name part

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
        free(dirPath);
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
        free(dirPath);
        return -1;
    }

    size_t read = fread(linuxFileContent, 1, linuxFileSize, linuxFile);
    if (read != linuxFileSize)
    {
        perror("Failed to read complete Linux file");
        free(linuxFileContent);
        free(dirPath);
        fclose(linuxFile);
        return -1;
    }
    fclose(linuxFile);

    // Create a new file in the target directory in our file system
    // char *fileName = strrchr(linuxFileName, '/');
    // fileName = fileName ? fileName + 1 : (char *)linuxFileName;

    // Find the directory and file entry
    DirectoryTable *dir;
    if (count != 1)
        dir = findDirectory(&superBlock.rootDirectory, dirPath);
    else
        dir = &superBlock.rootDirectory;

    if (count != 1 && dir == &superBlock.rootDirectory)
    {
        fprintf(stderr, "Invalid directory path: %s\n", dirPath);
        free(linuxFileContent);
        free(dirPath);
        return -1;
    }

    if (dir == NULL)
    {
        fprintf(stderr, "Directory not found: %s\n", dirPath);
        free(linuxFileContent);
        free(dirPath);
        return -1;
    }

    if (createFile(fsMemoryBase, dirPath, fileName, permissions, "none") != 0)
    {
        fprintf(stderr, "Failed to create file in directory: %s\n", dirPath);
        free(linuxFileContent);
        free(dirPath);
        return -1;
    }

    printf("File created successfully\n");

    DirectoryEntry *newEntry = findFileInDirectory(dir, fileName);
    if (!newEntry)
    {
        fprintf(stderr, "Failed to find new file entry in directory: %s\n", dirPath);
        free(linuxFileContent);
        free(dirPath);
        return -1;
    }

    // Check write permission
    if (!(newEntry->permissions & PERMISSION_WRITE))
    {
        fprintf(stderr, "Write permission denied for file: %s\n", fileName);
        free(linuxFileContent);
        free(dirPath);
        return -1;
    }

    if (strcmp(newEntry->password, "none") != 0 && strcmp(newEntry->password, password) != 0)
    {
        fprintf(stderr, "You did not provide correct password for file: %s\n", fileName);
        free(linuxFileContent);
        free(dirPath);
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
                free(dirPath);
                return -1;
            }
            printf("Allocating block %u -> %u\n", currentBlock, nextBlock);
            superBlock.fat.fat[currentBlock] = nextBlock;
            setBlockUsed(nextBlock);
            currentBlock = nextBlock;
            // increase file count
        }
        else
        {
            printf("Marking block %u as end of chain\n", currentBlock);
            superBlock.fat.fat[currentBlock] = 0xFFFF; // Mark as end of the chain
        }
    }

    newEntry->size = linuxFileSize;

    free(linuxFileContent);
    free(dirPath);
    return 0;
}

int read(const char *filePath, const char *linuxFileName, char *password)
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

    DirectoryTable *dir = findDirectory(&superBlock.rootDirectory, dirPath);

    if (dir == NULL)
    {
        fprintf(stderr, "Directory not found: %s\n", filePath);
        free(dirPath);
        return -1;
    }

    // Find the file in the directory
    DirectoryEntry *fileEntry = findFileInDirectory(dir, fileName);
    if (!fileEntry)
    {
        fprintf(stderr, "File not found: %s\n", fileName);
        free(dirPath);
        return -1;
    }

    // Check read permission
    if (!(fileEntry->permissions & PERMISSION_READ))
    {
        fprintf(stderr, "Read permission denied for file: %s\n", fileName);
        free(dirPath);
        return -1;
    }

    if (strcmp(fileEntry->password, "none") != 0 && strcmp(fileEntry->password, password) != 0)
    {
        fprintf(stderr, "You did not provide correct password for file: %s\n", fileName);
        free(dirPath);
        return -1;
    }

    // Open the Linux file for writing
    FILE *linuxFile = fopen(linuxFileName, "wb");
    if (!linuxFile)
    {
        perror("Failed to open Linux file for writing");
        free(dirPath);
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
        free(dirPath);
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
        free(dirPath);
        return -1;
    }
    free(dirPath);
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

int chmodFile(const char *filePath, uint16_t newPermissions, int addOrRemove, const char *password)
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

    DirectoryTable *dir = findDirectory(&superBlock.rootDirectory, dirPath);

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
            if (strcmp(entry->password, "none") != 0 && strcmp(entry->password, password) != 0)
            {
                fprintf(stderr, "You did not provide correct password for file: %s\n", fileName);
                free(dirPath);
                return -1;
            }

            time_t currentTime = time(NULL);
            if (currentTime == (time_t)-1)
            {
                entry->modificationDate = 0; // or some error value
            }
            else
            {
                // Safely cast to int32_t
                entry->modificationDate = (int32_t)currentTime;
            }

            // Update the file permissions based on addOrRemove flag
            if (addOrRemove == 0)
            {
                // Add the new permissions to the current ones
                entry->permissions |= newPermissions;
                free(dirPath);
                return 0;
            }
            else if (addOrRemove == 1)
            {
                // Remove the specified permissions from the current ones
                entry->permissions &= ~newPermissions;
                free(dirPath);
                return 0;
            }
        }
    }
    printf("File not found: %s\n", fileName);
    free(dirPath);
    return -1; // File not found
}

void addPassword(const char *filePath, const char *password, const char *filePassword)
{
    // Duplicate the file path to avoid modifying the original
    char *dirPath = strdup(filePath);
    if (!dirPath)
    {
        perror("Failed to allocate memory");
        return;
    }

    // Find the last occurrence of '/' in the file path
    char *fileName = strrchr(dirPath, '/');
    if (!fileName)
    {
        fprintf(stderr, "Invalid file path: %s\n", filePath);
        free(dirPath);
        return;
    }

    // Split the path into directory and file name
    *fileName = '\0'; // Terminate the directory path
    fileName++;       // Move to the file name part

    if (strlen(dirPath) == 0)
    {
        fprintf(stderr, "Invalid directory path: %s\n", filePath);
        free(dirPath);
        return;
    }

    DirectoryTable *dir = findDirectory(&superBlock.rootDirectory, dirPath);

    if (dir == NULL)
    {
        fprintf(stderr, "Directory not found: %s\n", filePath);
        return;
    }

    // Find the file in the directory
    for (int i = 0; i < dir->fileCount; i++)
    {
        DirectoryEntry *entry = &dir->entries[i];
        char *storedFileName = fsMemoryBase + superBlock.fileNameArea.offset + entry->fileNameOffset;
        if (strncmp(storedFileName, fileName, entry->fileNameLength) == 0)
        {
            if (strcmp(entry->password, "none") != 0 && strcmp(entry->password, filePassword) != 0)
            {
                fprintf(stderr, "You did not provide correct password for file: %s\n", fileName);
                free(dirPath);
                return;
            }

            // Update the file password
            strncpy(entry->password, password, sizeof(entry->password) - 1);

            time_t currentTime = time(NULL);
            if (currentTime == (time_t)-1)
            {
                entry->modificationDate = 0; // or some error value
            }
            else
            {
                // Safely cast to int32_t
                entry->modificationDate = (int32_t)currentTime;
            }
            free(dirPath);
            return; // Success
        }
    }
    printf("File not found: %s\n", fileName);
    free(dirPath);
}
