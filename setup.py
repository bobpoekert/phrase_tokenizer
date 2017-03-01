from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext


setup(
        name='phrase tokenizer',
        ext_modules = [
            Extension(
                'phrase_tokenizer',
                sources=['py_phrase_tokenizer.pyx'],
                extra_compile_args=['-Ofast'])
            ],
        cmdclass={'build_ext':build_ext})
