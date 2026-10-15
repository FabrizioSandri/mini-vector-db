import numpy as np
import pytest
from mvdb import MVDB, Distance


def test_add_batch_inserts_ids_and_vectors() -> None:
    vectors = np.eye(4, dtype=np.float32)
    ids = np.array([10, 20, 30, 40])
    index = MVDB(dim=4)

    index.add_batch(ids, vectors)

    assert index.size() == len(ids)
    for id_, vector in zip(ids, vectors):
        assert index.search(vector, k=1)[0][0] == id_


def test_add_batch_accepts_array_like_inputs() -> None:
    index = MVDB(dim=2, distance=Distance.COSINE)

    index.add_batch([1, 2], [[1, 0], [0, 1]])

    assert index.size() == 2
    assert index.search([1, 0], k=1)[0] == (1, pytest.approx(0.0))


def test_add_batch_can_extend_an_existing_index() -> None:
    index = MVDB(dim=2)
    index.add(1, [1, 0])

    index.add_batch([2, 3], [[0, 1], [-1, 0]])

    assert index.size() == 3
    assert index.search([-1, 0], k=1)[0][0] == 3


def test_add_batch_rejects_mismatched_item_counts() -> None:
    index = MVDB(dim=2)

    with pytest.raises(ValueError, match="same number of items"):
        index.add_batch([1, 2], [[1, 0]])

    assert index.size() == 0
