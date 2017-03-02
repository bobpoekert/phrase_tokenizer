#pragma once

#include "phrase_tokenizer.h"
#include "primes.h"
#include "hash.c"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>

pt_CountMinSketch *pt_CountMinSketch_alloc(size_t width, size_t height, uint32_t *mat) {

    if (height > 9000) { /* we only have 9000 primes (if you're trying to set height this high you're doing something wrong) */
        return 0;
    }

    void *buffer = malloc(sizeof(pt_CountMinSketch));

    pt_CountMinSketch *res = (pt_CountMinSketch *) buffer;


    res->width = width;
    res->height = height;
    res->mat = mat;

    return res;

}

void pt_CountMinSketch_free(pt_CountMinSketch *inp) {
    if (inp != 0) {
        free((void *)inp);
    }
}

void pt_CountMinSketch_addHashValue(pt_CountMinSketch *sketch, uint32_t value) {

    /* we're doing the "conservative update" strategy as described in
     * http://blog.demofox.org/2015/02/22/count-min-sketch-a-probabilistic-histogram/
     * */

    uint32_t min_value = INT_MAX;
    size_t min_index = INT_MAX;
    uint32_t *mat = sketch->mat;
    size_t max_row_idx = sketch->width - 1;
    size_t base_idx = 0;

    for (size_t i=0; i < sketch->height; i++) {
        uint32_t prime = pt_primes[i];
        uint64_t offset = (value ^ prime) % max_row_idx;
        size_t index = base_idx + offset;
        uint32_t sketch_value;
        sketch_value = mat[index];
        if (sketch_value < min_value) {
            min_value = sketch_value;
            min_index = index;
        }
        base_idx += max_row_idx;
    }

    if (min_index != INT_MAX) {
        sketch->mat[min_index]++;
    }

}

void pt_CountMinSketch_addString(pt_CountMinSketch *sketch, char *string, size_t size) {
    uint32_t hash_value = pt_hash(string, size);
    pt_CountMinSketch_addHashValue(sketch, hash_value);
}

uint32_t pt_CountMinSketch_lookupHash(pt_CountMinSketch *sketch, uint32_t value) {
    uint32_t min_value = INT_MAX;
    uint32_t *mat = sketch->mat;
    size_t max_row_idx = sketch->width - 1;
    size_t base_idx = 0;

    for (size_t i=0; i < sketch->height; i++) {
        uint32_t prime = pt_primes[i];
        uint64_t offset = (value ^ prime) % max_row_idx;
        size_t index = base_idx + offset;
        uint32_t sketch_value;
        sketch_value = mat[index];
        if (sketch_value < min_value) {
            min_value = sketch_value;
        }
        base_idx += max_row_idx;
    }

    return min_value;
}


size_t pt_chunkText(
        pt_CountMinSketch *token_sketch,
        char *characters,
        size_t length,
        size_t *result_target) {

    size_t result_len = 0;
    size_t start = 0;

    while (start < length)  {
        size_t idx = start;
        uint32_t best_score = 0;
        size_t best_idx = idx;
        while (idx < length) {
            size_t char_length = pt_UTF8CharacterLength(characters, idx, length);
            idx += char_length;
            uint32_t hash_value = pt_hash((characters + start), (idx - start));
            uint32_t token_count = pt_CountMinSketch_lookupHash(token_sketch, hash_value);
            size_t size = idx - start;
            uint64_t score = token_count * (size * size);
            if (score > best_score) {
                best_score = score;
                best_idx = idx;
            }
        }

        if (best_score > 0) {
            result_target[result_len] = best_idx;
            result_len++;
        }
        start = best_idx + 1;
    }

    return result_len;
}


#define READ_BUFFER_SIZE (1024 * 1024)
#define LINE_BUFFER_SIZE (1024 * 1024)

#define isWhitespace(v) (v == ' ' || v == '\t')

void pt_CountMinSketch_readFileLines(pt_CountMinSketch *token_sketch, int fd) {


   char read_buffer[READ_BUFFER_SIZE];
   char line_buffer[LINE_BUFFER_SIZE];
   size_t line_size = 0;
   size_t bytes_read = 0;
   size_t row_count = 0;

   while((bytes_read = read(fd, (void *) read_buffer, (int) READ_BUFFER_SIZE)) > 0) {

        for (size_t i=0; i < bytes_read; i++) {
            if (
                    (read_buffer[i] == '\n') ||
                    (i < bytes_read-1 && isWhitespace(read_buffer[i+1]) && read_buffer[i] == '.') ||
                    (i < bytes_read-1 && isWhitespace(read_buffer[i+1]) && read_buffer[i] == ';') ||
                    (i < bytes_read-1 && isWhitespace(read_buffer[i+1]) && read_buffer[i] == '!') ||
                    (i < bytes_read-1 && isWhitespace(read_buffer[i+1]) && read_buffer[i] == '?')
                    ) {
                pt_CountMinSketch_addString(token_sketch, line_buffer, line_size);
                line_size = 0;
                row_count++;
                if (row_count % 100000 == 0) {
                    printf("%lu\n", row_count);
                }
            } else {
                if (line_size >= LINE_BUFFER_SIZE) {
                    line_size = 0;
                }
                line_buffer[line_size] = read_buffer[i];
                line_size++;
            }
        }

   }

}
