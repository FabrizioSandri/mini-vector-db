import random

import numpy as np
from mvdb import MVDB, Distance

NUM_VECTORS = 10000
EMBEDDING_DIM = 128

mvdb = MVDB(
    dim=EMBEDDING_DIM,  # dimension of the vectors
    distance=Distance.L2,  # Choose among Distance.L2, Distance.COSINE
    M=16,
    ef_construction=100,
    ef_search=50,
    max_level=10,
)

rng = random.Random(42)
embeddings = [
    [rng.uniform(-1, 1) for _ in range(EMBEDDING_DIM)] for _ in range(NUM_VECTORS)
]

query = [rng.uniform(-1, 1) for _ in range(EMBEDDING_DIM)]
for i, embedding in enumerate(embeddings):
    mvdb.add(i, np.array(embedding, dtype=np.float32))

print(f"Index size: {mvdb.size()}")

# Search for the nearest neighbors of the query vector
results = mvdb.search(np.array(query, dtype=np.float32), k=10)
print(f"Nearest neighbors of the query vector: {results}")
