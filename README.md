# MVDB — Mini Vector DB

MVDB is a small implementation of an in-memory vector database based on **HNSW** (Hierarchical Navigable Small World graphs, [Malkov & Yashunin](https://arxiv.org/abs/1603.09320)). The core is written in modern C++20 and exposed to Python through [pybind11](https://github.com/pybind/pybind11).

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

## Python API

### `HNSWIndex(dim, M=16, ef_construction=100, ef_search=50, max_level=16, distance=Distance.L2)`

| Parameter         | Description                                                          |
| ----------------- | -------------------------------------------------------------------- |
| `dim`             | Dimensionality of the vectors. Must match on every `add` / `search`. |
| `M`               | Number of connections created for a newly inserted node.             |
| `ef_construction` | Size of the dynamic candidate list used while building the graph.    |
| `ef_search`       | Size of the dynamic candidate list used at query time.               |
| `max_level`       | Safety cap on the number of layers.                                  |
| `distance`        | `Distance.L2` or `Distance.COSINE`.                                  |

Derived internally: `Mmax = M` (upper layers), `Mmax0 = 2 * M` (layer 0), level multiplier `mL = 1 / ln(M)`.

### Methods

| Method             | Description                                                                                                      |
| ------------------ | ---------------------------------------------------------------------------------------------------------------- |
| `add(id, vector)`  | Insert a 1-D NumPy array under a unique `id`. Raises if the ID already exists or the dimension does not match.   |
| `remove(id)`       | Remove a vector by ID and unlink it from the graph. Raises if the ID does not exist.                             |
| `size()`           | Number of vectors currently in the index.                                                                        |
| `search(query, k)` | Returns up to `k` `(distance, id)` tuples, nearest first. Uses `max(ef_search, k)` as the base-layer beam width. |

### Distances

- `L2` returns the **squared** Euclidean distance (no square root), which preserves ranking and is cheaper to compute.
- `COSINE` returns `1 - cos(a, b)`; zero-norm vectors are assigned the maximum distance `1.0`.

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

`build-py.sh` configures CMake with `-DMVDB_BUILD_PYTHON=ON -DMVDB_BUILD_MAIN=OFF` against the `.venv` interpreter and writes the compiled `_core` module into `python/mvdb/`, next to the Python package sources. The vcpkg dependency is not needed for this build.

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
