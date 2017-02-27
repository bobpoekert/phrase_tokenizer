#pragma once

#include <stdint.h>

typedef struct pt_CountMinSketch {

    size_t width;
    size_t height;
    uint32_t *hash_primes;
    uint32_t *mat;

} pt_CountMinSketch;

pt_CountMinSketch pt_CountMinSketch_alloc(size_t width, size_t height, pt_Count *mat);
void pt_CountMinSketch_free(pt_CountMinSketch *inp);
void pt_CountMinSketch_addHashValue(pt_CountMinsketch *sketch, uint32_t value);
void pt_CountMinSketch_addString(char *characters, size_t length);
void pt_CountMinSketch_addAllSubstrings(pt_CountMinSketch *sketch, char *characters, size_t length);
size_t pt_chunkText(
        pt_CountMinSketch *token_sketch,
        pt_CountMinSketch *base_sketch,
        char *characters,
        size_t length,
        double threshold,
        size_t *result_target);
