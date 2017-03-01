#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

char *load(int fd, size_t size) {
    char *res = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
    if (res == MAP_FAILED) {
        return 0;
    } else {
        return res;
    }
}

void unload(char *ptr, size_t size) {
    msync(ptr, size, MS_SYNC);
    munmap(ptr, size);
}

size_t sketch_file_size(size_t width, size_t height) {
    return width * height * sizeof(uint32_t);
}
