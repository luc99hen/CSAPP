/*
 * mm-sfl.c - Segregated Free Lists
 * 3 lists for different-size block classes
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
static void* block_lists[LIST_NUM];
static int list_size_limits[LIST_NUM-1] = { 1024, 4096 }; 

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

static void panic(char* err_msg){
    printf(err_msg);
    fflush(stdout);
    while(1){}
}

static int get_appropriate_list(size_t size){
    int i;
    for(i=0; i<LIST_NUM-1; i++){
        if (list_size_limits[i] >= size) 
            break;
    }
    return i;
}

static void* first_fit(size_t size){
    size_t block_size;
    char* cur_block;
    
    for(int i=get_appropriate_list(size); i<LIST_NUM; i++){
        cur_block = NEXT_FREE_BLK(block_lists[i]);
        while(cur_block && (block_size = GET_SIZE(HDRP(cur_block))) > 0){
            if(GET_ALLOC(HDRP(cur_block)))
                panic("first_fit error: allocated block in free block list");
            if(block_size >= size)
                return cur_block;
            cur_block = NEXT_FREE_BLK(cur_block);
        }
    }

    return NULL;
}

/*
 * find_fit - find a fit free block given a size
 */
static void* find_fit(size_t size){
   return first_fit(size);
}

/*
 * delete_block - delete a block in a double linked list
 */
static void delete_block(void* bp){
    if (GET_ALLOC(HDRP(bp))) {
        panic("delete_block error: shouldn't delete an allocated block");
    }
    PUT(NEXT_PTR(PREV_FREE_BLK(bp)), NEXT_FREE_BLK(bp));
    if (NEXT_FREE_BLK(bp))
        PUT(PREV_PTR(NEXT_FREE_BLK(bp)), PREV_FREE_BLK(bp));
}

/*
 * add_block - add a block to a double linked list
 * strategy 1: LIFO, add this block to the head of the linked list
 */
static void add_block(void* bp){
    void* first_block = block_lists[get_appropriate_list(GET_SIZE(HDRP(bp)))];
    PUT(PREV_PTR(bp), first_block);
    PUT(NEXT_PTR(bp), NEXT_FREE_BLK(first_block));
    PUT(NEXT_PTR(first_block), bp);
    if (NEXT_FREE_BLK(bp))
        PUT(PREV_PTR(NEXT_FREE_BLK(bp)), bp);
}

/*
 * place - mark a free block as used (split if necessary)
 */
static void place(void* bp, size_t size, int allocated){
    size_t block_size = GET_SIZE(HDRP(bp));

    if (!allocated)
        delete_block(bp);
    // if size of the left part is small enough, leave it as internal fragment
    if (block_size - size < MIN_BLOCK_SIZE){
        PUT(HDRP(bp), PACK(block_size, ALLOC));
        PUT(FTRP(bp), PACK(block_size, ALLOC));  // don't need footer for allocated block
    } else { // split this block into two
        PUT(HDRP(bp), PACK(size, ALLOC));
        PUT(FTRP(bp), PACK(size, ALLOC));
        PUT(HDRP(NEXT_BLK(bp)), PACK(block_size-size, FREE));
        PUT(FTRP(NEXT_BLK(bp)), PACK(block_size-size, FREE));
        add_block(NEXT_BLK(bp));
    }   
}


/*
 * coalesce - coalesce previous or next free block
 */
static void *coalesce(void* bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLK(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLK(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    void* res;

    if (prev_alloc && next_alloc) {
        res = bp;
        add_block(bp);
    } else if (prev_alloc && !next_alloc) {
        res = bp;
        size += GET_SIZE(HDRP(NEXT_BLK(bp)));
        delete_block(NEXT_BLK(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        add_block(bp);        
    } else if (!prev_alloc && next_alloc) {
        res = PREV_BLK(bp);
        size += GET_SIZE(HDRP(PREV_BLK(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLK(bp)), PACK(size, 0));
        delete_block(PREV_BLK(bp));   // reinsert this block, beacause its size change
        add_block(PREV_BLK(bp));      
    } else {
        res = PREV_BLK(bp);
        size = size + GET_SIZE(HDRP(PREV_BLK(bp))) + GET_SIZE(HDRP(NEXT_BLK(bp)));
        PUT(HDRP(PREV_BLK(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLK(bp)), PACK(size, 0));
        delete_block(PREV_BLK(bp));
        delete_block(NEXT_BLK(bp));
        add_block(PREV_BLK(bp));
    }

    return res;
}

/*
 * extend_heap - extend heap with mem_sbrk and transform the 
 * additional space to a free block
 */
static void *extend_heap(size_t words){
    char* addr;
    size_t size;

    size = (words % 2) ? (words + 1)*WSIZE : words*WSIZE;
    if ((long)(addr = mem_sbrk(size)) == -1)
        return NULL;

    // get the real block pointer addr from allocated mem addr
    char* bp = addr;

    // initialize the new free block
    PUT(HDRP(bp), PACK(size, FREE));
    PUT(FTRP(bp), PACK(size, FREE));
    PUT(HDRP(NEXT_BLK(bp)), PACK(0, ALLOC));

    // coalesce if previous block is free
    return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* initialize a implicit block linklist data structure */
    void* cur_block;
    
    cur_block = mem_sbrk((2+4*LIST_NUM)*WSIZE);
    if(cur_block == (void*)-1)
        return -1;
    
    PUT(cur_block, 0);     // Alignment with epilogue header
    for(int i=0; i<LIST_NUM; i++){
        PUT(cur_block + (1*WSIZE), PACK(2*DSIZE, ALLOC));  // Prologue header
        PUT(cur_block + (2*WSIZE), NULL);                  // Prologue pred ptr
        PUT(cur_block + (3*WSIZE), NULL);                  // Prologue next ptr (point to epilogue)
        PUT(cur_block + (4*WSIZE), PACK(2*DSIZE, ALLOC));  // Prologue footer
        block_lists[i] = cur_block + 2*WSIZE;
        cur_block += 4*WSIZE;
    }
    
    PUT(cur_block + WSIZE, PACK(0, ALLOC));        // Epilogue header

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

// adjust size for alignment and metadata overhead
size_t adjust_size(size_t size){
    if (size <= DSIZE)
        return MIN_BLOCK_SIZE;
    else
        return DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
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

    place(bp, new_size, 0);
    return bp;
}

/*
 * mm_free - Freeing a block
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, FREE));
    PUT(FTRP(ptr), PACK(size, FREE));
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
    size_t new_size = adjust_size(size);

    if (new_size <= old_size) {
        place(ptr, new_size, 1);
        return ptr;
    } else {
        void* new_ptr;
        if ((new_ptr= mm_malloc(size)) == NULL){
            return NULL;
        }
        memcpy(new_ptr, ptr, old_size - DSIZE);
        mm_free(ptr);
        return new_ptr;
    }
}

