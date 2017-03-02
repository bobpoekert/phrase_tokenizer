from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext
import numpy

setup(
        name='phrase tokenizer',
        ext_modules = [
            Extension(
                'phrase_tokenizer',
                sources=['py_phrase_tokenizer.pyx'],
                include_dirs=[numpy.get_include()],
                extra_compile_args=['-g', '-Ofast'])
            ],
        cmdclass={'build_ext':build_ext})
