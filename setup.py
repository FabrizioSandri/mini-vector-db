"""Build configuration for the compiled `mvdb._core` extension."""

import sys

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

extra_compile_args: list[str] = []
if sys.platform != "win32":
    extra_compile_args = [
        "-O3",
        "-fno-math-errno",
        "-fno-trapping-math",
        "-fno-signed-zeros",
        "-fassociative-math",
    ]


ext_modules = [
    Pybind11Extension(
        "mvdb._core",
        sorted(["src/pybindings.cpp", "src/index.cpp", "src/distance.cpp"]),
        include_dirs=["include"],
        cxx_std=20,
        extra_compile_args=extra_compile_args,
    )
]

setup(ext_modules=ext_modules, cmdclass={"build_ext": build_ext})
