// Everything related to the encryption of the communication between Foudraal and Trigger
#include <stdint.h>

#define XTEA_ROUNDS 32

void xteaEncrypt(uint32_t v[2], const uint32_t key[4]) {
    uint32_t v0 = v[0], v1 = v[1];
    uint32_t sum = 0;
    const uint32_t delta = 0x9E3779B9;
    for (uint32_t i = 0; i < XTEA_ROUNDS; i++) {
        v0 += ((((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]));
        sum += delta;
        v1 += ((((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]));
    }
    v[0] = v0;
    v[1] = v1;
}

void xteaDecrypt(uint32_t v[2], const uint32_t key[4]) {
    uint32_t v0 = v[0], v1 = v[1];
    const uint32_t delta = 0x9E3779B9;
    uint32_t sum = delta * XTEA_ROUNDS;
    for (uint32_t i = 0; i < XTEA_ROUNDS; i++) {
        v1 -= ((((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]));
        sum -= delta;
        v0 -= ((((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]));
    }
    v[0] = v0;
    v[1] = v1;
}