#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * BEFORE GETTING STARTED:
 *
 * Familiarize yourself with the functions and constants/variables
 * in the following included files.
 * This will make the project a LOT easier as you go!!
 *
 * The diagram in Section 4.1 (Specification) of the handout will help you
 * understand the constants in mm.h
 * Section 4.2 (Support Routines) of the handout has information about
 * the functions in mminline.h and memlib.h
 */
#include "./memlib.h"
#include "./mm.h"
#include "./mminline.h"

block_t *prologue;
block_t *epilogue;

/*
 *
 * coalesces neighboring free blocks
 * arguments: my current free block
 * returns: void method, but will merge free blocks if neighboring
 */
void coalesce(block_t *myBlock) {
    block_t *previousBlock = block_prev(myBlock);
    block_t *nextBlock = block_next(myBlock);
    // if next block is free
    if (block_next_allocated(myBlock) == 0) {
        pull_free_block(nextBlock);
        block_set_size(myBlock, (block_size(nextBlock) + block_size(myBlock)));
    }
    // if previous block is free
    if (block_prev_allocated(myBlock) == 0) {
        pull_free_block(myBlock);
        block_set_size(previousBlock,
                       (block_size(previousBlock) + block_size(myBlock)));
    }
}

// rounds up to the nearest multiple of WORD_SIZE
static inline size_t align(size_t size) {
    return (((size) + (WORD_SIZE - 1)) & ~(WORD_SIZE - 1));
}

/*
 *                             _       _ _
 *     _ __ ___  _ __ ___     (_)_ __ (_) |_
 *    | '_ ` _ \| '_ ` _ \    | | '_ \| | __|
 *    | | | | | | | | | | |   | | | | | | |_
 *    |_| |_| |_|_| |_| |_|___|_|_| |_|_|\__|
 *                       |_____|
 *
 * initializes the dynamic storage allocator (allocate initial heap space)
 * arguments: none
 * returns: 0, if successful
 *         -1, if an error occurs
 */
int mm_init(void) {
    // initializes prologue and epilogue memory
    if ((prologue = mem_sbrk(16)) == (void *)-1) {
        fprintf(stderr, "mem_sbrk");
        return -1;
    }
    if ((epilogue = mem_sbrk(16)) == (void *)-1) {
        fprintf(stderr, "mem_sbrk");
        return -1;
    }
    // sets block size and allocation
    block_set_size_and_allocated(prologue, TAGS_SIZE, 1);
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
    flist_first = NULL;
    return 0;
}

/*     _ __ ___  _ __ ___      _ __ ___   __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '_ ` _ \ / _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | | | | | (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_| |_| |_|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * allocates a block of memory and returns a pointer to that block's payload
 * arguments: size: the desired payload size for the block
 * returns: a pointer to the newly-allocated block's payload (whose size
 *          is a multiple of ALIGNMENT), or NULL if an error occurred
 */
