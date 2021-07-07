#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12) /* Extend Heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/*pack a size and allocated bit in a word (meta data of a block) */
#define PACK(size, alloc) ((size) | (alloc)) 

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PRE_ALLOC(p) (GET(p) & 0x2)
#define GET_FLAGS(p) (GET(p) & 0x7)


/* Given block ptr bp, compute addr of its header and footer */
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute addr of the next and previous blocks */
#define NEXT_BLK(bp) ((char*)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLK(bp) ((char*)(bp) - GET_SIZE((char *)(bp) - DSIZE))

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define MIN_BLOCK_SIZE DSIZE

#define FREE        0x0
#define ALLOC       0x1
#define FREE_FREE   0x0
#define FREE_ALLOC  0x1
#define ALLOC_FREE  0x2
#define ALLOC_ALLOC 0x3
#define SET_FREE(v) (v & ~0x1)
#define SET_ALLOC(v) (v | 0x1)

#define IS_DEBUG    0