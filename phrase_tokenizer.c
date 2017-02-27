#include "phrase_tokenizer.h"
#include "primes.h"
#include "hash.c"

pt_CountMinSketch *pt_CountMinSketch_alloc(size_t width, size_t height, uint32_t *mat) {

    if (height > 9000) { /* we only have 9000 primes (if you're trying to set height this high you're doing something wrong) */
        return 0;
    }

    void *buffer = malloc(sizeof(pt_CountMinSketch) + sizeof(uint32_t) * height);

    pt_CountMinSketch *res = (pt_CountMinSketch *) buffer;
    uint32_t *hash_primes = (uint32_t *) (buffer + sizeof(pt_CountMinSketch));


    for (int i=0; i < height; i++) {
        hash_primes[i] = pt_primes[i];
    }

    res.width = width;
    res.height = height;
    res.hash_primes = hash_primes;

    return res;

}

void pt_CountMinSketch_free(pt_CountMinSketch *inp) {
    if (inp != 0) {
        free(inp);
    }
}

void pt_CountMinSketch_addHashValue(pt_CountMinsketch *sketch, uint64_t value) {

    /* we're doing the "conservative update" strategy as described in
     * http://blog.demofox.org/2015/02/22/count-min-sketch-a-probabilistic-histogram/
     * */

    uint32_t min_value = -1;
    size_t min_row = -1;
    uint32_t min_offset = 0;

    for (size_t i=0; i < sketch->height; i++) {
        uint64_t prime = sketch->primes[i];
        uint64_t offset = (value ^ prime) % sketch->width;
        uint32_t sketch_value = sketch->mat[(width * i) + offset];
        if (sketch_value < min_value) {
            min_value = sketch_value;
            min_row = i;
            min_offset = offset;
        }
    }

    if (min_row != -1) {
        sketch->mat[(width * min_row) + min_offset]++;
    }

}

uint32_t pt_CountMinSketch_lookupHash(pt_CountMinSketch *sketch, uint32_t value) {
    uint32_t min_value = -1;

    for (size_t i=0; i < sketch->height; i++) {
        uint32_t prime = sketch->primes[i];
        uint32_t offset = (value ^ prime) % sketch->width;
        uint32_t sketch_value = sketch->mat[(width * i) + offset];
        if (sketch_value < min_value) {
            min_value = sketch_value;
        }
    }

    return min_value;
}

void pt_CountMinSketch_addAllSubstrings(pt_CountMinSketch *sketch, char *characters, size_t length) {
    pt_HashState hash_state = pt_HashState_new();
    for (size_t start=0; start < length-1; start++) {
        pt_HashState_reset(&hash_state);
        for (size_t i=start; i < length; i++) {
            uint32_t hash_value = pt_HashState_crank(&hash_state, characters[i]);
            pt_CountMinSketch_addHashValue(sketch, hash_value);
        }
    }
}

size_t pt_chunkText(
        pt_CountMinSketch *token_sketch,
        pt_CountMinSketch *base_sketch,
        char *characters,
        size_t length,
        double threshold,
        size_t *result_target) {

    size_t result_len = 0;
    pt_HashState hash_state = pt_HashState_new();
    
    for (size_t start=0; start < length; start++) {
        pt_HashState_reset(&hash_state);
        double prob = 0;
        for (size_t idx=start; idx < length; idx++) {
            uint64_t hash_value = pt_HashState_crank(&hash_state, characters[idx]);
            uint32_t base_count = pt_CountMinSketch_lookupHash(base_sketch, hash_value);
            uint32_t token_count = pt_CountMinSketch_lookupHash(token_sketch, hash_value);
            double prob = token_count / (token_count + base_count);
            if (prob >= threshold) {
                result_target[result_len] = start;
                result_len++;
                result_target[result_len] = idx;
                result_len++;
                start = idx;
                break;
            }
        }
    }

    return result_len;
}

