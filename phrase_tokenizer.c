#pragma once

#include "phrase_tokenizer.h"
#include "primes.h"
#include "hash.c"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <alloca.h>
#include <string.h>

void *alloc(size_t s) {
    void *res = malloc(s);
    if (res == 0) {
        return 0;
    }
    memset(res, 0, s);
    return res;
}

pt_CountMinSketch *pt_CountMinSketch_alloc(size_t width, size_t height, uint32_t *mat) {

    if (height > 9000) { /* we only have 9000 primes (if you're trying to set height this high you're doing something wrong) */
        return 0;
    }

    void *buffer = alloc(sizeof(pt_CountMinSketch));

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

/*
 *
 * takes: char *buffer, size_t length, uint64_t *score_buffer, size_t score_buffer_size, 
 *
 * 1. generate candidate split points
 * 2. for each candidate split point, recur on the right side of the split and get the best partitioning
 * 3. choose the partitioning with the highest total score
 * 4. return that partitioning
 */

uint64_t pt_substringScore(pt_CountMinSketch *sketch, char *string, size_t size) {
    uint32_t hash_value = pt_hash(string, size);
    uint32_t token_count = pt_CountMinSketch_lookupHash(sketch, hash_value);
    return token_count * size;
}


size_t pt_candidateSplitPoints(
        pt_CountMinSketch *sketch,
        char *data,
        size_t data_size,
        size_t *split_points,
        uint64_t *score,
        size_t outp_buffer_size,
        size_t recursion_depth) {


    size_t outp_offset = 0;
    size_t string_length = 0;
    uint64_t *candidate_scores = alloc(outp_buffer_size * sizeof(uint64_t));
    size_t *candidate_split_points = alloc(data_size * sizeof(size_t));

    while (string_length < data_size && outp_offset < outp_buffer_size) {
        uint64_t score = pt_substringScore(sketch, data, string_length);
        if (score > 5) {
            candidate_scores[outp_offset] = score;
            candidate_split_points[outp_offset] = string_length;
            outp_offset++;
        }
        string_length += pt_UTF8CharacterLength(data, string_length, data_size);
    }


    size_t res;
    if (recursion_depth > 8) {

        uint64_t total_score = 0;

        for (size_t i=0; i < outp_offset; i++) {
            total_score += candidate_scores[outp_offset];
        }

        *score = total_score;

        memcpy(candidate_split_points, split_points, outp_offset);

        res = outp_offset;

    } else if (outp_offset > 0) {

        size_t res_size = data_size - candidate_split_points[0];
        size_t *splits_buffer = alloc(res_size * sizeof(size_t));
            size_t *max_splits_buffer = alloc((res_size + 1) * sizeof(size_t));
                size_t max_buffer_size = 0;
                uint64_t max_score = 0;

                for (size_t i=0; i < outp_offset; i++) {
                    size_t split_point = candidate_split_points[i];
                    size_t res_splits_count;
                    uint64_t inner_score;
                    uint64_t total_score;

                    res_splits_count = pt_candidateSplitPoints(
                            sketch,
                            (data + split_point),
                            (data_size - split_point),
                            splits_buffer,
                            &inner_score,
                            res_size,
                            recursion_depth + 1);

                    total_score = inner_score + candidate_scores[i];

                    if (res_splits_count > 0 && total_score > max_score) {
                        max_score = total_score;
                        max_buffer_size = res_splits_count;
                        max_splits_buffer[0] = split_point;
                        memcpy(splits_buffer, max_splits_buffer + sizeof(size_t), res_splits_count);
                    }

                }
                free(max_splits_buffer);

            free(splits_buffer);

        if (max_score > 0) {

            *score = max_score;
            memcpy(max_splits_buffer, split_points, max_buffer_size);
            res = max_buffer_size;

        } else {
            res = 0;
        }

    } else {

        res = 0;
    
    }

    free(candidate_scores);
    free(candidate_split_points);
    return res;
}

size_t pt_chunkText(
        pt_CountMinSketch *token_sketch,
        char *characters,
        size_t length,
        size_t *result_target) {

    size_t result_len = 0;
    size_t start = 0;
    uint64_t res_score;

    result_len = pt_candidateSplitPoints(
            token_sketch,
            characters,
            length,
            result_target,
            &res_score,
            length,
            0);


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
                line_buffer[line_size] = (char) tolower(read_buffer[i]);
                line_size++;
            }
        }

   }

}
