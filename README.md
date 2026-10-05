# MVDB — A mini vector DB

An in-memory vector index based on [HNSW](https://arxiv.org/abs/1603.09320). C++20 core with Python bindings via pybind11.

## Install

```bash
uv add mvdb
```

Requires Python ≥ 3.10 and NumPy.

## Usage

```python
import numpy as np
from mvdb import MVDB, Distance

# Create index
index = MVDB(dim=128, distance=Distance.L2)

# Add vectors
rng = np.random.default_rng(0)
data = rng.uniform(-1, 1, (10_000, 128)).astype(np.float32)
for id, vec in enumerate(data):
    index.add(id, vec)

# Search
query = rng.uniform(-1, 1, 128).astype(np.float32)
for id_, dist in index.search(query, k=5):  # (id, distance), nearest first
    print(id_, dist)

# Remove and check size
index.remove(42)
print(index.size())  # 9999
```

Constructor parameters:

| Parameter         | Default | Description                         |
| ----------------- | ------- | ----------------------------------- |
| `dim`             | —       | Vector dimensionality               |
| `distance`        | `L2`    | `Distance.L2` (squared) or `COSINE` |
| `M`               | 16      | Max neighbors per node              |
| `ef_construction` | 100     | Candidate list size while building  |
| `ef_search`       | 50      | Candidate list size while searching |
| `max_level`       | 16      | Cap on the number of graph layers   |

## Development

```bash
uv sync    # builds the extension in place
```

## C++ library

```bash
cmake -S . -B build
cmake --build build
```

The demo executables need [fmt](https://github.com/fmtlib/fmt) via vcpkg.

```cpp
#include "index.hpp"

HNSWConfig cfg;
cfg.dim = 128;
cfg.distance = cosine_distance;

HNSWIndex index(cfg);
index.add(0, vec_ptr, 128);
auto hits = index.search(query_ptr, 10, 128);  // vector<pair<size_t, float>>
```
