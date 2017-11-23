#!/usr/bin/env python3

import os
from os import path
from distutils.core import setup, Extension
from Cython.Build import cythonize

DEBUG = True

define_macros = {}
# define_macros["CYTHON_CLINE_IN_TRACEBACK"] = 0
undef_macros = []
extra_compile_args = []

# undef_macros.append("NDEBUG")

if os.name == "nt":
    optimize = "-Ox"
    define_macros["UNICODE"] = 1
else:
    optimize = "-O3"
    extra_compile_args.append("-std=c++11")

if not DEBUG:
    extra_compile_args.append(optimize)
    undef_macros.append("_DEBUG")
else:
    undef_macros.append("NDEBUG")


# def extension(name, file):
#     return cythonize(
#         Extension(name,
#                   [file],
#                   extra_compile_args=extra_compile_args,
#                   define_macros=list(define_macros.items()),
#                   language="c++",
#                   undef_macros=undef_macros))


def root(*p):
    return path.join(path.dirname(path.abspath(__file__)), *p)


# cython_ext = cythonize(
#     Extension("zeno.di.injector_new",
#               ["src/zeno/di/injector_new.pyx"],
#               extra_compile_args=extra_compile_args,
#               define_macros=list(define_macros.items()),
#               language="c++",
#               include_dirs=[root("vendor", "sparsepp")],
#               undef_macros=undef_macros),
#     language_level=3,
#     cplus=True,
#     generate_pxi=True,
#     c_line_in_traceback=False,
#     compiler_directives={
#         "always_allow_keywords": False,
#         "wraparound": False,
#         "binding": True,
#         "nonecheck": False
#     })

cpp_ext = Extension(
    name="zeno.di",
    sources=["src/di.cpp"],
    depends=[
        "src/injector.hpp",
        "src/module.hpp",
        "src/provider.hpp",
        "src/resolver.hpp"
    ],
    extra_compile_args=extra_compile_args,
    define_macros=list(define_macros.items()),
    undef_macros=undef_macros,
    language="c++")


# typing: https://github.com/python/typing/issues/84
setup(
    name="zeno.di",
    packages=["zeno"],
    package_dir={"zeno": "src"},
    ext_modules=[cpp_ext])
