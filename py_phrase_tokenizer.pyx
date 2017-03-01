from libc.stdint cimport *
from libc.stdlib cimport malloc, free

cdef extern from "phrase_tokenizer.c":
    struct pt_CountMinSketch:
        size_t width
        size_t height
        uint32_t *primes
        uint32_t *mat


    pt_CountMinSketch *pt_CountMinSketch_alloc(size_t width, size_t height, uint32_t *mat)
    void pt_CountMinSketch_free(pt_CountMinSketch *inp)
    void pt_CountMinSketch_addHashValue(pt_CountMinSketch *sketch, uint32_t value)
    void pt_CountMinSketch_addString(pt_CountMinSketch *sketch, char *characters, size_t length)
    size_t pt_chunkText(
        pt_CountMinSketch *token_sketch,
        char *characters,
        size_t length,
        size_t *result_target)

cdef extern from "map_file.c":
    char *load(int fd, size_t size)
    void unload(char *ptr, size_t size)
    size_t sketch_file_size(size_t width, size_t height)

cdef class PhraseTokenizer:

    cdef pt_CountMinSketch *token_sketch
    cdef int sketch_fd
    cdef size_t sketch_size
    cdef uint32_t *mat

    def __cinit__(self, sketch_file, width, height):
        self.sketch_fd = sketch_file.fileno()
        self.sketch_size = sketch_file_size(width, height)
        self.mat = <uint32_t *> load(self.sketch_fd, self.sketch_size)
        if <int>self.mat == 0:
            raise IOError('failed to mmap file')
        self.token_sketch = pt_CountMinSketch_alloc(width, height, self.mat)

    def chunk(self, text):
        unicode_text = text.encode('utf-8')
        cdef char *raw_text = unicode_text
        cdef size_t text_size = len(text)
        cdef size_t *splits = <size_t *> malloc(text_size * sizeof(uint32_t))
        cdef size_t split_size
        split_size = pt_chunkText(self.token_sketch, raw_text, text_size, splits)
        try:
            prev_idx = 0
            res = []
            for i in xrange(split_size):
                idx = splits[i]
                res.append(text[prev_idx:idx])
                prev_idx = idx
        finally:
            free(splits)
        return res

    def __dealloc__(self):
        pt_CountMinSketch_free(self.token_sketch)
        unload(<char *>self.mat, self.sketch_size)
