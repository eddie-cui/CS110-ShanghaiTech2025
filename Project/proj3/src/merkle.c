#include "mercha.h"
#include <pthread.h>
#include <stdio.h>
typedef struct {
    uint8_t* pool1;
    uint8_t* pool2;
    size_t pool_size;
    pthread_mutex_t mutex;
    int initialized;
} MerkleMemoryPool;

static __attribute__((always_inline)) MerkleMemoryPool g_memory_pool = {
    .pool1 = NULL,
    .pool2 = NULL,
    .pool_size = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .initialized = 0
};

static __attribute__((always_inline)) int init_merkle_memory_pool(size_t max_size) {
    pthread_mutex_lock(&g_memory_pool.mutex);
    
    if (g_memory_pool.initialized) {
        pthread_mutex_unlock(&g_memory_pool.mutex);
        return 0;
    }
    
    size_t aligned_size = (max_size + 63) & ~63;
    
    g_memory_pool.pool1 = (uint8_t*)aligned_alloc(64, aligned_size);
    g_memory_pool.pool2 = (uint8_t*)aligned_alloc(64, aligned_size);
    
    if (!g_memory_pool.pool1 || !g_memory_pool.pool2) {
        free(g_memory_pool.pool1);
        free(g_memory_pool.pool2);
        g_memory_pool.pool1 = g_memory_pool.pool2 = NULL;
        pthread_mutex_unlock(&g_memory_pool.mutex);
        return -1;
    }
    
    g_memory_pool.pool_size = aligned_size;
    g_memory_pool.initialized = 1;
    
    pthread_mutex_unlock(&g_memory_pool.mutex);
    return 0;
}

static __attribute__((always_inline)) void cleanup_merkle_memory_pool() {
    pthread_mutex_lock(&g_memory_pool.mutex);
    
    if (g_memory_pool.initialized) {
        free(g_memory_pool.pool1);
        free(g_memory_pool.pool2);
        g_memory_pool.pool1 = g_memory_pool.pool2 = NULL;
        g_memory_pool.pool_size = 0;
        g_memory_pool.initialized = 0;
    }
    
    pthread_mutex_unlock(&g_memory_pool.mutex);
}

