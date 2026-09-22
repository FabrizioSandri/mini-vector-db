#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include "index.hpp"

using ComparePairFn = bool (*)(const Candidate&, const Candidate&);

const ComparePairFn compare_lowest_first = [](const Candidate& a, const Candidate& b) {
    return a.first < b.first;
};
const ComparePairFn compare_highest_first = [](const Candidate& a, const Candidate& b) {
    return a.first > b.first;
};

#endif