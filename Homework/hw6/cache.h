#ifndef _CS110_HW6_CACHE_H_
#define _CS110_HW6_CACHE_H_
#include <stdint.h>
#define BLOCK_SIZE 16
#define NUM_BLOCKS 4
typedef struct {
    uint32_t tag;             
    uint8_t valid;            
    uint8_t lru_counter;      
} cache_main;
typedef struct {
    uint32_t inst_accesses;
    uint32_t data_accesses;
    uint32_t inst_hits;
    uint32_t data_hits;
    int cache_valid;
} cache_count;
typedef enum {
    instruction,
    data
} type_access_t;
void init_cache_system();
void add_inst_access(uint32_t);
void add_data_access(uint32_t);
void print_cache_statistics();

#endif
