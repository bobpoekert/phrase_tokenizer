#include "phrase_tokenizer.c"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

void touch(char *path) {
    int fd2 = open(path, O_RDWR|O_CREAT, 0777);  // Originally 777 (see comments)

    if (fd2 != -1) {
        // use file descriptor
        close(fd2);
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("usage: %s <token sketch> <token width> <token height>\n", argv[0]);
        return 1;
    }

    long token_width;
    long token_height;

    size_t token_size;

    int token_fd;
    
    uint32_t *token_mat;

    pt_CountMinSketch *token_sketch;


    token_width = strtol(argv[2], NULL, 10);
    if (token_width == 0) {
        printf("invalid token width\n");
        return 1;
    }
    token_height = strtol(argv[3], NULL, 10);
    if (token_height == 0) {
        printf("invalid token height: %s\n", argv[6]);
        return 1;
    }

    token_size = token_width * token_height * sizeof(uint32_t);
   
    touch(argv[1]);
    if (truncate(argv[1], token_size) != 0) {
        printf("failed to resize token file\n");
        return 1;
    }

    token_fd = open(argv[1], O_RDWR, S_IWRITE | S_IREAD);
    if (token_fd == -1) {
        printf("failed to open file: %s\n", argv[2]);
        return 1;
    }

   
    token_mat = mmap(0, token_size, PROT_READ | PROT_WRITE, MAP_SHARED, token_fd, 0);
    if (token_mat == MAP_FAILED) {
        printf("failed to load token file\n");
        return 1;
    }

    token_sketch = pt_CountMinSketch_alloc(token_width, token_height, token_mat);

    pt_CountMinSketch_readFileLines(token_sketch, STDIN_FILENO);

    msync(token_mat, token_size, MS_SYNC);
    munmap(token_mat, token_size);
    close(token_fd);


    return 0;

}
