#ifndef UTILITY_H
#define UTILITY_H

#include "filesystem.h"

/**
 * Sets a block as free in the free block bitmap.
 *
 * @param blockNumber The block number to set as free.
 */
void setBlockFree(uint16_t blockNumber);

/**
 * Sets a block as used in the free block bitmap.
 *
 * @param blockNumber The block number to set as used.
 */
void setBlockUsed(uint16_t blockNumber);

/**
 * Finds a free block in the file system.
 *
 * @return The number of the free block, or (uint16_t)-1 if no free block is found.
 */
uint16_t findFreeBlock();

/**
 * Prints Date of the file
 * @param date The date
 * @param flag 0 for creation, 1 for modification date
*/
void printDate(uint32_t date, int flag);

#endif // UTILITY_H
