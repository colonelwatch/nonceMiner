from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

extension = Extension(
    name='nonceMiner',
    sources=['src/nonceMiner.pyx', 'src/mine_DUCO_S1.c', 'src/sha1.c'],
    language='c',
    extra_compile_args=['-O2']
)
setup(
    name='nonceMiner',
    ext_modules=cythonize([extension])
)