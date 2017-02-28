#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct pt_CountMinSketch {

    size_t width;
    size_t height;
    uint32_t *primes;
    uint32_t *mat;

} pt_CountMinSketch;

pt_CountMinSketch *pt_CountMinSketch_alloc(size_t width, size_t height, uint32_t *mat);
void pt_CountMinSketch_free(pt_CountMinSketch *inp);
void pt_CountMinSketch_addHashValue(pt_CountMinSketch *sketch, uint32_t value);
void pt_CountMinSketch_addString(pt_CountMinSketch *sketch, char *characters, size_t length);
void pt_CountMinSketch_addAllSubstrings(pt_CountMinSketch *sketch, char *characters, size_t length);
size_t pt_chunkText(
        pt_CountMinSketch *token_sketch,
        pt_CountMinSketch *base_sketch,
        char *characters,
        size_t length,
        double threshold,
        size_t *result_target);
