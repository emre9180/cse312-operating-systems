#ifndef FILE_H
#define FILE_H

#include "filesystem.h"
#include "directory.h"

/**
 * Creates a new file in the specified directory.
 *
 * @param fsBase The base pointer for the file system memory.
 * @param directoryName The name of the directory to create the file in.
 * @param fileName The name of the new file.
 * @param permissions The permissions for the new file.
 * @param password The password for the new file.
 * @return 0 on success, -1 on failure.
 */
int createFile(char *fsBase, char *directoryName, const char *fileName, uint16_t permissions, const char *password);

/**
 * Deletes a file from the file system.
 *
 * @param fileName The name of the file to delete.
 * @return 0 on success, -1 on failure.
 */
int deleteFile(const char *fileName);

/**
 * Prints the details of a file.
 *
 * @param directoryName The name of the directory containing the file.
 * @param fileName The name of the file.
 */
void printFileDetails(char *directoryName, const char *fileName);

/**
 * Writes a file from the Linux file system to the file system.
 *
 * @param directoryName The name of the directory to write the file to.
 * @param linuxFileName The name of the Linux file to write.
 * @param password The password for the file.
 * @return 0 on success, -1 on failure.
 */
int write(char *directoryName, char *linuxFileName, char *password);

/**
 * Reads a file from the file system to the Linux file system.
 *
 * @param filePath The path of the file in the file system.
 * @param linuxFileName The name of the Linux file to write to.
 * @param password The password for the file.
 * @return 0 on success, -1 on failure.
 */
int read(const char *filePath, const char *linuxFileName, char *password);

/**
 * Finds a file in a directory.
 *
 * @param dir The directory table to search within.
 * @param fileName The name of the file to find.
 * @return A pointer to the directory entry if found, NULL otherwise.
 */
DirectoryEntry *findFileInDirectory(DirectoryTable *dir, const char *fileName);

/**
 * Changes the permissions of a file.
 *
 * @param filePath The path of the file.
 * @param newPermissions The new permissions to set.
 * @param addOrRemove Whether to add or remove the permissions.
 * @return 0 on success, -1 on failure.
 */
int chmodFile(const char *filePath, uint16_t newPermissions, int addOrRemove);

/**
 * Adds a password to a file.
 *
 * @param filePath The path of the file.
 * @param password The password to add.
 */
void addPassword(const char *filePath, const char *password);

#endif // FILE_H
