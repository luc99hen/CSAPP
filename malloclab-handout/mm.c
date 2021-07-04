/*
 * mm-idb.c - implicit data block
 * 
 * In this approach, a block is represented by the first (and optionally last) byte in 
 * a data block. For the placement strategy, we choose first-fit which is substituable. 
 * Also we choose immediate coalecing rather than deffered one.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

#include "mm-macro.h"
static void* first_block;

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "luc99hen",
    /* First member's email address */
    "1813927768@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};


/*
 * find_fit - find a fit free block given a size
 */
static void* find_fit(size_t size){
    size_t block_size;
    char* cur_block = first_block;

    while((block_size = GET_SIZE(HDRP(cur_block))) > 0){
        if(!GET_ALLOC(HDRP(cur_block)) && block_size >= size)
            return cur_block;
        cur_block = NEXT_BLK(cur_block);
    }
    return NULL;
}

/*
 * place - mark a free block as used (split if necessary)
 */
static void place(void* bp, size_t size){
    size_t block_size = GET_SIZE(HDRP(bp));

    // if size of the left part is small enough, leave it as internal fragment
    if (block_size - size < MIN_BLOCK_SIZE){
        PUT(HDRP(bp), PACK(block_size, 1));
        PUT(FTRP(bp), PACK(block_size, 1));
    } else { // split this block into two
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLK(bp)), PACK(block_size-size, 0));
        PUT(FTRP(NEXT_BLK(bp)), PACK(block_size-size, 0));
    }   
}


/*
 * coalesce - coalesce previous or next free block
 */
static void *coalesce(void* bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLK(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLK(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        return bp;
    } else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLK(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return bp;
    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLK(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLK(bp)), PACK(size, 0));
        return PREV_BLK(bp);
    } else {
        size = size + GET_SIZE(HDRP(PREV_BLK(bp))) + GET_SIZE(HDRP(NEXT_BLK(bp)));
        PUT(HDRP(PREV_BLK(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLK(bp)), PACK(size, 0));
        return PREV_BLK(bp);
    }
}

/*
 * extend_heap - extend heap with mem_sbrk and transform the 
 * additional space to a free block
 */
static void *extend_heap(size_t words){
    char* bp;
    size_t size;

    size = (words % 2) ? (words + 1)*WSIZE : words*WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    // initialize the new free block
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLK(bp)), PACK(0, 1));

    // coalesce if previous block is free
    return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* initialize a implicit block linklist data structure */
    first_block = mem_sbrk(4*WSIZE);
    if(first_block == (void*)-1)
        return -1;
    
    PUT(first_block, 0);     // Alignment with epilogue header
    PUT(first_block + (1*WSIZE), PACK(DSIZE, 1));  // Prologue header
    PUT(first_block + (2*WSIZE), PACK(DSIZE, 1));  // Prologue footer
    PUT(first_block + (3*WSIZE), PACK(0, 1));      // Epilogue header
    first_block += (2*WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t new_size;
    size_t extend_size;
    char* bp;

    if(size == 0)
        return NULL;
    
    // adjust size for alignment and metadata overhead
    if (size < DSIZE)
        new_size = MIN_BLOCK_SIZE;
    else
        new_size = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    
    if ((bp = find_fit(new_size)) == NULL){
        extend_size = MAX(new_size, CHUNKSIZE);
        if ((bp = extend_heap(extend_size)) == NULL)
            return NULL;
    }

    place(bp, new_size);
    return bp;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (size == 0){
        mm_free(ptr);
        return NULL;
    } else if (ptr == NULL) {
        return mm_malloc(size);
    } 

    size_t old_size = GET_SIZE(HDRP(ptr));
    if (size < old_size) {
        place(ptr, size);
        return ptr;
    } else {
        void* new_ptr;
        if ((new_ptr= mm_malloc(size)) == NULL){
            return NULL;
        }
        memcpy(new_ptr, ptr, old_size);
        mm_free(ptr);
        return new_ptr;
    }
}