void *mm_malloc(size_t size) {
    // TODO
    if (size <= 0) {
        fprintf(stderr, "size is 0 or below");
        return NULL;
    }
    // align the size at the beginning to avoid any weird errors
    size_t minimumSize = 24;
    size_t alignedSize = align(size);
    size_t newBlockSize = (alignedSize + TAGS_SIZE);
    block_t *freeBlock;
    freeBlock = flist_first;
    // loop through free list
    while (flist_first != NULL) {
        size_t freeBlockSize = block_size(freeBlock);
        // size_t freeBlockPayload = block_size(freeBlock)-(TAGS_SIZE);
        if (freeBlockSize >= newBlockSize) {
            // make sure that split block will then be >= minimum size or will
            // fit exactly leftover size of block must be at least big enough to
            // fit the tags and wordsize (24 bytes)
            size_t leftOverSize = freeBlockSize - newBlockSize;
            // perfect size
            if (leftOverSize == 0) {
                pull_free_block(freeBlock);
                block_set_size_and_allocated(freeBlock, newBlockSize, 1);
                return &freeBlock->payload[0];
            }
            // able to split
            else if (leftOverSize >= minimumSize) {
                pull_free_block(freeBlock);
                block_set_size_and_allocated(freeBlock, newBlockSize, 1);
                block_t *splitBlock = block_next(freeBlock);
                // size_t alignedLeftoverSize = align(leftOverSize-TAGS_SIZE);
                block_set_size_and_allocated(splitBlock, leftOverSize, 0);

                // pull_free_block(splitBlock);
                insert_free_block(splitBlock);
                coalesce(splitBlock);
                return &freeBlock->payload[0];
            }
            // cannot split, then will take up the entire free block
            else {
                pull_free_block(freeBlock);
                block_set_size_and_allocated(freeBlock, freeBlockSize, 1);
                return &freeBlock->payload[0];
            }
        }
        // if I have looped through the entire free list, then break
        if (block_flink(freeBlock) == flist_first) {
            break;
        }
        freeBlock = block_flink(freeBlock);
    }

    if (block_prev_allocated(epilogue) == 0) {
        block_t *previousFreeBlock = block_prev(epilogue);
        block_t *newBlock;
        size_t sizeToExtend = newBlockSize - block_size(previousFreeBlock);
        if ((newBlock = mem_sbrk(sizeToExtend)) == (void *)-1) {
            fprintf(stderr, "mem_sbrk");
            return (void *)-1;
        }
        // reset the address of my new block to behind the epilogue
        newBlock = (block_t *)((char *)newBlock - block_size(epilogue));

        block_set_size_and_allocated(newBlock, sizeToExtend, 0);
        insert_free_block(newBlock);
        // move epilogue foward
        epilogue = (block_t *)((char *)epilogue + block_size(newBlock));
        block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
        coalesce(previousFreeBlock);
        pull_free_block(previousFreeBlock);
        // set previous free block
        block_set_size_and_allocated(previousFreeBlock, newBlockSize, 1);

        return &previousFreeBlock->payload[0];
    }

    size_t extraAlignedSize = 128;

    // if chunk size is greater than precise size
    if (extraAlignedSize > newBlockSize) {
        if ((extraAlignedSize - newBlockSize) >= (int)MINBLOCKSIZE) {
            // now check for extending the heap
            block_t *newBlock;
            if ((newBlock = mem_sbrk(extraAlignedSize)) == (void *)-1) {
                fprintf(stderr, "mem_sbrk");
                return (void *)-1;
            }
            // reset the address of my new block to behind the epilogue
            newBlock = (block_t *)((char *)newBlock - block_size(epilogue));
            // set block as huge free block
            block_set_size_and_allocated(newBlock, extraAlignedSize, 1);
            // move epilogue foward
            epilogue = (block_t *)((char *)epilogue + block_size(newBlock));
            block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
            // now focus on splitting the new block
            block_set_size(newBlock, newBlockSize);

            block_t *extraSpaceBlock = block_next(newBlock);
            block_set_size_and_allocated(extraSpaceBlock,
                                         extraAlignedSize - newBlockSize, 0);
            insert_free_block(extraSpaceBlock);
            coalesce(extraSpaceBlock);
            return &newBlock->payload[0];
        }
    }

    // now check for extending the heap
    block_t *newBlock;
    if ((newBlock = mem_sbrk(newBlockSize)) == (void *)-1) {
        fprintf(stderr, "mem_sbrk");
        return (void *)-1;
    }
    // reset the address of my new block to behind the epilogue
    newBlock = (block_t *)((char *)newBlock - block_size(epilogue));
    // set block as huge free block
    block_set_size_and_allocated(newBlock, (newBlockSize), 1);
    // move epilogue foward
    epilogue = (block_t *)((char *)epilogue + block_size(newBlock));
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);

    return &newBlock->payload[0];
}

/*                              __
 *     _ __ ___  _ __ ___      / _|_ __ ___  ___
 *    | '_ ` _ \| '_ ` _ \    | |_| '__/ _ \/ _ \
 *    | | | | | | | | | | |   |  _| | |  __/  __/
 *    |_| |_| |_|_| |_| |_|___|_| |_|  \___|\___|
 *                       |_____|
 *
 * frees a block of memory, enabling it to be reused later
 * arguments: ptr: pointer to the block's payload
 * returns: nothing
 */
void mm_free(void *ptr) {
    if (ptr != NULL) {
        block_t *myFreeBlock = payload_to_block(ptr);
        block_set_allocated(myFreeBlock, 0);
        insert_free_block(myFreeBlock);
        coalesce(myFreeBlock);
    }
}

/*
 *                                            _ _
 *     _ __ ___  _ __ ___      _ __ ___  __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '__/ _ \/ _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | |  __/ (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_|  \___|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * reallocates a memory block to update it with a new given size
 * arguments: ptr: a pointer to the memory block's payload
 *            size: the desired new payload size
 * returns: a pointer to the new memory block's payload
 */
