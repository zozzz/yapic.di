#!/usr/bin/env python3

import os
import sys
from glob import glob
from os import path
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.test import test as TestCommand
from setuptools import Command

VERSION = "1.1.1"

define_macros = {
    "YAPIC_DI_VERSION_MAJOR": VERSION.split(".")[0],
    "YAPIC_DI_VERSION_MINOR": VERSION.split(".")[1],
    "YAPIC_DI_VERSION_PATCH": VERSION.split(".")[2]
}
undef_macros = []
extra_compile_args = []  # -flto

subcommand_args = []
if "--" in sys.argv:
    subcommand_args = sys.argv[sys.argv.index("--") + 1:]
    del sys.argv[sys.argv.index("--"):]

if sys.platform == "win32":
    define_macros["UNICODE"] = "1"

    DEVELOP = sys.executable.endswith("python_d.exe")
    if DEVELOP:
        define_macros["_DEBUG"] = "1"
        undef_macros.append("NDEBUG")
        extra_compile_args.append("/MTd")
        extra_compile_args.append("/Zi")
    else:
        undef_macros.append("_DEBUG")
        extra_compile_args.append("/Ox")

    extra_compile_args.append("/FAs")
else:
    extra_compile_args.append("-std=c++11")
    extra_compile_args.append("-Wno-unknown-pragmas")
    extra_compile_args.append("-Wno-write-strings")

    DEVELOP = sys.executable.endswith("-dbg")
    if DEVELOP:
        define_macros["_DEBUG"] = 1
        undef_macros.append("NDEBUG")
    else:
        extra_compile_args.append("-O3")


def root(*p):
    return path.join(path.dirname(path.abspath(__file__)), *p)


cpp_ext = Extension(
    name="yapic.di._di",
    sources=["src/di.cpp"],
    include_dirs=["./libs/yapic.core/src/yapic/core/include"],
    depends=glob("src/*.hpp") + glob("./libs/yapic.core/src/yapic/core/include/**/*.hpp"),
    extra_compile_args=extra_compile_args,
    define_macros=list(define_macros.items()),
    undef_macros=undef_macros,
    language="c++")


def cmd_prerun(cmd, requirements):
    for r in requirements(cmd.distribution):
        if r:
            installed = cmd.distribution.fetch_build_eggs(r)

            if installed:
                for dp in map(lambda x: x.location, installed):
                    if dp not in sys.path:
                        sys.path.insert(0, dp)

    cmd.distribution.get_command_obj("build").force = True
    cmd.run_command("build")

    ext = cmd.get_finalized_command("build_ext")
    ep = str(Path(ext.build_lib).absolute())

    if ep not in sys.path:
        sys.path.insert(0, ep)

    for e in ext.extensions:
        if e._needs_stub:
            ext.write_stub(ep, e, False)


class PyTest(TestCommand):
    user_options = [
        ("file=", "f", "File to run"),
    ]

    def initialize_options(self):
        super().initialize_options()
        self.file = "./tests/"

    def finalize_options(self):
        super().finalize_options()

    def run(self):
        def requirements(dist):
            yield dist.install_requires
            yield dist.tests_require

        cmd_prerun(self, requirements)
        self.run_tests()

    def run_tests(self):
        import pytest
        errno = pytest.main(subcommand_args)
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
    name="yapic.di",
    version=VERSION,
    url="https://github.com/zozzz/yapic.di",
    author="Zoltán Vetési",
    author_email="vetesi.zoltan@gmail.com",
    long_description=(Path(__file__).parent / "README.rst").read_text(encoding="utf-8"),
    license="BSD",
    packages=["yapic.di"],
    package_dir={"yapic.di": "src"},
    package_data={"yapic.di": ["_di.pyi"]},
    ext_modules=[cpp_ext],
    tests_require=["pytest"],
    python_requires=">=3.7",
    extras_require={"benchmark": ["pytest", "pytest-benchmark"]},
    cmdclass={
        "test": PyTest,
        "bench": Benchmark
    },
    classifiers=[
        "Development Status :: 4 - Beta",
        "License :: OSI Approved :: BSD License",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: Unix",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3 :: Only",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Utilities",
        "Typing :: Typed",
    ])
