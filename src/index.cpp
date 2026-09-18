#include "index.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <set>
#include <stdexcept>

using namespace std;

HNSWIndex::HNSWIndex(HNSWConfig config)
    : config_(move(config)), dim_(config_.dim), rng_(42) {}

size_t HNSWIndex::random_level() {
    // l <- floor(-ln(unif(0,1)) * mL)
    uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(rng_);
    while (r <= 0.0) { // guard against log(0)
        r = dist(rng_);
    }
    const size_t level = static_cast<size_t>(std::floor(-std::log(r) * this->config_.mL_));
    return min(level, config_.max_level);
}

vector<Candidate> HNSWIndex::select_neighbors_simple(vector<Candidate> candidates, size_t M) const {
    // return the M nearest elements to q from candidates
    sort(candidates.begin(), candidates.end(),
         [](const Candidate &a, const Candidate &b) {
             return a.first < b.first;
         });
    if (candidates.size() > M) {
        candidates.resize(M);
    }
    return candidates;
}

void HNSWIndex::add(size_t id, const float *vec, size_t dim) {
    if (dim != this->config_.dim) {
        throw invalid_argument(
            "Vector dimension " + std::to_string(dim) +
            " does not match index dimension " + std::to_string(this->config_.dim) + ".");
    }

    if (nodes_.find(id) != nodes_.end()) {
        throw runtime_error("Node with ID " + std::to_string(id) + " already exists.");
    }

    const size_t l = random_level();
    count_++;

    Node node = Node{
        id,
        l,
        std::vector<float>(vec, vec + dim),
        std::vector<std::vector<size_t>>(l + 1)};

    nodes_[id] = std::move(node);
    if (count_ == 1) {
        entry_point_ = 0;
        return;
    }

    size_t ep = entry_point_;
    const size_t L = nodes_[ep].level; // top layer of the graph, *before* this insertion

    // Phase 1: greedy descent with ef=1.
    for (size_t lc = L; lc > l; lc--) {
        auto candidates = search_layer(vec, ep, 1, lc, dim);
        if (!candidates.empty()) {
            ep = candidates[0].second;
        }
    }

    // Phase 2: real search + insertion.
    for (size_t lc = min(L, l) + 1; lc-- > 0;) {
        auto candidates = search_layer(vec, ep, config_.ef_construction, lc, dim);
        auto neighbors = select_neighbors_simple(candidates, config_.M);

        // Add bidirectional connections from neighbors to q at layer lc
        for (const auto &[dist, neighbor_id] : neighbors) {
            nodes_[id].neighbors[lc].push_back(neighbor_id);
            nodes_[neighbor_id].neighbors[lc].push_back(id);
        }

        // Shrink connections of each neighbor if it now exceeds Mmax at this layer.
        //
        // We only ever add ONE reverse edge per neighbor here, and every list is
        // already <= cap going into this call (the invariant is maintained by this
        // same code path on every insertion), so a list can be at most cap+1 long
        // at this point. That means "select the M nearest" reduces to "drop the
        // single farthest element"
        const size_t cap = lc == 0 ? config_.Mmax0 : config_.Mmax;
        for (const auto &[dist, neighbor_id] : neighbors) {
            auto &e_conn = nodes_[neighbor_id].neighbors[lc];
            if (e_conn.size() <= cap) {
                continue;
            }

            size_t to_prune = -1; // the index of the farthest neighbor to prune
            float max_dist = -std::numeric_limits<float>::max();
            for (size_t i = 0; i < e_conn.size(); i++) {
                const float d = config_.distance(nodes_[neighbor_id].embedding.data(), nodes_[e_conn[i]].embedding.data(), dim_);
                if (d > max_dist) {
                    max_dist = d;
                    to_prune = i;
                }
            }
            if (to_prune == -1) {
                throw runtime_error("Failed to find a neighbor to prune, this should never happen.");
            }
            e_conn.erase(e_conn.begin() + to_prune);
        }

        // ep <- W: continue from the nearest element found at this layer.
        if (!candidates.empty()) {
            ep = min_element(candidates.begin(), candidates.end(),
                             [](const Candidate &a, const Candidate &b) {
                                 return a.first < b.first;
                             })
                     ->second;
        }
    }

    // Update the global entry point if the new node is at a higher level than the previous entry point.
    if (l > L) {
        entry_point_ = id;
    }
}

vector<Candidate> HNSWIndex::search_layer(
    const float *query,
    size_t ep,
    size_t ef,
    size_t level,
    size_t dim) const {

    priority_queue<Candidate, vector<Candidate>, greater<>> candidates; // Min-priority queue C
    priority_queue<Candidate> max_queue;                                // Max-priority queue W
    std::set<size_t> seen;

    const float ep_dist = config_.distance(query, nodes_.at(ep).embedding.data(), dim);

    max_queue.emplace(ep_dist, ep);
    candidates.emplace(ep_dist, ep);
    seen.insert(ep);

    while (!candidates.empty()) {
        const auto [dc, closest] = candidates.top(); // extract nearest element from candidates to q
        const auto [df, furthest] = max_queue.top(); // extract farthest element from visited

        if (dc > df) {
            break;
        }

        candidates.pop();
        for (size_t nb : nodes_.at(closest).neighbors[level]) {
            if (seen.find(nb) != seen.end()) {
                continue;
            }
            seen.insert(nb);
            const float nd = config_.distance(query, nodes_.at(nb).embedding.data(), dim);
            const auto [df, furthest] = max_queue.top(); // extract farthest element from visited

            if (nd < df || max_queue.size() < ef) {
                candidates.emplace(nd, nb);
                max_queue.emplace(nd, nb);

                if (max_queue.size() > ef) {
                    max_queue.pop();
                }
            }
        }
    }
    vector<Candidate> result;
    while (!max_queue.empty()) {
        const auto [d, id] = max_queue.top();
        max_queue.pop();
        result.push_back({d, id});
    }
    reverse(result.begin(), result.end());

    return result;
}

std::vector<Candidate> HNSWIndex::search(const float *query, size_t k) const {
    if (count_ == 0) {
        return {};
    }

    size_t ep = entry_point_;
    const size_t L = nodes_.at(entry_point_).level;

    // K-NN-SEARCH: for lc <- L ... 1, greedy descent with ef=1.
    for (size_t lc = L; lc > 0; --lc) {
        auto layer_candidates = search_layer(query, ep, 1, lc, dim_);
        if (!layer_candidates.empty()) {
            ep = layer_candidates[0].second;
        }
    }

    // Final search at layer 0 must start from the descended entry point `ep`,
    // not the original global entry point.
    const auto layer_results = search_layer(query, ep, config_.ef_search, 0, dim_);

    std::vector<Candidate> results;
    results.reserve(min(k, layer_results.size()));
    for (const auto &[distance, id] : layer_results) {
        if (results.size() == k) {
            break;
        }
        results.push_back({distance, nodes_.at(id).id});
    }
    return results;
}
