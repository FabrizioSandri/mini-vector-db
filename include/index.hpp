#ifndef __INDEX_HPP__
#define __INDEX_HPP__

#include <cstddef>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "distance.hpp"

using Candidate = std::pair<float, std::size_t>; // (distance, id)

struct HNSWConfig {
    std::size_t dim;                   // Dimensionality of the vectors
    std::size_t M = 16;                // Connections assigned to a newly inserted node
    std::size_t ef_construction = 200; // Size of dynamic candidate list during build
    std::size_t ef_search = 50;        // Size of dynamic candidate list during search
    std::size_t max_level = 16;        // Safety cap on number of layers
    DistanceFunc distance = l2_distance;
    std::size_t Mmax0 = 2 * M;
    std::size_t Mmax = M;
    double mL_ = 1.0 / log(std::max((int)M, 2)); // 1/ln(M)
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
        std::size_t level;
        std::vector<float> embedding; // dim floats
        // Neighbors per layer: [level][neighbor ids]
        std::vector<std::vector<std::size_t>> neighbors;
        std::vector<std::unordered_set<std::size_t>>
            incoming_neighbors; // for reverse edges. Used for pruning when removing nodes.
    };

    HNSWConfig config_;
    std::mt19937 rng_;

    std::unordered_map<std::size_t, Node*> nodes_;
    Node* entry_point_ = nullptr;

    // Internal helpers
    std::size_t random_level();

    std::vector<Candidate> search_layer(const float* query,
                                        Node* ep,
                                        std::size_t ef,
                                        std::size_t level,
                                        std::size_t dim) const;

    std::vector<Candidate> select_neighbors_simple(std::vector<Candidate> candidates,
                                                   std::size_t M) const;
};

#endif