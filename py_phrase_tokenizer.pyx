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

def splits(text, L=30):
    "Return a list of all possible (first, rem) pairs, len(first)<=L."
    return [(text[:i+1], text[i+1:])
               for i in range(min(len(text), L))]

class Segment(object):

    def __init__(self, tokenizer):
        self.cache = {}
        self.tokenizer = tokenizer

    def segment(self, text, recursion_depth):
        "Return a list of words that is the best segmentation of text."
        cdef double max_score
        cdef object max_split
        if not text:
            return ([], None)
        try:
            return self.cache[text]
        except KeyError:
            max_score = 0
            max_split = None
            if recursion_depth > 100:
                for i in xrange(min(len(text), 30)):
                    left = text[:i+1]
                    right = text[i+1:]
                    score = self.tokenizer.Pr(left) * self.tokenizer.Pr(right)
                    if max_split is None or score > max_score:
                        max_score = score
                        max_split = [left, right]
            else:
                for i in xrange(min(len(text), 30)):
                    left = text[:i+1]
                    right = text[i+1:]
                    right_chunk, right_score = self.segment(right, recursion_depth+1)
                    if right_score is None:
                        score = self.tokenizer.Pr(left)
                        if max_split is None or score > max_score:
                            max_score = score
                            max_split = [left]
                    else:
                        score = self.tokenizer.Pr(left) * right_score
                        if max_split is None or score > max_score:
                            max_score = score
                            max_split = [left] + right_chunk

            res = (max_split, max_score)
            self.cache[text] = res
            return res

cdef class PhraseTokenizer:

    cdef pt_CountMinSketch *token_sketch
    cdef int sketch_fd
    cdef size_t sketch_size
    cdef uint32_t *mat
    cdef double total

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
        arr = self.toarray()
        self.total = min([np.sum(arr[:, k]) for k in xrange(height)])

    def toarray(self):
        cdef np.uint32_t[:, :] view = <np.uint32_t[:self.token_sketch.width, :self.token_sketch.height]> self.mat
        return np.asarray(view)

    def __getitem__(self, k):
        unicode_text = k.lower().encode('utf-8')
        cdef char *raw_text = unicode_text
        cdef uint32_t token_count
        cdef uint32_t hash_value

        hash_value = pt_hash(raw_text, len(raw_text))
        token_count = pt_CountMinSketch_lookupHash(self.token_sketch, hash_value)

        return token_count

    def Pr(self, k):
        return self[k] / self.total

    def Pwords(self, words):
        res = 1
        for word in words:
            res = res * self.Pr(word)
        return res

    def chunk(self, text):
        return Segment(self).segment(text, 0)[0]

    def __dealloc__(self):
        unload(<char *>self.mat, self.sketch_size)
        pt_CountMinSketch_free(self.token_sketch)
