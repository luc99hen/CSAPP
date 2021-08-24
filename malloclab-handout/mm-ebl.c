/*
 * mm-ebl.c - explicit block list
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
static const char* fit_strategy = "best";

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
        // if (pre_alloc != (GET_PRE_ALLOC(HDRP(cur_block)) >> 1)){
        //     printf("mm_check fail: incosistent prealloc state\n");
        //     return 0;
        // }
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
    char* cur_block = NEXT_FREE_BLK(first_block);

    while(cur_block && (block_size = GET_SIZE(HDRP(cur_block))) > 0){
        if(GET_ALLOC(HDRP(cur_block)))
            panic("first_fit error: allocated block in free block list");
        if(block_size >= size)
            return cur_block;
        cur_block = NEXT_FREE_BLK(cur_block);
    }
    return NULL;
}

static void* next_fit_block;
static void* next_fit(size_t size){
    if (!NEXT_FREE_BLK(first_block))  // no free block in the list
        return NULL;

    void* cur_block = (next_fit_block ? next_fit_block : NEXT_FREE_BLK(first_block));
    size_t block_size;
    void* loop_start = cur_block;

    while(1){
        
        if(GET_ALLOC(HDRP(cur_block)))
            panic("next_fit error: allocated block in free block list");
        
        if((block_size = GET_SIZE(HDRP(cur_block))) >= size) {
            next_fit_block = NEXT_FREE_BLK(cur_block);  // next_fit_block point to next free block
            break;
        }

        cur_block = NEXT_FREE_BLK(cur_block) == NULL ? NEXT_FREE_BLK(first_block) : NEXT_FREE_BLK(cur_block);
        if(cur_block == loop_start){
            next_fit_block = cur_block = NULL;
            break;
        }
    }

    return cur_block;
}

static void* best_fit(size_t size){
    size_t block_size;
    char* cur_block = NEXT_FREE_BLK(first_block);
    size_t best_size = mem_heapsize();
    char* best_block = NULL;

     while(cur_block && (block_size = GET_SIZE(HDRP(cur_block))) > 0){
        if(GET_ALLOC(HDRP(cur_block)))
            panic("best_fit error: allocated block in free block list");
        if(block_size >= size && block_size < best_size) {
            best_size = block_size;
            best_block = cur_block;
        }
        cur_block = NEXT_FREE_BLK(cur_block);
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
 * strategu 2: arrange the list in the addr order
 */
static void add_block(void* bp){
    void* cur_block = first_block;
    void* prev_block;
    void* next_block;

    // find the proper position to insert this block
    while (cur_block < bp && NEXT_FREE_BLK(cur_block) != NULL) {
        cur_block = NEXT_FREE_BLK(cur_block);
    }
    
    if (NEXT_FREE_BLK(cur_block) != NULL) {
        prev_block = PREV_FREE_BLK(cur_block);
        next_block = cur_block;
    } else { // bp should be the last item in the list
        prev_block = cur_block;
        next_block = NULL;
    }
    
    PUT(PREV_PTR(bp), prev_block);
    PUT(NEXT_PTR(bp), next_block);
    PUT(NEXT_PTR(prev_block), bp);
    if (next_block)
        PUT(PREV_PTR(next_block), bp);
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

    if(IS_DEBUG && !mm_check())
        print_list();
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
        size += GET_SIZE(HDRP(NEXT_BLK(bp)));
        delete_block(NEXT_BLK(bp));
        if(NEXT_BLK(bp) == next_fit_block && !strcmp(fit_strategy, "next"))
            next_fit_block = bp;
        add_block(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        res = bp;
    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLK(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLK(bp)), PACK(size, 0));
        res = PREV_BLK(bp);
    } else {
        size = size + GET_SIZE(HDRP(PREV_BLK(bp))) + GET_SIZE(HDRP(NEXT_BLK(bp)));
        delete_block(NEXT_BLK(bp));
        if((NEXT_BLK(bp) == next_fit_block) && !strcmp(fit_strategy, "next"))
            next_fit_block = PREV_BLK(bp);
        PUT(HDRP(PREV_BLK(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLK(bp)), PACK(size, 0));
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
    first_block = mem_sbrk(6*WSIZE);
    if(first_block == (void*)-1)
        return -1;
    
    PUT(first_block, 0);     // Alignment with epilogue header
    PUT(first_block + (1*WSIZE), PACK(2*DSIZE, ALLOC));  // Prologue header
    PUT(first_block + (2*WSIZE), NULL);                  // Prologue pred ptr
    PUT(first_block + (3*WSIZE), NULL);                  // Prologue next ptr (point to epilogue)
    PUT(first_block + (4*WSIZE), PACK(2*DSIZE, ALLOC));  // Prologue footer
    PUT(first_block + (5*WSIZE), PACK(0, ALLOC));        // Epilogue header
    first_block += (2*WSIZE);

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

