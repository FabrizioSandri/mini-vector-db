#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include "index.hpp"

using ComparePairFn = bool (*)(const Candidate&, const Candidate&);

const ComparePairFn compare_lowest_first = [](const Candidate& a, const Candidate& b) {
    return a.second < b.second;
};
const ComparePairFn compare_highest_first = [](const Candidate& a, const Candidate& b) {
    return a.second > b.second;
};

struct CandidateMinHeapCompare {
    bool operator()(const Candidate& a, const Candidate& b) const {
        return a.second > b.second;
    }
};

struct CandidateMaxHeapCompare {
    bool operator()(const Candidate& a, const Candidate& b) const {
        return a.second < b.second;
    }
};

#endif