void *mm_realloc(void *ptr, size_t size) {
    // TODO
    // edge cases
    if (ptr == NULL) {
        return mm_malloc(size);
    }

    if (size == 0) {
        return NULL;
    }
    // keeping tracks of sizes
    size_t newAlignedSize = align(size);
    size_t requestedSize = newAlignedSize + TAGS_SIZE;
    block_t *myBlock = payload_to_block(ptr);
    size_t originalSize = block_size(myBlock);
    size_t availableSpace = originalSize;

    // if I am shrinking, then can coalesce if minimum size is not found, if no
    // free blocks and not minimum size then don't do anything
    if (requestedSize < originalSize) {
        // check for surrounding free blocks
        // check next block is free
        if (block_next_allocated(myBlock) == 0) {
            availableSpace = availableSpace + block_next_size(myBlock);
            // set sizing
            pull_free_block(block_next(myBlock));
            block_set_size_and_allocated(myBlock, requestedSize, 1);
            block_t *splitBlock = block_next(myBlock);
            block_set_size_and_allocated(splitBlock,
                                         (availableSpace - requestedSize), 0);
            insert_free_block(splitBlock);
            coalesce(splitBlock);
            // set surrounding block size
            block_set_size(block_prev(myBlock), block_prev_size(myBlock));
            block_set_size_and_allocated(myBlock, (requestedSize), 1);
            block_set_size(block_next(myBlock), block_next_size(myBlock));
            return &myBlock->payload[0];
        } else if (block_prev_allocated(myBlock) == 0) {
            availableSpace = availableSpace + block_prev_size(myBlock);
            block_t *previousBlock = block_prev(myBlock);
            // set sizing
            pull_free_block(previousBlock);
            block_set_size_and_allocated(previousBlock,
                                         availableSpace - requestedSize, 0);
            block_t *splitBlock = block_next(previousBlock);
            block_set_size_and_allocated(splitBlock, (requestedSize), 1);
            insert_free_block(previousBlock);
            coalesce(previousBlock);
            // set surrounding block size
            block_set_size(block_prev(splitBlock), block_prev_size(splitBlock));
            block_set_size_and_allocated(splitBlock, (requestedSize), 1);
            block_set_size(block_next(splitBlock), block_next_size(splitBlock));
            return &splitBlock->payload[0];
        }
        // if no free block is found
        else {
            // check to see if shrinking will cause less than minimum size, then
            // do nothing
            if (((int)availableSpace - (int)requestedSize) <
                (int)MINBLOCKSIZE) {
                // set sizing
                block_set_size_and_allocated(myBlock, originalSize, 1);
                return &myBlock->payload[0];
            } else {
                block_set_size_and_allocated(myBlock, requestedSize, 1);
                block_t *splitBlock = block_next(myBlock);
                block_set_size_and_allocated(
                    splitBlock, (availableSpace - requestedSize), 0);
                insert_free_block(splitBlock);
                coalesce(splitBlock);
                // set surrounding block size
                block_set_size(block_prev(myBlock), block_prev_size(myBlock));
                block_set_size_and_allocated(myBlock, (requestedSize), 1);
                block_set_size(block_next(myBlock), block_next_size(myBlock));
                return &myBlock->payload[0];
            }
        }
    }
    // else if requested > original size
    if ((int)requestedSize > (int)originalSize) {
        ////check for surrounding free blocks
        if (block_next_allocated(myBlock) == 0) {
            availableSpace = availableSpace + block_next_size(myBlock);

            if (((int)availableSpace - (int)requestedSize) >=
                (int)MINBLOCKSIZE) {
                // now reallocate and coalesce
                block_t *nextBlock = block_next(myBlock);
                pull_free_block(nextBlock);
                block_set_size_and_allocated(myBlock, requestedSize, 1);
                block_t *splitBlock = block_next(myBlock);
                block_set_size_and_allocated(
                    splitBlock, (availableSpace - requestedSize), 0);
                insert_free_block(splitBlock);
                coalesce(splitBlock);
                block_set_size_and_allocated(myBlock, requestedSize, 1);
                block_set_size_and_allocated(
                    splitBlock, (availableSpace - requestedSize), 0);
                // set surrounding block size
                block_set_size(block_prev(myBlock), block_prev_size(myBlock));
                block_set_size(block_next(myBlock), block_next_size(myBlock));

                return &myBlock->payload[0];
            }
            // if minimblock size is not met but we have the room, then
            // reallocate all of it
            else if (((int)availableSpace - (int)requestedSize) <
                         (int)MINBLOCKSIZE &&
                     ((int)availableSpace - (int)requestedSize) >= 0) {
                pull_free_block(block_next(myBlock));
                block_set_size_and_allocated(myBlock, availableSpace, 1);

                // set surrounding block size
                block_set_size(block_prev(myBlock), block_prev_size(myBlock));
                block_set_size(block_next(myBlock), block_next_size(myBlock));
                return &myBlock->payload[0];
            }
            // if no space then loop through and find free block
            else {
                void *newPtr = mm_malloc(size);
                block_t *newMallocBlock = payload_to_block(newPtr);
                // preserve memory
                memmove(newPtr, ptr, originalSize - TAGS_SIZE);
                mm_free(ptr);

                // set surrounding block size
                block_set_size(block_prev(newMallocBlock),
                               block_prev_size(newMallocBlock));
                block_set_size_and_allocated(newMallocBlock,
                                             block_size(newMallocBlock), 1);
                block_set_size(block_next(newMallocBlock),
                               block_next_size(newMallocBlock));
                return &newMallocBlock->payload[0];
            }
        }
        // loop through free list
        else {
            void *newPtr = mm_malloc(size);
            block_t *newMallocBlock = payload_to_block(newPtr);
            // preserve memory
            memmove(newPtr, ptr, originalSize - TAGS_SIZE);
            mm_free(ptr);
            // set surrounding block size
            block_set_size(block_prev(newMallocBlock),
                           block_prev_size(newMallocBlock));
            block_set_size_and_allocated(newMallocBlock,
                                         block_size(newMallocBlock), 1);
            block_set_size(block_next(newMallocBlock),
                           block_next_size(newMallocBlock));
            return &newMallocBlock->payload[0];
        }
    }
    // if equal
    else {
        block_set_size_and_allocated(myBlock, requestedSize, 1);
        block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
        return &myBlock->payload[0];
    }
}
