#include "phrase_tokenizer.c"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

int main(int argc, char **argv) {

    if (argc != 6) {
        printf("usage: %s <base sektch> <base width> <base height> <token sketch> <token width> <token height>\n", argv[0]);
        return 1;
    }

    long base_width;
    long base_height;
    long token_width;
    long token_height;

    size_t base_size;
    size_t token_size;

    int base_fd;
    int token_fd;
    
    uint32_t *base_mat;
    uint32_t *token_mat;

    pt_CountMinSketch *token_sketch;
    pt_CountMinSketch *base_sketch;

    base_width = strtol(argv[2], NULL, 10);
    if (base_width == 0) {
        printf("invalid base width\n");
        return 1;
    }
    base_height = strtol(argv[3], NULL, 10);
    if (base_height == 0) {
        printf("invalid base height\n");
        return 1;
    }

    base_size = base_width * base_height * sizeof(uint32_t);

    token_width = strtol(argv[5], NULL, 10);
    if (token_width == 0) {
        printf("invalid token width\n");
        return 1;
    }
    token_width = strtol(argv[6], NULL, 10);
    if (token_height == 0) {
        printf("invalid token height\n");
        return 1;
    }

    token_size = token_width * token_height * sizeof(uint32_t);
    
    if (truncate(argv[1], base_size) != 0) {
        printf("failed to resize base file\n'");
        return 1;
    }
    if (truncate(argv[4], token_size) != 0) {
        printf("failed to resize token file\n'");
        return 1;
    }

    base_fd = open(argv[1], O_RDWR, S_IWRITE | S_IREAD);
    if (base_fd == -1) {
        printf("failed to open file: %s\n", argv[1]);
        return 1;
    }
    token_fd = open(argv[4], O_RDWR, S_IWRITE | S_IREAD);
    if (base_fd == -1) {
        printf("failed to open file: %s\n", argv[2]);
        return 1;
    }

   
    base_mat = mmap(0, base_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, base_fd, 0);
    if (base_mat == MAP_FAILED) {
        printf("failed to load base file\n");
        return 1;
    }
    token_mat = mmap(0, token_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, token_fd, 0);
    if (token_mat == MAP_FAILED) {
        printf("failed to load token file\n");
        return 1;
    }

    token_sketch = pt_CountMinSketch_alloc(token_width, token_height, token_mat);
    base_sketch = pt_CountMinSketch_alloc(base_width, base_height, base_mat);

    pt_CountMinSketch_readFileLines(token_sketch, base_sketch, STDIN_FILENO);

    return 0;

}
