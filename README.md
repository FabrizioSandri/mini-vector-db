# MVDB — A mini vector DB

An in-memory vector index based on [HNSW](https://arxiv.org/abs/1603.09320). C++20 core, Python bindings via [pybind11](https://github.com/pybind/pybind11), and JavaScript/TypeScript support through WebAssembly.

## Install

```bash
uv add mvdb
```

Requires Python ≥ 3.10 and NumPy.

## Usage

#### Python

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

#### JavaScript (WebAssembly)

Build the WebAssembly module with [Emscripten](https://emscripten.org/) before
using the JavaScript wrapper:

```bash
make wasm
```

The wrapper is an ES module and works in Node.js and browsers:

```js
import { Distance, HNSWIndex } from "./wasm/mvdb.mjs";

const index = new HNSWIndex(3, 16, 100, 50, 16, Distance.COSINE);

index.add(1, [1, 0, 0]);
index.add(2, new Float32Array([0, 1, 0]));

const results = index.search([1, 0, 0], 2);
console.log(results); // [{ id: 1, distance: 0 }, { id: 2, ... }]

index.remove(1);
console.log(index.size()); // 1
```

`HNSWIndex` takes `dim`, `M`, `efConstruction`, `efSearch`, `maxLevel`, and
the distance metric (`Distance.L2` or `Distance.COSINE`). Vectors can be
regular number arrays or `Float32Array` instances.

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
