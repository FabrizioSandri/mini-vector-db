#include <chrono>
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
    }

    fmt::print("Built index with {} vectors of dimension {}\n", N, D);

    // Measure the time taken to add all vectors to the index
    auto start_time = chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; ++i) {
        index.add(i, buf[i], D);
    }
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

    fmt::print("Built index with {} vectors of dimension {} in {} ms\n", N, D, duration.count());

    // Check size
    fmt::print("Index size: {}\n", index.size());

    // Query with a random vector
    float* query = new float[D];
    for (size_t j = 0; j < D; ++j) {
        query[j] = dist(rng);
    }

    const size_t k = 10;
    auto start_search_time = chrono::high_resolution_clock::now();
    auto results = index.search(query, k, D);
    auto end_search_time = chrono::high_resolution_clock::now();
    auto search_duration =
        chrono::duration_cast<chrono::milliseconds>(end_search_time - start_search_time);

    fmt::print("Top-{} neighbors found in {} ms:\n", k, search_duration.count());
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
