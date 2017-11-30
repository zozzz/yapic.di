#!/usr/bin/env python3

import os
import sys
from glob import glob
from os import path
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.test import test as TestCommand
from setuptools import Command

VERSION = "1.0.0"

define_macros = {
    "ZENO_DI_VERSION_MAJOR": VERSION.split(".")[0],
    "ZENO_DI_VERSION_MINOR": VERSION.split(".")[1],
    "ZENO_DI_VERSION_PATCH": VERSION.split(".")[2],
}
undef_macros = []
extra_compile_args = []  # -flto

if sys.platform == "win32":
    define_macros["UNICODE"] = 1

    DEVELOP = sys.executable.endswith("python_d.exe")
    if DEVELOP:
        define_macros["_DEBUG"] = 1
        undef_macros.append("NDEBUG")
        extra_compile_args.append("/MTd")
        extra_compile_args.append("/Zi")
    else:
        extra_compile_args.append("-Ox")

    extra_compile_args.append("/FAs")
else:
    extra_compile_args.append("-std=c++11")

    DEVELOP = False
    if not DEVELOP:
        extra_compile_args.append("-O3")


def root(*p):
    return path.join(path.dirname(path.abspath(__file__)), *p)


cpp_ext = Extension(
    name="zeno.di",
    sources=["src/di.cpp"],
    include_dirs=[
        "./libs/yapic.core/src/yapic/core/include"
    ],
    depends=glob("src/*.hpp") + glob("./libs/yapic.core/src/yapic/core/include/**/*.hpp"),
    extra_compile_args=extra_compile_args,
    define_macros=list(define_macros.items()),
    undef_macros=undef_macros,
    language="c++"
)


def cmd_prerun(cmd, requirements):
    for r in requirements(cmd.distribution):
        installed = cmd.distribution.fetch_build_eggs(r if r else [])

        for dp in map(lambda x: x.location, installed):
            if dp not in sys.path:
                sys.path.insert(0, dp)

    cmd.run_command('build_ext')

    ext = cmd.get_finalized_command("build_ext")
    ep = str(Path(ext.build_lib).absolute())

    if ep not in sys.path:
        sys.path.insert(0, ep)

    for e in ext.extensions:
        if e._needs_stub:
            ext.write_stub(ep, e, False)


class PyTest(TestCommand):
    user_options = [
        ("pytest-args=", "a", "Arguments to pass to pytest"),
        ("file=", "f", "File to run"),
    ]

    def initialize_options(self):
        super().initialize_options()
        self.pytest_args = "-x -s"
        self.file = "./tests/"

    def finalize_options(self):
        super().finalize_options()
        if self.file:
            self.pytest_args += " " + self.file.replace("\\", "/")

    def run(self):
        def requirements(dist):
            yield dist.install_requires
            yield dist.tests_require

        cmd_prerun(self, requirements)
        self.run_tests()

    def run_tests(self):
        import shlex
        import pytest
        errno = pytest.main(shlex.split(self.pytest_args))
        sys.exit(errno)


class Benchmark(Command):
    user_options = [
        ("file=", "f", "File to run"),
    ]

    def initialize_options(self):
        self.file = None
        self.pytest_args = "-x -s"

    def finalize_options(self):
        if self.file:
            self.pytest_args += " " + self.file.replace("\\", "/")

    def run(self):
        def requirements(dist):
            yield dist.extras_require["benchmark"]

        cmd_prerun(self, requirements)
        import shlex
        import pytest
        errno = pytest.main(shlex.split(self.pytest_args))
        sys.exit(errno)



# typing: https://github.com/python/typing/issues/84
setup(
    name="zeno.di",
    packages=["zeno"],
    package_dir={"zeno": "src"},
    ext_modules=[cpp_ext],
    tests_require=["pytest"],
    python_requires=">=3.5",
    extras_require={
        "benchmark": [
            "pytest",
            "pytest-benchmark"
        ]
    },
    cmdclass={
        "test": PyTest,
        "bench": Benchmark
    }
)
