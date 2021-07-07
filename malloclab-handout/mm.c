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
static const char* fit_strategy = "next";

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

// print out the whole list to debug
static void print_list() {
    size_t block_size;
    char* cur_block = first_block;

     while((block_size = GET_SIZE(HDRP(cur_block))) > 0){
        printf("{%p, %lu, %d} ",cur_block, block_size,GET_FLAGS(HDRP(cur_block)));
        cur_block = NEXT_BLK(cur_block);
    }
    printf("{%p, %lu}\n",cur_block, block_size);
}

// consistency check
static int mm_check() {
    size_t block_size;
    size_t total_size = 8;  // Epilogue header + First byte
    char* cur_block = first_block;
    int pre_alloc = 0, cur_alloc;

     while((block_size = GET_SIZE(HDRP(cur_block))) > 0){
        cur_alloc = GET_ALLOC(HDRP(cur_block));
        if (pre_alloc == 0 && cur_alloc == 0){
            printf("mm_check fail: block escape coalescing\n");
            return 0;
        }
        if (pre_alloc != (GET_PRE_ALLOC(HDRP(cur_block)) >> 1)){
            printf("mm_check fail: incosistent prealloc state\n");
            return 0;
        }
        if (block_size % ALIGNMENT > 0 || (long)cur_block % ALIGNMENT > 0){
            printf("mm_check fail: alignment\n");
            return 0;
        }
        
        cur_block = NEXT_BLK(cur_block);
        
        total_size += block_size;
        pre_alloc = cur_alloc;
    }

    size_t expect_size = mem_heapsize();
    if(total_size != expect_size){
        printf("mm_check fail: block miss\n");
        return 0;
    }

    return 1;
}

static void* first_fit(size_t size){
    size_t block_size;
    char* cur_block = first_block;

    while((block_size = GET_SIZE(HDRP(cur_block))) > 0){
        if(!GET_ALLOC(HDRP(cur_block)) && block_size >= size)
            return cur_block;
        cur_block = NEXT_BLK(cur_block);
    }
    return NULL;
}

static void* next_fit_block;
static void* next_fit(size_t size){
    void* cur_block = (next_fit_block ? NEXT_BLK(next_fit_block) : NEXT_BLK(first_block));
    size_t block_size;
    void* loop_start = cur_block;

    while(1){
        block_size = GET_SIZE(HDRP(cur_block));
        if(block_size == 0){ // the last block in the list, start from first
            cur_block = first_block;
            continue;
        }

        if(!GET_ALLOC(HDRP(cur_block)) && block_size >= size) {
            next_fit_block = cur_block;
            break;
        }

        cur_block = NEXT_BLK(cur_block);
        if(cur_block == loop_start){
            next_fit_block = cur_block = NULL;
            break;
        }
    }

    return cur_block;
}

static void* best_fit(size_t size){
    size_t block_size;
    char* cur_block = first_block;
    size_t best_size = mem_heapsize();
    char* best_block = NULL;

     while((block_size = GET_SIZE(HDRP(cur_block))) > 0){
        if(!GET_ALLOC(HDRP(cur_block)) && block_size >= size && block_size < best_size) {
            best_size = block_size;
            best_block = cur_block;
        }
        cur_block = NEXT_BLK(cur_block);
    }
    return best_block;
}


/*
 * find_fit - find a fit free block given a size
 */
static void* find_fit(size_t size){
   if(!strcmp(fit_strategy, "next")){
       return next_fit(size);
   } else if(!strcmp(fit_strategy, "best")){
       return best_fit(size);
   } else {
       return first_fit(size);
   }
}

/*
 * place - mark a free block as used (split if necessary)
 */
static void place(void* bp, size_t size){
    size_t block_size = GET_SIZE(HDRP(bp));
    int block_flags = GET_FLAGS(HDRP(bp));

    // if size of the left part is small enough, leave it as internal fragment
    if (block_size - size < MIN_BLOCK_SIZE){
        PUT(HDRP(bp), PACK(block_size, SET_ALLOC(block_flags)));
        // PUT(FTRP(bp), PACK(block_size, 1));  // don't need footer for allocated block
        int next_header = GET(HDRP(NEXT_BLK(bp))) | 0x2;
        PUT(HDRP(NEXT_BLK(bp)), next_header);
        if (!(next_header & 0x1))  // if this next_block is free, set footer
            PUT(FTRP(NEXT_BLK(bp)), next_header);
    } else { // split this block into two
        PUT(HDRP(bp), PACK(size, SET_ALLOC(block_flags)));
        // PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLK(bp)), PACK(block_size-size, ALLOC_FREE));
        PUT(FTRP(NEXT_BLK(bp)), PACK(block_size-size, ALLOC_FREE));
    }   

    if(IS_DEBUG && !mm_check())
        print_list();
}


