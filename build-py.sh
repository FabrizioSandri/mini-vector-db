#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build/py --fresh \
  -DCMAKE_BUILD_TYPE=Release \
  -DMVDB_BUILD_PYTHON=ON -DMVDB_BUILD_MAIN=OFF \
  -DPython_EXECUTABLE=".venv/bin/python" \
  -Dpybind11_DIR="$(.venv/bin/python -m pybind11 --cmakedir)"

cmake --build build/py