static __attribute__((always_inline)) int ensure_pool_initialized(size_t required_size) {
    if (!g_memory_pool.initialized || g_memory_pool.pool_size < required_size) {
        size_t new_size = required_size * 2;
        if (new_size < 64 * 1024 * 1024) {
            new_size = 64 * 1024 * 1024;
        }
                
        if (g_memory_pool.initialized) {
            cleanup_merkle_memory_pool();
        }
        
        return init_merkle_memory_pool(new_size);
    }
    return 0;
}
static __attribute__((always_inline)) void fast_copy_dest_aligned(void *dest, void *src) {
    asm volatile (
        "vmovdqa64 (%1), %%zmm0    \n\t" 
        "vmovntdq  %%zmm0, (%0)    \n\t"
        :
        : "r"(dest), "r"(src)
        : "zmm0", "memory"
    );
}
static __attribute__((always_inline)) void fast_copy_dest_unaligned(void *dest, void *src) {
    asm volatile (
        "vmovdqa64 (%1), %%zmm0    \n\t"
        "vmovdqu64 %%zmm0, (%0)    \n\t"
        :
        : "r"(dest), "r"(src)
        : "zmm0", "memory"
    );
}
static __attribute__((always_inline)) void fast_copy_src_unaligned(void *dest, void *src, unsigned int size) {
    uint8_t *dest_ptr = (uint8_t*)dest;
    uint8_t *src_ptr = (uint8_t*)src;
    const size_t chunk_size = 2048;
    const size_t num_chunks = size / chunk_size;
    
    #pragma omp parallel for
    for (size_t chunk = 0; chunk < num_chunks; ++chunk) {
        size_t base_offset = chunk * chunk_size;
        
        if ((chunk + 1) * chunk_size < size) {
            _mm_prefetch((char*)(src_ptr + base_offset + chunk_size), _MM_HINT_T0);
            _mm_prefetch((char*)(src_ptr + base_offset + chunk_size + 64), _MM_HINT_T0);
            _mm_prefetch((char*)(src_ptr + base_offset + chunk_size + 256), _MM_HINT_T1);
            _mm_prefetch((char*)(src_ptr + base_offset + chunk_size + 512), _MM_HINT_T1);
            _mm_prefetch((char*)(src_ptr + base_offset + chunk_size + 1024), _MM_HINT_T1);
            _mm_prefetch((char*)(src_ptr + base_offset + chunk_size + 1536), _MM_HINT_T2);
        }
        asm volatile (
            "vmovdqu64 0*64(%1), %%zmm0    \n\t"
            "vmovdqu64 1*64(%1), %%zmm1    \n\t"
            "vmovdqu64 2*64(%1), %%zmm2    \n\t"
            "vmovdqu64 3*64(%1), %%zmm3    \n\t"
            "vmovdqu64 4*64(%1), %%zmm4    \n\t"
            "vmovdqu64 5*64(%1), %%zmm5    \n\t"
            "vmovdqu64 6*64(%1), %%zmm6    \n\t"
            "vmovdqu64 7*64(%1), %%zmm7    \n\t"
            "vmovdqu64 8*64(%1), %%zmm8    \n\t"
            "vmovdqu64 9*64(%1), %%zmm9    \n\t"
            "vmovdqu64 10*64(%1), %%zmm10  \n\t"
            "vmovdqu64 11*64(%1), %%zmm11  \n\t"
            "vmovdqu64 12*64(%1), %%zmm12  \n\t"
            "vmovdqu64 13*64(%1), %%zmm13  \n\t"
            "vmovdqu64 14*64(%1), %%zmm14  \n\t"
            "vmovdqu64 15*64(%1), %%zmm15  \n\t"
            "vmovdqu64 16*64(%1), %%zmm16  \n\t"
            "vmovntdq %%zmm0,  0*64(%0)    \n\t"
            "vmovdqu64 17*64(%1), %%zmm17  \n\t"
            "vmovntdq %%zmm1,  1*64(%0)    \n\t"
            "vmovdqu64 18*64(%1), %%zmm18  \n\t"
            "vmovntdq %%zmm2,  2*64(%0)    \n\t"
            "vmovdqu64 19*64(%1), %%zmm19  \n\t"
            "vmovntdq %%zmm3,  3*64(%0)    \n\t"
            "vmovdqu64 20*64(%1), %%zmm20  \n\t"
            "vmovntdq %%zmm4,  4*64(%0)    \n\t"
            "vmovdqu64 21*64(%1), %%zmm21  \n\t"
            "vmovntdq %%zmm5,  5*64(%0)    \n\t"
            "vmovdqu64 22*64(%1), %%zmm22  \n\t"
            "vmovntdq %%zmm6,  6*64(%0)    \n\t"
            "vmovdqu64 23*64(%1), %%zmm23  \n\t"
            "vmovntdq %%zmm7,  7*64(%0)    \n\t"
            "vmovdqu64 24*64(%1), %%zmm24  \n\t"
            "vmovntdq %%zmm8,  8*64(%0)    \n\t"
            "vmovdqu64 25*64(%1), %%zmm25  \n\t"
            "vmovntdq %%zmm9,  9*64(%0)    \n\t"
            "vmovdqu64 26*64(%1), %%zmm26  \n\t"
            "vmovntdq %%zmm10, 10*64(%0)   \n\t"
            "vmovdqu64 27*64(%1), %%zmm27  \n\t"
            "vmovntdq %%zmm11, 11*64(%0)   \n\t"
            "vmovdqu64 28*64(%1), %%zmm28  \n\t"
            "vmovntdq %%zmm12, 12*64(%0)   \n\t"
            "vmovdqu64 29*64(%1), %%zmm29  \n\t"
            "vmovntdq %%zmm13, 13*64(%0)   \n\t"
            "vmovdqu64 30*64(%1), %%zmm30  \n\t"
            "vmovntdq %%zmm14, 14*64(%0)   \n\t"
            "vmovdqu64 31*64(%1), %%zmm31  \n\t"
            "vmovntdq %%zmm15, 15*64(%0)   \n\t"
            "vmovntdq %%zmm16, 16*64(%0)   \n\t"
            "vmovntdq %%zmm17, 17*64(%0)   \n\t"
            "vmovntdq %%zmm18, 18*64(%0)   \n\t"
            "vmovntdq %%zmm19, 19*64(%0)   \n\t"
            "vmovntdq %%zmm20, 20*64(%0)   \n\t"
            "vmovntdq %%zmm21, 21*64(%0)   \n\t"
            "vmovntdq %%zmm22, 22*64(%0)   \n\t"
            "vmovntdq %%zmm23, 23*64(%0)   \n\t"
            "vmovntdq %%zmm24, 24*64(%0)   \n\t"
            "vmovntdq %%zmm25, 25*64(%0)   \n\t"
            "vmovntdq %%zmm26, 26*64(%0)   \n\t"
            "vmovntdq %%zmm27, 27*64(%0)   \n\t"
            "vmovntdq %%zmm28, 28*64(%0)   \n\t"
            "vmovntdq %%zmm29, 29*64(%0)   \n\t"
            "vmovntdq %%zmm30, 30*64(%0)   \n\t"
            "vmovntdq %%zmm31, 31*64(%0)   \n\t"
            :
            : "r"(dest_ptr + base_offset), "r"(src_ptr + base_offset)
            : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5", "zmm6", "zmm7",
              "zmm8", "zmm9", "zmm10", "zmm11", "zmm12", "zmm13", "zmm14", "zmm15",
              "zmm16", "zmm17", "zmm18", "zmm19", "zmm20", "zmm21", "zmm22", "zmm23",
              "zmm24", "zmm25", "zmm26", "zmm27", "zmm28", "zmm29", "zmm30", "zmm31", "memory"
        );
    }
    _mm_sfence();
}
static __attribute__((always_inline)) void fast_copy_fully_aligned(void *dest, void *src, unsigned int size) {
    uint8_t *dest_ptr = (uint8_t*)dest;
    uint8_t *src_ptr = (uint8_t*)src;
    const size_t chunk_size = 1024;
    const size_t num_chunks = size / chunk_size;
    
    #pragma omp parallel for
    for (size_t chunk = 0; chunk < num_chunks; ++chunk) {
        size_t base_offset = chunk * chunk_size;
        
        if ((chunk + 1) * chunk_size < size) {
            _mm_prefetch((char*)(src_ptr + base_offset + chunk_size), _MM_HINT_T0);
            _mm_prefetch((char*)(src_ptr + base_offset + chunk_size + 64), _MM_HINT_T0);
            _mm_prefetch((char*)(src_ptr + base_offset + chunk_size + 256), _MM_HINT_T1);
            _mm_prefetch((char*)(src_ptr + base_offset + chunk_size + 512), _MM_HINT_T2);
        }
        asm volatile (
            "vmovdqa64 0*64(%1), %%zmm0    \n\t"
            "vmovdqa64 1*64(%1), %%zmm1    \n\t"
            "vmovdqa64 2*64(%1), %%zmm2    \n\t"
            "vmovdqa64 3*64(%1), %%zmm3    \n\t"
            "vmovdqa64 4*64(%1), %%zmm4    \n\t"
            "vmovdqa64 5*64(%1), %%zmm5    \n\t"
            "vmovdqa64 6*64(%1), %%zmm6    \n\t"
            "vmovdqa64 7*64(%1), %%zmm7    \n\t"
            "vmovdqa64 8*64(%1), %%zmm8    \n\t"
            "vmovdqa64 9*64(%1), %%zmm9    \n\t"
            "vmovdqa64 10*64(%1), %%zmm10  \n\t"
            "vmovdqa64 11*64(%1), %%zmm11  \n\t"
            "vmovdqa64 12*64(%1), %%zmm12  \n\t"
            "vmovdqa64 13*64(%1), %%zmm13  \n\t"
            "vmovdqa64 14*64(%1), %%zmm14  \n\t"
            "vmovdqa64 15*64(%1), %%zmm15  \n\t"
            "vmovdqa64 16*64(%1), %%zmm16  \n\t"
            "vmovntdq %%zmm0,  0*64(%0)    \n\t"
            "vmovdqa64 17*64(%1), %%zmm17  \n\t"
            "vmovntdq %%zmm1,  1*64(%0)    \n\t"
            "vmovdqa64 18*64(%1), %%zmm18  \n\t"
            "vmovntdq %%zmm2,  2*64(%0)    \n\t"
            "vmovdqa64 19*64(%1), %%zmm19  \n\t"
            "vmovntdq %%zmm3,  3*64(%0)    \n\t"
            "vmovdqa64 20*64(%1), %%zmm20  \n\t"
            "vmovntdq %%zmm4,  4*64(%0)    \n\t"
            "vmovdqa64 21*64(%1), %%zmm21  \n\t"
            "vmovntdq %%zmm5,  5*64(%0)    \n\t"
            "vmovdqa64 22*64(%1), %%zmm22  \n\t"
            "vmovntdq %%zmm6,  6*64(%0)    \n\t"
            "vmovdqa64 23*64(%1), %%zmm23  \n\t"
            "vmovntdq %%zmm7,  7*64(%0)    \n\t"
            "vmovdqa64 24*64(%1), %%zmm24  \n\t"
            "vmovntdq %%zmm8,  8*64(%0)    \n\t"
            "vmovdqa64 25*64(%1), %%zmm25  \n\t"
            "vmovntdq %%zmm9,  9*64(%0)    \n\t"
            "vmovdqa64 26*64(%1), %%zmm26  \n\t"
            "vmovntdq %%zmm10, 10*64(%0)   \n\t"
            "vmovdqa64 27*64(%1), %%zmm27  \n\t"
            "vmovntdq %%zmm11, 11*64(%0)   \n\t"
            "vmovdqa64 28*64(%1), %%zmm28  \n\t"
            "vmovntdq %%zmm12, 12*64(%0)   \n\t"
            "vmovdqa64 29*64(%1), %%zmm29  \n\t"
            "vmovntdq %%zmm13, 13*64(%0)   \n\t"
            "vmovdqa64 30*64(%1), %%zmm30  \n\t"
            "vmovntdq %%zmm14, 14*64(%0)   \n\t"
            "vmovdqa64 31*64(%1), %%zmm31  \n\t"
            "vmovntdq %%zmm15, 15*64(%0)   \n\t"
            "vmovntdq %%zmm16, 16*64(%0)   \n\t"
            "vmovntdq %%zmm17, 17*64(%0)   \n\t"
            "vmovntdq %%zmm18, 18*64(%0)   \n\t"
            "vmovntdq %%zmm19, 19*64(%0)   \n\t"
            "vmovntdq %%zmm20, 20*64(%0)   \n\t"
            "vmovntdq %%zmm21, 21*64(%0)   \n\t"
            "vmovntdq %%zmm22, 22*64(%0)   \n\t"
            "vmovntdq %%zmm23, 23*64(%0)   \n\t"
            "vmovntdq %%zmm24, 24*64(%0)   \n\t"
            "vmovntdq %%zmm25, 25*64(%0)   \n\t"
            "vmovntdq %%zmm26, 26*64(%0)   \n\t"
            "vmovntdq %%zmm27, 27*64(%0)   \n\t"
            "vmovntdq %%zmm28, 28*64(%0)   \n\t"
            "vmovntdq %%zmm29, 29*64(%0)   \n\t"
            "vmovntdq %%zmm30, 30*64(%0)   \n\t"
            "vmovntdq %%zmm31, 31*64(%0)   \n\t"
            :
            : "r"(dest_ptr + base_offset), "r"(src_ptr + base_offset)
            : "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5", "zmm6", "zmm7",
              "zmm8", "zmm9", "zmm10", "zmm11", "zmm12", "zmm13", "zmm14", "zmm15",
              "zmm16", "zmm17", "zmm18", "zmm19", "zmm20", "zmm21", "zmm22", "zmm23",
              "zmm24", "zmm25", "zmm26", "zmm27", "zmm28", "zmm29", "zmm30", "zmm31", "memory"
        );
    }
    _mm_sfence();
}
static __attribute__((always_inline)) void merge_hash(const uint8_t block1[64], 
    const uint8_t block2[64],
    uint8_t output[64]) {
    _mm_prefetch((char*)block1, _MM_HINT_T0);
    _mm_prefetch((char*)block2, _MM_HINT_T0);
    const uint32_t *w1 = (const uint32_t*)block1;
    const uint32_t *w2 = (const uint32_t*)block2;
    __m256i rev_idx = _mm256_setr_epi32(7, 6, 5, 4, 3, 2, 1, 0);
    __m256i w1_data = _mm256_load_si256((__m256i*)w1);
    __m256i w2_data = _mm256_load_si256((__m256i*)w2);
    __m256i w2_rev = _mm256_permutevar8x32_epi32(w2_data, rev_idx);
    __m256i w1_rev = _mm256_permutevar8x32_epi32(w1_data, rev_idx);
    __m256i state_low = _mm256_xor_si256(w1_data, w2_rev);
    __m256i state_high = _mm256_xor_si256(w2_data, w1_rev);
    __m128i state_0_3 = _mm256_extracti128_si256(state_low, 0);
    __m128i state_4_7 = _mm256_extracti128_si256(state_low, 1);
    __m128i state_8_11 = _mm256_extracti128_si256(state_high, 0);
    __m128i state_12_15 = _mm256_extracti128_si256(state_high, 1);
    asm volatile(

        "movdqa     %0, %%xmm0                 \n\t"  // state_0_3
        "movdqa     %1, %%xmm1                 \n\t"  // state_4_7
        "movdqa     %2, %%xmm2                 \n\t"  // state_8_11
        "movdqa     %3, %%xmm3                 \n\t"  // state_12_15

        "paddd      %%xmm1, %%xmm0             \n\t"  // state_0_3 += state_4_7
        "paddd      %%xmm3, %%xmm2             \n\t"  // state_8_11 += state_12_15

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $7, %%xmm0                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm2, %%xmm4             \n\t"
        "pslld      $7, %%xmm2                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm2             \n\t"
        
        "paddd      %%xmm2, %%xmm0             \n\t"  // state_0_3 += state_8_11
        "paddd      %%xmm3, %%xmm1             \n\t"  // state_4_7 += state_12_15

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $9, %%xmm0                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm1, %%xmm4             \n\t"
        "pslld      $9, %%xmm1                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm1             \n\t"

        "paddd      %%xmm1, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm2             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $7, %%xmm0                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm2, %%xmm4             \n\t"
        "pslld      $7, %%xmm2                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm2             \n\t"
        
        "paddd      %%xmm2, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm1             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $9, %%xmm0                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm1, %%xmm4             \n\t"
        "pslld      $9, %%xmm1                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm1             \n\t"

        "paddd      %%xmm1, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm2             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $7, %%xmm0                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm2, %%xmm4             \n\t"
        "pslld      $7, %%xmm2                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm2             \n\t"
        
        "paddd      %%xmm2, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm1             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $9, %%xmm0                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm1, %%xmm4             \n\t"
        "pslld      $9, %%xmm1                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm1             \n\t"

        "paddd      %%xmm1, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm2             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $7, %%xmm0                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm2, %%xmm4             \n\t"
        "pslld      $7, %%xmm2                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm2             \n\t"
        
        "paddd      %%xmm2, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm1             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $9, %%xmm0                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm1, %%xmm4             \n\t"
        "pslld      $9, %%xmm1                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm1             \n\t"

        "paddd      %%xmm1, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm2             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $7, %%xmm0                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm2, %%xmm4             \n\t"
        "pslld      $7, %%xmm2                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm2             \n\t"
        
        "paddd      %%xmm2, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm1             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $9, %%xmm0                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"
        
        "movdqa     %%xmm1, %%xmm4             \n\t"
        "pslld      $9, %%xmm1                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm1             \n\t"

        "paddd      %%xmm1, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm2             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $7, %%xmm0                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm2, %%xmm4             \n\t"
        "pslld      $7, %%xmm2                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm2             \n\t"
        
        "paddd      %%xmm2, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm1             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $9, %%xmm0                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm1, %%xmm4             \n\t"
        "pslld      $9, %%xmm1                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm1             \n\t"

        "paddd      %%xmm1, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm2             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $7, %%xmm0                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm2, %%xmm4             \n\t"
        "pslld      $7, %%xmm2                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm2             \n\t"
        
        "paddd      %%xmm2, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm1             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $9, %%xmm0                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm1, %%xmm4             \n\t"
        "pslld      $9, %%xmm1                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm1             \n\t"

        "paddd      %%xmm1, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm2             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $7, %%xmm0                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm2, %%xmm4             \n\t"
        "pslld      $7, %%xmm2                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm2             \n\t"
        
        "paddd      %%xmm2, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm1             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $9, %%xmm0                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm1, %%xmm4             \n\t"
        "pslld      $9, %%xmm1                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm1             \n\t"

        "paddd      %%xmm1, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm2             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $7, %%xmm0                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"

        "movdqa     %%xmm2, %%xmm4             \n\t"
        "pslld      $7, %%xmm2                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm2             \n\t"
        
        "paddd      %%xmm2, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm1             \n\t"

        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $9, %%xmm0                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"
        
        "movdqa     %%xmm1, %%xmm4             \n\t"
        "pslld      $9, %%xmm1                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm1             \n\t"

        "paddd      %%xmm1, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm2             \n\t"
        
        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $7, %%xmm0                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"
        "movdqa     %%xmm2, %%xmm4             \n\t"
        "pslld      $7, %%xmm2                 \n\t"
        "psrld      $25, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm2             \n\t"
        
        "paddd      %%xmm2, %%xmm0             \n\t"
        "paddd      %%xmm3, %%xmm1             \n\t"
        "movdqa     %%xmm0, %%xmm4             \n\t"
        "pslld      $9, %%xmm0                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm0             \n\t"
        "movdqa     %%xmm1, %%xmm4             \n\t"
        "pslld      $9, %%xmm1                 \n\t"
        "psrld      $23, %%xmm4                \n\t"
        "por        %%xmm4, %%xmm1             \n\t"
        "pshufd     $0x1B, %%xmm3, %%xmm4        \n\t"
        "paddd      %%xmm4, %%xmm0               \n\t"
        
        "pshufd     $0x1B, %%xmm2, %%xmm4        \n\t"
        "paddd      %%xmm4, %%xmm1               \n\t"

        "movntdq    %%xmm0, 0(%4)                \n\t"
        "movntdq    %%xmm1, 16(%4)               \n\t"
        "movntdq    %%xmm2, 32(%4)               \n\t"
        "movntdq    %%xmm3, 48(%4)               \n\t"
        "sfence                                  \n\t"
        
        : "+m" (state_0_3), "+m" (state_4_7), "+m" (state_8_11), "+m" (state_12_15)
        : "r" (output)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "memory"
    );
}
static __attribute__((always_inline)) void merge_hash_simd_8blocks(
    const uint8_t* block1_1, const uint8_t* block1_2, 
    const uint8_t* block2_1, const uint8_t* block2_2,
    const uint8_t* block3_1, const uint8_t* block3_2,
    const uint8_t* block4_1, const uint8_t* block4_2,
    uint8_t* output1, uint8_t* output2, 
    uint8_t* output3, uint8_t* output4) {
    _mm_prefetch((char*)block1_1, _MM_HINT_T0);
    _mm_prefetch((char*)block1_2, _MM_HINT_T0);
    _mm_prefetch((char*)block2_1, _MM_HINT_T0);
    _mm_prefetch((char*)block2_2, _MM_HINT_T0);
    _mm_prefetch((char*)block3_1, _MM_HINT_T0);
    _mm_prefetch((char*)block3_2, _MM_HINT_T0);
    _mm_prefetch((char*)block4_1, _MM_HINT_T0);
    _mm_prefetch((char*)block4_2, _MM_HINT_T0);
    const uint32_t *w1_1 = (const uint32_t*)block1_1;
    const uint32_t *w2_1 = (const uint32_t*)block2_1;
    const uint32_t *w1_2 = (const uint32_t*)block1_2;
    const uint32_t *w2_2 = (const uint32_t*)block2_2;
    const uint32_t *w3_1 = (const uint32_t*)block3_1;
    const uint32_t *w4_1 = (const uint32_t*)block4_1;
    const uint32_t *w3_2 = (const uint32_t*)block3_2;
    const uint32_t *w4_2 = (const uint32_t*)block4_2;
    __m512i state_0, state_1, state_2, state_3;
    __m512i w1_data = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_load_si256((__m256i*)w1_1)),
        _mm256_load_si256((__m256i*)w2_1), 
        1
    );
    
    __m512i w2_data = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_load_si256((__m256i*)w1_2)),
        _mm256_load_si256((__m256i*)w2_2), 
        1
    );
    
    __m512i w3_data = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_load_si256((__m256i*)w3_1)),
        _mm256_load_si256((__m256i*)w4_1), 
        1
    );
    
    __m512i w4_data = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_load_si256((__m256i*)w3_2)),
        _mm256_load_si256((__m256i*)w4_2), 
        1
    );
    __m512i rev_idx = _mm512_setr_epi32(
        7, 6, 5, 4, 3, 2, 1, 0,
        15, 14, 13, 12, 11, 10, 9, 8
    );
    __m512i w1_rev = _mm512_permutexvar_epi32(rev_idx,w1_data);
    __m512i w2_rev = _mm512_permutexvar_epi32(rev_idx, w2_data);
    __m512i w3_rev = _mm512_permutexvar_epi32(rev_idx, w3_data);
    __m512i w4_rev = _mm512_permutexvar_epi32(rev_idx, w4_data);
    state_0 = _mm512_xor_epi32(w1_data, w2_rev);
    state_1 = _mm512_xor_epi32(w2_data, w1_rev);
    state_2 = _mm512_xor_epi32(w3_data, w4_rev);
    state_3 = _mm512_xor_epi32(w4_data, w3_rev);
    __m256i s0_low = _mm512_extracti64x4_epi64(state_0, 0);  
    __m256i s0_high = _mm512_extracti64x4_epi64(state_0, 1);
    __m256i s2_low = _mm512_extracti64x4_epi64(state_2, 0); 
    __m256i s2_high = _mm512_extracti64x4_epi64(state_2, 1); 
    
    __m256i s1_low = _mm512_extracti64x4_epi64(state_1, 0); 
    __m256i s1_high = _mm512_extracti64x4_epi64(state_1, 1); 
    __m256i s3_low = _mm512_extracti64x4_epi64(state_3, 0); 
    __m256i s3_high = _mm512_extracti64x4_epi64(state_3, 1); 
    
    __m512i state_0_new = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_permute2x128_si256(s0_low, s0_high, 0x20)),
        _mm256_permute2x128_si256(s2_low, s2_high, 0x20), 
        1
    );
    __m512i state_1_new = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_permute2x128_si256(s0_low, s0_high, 0x31)),
        _mm256_permute2x128_si256(s2_low, s2_high, 0x31), 
        1
    );
    
    __m512i state_2_new = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_permute2x128_si256(s1_low, s1_high, 0x20)),
        _mm256_permute2x128_si256(s3_low, s3_high, 0x20), 
        1
    );
    
    __m512i state_3_new = _mm512_inserti64x4(
        _mm512_castsi256_si512(_mm256_permute2x128_si256(s1_low, s1_high, 0x31)),
        _mm256_permute2x128_si256(s3_low, s3_high, 0x31), 
        1
    );
    asm volatile(
        "vmovdqa32   %0, %%zmm0               \n\t" 
        "vmovdqa32   %1, %%zmm1               \n\t" 
        "vmovdqa32   %2, %%zmm2               \n\t" 
        "vmovdqa32   %3, %%zmm3               \n\t"  
        
        "vpaddd      %%zmm1, %%zmm0, %%zmm0   \n\t" 
        "vprold      $7, %%zmm0, %%zmm0       \n\t" 
        "vpaddd      %%zmm3, %%zmm2, %%zmm2   \n\t"  
        "vprold      $7, %%zmm2, %%zmm2       \n\t"  
        "vpaddd      %%zmm2, %%zmm0, %%zmm0   \n\t"  
        "vprold      $9, %%zmm0, %%zmm0       \n\t" 
        "vpaddd      %%zmm3, %%zmm1, %%zmm1   \n\t"  
        "vprold      $9, %%zmm1, %%zmm1       \n\t" 
        
        "vpaddd      %%zmm1, %%zmm0, %%zmm0   \n\t"
        "vprold      $7, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm2, %%zmm2   \n\t"
        "vprold      $7, %%zmm2, %%zmm2       \n\t"
        "vpaddd      %%zmm2, %%zmm0, %%zmm0   \n\t"
        "vprold      $9, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm1, %%zmm1   \n\t"
        "vprold      $9, %%zmm1, %%zmm1       \n\t"
        
        "vpaddd      %%zmm1, %%zmm0, %%zmm0   \n\t"
        "vprold      $7, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm2, %%zmm2   \n\t"
        "vprold      $7, %%zmm2, %%zmm2       \n\t"
        "vpaddd      %%zmm2, %%zmm0, %%zmm0   \n\t"
        "vprold      $9, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm1, %%zmm1   \n\t"
        "vprold      $9, %%zmm1, %%zmm1       \n\t"
        
        "vpaddd      %%zmm1, %%zmm0, %%zmm0   \n\t"
        "vprold      $7, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm2, %%zmm2   \n\t"
        "vprold      $7, %%zmm2, %%zmm2       \n\t"
        "vpaddd      %%zmm2, %%zmm0, %%zmm0   \n\t"
        "vprold      $9, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm1, %%zmm1   \n\t"
        "vprold      $9, %%zmm1, %%zmm1       \n\t"
        
        "vpaddd      %%zmm1, %%zmm0, %%zmm0   \n\t"
        "vprold      $7, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm2, %%zmm2   \n\t"
        "vprold      $7, %%zmm2, %%zmm2       \n\t"
        "vpaddd      %%zmm2, %%zmm0, %%zmm0   \n\t"
        "vprold      $9, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm1, %%zmm1   \n\t"
        "vprold      $9, %%zmm1, %%zmm1       \n\t"
        
        "vpaddd      %%zmm1, %%zmm0, %%zmm0   \n\t"
        "vprold      $7, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm2, %%zmm2   \n\t"
        "vprold      $7, %%zmm2, %%zmm2       \n\t"
        "vpaddd      %%zmm2, %%zmm0, %%zmm0   \n\t"
        "vprold      $9, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm1, %%zmm1   \n\t"
        "vprold      $9, %%zmm1, %%zmm1       \n\t"
        
        "vpaddd      %%zmm1, %%zmm0, %%zmm0   \n\t"
        "vprold      $7, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm2, %%zmm2   \n\t"
        "vprold      $7, %%zmm2, %%zmm2       \n\t"
        "vpaddd      %%zmm2, %%zmm0, %%zmm0   \n\t"
        "vprold      $9, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm1, %%zmm1   \n\t"
        "vprold      $9, %%zmm1, %%zmm1       \n\t"
        
        "vpaddd      %%zmm1, %%zmm0, %%zmm0   \n\t"
        "vprold      $7, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm2, %%zmm2   \n\t"
        "vprold      $7, %%zmm2, %%zmm2       \n\t"
        "vpaddd      %%zmm2, %%zmm0, %%zmm0   \n\t"
        "vprold      $9, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm1, %%zmm1   \n\t"
        "vprold      $9, %%zmm1, %%zmm1       \n\t"
        
        "vpaddd      %%zmm1, %%zmm0, %%zmm0   \n\t"
        "vprold      $7, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm2, %%zmm2   \n\t"
        "vprold      $7, %%zmm2, %%zmm2       \n\t"
        "vpaddd      %%zmm2, %%zmm0, %%zmm0   \n\t"
        "vprold      $9, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm1, %%zmm1   \n\t"
        "vprold      $9, %%zmm1, %%zmm1       \n\t"
        
        "vpaddd      %%zmm1, %%zmm0, %%zmm0   \n\t"
        "vprold      $7, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm2, %%zmm2   \n\t"
        "vprold      $7, %%zmm2, %%zmm2       \n\t"
        "vpaddd      %%zmm2, %%zmm0, %%zmm0   \n\t"
        "vprold      $9, %%zmm0, %%zmm0       \n\t"
        "vpaddd      %%zmm3, %%zmm1, %%zmm1   \n\t"
        "vprold      $9, %%zmm1, %%zmm1       \n\t"
        
        "vmovntdq   %%zmm0, %0               \n\t"
        "vmovntdq   %%zmm1, %1               \n\t"
        "vmovntdq   %%zmm2, %2               \n\t"
        "vmovntdq   %%zmm3, %3               \n\t"
        "sfence                              \n\t"
        : "+m" (state_0_new), "+m" (state_1_new), "+m" (state_2_new), "+m" (state_3_new)
        :
        : "zmm0", "zmm1", "zmm2", "zmm3", "memory"
    );
        
    __m512i rev_idx2 = _mm512_setr_epi32(
        3, 2, 1, 0, 7, 6, 5, 4,
        11, 10, 9, 8, 15, 14, 13, 12
    );
    __m512i state_2_rev = _mm512_permutexvar_epi32(rev_idx2, state_2_new);
    __m512i state_3_rev = _mm512_permutexvar_epi32(rev_idx2, state_3_new);
    state_0_new = _mm512_add_epi32(state_0_new, state_3_rev);
    state_1_new = _mm512_add_epi32(state_1_new, state_2_rev);
    
    __m128i output1_low = _mm512_extracti32x4_epi32(state_0_new, 0);
    __m128i output2_low = _mm512_extracti32x4_epi32(state_0_new, 1);
    __m128i output3_low = _mm512_extracti32x4_epi32(state_0_new, 2);
    __m128i output4_low = _mm512_extracti32x4_epi32(state_0_new, 3);

    __m128i output1_mid_low = _mm512_extracti32x4_epi32(state_1_new, 0);
    __m128i output2_mid_low = _mm512_extracti32x4_epi32(state_1_new, 1);
    __m128i output3_mid_low = _mm512_extracti32x4_epi32(state_1_new, 2);
    __m128i output4_mid_low = _mm512_extracti32x4_epi32(state_1_new, 3);

    __m128i output1_mid_high = _mm512_extracti32x4_epi32(state_2_new, 0);
    __m128i output2_mid_high = _mm512_extracti32x4_epi32(state_2_new, 1);
    __m128i output3_mid_high = _mm512_extracti32x4_epi32(state_2_new, 2);
    __m128i output4_mid_high = _mm512_extracti32x4_epi32(state_2_new, 3);

    __m128i output1_high = _mm512_extracti32x4_epi32(state_3_new, 0);
    __m128i output2_high = _mm512_extracti32x4_epi32(state_3_new, 1);
    __m128i output3_high = _mm512_extracti32x4_epi32(state_3_new, 2);
    __m128i output4_high = _mm512_extracti32x4_epi32(state_3_new, 3);

    _mm_store_si128((__m128i*)output1, output1_low);
    _mm_store_si128((__m128i*)(output1 + 16), output1_mid_low);
    _mm_store_si128((__m128i*)(output1 + 32), output1_mid_high);
    _mm_store_si128((__m128i*)(output1 + 48), output1_high);

    _mm_store_si128((__m128i*)output2, output2_low);
    _mm_store_si128((__m128i*)(output2 + 16), output2_mid_low);
    _mm_store_si128((__m128i*)(output2 + 32), output2_mid_high);
    _mm_store_si128((__m128i*)(output2 + 48), output2_high);

    _mm_store_si128((__m128i*)output3, output3_low);
    _mm_store_si128((__m128i*)(output3 + 16), output3_mid_low);
    _mm_store_si128((__m128i*)(output3 + 32), output3_mid_high);
    _mm_store_si128((__m128i*)(output3 + 48), output3_high);

    _mm_store_si128((__m128i*)output4, output4_low);
    _mm_store_si128((__m128i*)(output4 + 16), output4_mid_low);
    _mm_store_si128((__m128i*)(output4 + 32), output4_mid_high);
    _mm_store_si128((__m128i*)(output4 + 48), output4_high);
}
void merkel_tree(const uint8_t *input, uint8_t *output, size_t length){
    uint8_t *cur_buf, *prev_buf;
    int use_pool = 0;
    
    if (ensure_pool_initialized(length) == 0 && length <= g_memory_pool.pool_size) {
        // printf("Using memory pool for length: %zu\n", length);
        pthread_mutex_lock(&g_memory_pool.mutex);
        cur_buf = g_memory_pool.pool1;
        prev_buf = g_memory_pool.pool2;
        use_pool = 1;
        pthread_mutex_unlock(&g_memory_pool.mutex);
    } else {

        // printf("Using traditional memory allocation for length: %zu\n", length);
        cur_buf = (uint8_t *)aligned_alloc(64, length);
        prev_buf = (uint8_t *)aligned_alloc(64, length);
        
        if (!cur_buf || !prev_buf) {
            free(cur_buf);
            free(prev_buf);
            return;
        }
    }
    size_t remain=((uintptr_t)input&0x3f);
    if(remain==0){
        // printf("Input is fully aligned, using fast copy.\n");
        fast_copy_fully_aligned(prev_buf, input, length);
    }
    else{
        fast_copy_src_unaligned(prev_buf, input, length);
    }
    size_t cur_length = length;
    while (cur_length >= 128) {
        size_t new_length = cur_length >> 1;
        size_t blocks = new_length >> 6;
        
        #pragma omp parallel
        {
            #pragma omp for
            for (size_t i = 0; i < (blocks >> 2); ++i) {
                if (i + 1 < (blocks >> 2)) {
                    size_t next_offset = (i + 1) << 9;
                    _mm_prefetch((char*)(prev_buf + next_offset), _MM_HINT_T0);
                    _mm_prefetch((char*)(prev_buf + next_offset + 64), _MM_HINT_T0);
                    _mm_prefetch((char*)(prev_buf + next_offset + 256), _MM_HINT_T1);
                    _mm_prefetch((char*)(prev_buf + next_offset + 384), _MM_HINT_T1);
                    _mm_prefetch((char*)(cur_buf + ((i + 1) << 8)), _MM_HINT_T1);
                }
                uint8_t *prev_base = prev_buf + (i << 9);
                uint8_t *cur_base = cur_buf + (i << 8);
                merge_hash_simd_8blocks(
                    prev_base + 0,   prev_base + 64,  prev_base + 128, prev_base + 192,
                    prev_base + 256, prev_base + 320, prev_base + 384, prev_base + 448,
                    cur_base + 0,    cur_base + 64,   cur_base + 128,  cur_base + 192
                );
            }
            for (size_t i = ((blocks >> 2) << 2); i < blocks; ++i) {
                if (i + 1 < blocks) {
                    _mm_prefetch((char*)(prev_buf + ((i + 1) << 7)), _MM_HINT_T0);
                    _mm_prefetch((char*)(prev_buf + ((i + 1) << 7) + 64), _MM_HINT_T0);
                    _mm_prefetch((char*)(cur_buf + ((i + 1) << 6)), _MM_HINT_T1);
                }
                uint8_t *prev_base = prev_buf + (i << 7);
                uint8_t *cur_base = cur_buf + (i << 6);
                merge_hash(prev_base, prev_base + 64, cur_base);    
            }
        }
        cur_length = new_length;
        uint8_t *tmp = cur_buf;
        cur_buf = prev_buf;
        prev_buf = tmp;
    }
    size_t remain_output = ((uintptr_t)output & 0x3f);
    _mm_prefetch((char*)prev_buf, _MM_HINT_T0);
    if (remain_output == 0) {
        // printf("Output is fully aligned, using fast copy.\n");
        fast_copy_dest_aligned(output, prev_buf);
    } 
    else {
        fast_copy_dest_unaligned(output, prev_buf);
    }
    if (!use_pool) {
        free(cur_buf);
        free(prev_buf);
    }

}