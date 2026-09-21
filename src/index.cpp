#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "index.hpp"

using namespace std;

HNSWIndex::HNSWIndex(HNSWConfig config) :
    config_(move(config)),
    rng_(42) {
}

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
    sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.first < b.first;
    });
    if (candidates.size() > M) {
        candidates.resize(M);
    }
    return candidates;
}

void HNSWIndex::add(size_t id, const float* vec, size_t dim) {
    if (dim != this->config_.dim) {
        throw invalid_argument("Vector dimension " + std::to_string(dim) +
                               " does not match index dimension " +
                               std::to_string(this->config_.dim) + ".");
    }

    if (nodes_.find(id) != nodes_.end()) {
        throw runtime_error("Node with ID " + std::to_string(id) + " already exists.");
    }

    const size_t l = random_level();
    Node* node = new Node{id,
                          l,
                          std::vector<float>(vec, vec + dim),
                          std::vector<std::vector<size_t>>(l + 1),
                          std::vector<std::unordered_set<size_t>>(l + 1)};

    nodes_[id] = node;
    if (entry_point_ == nullptr) {
        entry_point_ = node;
        return;
    }

    Node* ep = entry_point_;
    const size_t L = entry_point_->level; // top layer of the graph, *before* this insertion

    // Phase 1: greedy descent with ef=1.
    for (size_t lc = L; lc > l; lc--) {
        auto candidates = search_layer(vec, ep, 1, lc, dim);
        if (!candidates.empty()) {
            ep = nodes_[candidates[0].second];
        }
    }

    // Phase 2: real search + insertion.
    for (size_t lc = min(L, l) + 1; lc-- > 0;) {
        auto candidates = search_layer(vec, ep, config_.ef_construction, lc, dim);
        auto neighbors = select_neighbors_simple(candidates, config_.M);

        // Add bidirectional connections from neighbors to q at layer lc
        for (const auto& [dist, neighbor_id] : neighbors) {
            // edge: new node -> neighbor
            node->neighbors[lc].push_back(neighbor_id);
            nodes_[neighbor_id]->incoming_neighbors[lc].insert(id);

            // edge: neighbor -> new node
            nodes_[neighbor_id]->neighbors[lc].push_back(id);
            node->incoming_neighbors[lc].insert(neighbor_id);
        }

        // Shrink connections of each neighbor if it now exceeds Mmax at this layer.
        //
        // We only ever add ONE reverse edge per neighbor here, and every list is
        // already <= cap going into this call (the invariant is maintained by this
        // same code path on every insertion), so a list can be at most cap+1 long
        // at this point. That means "select the M nearest" reduces to "drop the
        // single farthest element"
        const size_t cap = lc == 0 ? config_.Mmax0 : config_.Mmax;
        for (const auto& [dist, neighbor_id] : neighbors) {
            auto& e_conn = nodes_[neighbor_id]->neighbors[lc];
            if (e_conn.size() <= cap || e_conn.empty()) {
                continue;
            }

            // find the index of the farthest neighbor to prune
            auto to_prune = e_conn.begin();
            float max_dist = config_.distance(nodes_[neighbor_id]->embedding.data(),
                                              nodes_[*to_prune]->embedding.data(),
                                              dim);
            for (auto it = e_conn.begin() + 1; it != e_conn.end(); it++) {
                const float d = config_.distance(nodes_[neighbor_id]->embedding.data(),
                                                 nodes_[*it]->embedding.data(),
                                                 dim);
                if (d > max_dist) {
                    max_dist = d;
                    to_prune = it;
                }
            }

            nodes_[*to_prune]->incoming_neighbors[lc].erase(neighbor_id);
            e_conn.erase(to_prune);
        }

        // ep <- W: continue from the nearest element found at this layer.
        if (!candidates.empty()) {
            ep = nodes_[min_element(candidates.begin(),
                                    candidates.end(),
                                    [](const Candidate& a, const Candidate& b) {
                                        return a.first < b.first;
                                    })
                            ->second];
        }
    }

    // Update the global entry point if the new node is at a higher level than the previous entry
    // point.
    if (l > L) {
        entry_point_ = node;
    }
}

