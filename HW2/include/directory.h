#ifndef DIRECTORY_H
#define DIRECTORY_H

#define MAX_FILES 15 // Maximum number of files in a directory
#define MAX_BLOCKS 4096 // 4 MB file system

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>


extern char *fsMemoryBase; // Base pointer for file system memory
extern size_t totalFsSize;
extern char *fsMemory;


// Enum to define the type of entry: file or directory
typedef enum {
    FILE_TYPE,
    DIRECTORY_TYPE
} EntryType;

// Directory Entry structure
typedef struct {
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

// Directory Table structure
typedef struct {
    DirectoryEntry entries[MAX_FILES]; // Array of directory entries
    uint16_t fileCount;                // Number of files/directories in the directory
} DirectoryTable;

/**
 * Creates a new directory.
 *
 * @param fsBase The base pointer for the file system memory.
 * @param dirName The name of the new directory.
 * @param permissions The permissions for the new directory.
 * @param password The password for the new directory.
 * @return 0 on success, -1 on failure.
 */
int createDirectory(char *fsBase, const char *dirName, uint16_t permissions, const char *password);

/**
 * Deletes a directory from the file system.
 *
 * @param fsBase The base pointer for the file system memory.
 * @param dirName The name of the directory to delete.
 * @return 0 on success, -1 on failure.
 */
int deleteDirectory(char *fsBase, const char *dirName);

/**
 * Prints the details of a directory.
 *
 * @param dirName The name of the directory to print the details of.
 */
void printDirectoryDetails(char *dirName);

/**
 * Finds a directory by name.
 *
 * @param dir The starting directory table to search within.
 * @param directoryName The name of the directory to find.
 * @return A pointer to the directory table if found, NULL otherwise.
 */
DirectoryTable *findDirectory(DirectoryTable *dir, char *directoryName);

/**
 * Prints the contents of a directory.
 *
 * @param dir The directory table to print.
 * @param path The path of the directory.
 */
void printDirectoryContents(DirectoryTable *dir, const char *path);

/**
 * Counts the number of files and directories in a directory.
 *
 * @param dir The directory table to count within.
 * @param fileCount Pointer to store the number of files.
 * @param dirCount Pointer to store the number of directories.
 */
void countFilesAndDirectories(DirectoryTable *dir, uint16_t *fileCount, uint16_t *dirCount);

#endif // DIRECTORY_H
