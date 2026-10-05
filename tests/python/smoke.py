"""Minimal post-build check, used by cibuildwheel on every wheel."""

import numpy as np
from mvdb import MVDB, Distance


def main() -> None:
    rng = np.random.default_rng(0)
    data = rng.standard_normal((200, 16)).astype(np.float32)

    for metric in (Distance.L2, Distance.COSINE):
        index = MVDB(dim=16, distance=metric)
        for i, vec in enumerate(data):
            index.add(i, vec)
        assert index.size() == 200

        id_, dist = index.search(data[7], k=1)[0]
        assert id_ == 7 and abs(dist) < 1e-4, (metric, id_, dist)

        index.remove(7)
        assert index.size() == 199

    print("ok")


if __name__ == "__main__":
    main()
