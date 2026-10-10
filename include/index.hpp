#ifndef __MVDB_INDEX_HPP__
#define __MVDB_INDEX_HPP__

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

#include "distance.hpp"

using Candidate = std::pair<std::size_t, float>; // (idx/id, distance)

struct Neighbor {
    std::size_t idx;
    float dist;
};

struct HNSWConfig {
    std::size_t dim;                   // Dimensionality of the vectors
    std::size_t M = 16;                // Connections assigned to a newly inserted node
    std::size_t ef_construction = 100; // Size of dynamic candidate list during build
    std::size_t ef_search = 50;        // Size of dynamic candidate list during search
    std::size_t max_level = 16;        // Safety cap on number of layers
    DistanceFunc distance = l2_distance;
    std::size_t Mmax0;
    std::size_t Mmax;
    bool keep_pruned_connections =
        false; // If true, pad R with nearest discarded candidates when heuristic yields < M
    bool extend_candidates =
        false;  // If true, also consider neighbors-of-candidates in the heuristic search
    double mL_; // 1/ln(M)
};

class HNSWIndex {
public:
    explicit HNSWIndex(HNSWConfig config = HNSWConfig{});

    ~HNSWIndex();

    // Add a vector with ID. dim must match consistently across calls.
    void add(std::size_t id, const float* vector, std::size_t dim);

    // Remove a vector with ID
    void remove(std::size_t id);

    // Get the number of vectors in the index
    std::size_t size() const;

    // k-nearest neighbors search
    std::vector<Candidate> search(const float* query, std::size_t k, std::size_t dim) const;

private:
    struct Node {
        std::size_t id;
        std::size_t idx;
        std::size_t level;
        std::uint64_t visit_generation = 0;

        // Neighbors per layer: [level][neighbor ids]
        std::vector<std::vector<Node*>> neighbors;
    };

    HNSWConfig config_;
    std::mt19937 rng_;

    std::unordered_map<std::size_t, std::size_t> idToIdx_;
    std::vector<Node*> nodes_;
    std::vector<float> embeddings_;

    Node* entry_point_ = nullptr;
    mutable uint64_t visit_counter_ = 0; // used to mark visited nodes during search

    // Internal helpers
    std::size_t random_level();

    const float* embedding(const Node* node) const;

    std::vector<Neighbor> search_layer(const float* query,
                                       Node* ep,
                                       std::size_t ef,
                                       std::size_t level,
                                       std::size_t dim) const;

    std::vector<Neighbor> select_neighbors_simple(std::vector<Neighbor> candidates,
                                                  std::size_t M) const;

    std::vector<Neighbor> select_neighbors_heuristic(const float* query,
                                                     const std::vector<Neighbor>& C,
                                                     std::size_t M,
                                                     std::size_t lc,
                                                     std::size_t dim,
                                                     bool extend_candidates,
                                                     bool keep_pruned_connections) const;
};

#endif