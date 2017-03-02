from libc.stdint cimport *
from libc.stdlib cimport malloc, free
from cpython cimport array
import array
import numpy as np
cimport numpy as np
import os

cdef extern from "phrase_tokenizer.c":
    struct pt_CountMinSketch:
        size_t width
        size_t height
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
    uint32_t pt_hash(char *string, size_t size)
    uint32_t pt_CountMinSketch_lookupHash(pt_CountMinSketch *sketch, uint32_t value)


cdef extern from "map_file.c":
    char *load(int fd, size_t size)
    void unload(char *ptr, size_t size)
    size_t sketch_file_size(size_t width, size_t height)

cdef extern from "alloca.h":
    void *alloca(size_t size)

cdef class PhraseTokenizer:

    cdef pt_CountMinSketch *token_sketch
    cdef int sketch_fd
    cdef size_t sketch_size
    cdef uint32_t *mat

    def __cinit__(self, sketch_file, height):
        self.sketch_fd = sketch_file.fileno()
        stat = os.fstat(self.sketch_fd)
        file_size = stat.st_size
        width = file_size / (height * sizeof(uint32_t))
        self.sketch_size = file_size
        self.mat = <uint32_t *> load(self.sketch_fd, self.sketch_size)
        if <int>self.mat == 0:
            raise IOError('failed to mmap file')
        self.token_sketch = pt_CountMinSketch_alloc(width, height, self.mat)

    def toarray(self):
        cdef np.uint32_t[:, :] view = <np.uint32_t[:self.token_sketch.width, :self.token_sketch.height]> self.mat
        return np.copy(np.asarray(view))

    def __getitem__(self, k):
        unicode_text = k.encode('utf-8')
        cdef char *raw_text = unicode_text
        cdef uint32_t token_count
        cdef uint32_t hash_value

        hash_value = pt_hash(raw_text, len(raw_text))
        token_count = pt_CountMinSketch_lookupHash(self.token_sketch, hash_value)

        return token_count

    def chunk(self, text):
        unicode_text = text.encode('utf-8')
        cdef char *raw_text = unicode_text
        cdef size_t text_size = len(raw_text)
        cdef size_t *splits = <size_t *> alloca(text_size * sizeof(size_t))
        cdef size_t split_size
        split_size = pt_chunkText(self.token_sketch, raw_text, text_size, splits)
        if split_size == 0:
            return [text]
        else:
            prev_idx = 0
            res = []
            for i in xrange(split_size):
                idx = splits[i]
                res.append(unicode_text[prev_idx:idx])
                prev_idx = idx
            res.append(unicode_text[prev_idx:])
            return res

    def __dealloc__(self):
        unload(<char *>self.mat, self.sketch_size)
        pt_CountMinSketch_free(self.token_sketch)
