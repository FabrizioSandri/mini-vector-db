"""Public Python interface for the mini-vector-db HNSW index.

This package exposes a small, user-friendly wrapper around the compiled
core implementation. The main entry point is :class:`MVDB`, which stores
float vectors and supports insertion, removal, and nearest-neighbor search.
"""

import enum

import numpy as np
import numpy.typing as npt

from mvdb._core import Distance as Distance_
from mvdb._core import HNSWIndex as HNSWIndex_


class Distance(enum.Enum):
    L2 = Distance_.L2
    COSINE = Distance_.COSINE


class MVDB:
    """A lightweight HNSW-based vector index.

    The index stores `dim`-dimensional floating point vectors and provides
    approximate nearest-neighbor search for a query vector.
    """

    def __init__(
        self,
        dim: int,
        M: int = 16,
        ef_construction: int = 100,
        ef_search: int = 50,
        max_level: int = 16,
        distance: Distance = Distance.L2,
    ):
        """Create a new vector index.

        Args:
            dim: Number of dimensions in each vector.
            M: Maximum number of neighbors per node in the HNSW graph.
            ef_construction: Size of the dynamic candidate list used during graph
                construction.
            ef_search: Size of the dynamic candidate list used during search.
            max_level: Maximum level allowed in the HNSW hierarchy.
            distance: Distance metric to use for similarity or dissimilarity
                comparisons.
        """
        self._index = HNSWIndex_(
            dim=dim,
            M=M,
            ef_construction=ef_construction,
            ef_search=ef_search,
            max_level=max_level,
            distance=distance.value,
        )
        self.dim = dim

    def add(self, id: int, vector: npt.ArrayLike):
        """Insert a vector in the index. If a vector with the same `id` already
        exists, it will throw an Exception.

        Args:
            id: Unique identifier associated with the vector.
            vector: A one-dimensional NumPy array containing the vector values.
        """
        contiguous_vector = np.ascontiguousarray(vector, dtype=np.float32)
        self._index.add(id, contiguous_vector)

    def add_batch(self, ids: npt.ArrayLike, vectors: npt.ArrayLike):
        """Insert multiple vectors in the index."""
        self._index.add_batch(ids, vectors)

    def remove(self, id: int):
        """Remove a vector from the index.

        Args:
            id: Identifier of the vector to remove.
        """
        self._index.remove(id)

    def size(self) -> int:
        """Return the current number of vectors stored in the index."""
        return self._index.size()

    def search(self, query: npt.ArrayLike, k: int) -> list[tuple[int, float]]:
        """Search for the `k` nearest vectors to a query.

        Args:
            query: Query vector expressed as a sequence of floats.
            k: Maximum number of results to return.

        Returns:
            A list of `(id, distance)` tuples sorted from nearest to farthest.
        """
        return self._index.search(np.ascontiguousarray(query, dtype=np.float32), k)

    def __len__(self) -> int:
        """Return the current number of vectors stored in the index."""
        return self.size()

    def __repr__(self) -> str:
        """Return a string representation of the index."""
        return f"MVDB(dim={self.dim}, size={self.size()})"


__all__ = ["MVDB", "Distance"]
