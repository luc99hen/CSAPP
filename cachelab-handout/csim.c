#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cachelab.h"

#define TRUE 1
typedef unsigned long Addr64;

typedef struct CacheLine
{
    unsigned long id;
    unsigned long last_tick;
} CacheLine;


typedef struct CacheSet{
    size_t capacity;
    size_t size;
    CacheLine* lines;
} CacheSet;

// find the least used CacheLine in a CacheSet
size_t LRU_evict(CacheSet* cs){
    size_t i = 0;
    size_t least_use = -1, min_tick = __INT32_MAX__;
    for(;i < cs->size;i++){
        if(cs->lines[i].last_tick < min_tick){
            least_use = i;
            min_tick = cs->lines[i].last_tick;
        }
    }
    return least_use;
}

int find_line(CacheSet* cs, unsigned long id){
    size_t i = 0;
    for(; i<cs->capacity; i++){
        if(cs->lines[i].id == id)
            return i;
    }
    return -1;
}

size_t get_setIndex(Addr64 addr, int block_bits, int set_bits){
    return (addr >> block_bits) & ((1 << set_bits) - 1);
}

unsigned long get_lineId(Addr64 addr, int block_bits, int set_bits ){
    return (addr >> (block_bits + set_bits));
}

void set_cacheLine(CacheSet* cs, unsigned long cur_tick, unsigned long lineId, size_t lineIndex){
    cs->lines[lineIndex].last_tick = cur_tick;
    cs->lines[lineIndex].id = lineId;
}


int main(int argc, char *argv[])
{
    // parse arguments (assume formal format)
    int set_bits, asso_nums, block_bits, verbose = 0;
    char* fileName;

    int i = 0;
    while(i < argc){
        if(strlen(argv[i]) > 1 && argv[i][0] == '-'){
            switch (argv[i][1])
            {
                case 's':
                    set_bits = atoi(argv[i+1]);
                    break;
                case 'E':
                    asso_nums = atoi(argv[i+1]);
                    break;
                case 'b':
                    block_bits = atoi(argv[i+1]);
                    break;
                case 't':
                    fileName = argv[i+1];
                    break;
                case 'v':
                    verbose = 1;
                    break;
                default:
                    break;
            }
        }
        i += 1;
    }

    // construct cache structure
    int hit_count = 0, miss_count = 0, eviction_count = 0;
    
    CacheSet* sets = malloc((1 << set_bits)*sizeof(CacheSet));  // omit free mem parts for simplicity
    memset(sets, (1 << set_bits)*sizeof(CacheSet), 0);

    // read input trace file
    FILE *fptr = fopen(fileName, "r");
    if(fptr == NULL){
        printf("Error: Can't open file");
        exit(1);
    }

    int ret, size, cur_tick = 0;
    char operation;
    Addr64 addr;
    while (TRUE) {
        ret = fscanf(fptr, "%c %lx,%d\n", &operation, &addr, &size);
        if(ret == 3){
            int total_move = 0;
            if(operation == 'I')
                continue;   // omit intruction load
            else if(operation == 'M')
                total_move = 2;  // 'M' = 'L' + 'S'
            else
                total_move = 1;
            cur_tick += 1;  // update currnet tick

            if(verbose)
                printf("%c %lx, %d ",operation, addr, size);

            size_t setIndex = get_setIndex(addr, block_bits, set_bits);
            unsigned long lineId = get_lineId(addr, block_bits, set_bits);
            size_t lineIndex;

            int i = 0;
            for(; i<total_move; i++){
                if(sets[setIndex].size == 0){
                    // miss, set never initalized
                    sets[setIndex].size = asso_nums;
                    sets[setIndex].lines = malloc((sets->size)*sizeof(CacheLine));
                    lineIndex = 0;
                    sets[setIndex].capacity += 1; 
                    miss_count += 1;
                    if(verbose)
                        printf("miss ");
                } else{
                    lineIndex = find_line(&sets[setIndex], lineId);
                    if(lineIndex != -1){
                        // hit
                        hit_count += 1;
                        if(verbose)
                            printf("hit ");
                    } else if(sets[setIndex].capacity < sets[setIndex].size){
                        // miss
                        lineIndex = sets[setIndex].capacity;
                        sets[setIndex].capacity += 1; 
                        miss_count += 1;
                        if(verbose)
                            printf("miss ");
                    } else {
                        // miss evict
                        lineIndex = LRU_evict(&sets[setIndex]);
                        miss_count += 1;
                        eviction_count += 1;
                        if(verbose)
                            printf("miss eviction ");
                    }
                }
                set_cacheLine(&sets[setIndex], cur_tick, lineId, lineIndex);               
            }

            if(verbose)
                printf("\n");

        } else if(ret == EOF){
            break;
        } else {
            // printf("no match");
        }
    }

    fclose(fptr);

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