/*
 * coalesce - coalesce previous or next free block
 */
static void *coalesce(void* bp){
    size_t prev_alloc = GET_PRE_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLK(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    int flags;
    void* res;

    if (prev_alloc && next_alloc) {
        res = bp;
    } else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLK(bp)));
        if(NEXT_BLK(bp) == next_fit_block && !strcmp(fit_strategy, "next"))
            next_fit_block = bp;
        flags = GET_FLAGS(HDRP(bp));
        PUT(HDRP(bp), PACK(size, SET_FREE(flags)));
        PUT(FTRP(bp), PACK(size, SET_FREE(flags)));
        res = bp;
    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLK(bp)));
        if(bp == next_fit_block && !strcmp(fit_strategy, "next"))    
            next_fit_block = PREV_BLK(bp);
        flags = GET_FLAGS(HDRP(PREV_BLK(bp)));
        PUT(FTRP(bp), PACK(size, SET_FREE(flags)));
        PUT(HDRP(PREV_BLK(bp)), PACK(size, SET_FREE(flags)));
        res = PREV_BLK(bp);
    } else {
        size = size + GET_SIZE(HDRP(PREV_BLK(bp))) + GET_SIZE(HDRP(NEXT_BLK(bp)));
        if((bp == next_fit_block || NEXT_BLK(bp) == next_fit_block) && !strcmp(fit_strategy, "next"))
            next_fit_block = PREV_BLK(bp);
        flags = GET_FLAGS(HDRP(PREV_BLK(bp)));
        PUT(HDRP(PREV_BLK(bp)), PACK(size, SET_FREE(flags)));
        PUT(FTRP(NEXT_BLK(bp)), PACK(size, SET_FREE(flags)));
        res = PREV_BLK(bp);
    }

    if(IS_DEBUG && !mm_check())
        print_list();
    
    return res;
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
    
    // get previous last block's state from epilogue flag
    int prev_alloc = GET_PRE_ALLOC(HDRP(bp));

    // initialize the new free block
    PUT(HDRP(bp), PACK(size, prev_alloc));
    PUT(FTRP(bp), PACK(size, prev_alloc));
    PUT(HDRP(NEXT_BLK(bp)), PACK(0, FREE_ALLOC));

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
    PUT(first_block + (1*WSIZE), PACK(DSIZE, FREE_ALLOC));  // Prologue header
    PUT(first_block + (2*WSIZE), PACK(DSIZE, FREE_ALLOC));  // Prologue footer
    PUT(first_block + (3*WSIZE), PACK(0, ALLOC_ALLOC));     // Epilogue header
    first_block += (2*WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

// adjust size for alignment and metadata overhead
size_t adjust_size(size_t size){
    if (size <= WSIZE)
        return MIN_BLOCK_SIZE;
    else
        return DSIZE * ((size + WSIZE + (DSIZE - 1)) / DSIZE);
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
    
    new_size = adjust_size(size);
    if ((bp = find_fit(new_size)) == NULL){
        extend_size = MAX(new_size, CHUNKSIZE);
        if ((bp = extend_heap(extend_size/WSIZE)) == NULL)
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
    int flags = GET_FLAGS(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, SET_FREE(flags)));
    PUT(FTRP(ptr), PACK(size, SET_FREE(flags)));
    int next_header = GET(HDRP(NEXT_BLK(ptr))) & ~0x2;
    PUT(HDRP(NEXT_BLK(ptr)), next_header);
    if (!(next_header & 0x1))  // if this next_block is free, set footer
        PUT(FTRP(NEXT_BLK(ptr)), next_header);
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

    size_t old_size = GET_SIZE(HDRP(ptr)) - WSIZE;

    if (size <= old_size) {
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














