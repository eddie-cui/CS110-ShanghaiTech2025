#include "cache.h"
cache_main cache[NUM_BLOCKS];
cache_count CC;
void init_cache_system() {
    memset(cache, 0, sizeof(cache));
    CC.inst_accesses = 0;
    CC.data_accesses = 0;
    CC.inst_hits = 0;
    CC.data_hits = 0;
    CC.cache_valid = 1;
}
void update_lru(int hit_index) {
    cache[hit_index].lru_counter = 0;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if(i != hit_index&&cache[i].valid) {
            cache[i].lru_counter++;
        }
    }
}
int find_lru_target() {
    int target = 0;
    uint8_t max_lru = 0;
    if(CC.cache_valid) {
        for (int i = 0; i < NUM_BLOCKS; i++) {
            if (!cache[i].valid) {
                return i;
            }
        }
        CC.cache_valid = 0;
    }
    for(int i = 0; i < NUM_BLOCKS; i++) {
        if (cache[i].lru_counter > max_lru) {
            max_lru = cache[i].lru_counter;
            target = i;
        }
    }
    return target;
}


int access_cache(uint32_t address, type_access_t type) {
    uint32_t tag = address >> 4;
    int hit = 0;
    int hit_index;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (cache[i].valid && cache[i].tag == tag) {
            hit = 1;
            hit_index = i;
            break;
        }
    }
    uint32_t access;
    if (hit) {
        if (type == instruction) {
            CC.inst_hits++;
        } 
        else {
            CC.data_hits++;
        }
        access = hit_index;
    } 
    else {
        int target = find_lru_target();
        cache[target].valid = 1;
        cache[target].tag = tag;
        access = target;
    }
    update_lru(access);
    if (type == instruction) {
        CC.inst_accesses++;
    } 
    else {
        CC.data_accesses++;
    }
    return hit;
}
void add_inst_access(uint32_t address) {
    access_cache(address, instruction);
}

void add_data_access(uint32_t address) {
    access_cache(address, data);
}

void print_cache_statistics() {
    uint32_t total_accesses = CC.inst_accesses + CC.data_accesses;
    uint32_t total_hits = CC.inst_hits + CC.data_hits;
    uint32_t total_misses = total_accesses - total_hits;
    
    printf("Total memory accesses: %u.\n", total_accesses);
    printf("Instruction cache hit: %u.\n", CC.inst_hits);
    printf("Data cache hit: %u.\n", CC.data_hits);
    printf("Total cache misses: %u.\n", total_misses);
}