vector<Candidate>
HNSWIndex::search_layer(const float* query, Node* ep, size_t ef, size_t level, size_t dim) const {

    priority_queue<Candidate, vector<Candidate>, greater<>> candidates; // Min-priority queue C
    priority_queue<Candidate> max_queue;                                // Max-priority queue W
    std::unordered_set<size_t> seen;

    const float ep_dist = config_.distance(query, ep->embedding.data(), dim);

    max_queue.emplace(ep_dist, ep->id);
    candidates.emplace(ep_dist, ep->id);
    seen.insert(ep->id);

    while (!candidates.empty()) {
        const auto [dc, closest] = candidates.top(); // extract nearest element from candidates to q
        const auto [df, furthest] = max_queue.top(); // extract farthest element from visited

        if (dc > df) {
            break;
        }

        candidates.pop();
        for (size_t nb : nodes_.at(closest)->neighbors[level]) {
            if (seen.find(nb) != seen.end()) {
                continue;
            }
            seen.insert(nb);
            const float nd = config_.distance(query, nodes_.at(nb)->embedding.data(), dim);
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

std::vector<Candidate> HNSWIndex::search(const float* query, size_t k, std::size_t dim) const {
    if (dim != this->config_.dim) {
        throw invalid_argument("Query dimension " + std::to_string(dim) +
                               " does not match index dimension " +
                               std::to_string(this->config_.dim) + ".");
    }

    if (nodes_.size() == 0) {
        return {};
    }

    Node* ep = entry_point_;
    const size_t L = entry_point_->level;

    // K-NN-SEARCH: for lc <- L ... 1, greedy descent with ef=1.
    for (size_t lc = L; lc > 0; --lc) {
        auto layer_candidates = search_layer(query, ep, 1, lc, dim);
        if (!layer_candidates.empty()) {
            ep = nodes_.at(layer_candidates[0].second);
        }
    }

    // Final search at layer 0 must start from the descended entry point `ep`,
    // not the original global entry point.
    const auto layer_results = search_layer(query, ep, config_.ef_search, 0, dim);

    std::vector<Candidate> results;
    results.reserve(min(k, layer_results.size()));
    for (const auto& [distance, id] : layer_results) {
        if (results.size() == k) {
            break;
        }
        results.push_back({distance, id});
    }
    return results;
}

void HNSWIndex::remove(size_t id) {
    auto node_it = nodes_.find(id);
    if (node_it == nodes_.end()) {
        throw runtime_error("Node with ID " + std::to_string(id) + " does not exist.");
    }
    Node* node = node_it->second;

    // if the node was the entry point and there are other nodes, update the entry point, by picking
    // the node with the highest level among the remaining nodes
    if (nodes_.size() > 1 && id == entry_point_->id) {
        Node* best = nullptr;
        for (const auto& [nid, n] : nodes_) {
            if (nid == id)
                continue;
            if (!best || n->level > best->level)
                best = n;
        }
        entry_point_ = best;
    }

    // remove the node from its neighbors' neighbor lists
    for (size_t lvl = 0; lvl <= node->level; ++lvl) {
        // Outgoing edges (node -> o): o no longer has an incoming edge from node.
        for (size_t out_id : node->neighbors[lvl]) {
            nodes_.at(out_id)->incoming_neighbors[lvl].erase(id);
        }

        // Incoming edges (p -> node): remove the dangling forward edge from p.
        for (size_t in_id : node->incoming_neighbors[lvl]) {
            auto& list = nodes_.at(in_id)->neighbors[lvl];
            list.erase(std::remove(list.begin(), list.end(), id), list.end());
        }
    }

    // remove the node from the index and deallocate its memory
    nodes_.erase(node_it);
    delete node;
}

std::size_t HNSWIndex::size() const {
    return nodes_.size();
}

HNSWIndex::~HNSWIndex() {
    for (const auto& [id, node] : nodes_) {
        delete node;
    }
}