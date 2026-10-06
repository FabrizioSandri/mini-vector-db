#ifndef __MVDB_DISTANCE_HPP__
#define __MVDB_DISTANCE_HPP__

#include <cstddef>
#include <cstdint>

// Distance function types for float vectors
using DistanceFunc = float (*)(const float* a, const float* b, std::size_t dim);

// Euclidean L2 distance: ||a - b||^2
float l2_distance(const float* a, const float* b, std::size_t dim);

// Cosine distance: 1 - cosine_similarity
float cosine_distance(const float* a, const float* b, std::size_t dim);

// Dot product (for inner-product search variants)
float dot_product(const float* a, const float* b, std::size_t dim);

#endif