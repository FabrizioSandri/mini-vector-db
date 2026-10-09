#ifndef __MVDB_UTILS_HPP__
#define __MVDB_UTILS_HPP__

#include "index.hpp"

using CompareNeighborFn = bool (*)(const Neighbor&, const Neighbor&);

const CompareNeighborFn compare_lowest_first = [](const Neighbor& a, const Neighbor& b) {
    return a.dist < b.dist;
};
const CompareNeighborFn compare_highest_first = [](const Neighbor& a, const Neighbor& b) {
    return a.dist > b.dist;
};

struct NeighborMinHeapCompare {
    bool operator()(const Neighbor& a, const Neighbor& b) const {
        return a.dist > b.dist;
    }
};

struct NeighborMaxHeapCompare {
    bool operator()(const Neighbor& a, const Neighbor& b) const {
        return a.dist < b.dist;
    }
};

#endif