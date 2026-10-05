#include <fmt/core.h>
#include <iostream>
#include <random>
#include <vector>

#include "index.hpp"

using namespace std;

int main() {

    // Demo: build a tiny index of 10000 random 128-d vectors
    const size_t N = 10000;
    const size_t D = 128;

    HNSWConfig cfg;
    cfg.dim = D;
    cfg.M = 16;
    cfg.ef_construction = 100;
    cfg.ef_search = 50;
    cfg.distance = l2_distance;

    HNSWIndex index(cfg);

    mt19937 rng(123);
    uniform_real_distribution<float> dist(-1.0f, 1.0f);

    float** buf = new float*[N];
    for (size_t i = 0; i < N; ++i) {
        buf[i] = new float[D];
        for (size_t j = 0; j < D; ++j) {
            buf[i][j] = dist(rng);
        }
        index.add(i, buf[i], D);
    }

    fmt::print("Built index with {} vectors of dimension {}\n", N, D);

    // Check size
    fmt::print("Index size: {}\n", index.size());

    // Remove a vector by ID
    size_t remove_id = 42;
    index.remove(remove_id);
    fmt::print("Removed vector with ID {}\n", remove_id);

    // Check size
    fmt::print("Index size: {}\n", index.size());

    // Query with a random vector
    float* query = new float[D];
    for (size_t j = 0; j < D; ++j) {
        query[j] = dist(rng);
    }

    const size_t k = 10;
    auto results = index.search(query, k, D);

    fmt::print("Top-{} neighbors:\n", k);
    for (const auto& r : results) {
        fmt::print("  id={} distance={:.4f}\n", r.first, r.second);
    }

    // Clean up
    delete[] query;
    for (size_t i = 0; i < N; ++i) {
        delete[] buf[i];
    }
    delete[] buf;

    return 0;
}
