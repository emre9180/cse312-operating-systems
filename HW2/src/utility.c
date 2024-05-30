#include "../include/filesystem.h"
#include "../include/directory.h"
#include "../include/file.h"
#include "../include/utility.h"


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

uint16_t findFreeBlock()
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
