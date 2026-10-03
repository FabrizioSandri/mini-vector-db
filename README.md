# MVDB — Mini Vector DB

MVDB is an in-memory vector database based on **HNSW** (Hierarchical Navigable Small World graphs, [Malkov & Yashunin](https://arxiv.org/abs/1603.09320)). The core is written in modern C++20 and exposed to Python through [pybind11](https://github.com/pybind/pybind11).

## Quick start (Python)

```python
import numpy as np
from mvdb import MVDB, Distance

dim = 128
rng = np.random.default_rng(0)
data = rng.standard_normal((10_000, dim)).astype(np.float32)

index = MVDB(
    dim=dim,                # dimension of the vectors
    distance=Distance.L2,   # Choose among Distance.L2, Distance.COSINE
    M=16,
    ef_construction=100,
    ef_search=50,
    max_level=10,
)
for i, vec in enumerate(data):
    index.add(i, vec)

print(index.size())  # 10000

query = rng.standard_normal(dim).astype(np.float32)
results = index.search(query, k=5)  # list of (distance, id)
for dist, id_ in results:
    print(id_, dist)

index.remove(42)
```

## Building

### Requirements

- CMake ≥ 3.30
- A C++20 compiler (GCC, Clang or MSVC)
- Python ≥ 3.13 and [uv](https://docs.astral.sh/uv/) for the Python package
- [vcpkg](https://github.com/microsoft/vcpkg) for the C++ executables (provides `fmt`)

### Python bindings

```bash
uv sync
uv run bash build-py.sh     # configures and builds the python bindings
```

`build-py.sh` configures CMake and builds the compiled `_core` module into `python/mvdb/`, next to the Python package sources. Vcpkg dependencies are not needed for this build.

### C++ library and executables

```bash
cmake -S . -B build/cpp
cmake --build build/cpp
```

This builds the static `mvdb` library.

### CMake options

| Option              | Default   | Description                   |
| ------------------- | --------- | ----------------------------- |
| `MVDB_BUILD_PYTHON` | `OFF`     | Build the Python bindings.    |
| `MVDB_BUILD_MAIN`   | `ON`      | Build the `main` executable.  |
| `CMAKE_BUILD_TYPE`  | `Release` | Defaults to Release if unset. |

> **Note:** non-MSVC builds use `-march=native -ffast-math`. Binaries are tuned to the build machine's CPU and are not portable to other hardware; `-ffast-math` also relaxes strict IEEE floating point semantics.

## Using the C++ library

```cpp
#include "index.hpp"

HNSWConfig cfg;
cfg.dim = 128;
cfg.M = 16;
cfg.distance = cosine_distance;
cfg.keep_pruned_connections = true;   // optional heuristic extensions
cfg.extend_candidates = false;

HNSWIndex index(cfg);
index.add(/*id=*/0, vec_ptr, /*dim=*/128);
auto hits = index.search(query_ptr, /*k=*/10, /*dim=*/128);  // vector<pair<float, size_t>>
```
