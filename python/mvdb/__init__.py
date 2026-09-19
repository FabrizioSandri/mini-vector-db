import numpy as np

from mvdb._core import Distance, HNSWIndex


class MVDB:
    def __init__(
        self,
        dim: int,
        M: int = 16,
        ef_construction: int = 200,
        ef_search: int = 50,
        max_level: int = 16,
        distance: Distance = Distance.L2,
    ):
        self.index = HNSWIndex(
            dim=dim,
            M=M,
            ef_construction=ef_construction,
            ef_search=ef_search,
            max_level=max_level,
            distance=distance,
        )

    def add(self, id: int, vector: np.ndarray):
        self.index.add(id, vector)

    def remove(self, id: int):
        self.index.remove(id)

    def size(self) -> int:
        return self.index.size()

    def search(self, query: list[float], k: int) -> list[tuple[int, float]]:
        return self.index.search(query, k)


__all__ = ["MVDB", "Distance"]
