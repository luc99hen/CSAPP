#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12) /* Extend Heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/*pack a size and allocated bit in a word (meta data of a block) */
#define PACK(size, alloc) ((size) | (alloc)) 

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 1)

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

#define MIN_BLOCK_SIZE 2*DSIZE