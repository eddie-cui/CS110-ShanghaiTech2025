#include "mercha.h"
static __attribute__((always_inline)) void fast_copy(void *dest, void *src) {
    asm volatile (
        "vmovdqa64 (%1), %%zmm0\n\t"
        "vmovntdq %%zmm0, (%0)\n\t"
        :
        : "r"(dest), "r"(src)
        : "memory","zmm0"
    );
}

static __attribute__((always_inline)) void chacha_quarter_round(__m512i* x, size_t a, size_t b, size_t c, size_t d) {
    // x[a] = _mm512_add_epi32(x[a], x[b]);                // a += b
    // x[d] = _mm512_xor_epi32(x[d], x[a]);                // d ^= a
    // x[d] = _mm512_rol_epi32(x[d], 16);                  // d = ROTL(d, 16)
    
    // x[c] = _mm512_add_epi32(x[c], x[d]);                // c += d
    // x[b] = _mm512_xor_epi32(x[b], x[c]);                // b ^= c
    // x[b] = _mm512_rol_epi32(x[b], 12);                  // b = ROTL(b, 12)
    
    // x[a] = _mm512_add_epi32(x[a], x[b]);                // a += b
    // x[d] = _mm512_xor_epi32(x[d], x[a]);                // d ^= a
    // x[d] = _mm512_rol_epi32(x[d], 8);                   // d = ROTL(d, 8)
    
    // x[c] = _mm512_add_epi32(x[c], x[d]);                // c += d
    // x[b] = _mm512_xor_epi32(x[b], x[c]);                // b ^= c
    // x[b] = _mm512_rol_epi32(x[b], 7);                   // b = ROTL(b, 7)
    asm volatile (
        "movq %1, %%rax \n\t"            
        "movq %2, %%rbx \n\t"               
        "movq %3, %%rcx \n\t"             
        "movq %4, %%rdx \n\t"            
        
        "shlq $6, %%rax \n\t"               
        "shlq $6, %%rbx \n\t"               
        "shlq $6, %%rcx \n\t"              
        "shlq $6, %%rdx \n\t"              
        
        "vmovdqa64 (%0, %%rax), %%zmm0 \n\t"  
        "vmovdqa64 (%0, %%rbx), %%zmm1 \n\t"  
        "vmovdqa64 (%0, %%rcx), %%zmm2 \n\t"  
        "vmovdqa64 (%0, %%rdx), %%zmm3 \n\t"  
        
        "vpaddd %%zmm1, %%zmm0, %%zmm0 \n\t"
        
        "vpxord %%zmm0, %%zmm3, %%zmm3 \n\t"
        
        "vprold $16, %%zmm3, %%zmm3 \n\t"
        
        "vpaddd %%zmm3, %%zmm2, %%zmm2 \n\t"
        
        "vpxord %%zmm2, %%zmm1, %%zmm1 \n\t"
        
        "vprold $12, %%zmm1, %%zmm1 \n\t"
        
        "vpaddd %%zmm1, %%zmm0, %%zmm0 \n\t"
        
        "vpxord %%zmm0, %%zmm3, %%zmm3 \n\t"
        
        "vprold $8, %%zmm3, %%zmm3 \n\t"
        
        "vpaddd %%zmm3, %%zmm2, %%zmm2 \n\t"
        
        "vpxord %%zmm2, %%zmm1, %%zmm1 \n\t"
        
        "vprold $7, %%zmm1, %%zmm1 \n\t"
        
        "vmovdqa64 %%zmm0, (%0, %%rax) \n\t"
        "vmovdqa64 %%zmm1, (%0, %%rbx) \n\t"
        "vmovdqa64 %%zmm2, (%0, %%rcx) \n\t"
        "vmovdqa64 %%zmm3, (%0, %%rdx) \n\t"
        
        : 
        : "r"(x), "r"(a), "r"(b), "r"(c), "r"(d)
        : "%rax", "%rbx", "%rcx", "%rdx",
          "%zmm0", "%zmm1", "%zmm2", "%zmm3", "memory"
    );
}
static __attribute__((always_inline)) void transpose(__m512i* working_states) {
    _mm_prefetch((char*)&working_states[8], _MM_HINT_T0);
    
    __m512i vec_perm1 = _mm512_setr_epi64(2, 3, 0, 1, 6, 7, 4, 5);
    __m512i vec_perm2 = _mm512_setr_epi64(1, 0, 3, 2, 5, 4, 7, 6);
    __m512i vec_perm3 = _mm512_setr_epi32(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
    
    __m512i working_state[16];
    
    asm volatile (
        "vmovdqa64 (%1), %%zmm0 \n\t"
        "vmovdqa64 512(%1), %%zmm1 \n\t"
        "vshufi32x4 $0x44, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, (%0) \n\t"
        
        "vmovdqa64 64(%1), %%zmm0 \n\t"
        "vmovdqa64 576(%1), %%zmm1 \n\t"
        "vshufi32x4 $0x44, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 64(%0) \n\t"
        
        "vmovdqa64 128(%1), %%zmm0 \n\t"
        "vmovdqa64 640(%1), %%zmm1 \n\t"
        "vshufi32x4 $0x44, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 128(%0) \n\t"
        
        "vmovdqa64 192(%1), %%zmm0 \n\t"
        "vmovdqa64 704(%1), %%zmm1 \n\t"
        "vshufi32x4 $0x44, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 192(%0) \n\t"
        
        "vmovdqa64 256(%1), %%zmm0 \n\t"
        "vmovdqa64 768(%1), %%zmm1 \n\t"
        "vshufi32x4 $0x44, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 256(%0) \n\t"
        
        "vmovdqa64 320(%1), %%zmm0 \n\t"
        "vmovdqa64 832(%1), %%zmm1 \n\t"
        "vshufi32x4 $0x44, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 320(%0) \n\t"
        
        "vmovdqa64 384(%1), %%zmm0 \n\t"
        "vmovdqa64 896(%1), %%zmm1 \n\t"
        "vshufi32x4 $0x44, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 384(%0) \n\t"
        
        "vmovdqa64 448(%1), %%zmm0 \n\t"
        "vmovdqa64 960(%1), %%zmm1 \n\t"
        "vshufi32x4 $0x44, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 448(%0) \n\t" 

        "vmovdqa64 (%1), %%zmm0 \n\t"
        "vmovdqa64 512(%1), %%zmm1 \n\t"
        "vshufi32x4 $0xEE, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 512(%0) \n\t"
        
        "vmovdqa64 64(%1), %%zmm0 \n\t"
        "vmovdqa64 576(%1), %%zmm1 \n\t"
        "vshufi32x4 $0xEE, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 576(%0) \n\t"  
        
        "vmovdqa64 128(%1), %%zmm0 \n\t"
        "vmovdqa64 640(%1), %%zmm1 \n\t"
        "vshufi32x4 $0xEE, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 640(%0) \n\t"  
        
        "vmovdqa64 192(%1), %%zmm0 \n\t"
        "vmovdqa64 704(%1), %%zmm1 \n\t"
        "vshufi32x4 $0xEE, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 704(%0) \n\t" 
        
        "vmovdqa64 256(%1), %%zmm0 \n\t"
        "vmovdqa64 768(%1), %%zmm1 \n\t"
        "vshufi32x4 $0xEE, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 768(%0) \n\t"  
        
        "vmovdqa64 320(%1), %%zmm0 \n\t"
        "vmovdqa64 832(%1), %%zmm1 \n\t"
        "vshufi32x4 $0xEE, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 832(%0) \n\t"    
        
        "vmovdqa64 384(%1), %%zmm0 \n\t"
        "vmovdqa64 896(%1), %%zmm1 \n\t"
        "vshufi32x4 $0xEE, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 896(%0) \n\t"  
        
        "vmovdqa64 448(%1), %%zmm0 \n\t"
        "vmovdqa64 960(%1), %%zmm1 \n\t"
        "vshufi32x4 $0xEE, %%zmm1, %%zmm0, %%zmm2 \n\t"
        "vmovdqa64 %%zmm2, 960(%0) \n\t"  
        
        :
        : "r"(working_state), "r"(working_states)
        : "%zmm0", "%zmm1", "%zmm2", "memory"
    );
    
    working_states[0] = _mm512_mask_permutexvar_epi64(working_state[0], 0xcc, vec_perm1, working_state[4]);
    working_states[1] = _mm512_mask_permutexvar_epi64(working_state[1], 0xcc, vec_perm1, working_state[5]);
    working_states[2] = _mm512_mask_permutexvar_epi64(working_state[2], 0xcc, vec_perm1, working_state[6]);
    working_states[3] = _mm512_mask_permutexvar_epi64(working_state[3], 0xcc, vec_perm1, working_state[7]);
    working_states[8] = _mm512_mask_permutexvar_epi64(working_state[8], 0xcc, vec_perm1, working_state[12]);
    working_states[9] = _mm512_mask_permutexvar_epi64(working_state[9], 0xcc, vec_perm1, working_state[13]);
    working_states[10] = _mm512_mask_permutexvar_epi64(working_state[10], 0xcc, vec_perm1, working_state[14]);
    working_states[11] = _mm512_mask_permutexvar_epi64(working_state[11], 0xcc, vec_perm1, working_state[15]);

    working_states[4] = _mm512_mask_permutexvar_epi64(working_state[4], 0x33, vec_perm1, working_state[0]);
    working_states[5] = _mm512_mask_permutexvar_epi64(working_state[5], 0x33, vec_perm1, working_state[1]);
    working_states[6] = _mm512_mask_permutexvar_epi64(working_state[6], 0x33, vec_perm1, working_state[2]);
    working_states[7] = _mm512_mask_permutexvar_epi64(working_state[7], 0x33, vec_perm1, working_state[3]);
    working_states[12] = _mm512_mask_permutexvar_epi64(working_state[12], 0x33, vec_perm1, working_state[8]);
    working_states[13] = _mm512_mask_permutexvar_epi64(working_state[13], 0x33, vec_perm1, working_state[9]);
    working_states[14] = _mm512_mask_permutexvar_epi64(working_state[14], 0x33, vec_perm1, working_state[10]);
    working_states[15] = _mm512_mask_permutexvar_epi64(working_state[15], 0x33, vec_perm1, working_state[11]);

    working_state[0] = _mm512_mask_permutexvar_epi64(working_states[0], 0xaa, vec_perm2, working_states[2]);
    working_state[1] = _mm512_mask_permutexvar_epi64(working_states[1], 0xaa, vec_perm2, working_states[3]);
    working_state[4] = _mm512_mask_permutexvar_epi64(working_states[4], 0xaa, vec_perm2, working_states[6]);
    working_state[5] = _mm512_mask_permutexvar_epi64(working_states[5], 0xaa, vec_perm2, working_states[7]);
    working_state[8] = _mm512_mask_permutexvar_epi64(working_states[8], 0xaa, vec_perm2, working_states[10]);
    working_state[9] = _mm512_mask_permutexvar_epi64(working_states[9], 0xaa, vec_perm2, working_states[11]);
    working_state[12] = _mm512_mask_permutexvar_epi64(working_states[12], 0xaa, vec_perm2, working_states[14]);
    working_state[13] = _mm512_mask_permutexvar_epi64(working_states[13], 0xaa, vec_perm2, working_states[15]);
  
    working_state[2] = _mm512_mask_permutexvar_epi64(working_states[2], 0x55, vec_perm2, working_states[0]);
    working_state[3] = _mm512_mask_permutexvar_epi64(working_states[3], 0x55, vec_perm2, working_states[1]);
    working_state[6] = _mm512_mask_permutexvar_epi64(working_states[6], 0x55, vec_perm2, working_states[4]);
    working_state[7] = _mm512_mask_permutexvar_epi64(working_states[7], 0x55, vec_perm2, working_states[5]);
    working_state[10] = _mm512_mask_permutexvar_epi64(working_states[10], 0x55, vec_perm2, working_states[8]);
    working_state[11] = _mm512_mask_permutexvar_epi64(working_states[11], 0x55, vec_perm2, working_states[9]);
    working_state[14] = _mm512_mask_permutexvar_epi64(working_states[14], 0x55, vec_perm2, working_states[12]);
    working_state[15] = _mm512_mask_permutexvar_epi64(working_states[15], 0x55, vec_perm2, working_states[13]);

    working_states[0] = _mm512_mask_permutexvar_epi32(working_state[0], 0xaaaa, vec_perm3, working_state[1]);
    working_states[2] = _mm512_mask_permutexvar_epi32(working_state[2], 0xaaaa, vec_perm3, working_state[3]);
    working_states[4] = _mm512_mask_permutexvar_epi32(working_state[4], 0xaaaa, vec_perm3, working_state[5]);
    working_states[6] = _mm512_mask_permutexvar_epi32(working_state[6], 0xaaaa, vec_perm3, working_state[7]);
    working_states[8] = _mm512_mask_permutexvar_epi32(working_state[8], 0xaaaa, vec_perm3, working_state[9]);
    working_states[10] = _mm512_mask_permutexvar_epi32(working_state[10], 0xaaaa, vec_perm3, working_state[11]);
    working_states[12] = _mm512_mask_permutexvar_epi32(working_state[12], 0xaaaa, vec_perm3, working_state[13]);
    working_states[14] = _mm512_mask_permutexvar_epi32(working_state[14], 0xaaaa, vec_perm3, working_state[15]);
  
    working_states[1] = _mm512_mask_permutexvar_epi32(working_state[1], 0x5555, vec_perm3, working_state[0]);
    working_states[3] = _mm512_mask_permutexvar_epi32(working_state[3], 0x5555, vec_perm3, working_state[2]);
    working_states[5] = _mm512_mask_permutexvar_epi32(working_state[5], 0x5555, vec_perm3, working_state[4]);
    working_states[7] = _mm512_mask_permutexvar_epi32(working_state[7], 0x5555, vec_perm3, working_state[6]);
    working_states[9] = _mm512_mask_permutexvar_epi32(working_state[9], 0x5555, vec_perm3, working_state[8]);
    working_states[11] = _mm512_mask_permutexvar_epi32(working_state[11], 0x5555, vec_perm3, working_state[10]);
    working_states[13] = _mm512_mask_permutexvar_epi32(working_state[13], 0x5555, vec_perm3, working_state[12]);
    working_states[15] = _mm512_mask_permutexvar_epi32(working_state[15], 0x5555, vec_perm3, working_state[14]);
}
static __attribute__((always_inline)) void chacha20_block(uint32_t state[16], __m512i *working_state) {
    _mm_prefetch((char*)state, _MM_HINT_T0);
    ALIGN(64) static const uint32_t inc_data[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    
    asm volatile (
        "vmovdqa32 (%2), %%zmm31 \n\t"
        "movl (%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm0, %%xmm0 \n\t"
        "vpbroadcastd %%xmm0, %%zmm0 \n\t"
        
        "movl 4(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm1, %%xmm1 \n\t"
        "vpbroadcastd %%xmm1, %%zmm1 \n\t"
        
        "movl 8(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm2, %%xmm2 \n\t"
        "vpbroadcastd %%xmm2, %%zmm2 \n\t"
        
        "movl 12(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm3, %%xmm3 \n\t"
        "vpbroadcastd %%xmm3, %%zmm3 \n\t"
        
        "movl 16(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm4, %%xmm4 \n\t"
        "vpbroadcastd %%xmm4, %%zmm4 \n\t"
        
        "movl 20(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm5, %%xmm5 \n\t"
        "vpbroadcastd %%xmm5, %%zmm5 \n\t"
        
        "movl 24(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm6, %%xmm6 \n\t"
        "vpbroadcastd %%xmm6, %%zmm6 \n\t"
        
        "movl 28(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm7, %%xmm7 \n\t"
        "vpbroadcastd %%xmm7, %%zmm7 \n\t"
        
        "movl 32(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm8, %%xmm8 \n\t"
        "vpbroadcastd %%xmm8, %%zmm8 \n\t"
        
        "movl 36(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm9, %%xmm9 \n\t"
        "vpbroadcastd %%xmm9, %%zmm9 \n\t"
        
        "movl 40(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm10, %%xmm10 \n\t"
        "vpbroadcastd %%xmm10, %%zmm10 \n\t"
        
        "movl 44(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm11, %%xmm11 \n\t"
        "vpbroadcastd %%xmm11, %%zmm11 \n\t"
        
        "movl 48(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm12, %%xmm12 \n\t"
        "vpbroadcastd %%xmm12, %%zmm12 \n\t"
        "vpaddd %%zmm31, %%zmm12, %%zmm12 \n\t"
        
        "movl 52(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm13, %%xmm13 \n\t"
        "vpbroadcastd %%xmm13, %%zmm13 \n\t"
        
        "movl 56(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm14, %%xmm14 \n\t"
        "vpbroadcastd %%xmm14, %%zmm14 \n\t"
        
        "movl 60(%1), %%eax \n\t"
        "vpinsrd $0, %%eax, %%xmm15, %%xmm15 \n\t"
        "vpbroadcastd %%xmm15, %%zmm15 \n\t"
        
        "vmovdqa32 %%zmm0, 0*64(%0) \n\t"
        "vmovdqa32 %%zmm1, 1*64(%0) \n\t"
        "vmovdqa32 %%zmm2, 2*64(%0) \n\t"
        "vmovdqa32 %%zmm3, 3*64(%0) \n\t"
        "vmovdqa32 %%zmm4, 4*64(%0) \n\t"
        "vmovdqa32 %%zmm5, 5*64(%0) \n\t"
        "vmovdqa32 %%zmm6, 6*64(%0) \n\t"
        "vmovdqa32 %%zmm7, 7*64(%0) \n\t"
        "vmovdqa32 %%zmm8, 8*64(%0) \n\t"
        "vmovdqa32 %%zmm9, 9*64(%0) \n\t"
        "vmovdqa32 %%zmm10, 10*64(%0) \n\t"
        "vmovdqa32 %%zmm11, 11*64(%0) \n\t"
        "vmovdqa32 %%zmm12, 12*64(%0) \n\t"
        "vmovdqa32 %%zmm13, 13*64(%0) \n\t"
        "vmovdqa32 %%zmm14, 14*64(%0) \n\t"
        "vmovdqa32 %%zmm15, 15*64(%0) \n\t"
        
        :
        : "r"(working_state), "r"(state), "r"(inc_data)
        : "%rax", "%zmm0", "%zmm1", "%zmm2", "%zmm3", "%zmm4", "%zmm5", "%zmm6", "%zmm7",
          "%zmm8", "%zmm9", "%zmm10", "%zmm11", "%zmm12", "%zmm13", "%zmm14", "%zmm15", 
          "%zmm31", "memory"
    );
    chacha_quarter_round(working_state, 0, 4, 8, 12);
    chacha_quarter_round(working_state, 1, 5, 9, 13);
    chacha_quarter_round(working_state, 2, 6, 10, 14);
    chacha_quarter_round(working_state, 3, 7, 11, 15);
        
    chacha_quarter_round(working_state, 0, 5, 10, 15);
    chacha_quarter_round(working_state, 1, 6, 11, 12);
    chacha_quarter_round(working_state, 2, 7, 8, 13);
    chacha_quarter_round(working_state, 3, 4, 9, 14);
    chacha_quarter_round(working_state, 0, 4, 8, 12);
    chacha_quarter_round(working_state, 1, 5, 9, 13);
    chacha_quarter_round(working_state, 2, 6, 10, 14);
    chacha_quarter_round(working_state, 3, 7, 11, 15);
        
    chacha_quarter_round(working_state, 0, 5, 10, 15);
    chacha_quarter_round(working_state, 1, 6, 11, 12);
    chacha_quarter_round(working_state, 2, 7, 8, 13);
    chacha_quarter_round(working_state, 3, 4, 9, 14);
    chacha_quarter_round(working_state, 0, 4, 8, 12);
    chacha_quarter_round(working_state, 1, 5, 9, 13);
    chacha_quarter_round(working_state, 2, 6, 10, 14);
    chacha_quarter_round(working_state, 3, 7, 11, 15);
        
    chacha_quarter_round(working_state, 0, 5, 10, 15);
    chacha_quarter_round(working_state, 1, 6, 11, 12);
    chacha_quarter_round(working_state, 2, 7, 8, 13);
    chacha_quarter_round(working_state, 3, 4, 9, 14);
    chacha_quarter_round(working_state, 0, 4, 8, 12);
    chacha_quarter_round(working_state, 1, 5, 9, 13);
    chacha_quarter_round(working_state, 2, 6, 10, 14);
    chacha_quarter_round(working_state, 3, 7, 11, 15);
        
    chacha_quarter_round(working_state, 0, 5, 10, 15);
    chacha_quarter_round(working_state, 1, 6, 11, 12);
    chacha_quarter_round(working_state, 2, 7, 8, 13);
    chacha_quarter_round(working_state, 3, 4, 9, 14);
    chacha_quarter_round(working_state, 0, 4, 8, 12);
    chacha_quarter_round(working_state, 1, 5, 9, 13);
    chacha_quarter_round(working_state, 2, 6, 10, 14);
    chacha_quarter_round(working_state, 3, 7, 11, 15);
        
    chacha_quarter_round(working_state, 0, 5, 10, 15);
    chacha_quarter_round(working_state, 1, 6, 11, 12);
    chacha_quarter_round(working_state, 2, 7, 8, 13);
    chacha_quarter_round(working_state, 3, 4, 9, 14);
    chacha_quarter_round(working_state, 0, 4, 8, 12);
    chacha_quarter_round(working_state, 1, 5, 9, 13);
    chacha_quarter_round(working_state, 2, 6, 10, 14);
    chacha_quarter_round(working_state, 3, 7, 11, 15);
        
    chacha_quarter_round(working_state, 0, 5, 10, 15);
    chacha_quarter_round(working_state, 1, 6, 11, 12);
    chacha_quarter_round(working_state, 2, 7, 8, 13);
    chacha_quarter_round(working_state, 3, 4, 9, 14);
    chacha_quarter_round(working_state, 0, 4, 8, 12);
    chacha_quarter_round(working_state, 1, 5, 9, 13);
    chacha_quarter_round(working_state, 2, 6, 10, 14);
    chacha_quarter_round(working_state, 3, 7, 11, 15);
        
    chacha_quarter_round(working_state, 0, 5, 10, 15);
    chacha_quarter_round(working_state, 1, 6, 11, 12);
    chacha_quarter_round(working_state, 2, 7, 8, 13);
    chacha_quarter_round(working_state, 3, 4, 9, 14);
    chacha_quarter_round(working_state, 0, 4, 8, 12);
    chacha_quarter_round(working_state, 1, 5, 9, 13);
    chacha_quarter_round(working_state, 2, 6, 10, 14);
    chacha_quarter_round(working_state, 3, 7, 11, 15);
        
    chacha_quarter_round(working_state, 0, 5, 10, 15);
    chacha_quarter_round(working_state, 1, 6, 11, 12);
    chacha_quarter_round(working_state, 2, 7, 8, 13);
    chacha_quarter_round(working_state, 3, 4, 9, 14);
    chacha_quarter_round(working_state, 0, 4, 8, 12);
    chacha_quarter_round(working_state, 1, 5, 9, 13);
    chacha_quarter_round(working_state, 2, 6, 10, 14);
    chacha_quarter_round(working_state, 3, 7, 11, 15);
        
    chacha_quarter_round(working_state, 0, 5, 10, 15);
    chacha_quarter_round(working_state, 1, 6, 11, 12);
    chacha_quarter_round(working_state, 2, 7, 8, 13);
    chacha_quarter_round(working_state, 3, 4, 9, 14);
    chacha_quarter_round(working_state, 0, 4, 8, 12);
    chacha_quarter_round(working_state, 1, 5, 9, 13);
    chacha_quarter_round(working_state, 2, 6, 10, 14);
    chacha_quarter_round(working_state, 3, 7, 11, 15);
        
    chacha_quarter_round(working_state, 0, 5, 10, 15);
    chacha_quarter_round(working_state, 1, 6, 11, 12);
    chacha_quarter_round(working_state, 2, 7, 8, 13);
    chacha_quarter_round(working_state, 3, 4, 9, 14);
    transpose(working_state);
    asm volatile (
        "vmovdqa64 (%1), %%zmm31 \n\t"
        
        "vmovdqa64 0*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 0*64(%0) \n\t"
        
        "vmovdqa64 1*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 1*64(%0) \n\t"
        
        "vmovdqa64 2*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 2*64(%0) \n\t"
        
        "vmovdqa64 3*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 3*64(%0) \n\t"
        
        "vmovdqa64 4*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 4*64(%0) \n\t"
        
        "vmovdqa64 5*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 5*64(%0) \n\t"
        
        "vmovdqa64 6*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 6*64(%0) \n\t"
        
        "vmovdqa64 7*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 7*64(%0) \n\t"
        
        "vmovdqa64 8*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 8*64(%0) \n\t"
        
        "vmovdqa64 9*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 9*64(%0) \n\t"
        
        "vmovdqa64 10*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 10*64(%0) \n\t"
        
        "vmovdqa64 11*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 11*64(%0) \n\t"
        
        "vmovdqa64 12*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 12*64(%0) \n\t"
        
        "vmovdqa64 13*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 13*64(%0) \n\t"
        
        "vmovdqa64 14*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 14*64(%0) \n\t"
        
        "vmovdqa64 15*64(%0), %%zmm0 \n\t"
        "vpaddd %%zmm31, %%zmm0, %%zmm0 \n\t"
        "vmovdqa64 %%zmm0, 15*64(%0) \n\t"
        
        :
        : "r"(working_state), "r"(state)
        : "%zmm0", "%zmm31", "memory"
    );
}

void chacha20_encrypt(const uint8_t key[32], const uint8_t nonce[12], uint32_t initial_counter, uint8_t *buffer, size_t length) {
    
    ALIGN(64) uint32_t key_words[8];
    ALIGN(64) uint32_t nonce_words[3];
    
    key_words[0] = (uint32_t)key[0] | ((uint32_t)key[1] << 8) | ((uint32_t)key[2] << 16) | ((uint32_t)key[3] << 24);
    key_words[1] = (uint32_t)key[4] | ((uint32_t)key[5] << 8) | ((uint32_t)key[6] << 16) | ((uint32_t)key[7] << 24);
    key_words[2] = (uint32_t)key[8] | ((uint32_t)key[9] << 8) | ((uint32_t)key[10] << 16) | ((uint32_t)key[11] << 24);
    key_words[3] = (uint32_t)key[12] | ((uint32_t)key[13] << 8) | ((uint32_t)key[14] << 16) | ((uint32_t)key[15] << 24);
    key_words[4] = (uint32_t)key[16] | ((uint32_t)key[17] << 8) | ((uint32_t)key[18] << 16) | ((uint32_t)key[19] << 24);
    key_words[5] = (uint32_t)key[20] | ((uint32_t)key[21] << 8) | ((uint32_t)key[22] << 16) | ((uint32_t)key[23] << 24);
    key_words[6] = (uint32_t)key[24] | ((uint32_t)key[25] << 8) | ((uint32_t)key[26] << 16) | ((uint32_t)key[27] << 24);
    key_words[7] = (uint32_t)key[28] | ((uint32_t)key[29] << 8) | ((uint32_t)key[30] << 16) | ((uint32_t)key[31] << 24);
    nonce_words[0] = (uint32_t)nonce[0] | ((uint32_t)nonce[1] << 8) | ((uint32_t)nonce[2] << 16) | ((uint32_t)nonce[3] << 24);
    nonce_words[1] = (uint32_t)nonce[4] | ((uint32_t)nonce[5] << 8) | ((uint32_t)nonce[6] << 16) | ((uint32_t)nonce[7] << 24);
    nonce_words[2] = (uint32_t)nonce[8] | ((uint32_t)nonce[9] << 8) | ((uint32_t)nonce[10] << 16) | ((uint32_t)nonce[11] << 24);

    ALIGN(64) uint32_t state[16] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,             
        key_words[0], key_words[1], key_words[2], key_words[3],     
        key_words[4], key_words[5], key_words[6], key_words[7],     
        initial_counter,                                            
        nonce_words[0], nonce_words[1], nonce_words[2]              
    };

    int block_num = length >> 10;    
    #pragma omp parallel
    {
        __m512i key_stream_buffers[16];
        ALIGN(64) uint32_t private_state[16];
        fast_copy(private_state, state);
        
        #pragma omp for
        for (int block = 0; block < block_num; block++) {
            if (block + 1 < block_num) {
                size_t next_offset = (block + 1) << 10;
                _mm_prefetch((char*)&buffer[next_offset], _MM_HINT_T0);
                _mm_prefetch((char*)&buffer[next_offset + 64], _MM_HINT_T0);
                _mm_prefetch((char*)&buffer[next_offset + 128], _MM_HINT_T0);
                _mm_prefetch((char*)&buffer[next_offset + 256], _MM_HINT_T1);
            }
            private_state[12] = (block << 4);
            
            chacha20_block(private_state, key_stream_buffers);
    
            size_t offset = block << 10;
            size_t bytes_to_process = (offset + 1024 <= length) ? 1024 : (length - offset);
            size_t full_blocks = bytes_to_process >> 6;
            
            for (size_t i = 0; i < full_blocks; i++) {
                if (i + 1 < full_blocks) {
                    _mm_prefetch((char*)&buffer[offset + ((i + 1) << 6)], _MM_HINT_T0);
                }
                if (i + 2 < full_blocks) {
                    _mm_prefetch((char*)&buffer[offset + ((i + 2) << 6)], _MM_HINT_T1);
                }
                __m512i buffer_vec = _mm512_loadu_si512((__m512i*)&buffer[offset + (i << 6)]);
                __m512i result = _mm512_xor_si512(buffer_vec, key_stream_buffers[i]);
                _mm512_storeu_si512((__m512i*)&buffer[offset + (i << 6)], result);
            }
        }
    }
}