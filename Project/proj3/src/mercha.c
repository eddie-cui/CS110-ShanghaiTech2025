// #include "mercha.h"
// void mercha(const uint8_t key[32], const uint8_t nonce[12], uint8_t *input, uint8_t *output, size_t length) {
//     chacha20_encrypt(key, nonce, 0, input, length);
//     merkel_tree(input, output, length);
// }
#include "mercha.h"
#include <time.h>
#include <stdio.h>

void mercha(const uint8_t key[32], const uint8_t nonce[12], uint8_t *input, uint8_t *output, size_t length) {
    struct timespec start, mid, end;
    long long encrypt_ns, hash_ns, total_ns;
    double encrypt_sec, hash_sec, total_sec;
    
    // 开始计时
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // 执行加密操作
    chacha20_encrypt(key, nonce, 0, input, length);
    
    // 加密完成计时
    clock_gettime(CLOCK_MONOTONIC, &mid);
    
    // 执行哈希操作
    merkel_tree(input, output, length);
    
    // 结束计时
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // 计算各阶段执行时间（纳秒）
    encrypt_ns = (mid.tv_sec - start.tv_sec) * 1000000000LL + 
                 (mid.tv_nsec - start.tv_nsec);
    
    hash_ns = (end.tv_sec - mid.tv_sec) * 1000000000LL + 
              (end.tv_nsec - mid.tv_nsec);
    
    total_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + 
               (end.tv_nsec - start.tv_nsec);
    
    // 转换为秒用于计算吞吐量
    encrypt_sec = encrypt_ns / 1000000000.0;
    hash_sec = hash_ns / 1000000000.0;
    total_sec = total_ns / 1000000000.0;
    
    // 输出各阶段执行时间和吞吐量
    printf("=== Performance Metrics ===\n");
    printf("Data processed: %zu bytes (%.2f MB)\n", 
           length, length / (1024.0 * 1024.0));
    
    printf("\n--- ChaCha20 Encryption ---\n");
    printf("Time: %lld ns (%.6f seconds)\n", encrypt_ns, encrypt_sec);
    printf("Throughput: %.2f MB/s\n", 
           (length / (1024.0 * 1024.0)) / encrypt_sec);
    
    printf("\n--- Merkle Tree Hashing ---\n");
    printf("Time: %lld ns (%.6f seconds)\n", hash_ns, hash_sec);
    printf("Throughput: %.2f MB/s\n", 
           (length / (1024.0 * 1024.0)) / hash_sec);
    
    printf("\n--- Total Processing ---\n");
    printf("Time: %lld ns (%.6f seconds)\n", total_ns, total_sec);
    printf("Throughput: %.2f MB/s\n", 
           (length / (1024.0 * 1024.0)) / total_sec);
    printf("===========================\n");
}