import numpy as np
from mvdb import MVDB, Distance


def test_add_remove_and_search_10000_vectors():
    rng = np.random.default_rng(42)
    embeddings = rng.uniform(-1, 1, (10000, 128)).astype(np.float32)
    index = MVDB(dim=128, distance=Distance.L2)

    for id_, embedding in enumerate(embeddings):
        index.add(id_, embedding)

    assert index.size() == 10000

    id_, distance = index.search(embeddings[42], k=1)[0]
    assert id_ == 42 and distance == 0

    index.remove(42)
    assert index.size() == 9999
    assert all(id_ != 42 for id_, _ in index.search(embeddings[42], k=10))

    index.add(42, embeddings[42])
    assert index.search(embeddings[42], k=1)[0][0] == 42
