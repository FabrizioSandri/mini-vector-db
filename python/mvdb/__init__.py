"""Public Python interface for the mini-vector-db HNSW index.

This package exposes a small, user-friendly wrapper around the compiled
core implementation. The main entry point is :class:`MVDB`, which stores
float vectors and supports insertion, removal, and nearest-neighbor search.
"""

import numpy as np

from mvdb._core import Distance, HNSWIndex


class MVDB:
    """A lightweight HNSW-based vector index.

    The index stores `dim`-dimensional floating point vectors and provides
    approximate nearest-neighbor search for a query vector.
    """

    def __init__(
        self,
        dim: int,
        M: int = 16,
        ef_construction: int = 200,
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
        self.index = HNSWIndex(
            dim=dim,
            M=M,
            ef_construction=ef_construction,
            ef_search=ef_search,
            max_level=max_level,
            distance=distance,
        )

    def add(self, id: int, vector: np.ndarray):
        """Insert or update a vector in the index.

        Args:
            id: Unique identifier associated with the vector.
            vector: A one-dimensional NumPy array containing the vector values.
        """
        self.index.add(id, vector)

    def remove(self, id: int):
        """Remove a vector from the index.

        Args:
            id: Identifier of the vector to remove.
        """
        self.index.remove(id)

    def size(self) -> int:
        """Return the current number of vectors stored in the index."""
        return self.index.size()

    def search(self, query: list[float], k: int) -> list[tuple[int, float]]:
        """Search for the `k` nearest vectors to a query.

        Args:
            query: Query vector expressed as a sequence of floats.
            k: Maximum number of results to return.

        Returns:
            A list of `(id, distance)` tuples sorted from nearest to farthest.
        """
        return self.index.search(query, k)


__all__ = ["MVDB", "Distance"]
