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
#include "utils.hpp"

using namespace std;

HNSWIndex::HNSWIndex(HNSWConfig config) :
    config_(move(config)),
    rng_(42) {

    if (config_.dim <= 0)
        throw invalid_argument("Vector dimension must be greater than 0.");

    if (config_.M <= 0)
        throw invalid_argument("M must be greater than 0.");

    if (config.Mmax0 < config.M || config.Mmax < config.M || config.Mmax0 < 1 || config.Mmax < 1)
        throw invalid_argument(
            "Mmax0 and Mmax must be greater than or equal to M and greater than 0.");

    if (config_.ef_search == 0)
        throw invalid_argument("ef_search must be greater than 0.");
}

size_t HNSWIndex::random_level() {
    // l <- floor(-ln(unif(0,1)) * mL)
    uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(rng_);
    r = (r == 0.0) ? 1.0 : r; // guard against log(0)
    const size_t level = static_cast<size_t>(std::floor(-std::log(r) * this->config_.mL_));
    return min(level, config_.max_level);
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
    const size_t embedding_offset = embeddings_.size();
    this->embeddings_.insert(embeddings_.end(), vec, vec + dim);
    Node* node = new Node{id, l, embedding_offset, std::vector<std::vector<Node*>>(l + 1)};

    nodes_[id] = node;
    if (entry_point_ == nullptr) {
        entry_point_ = node;
        return;
    }

    Node* ep = entry_point_;
    const size_t L = entry_point_->level; // top layer of the graph, *before* this insertion

    // Phase 1: greedy descent with ef=1.
    for (size_t lc = L; lc > l; lc--) {
        auto candidates = search_layer(embedding(node), ep, 1, lc, dim);
        if (!candidates.empty()) {
            ep = nodes_[candidates[0].second];
        }
    }

    // Phase 2: real search + insertion.
    for (size_t lc = min(L, l) + 1; lc-- > 0;) {
        auto candidates = search_layer(embedding(node), ep, config_.ef_construction, lc, dim);
        // auto neighbors = select_neighbors_simple(candidates, config_.M);
        auto neighbors = select_neighbors_heuristic(embedding(node),
                                                    candidates,
                                                    config_.M,
                                                    lc,
                                                    dim,
                                                    config_.extend_candidates,
                                                    config_.keep_pruned_connections);

        // Add bidirectional connections from neighbors to q at layer lc
        for (const auto& [dist, neighbor_id] : neighbors) {
            Node* neighbor = nodes_[neighbor_id];

            // edge: new node -> neighbor
            node->neighbors[lc].push_back(neighbor);

            // edge: neighbor -> new node
            neighbor->neighbors[lc].push_back(node);
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
            Node* neighbor = nodes_[neighbor_id];
            auto& e_conn = neighbor->neighbors[lc];
            if (e_conn.size() <= cap) {
                continue;
            }

            // find the index of the farthest neighbor to prune
            auto to_prune = e_conn.begin();
            float max_dist = config_.distance(embedding(neighbor), embedding(*to_prune), dim);
            for (auto it = e_conn.begin() + 1; it != e_conn.end(); it++) {
                const float d = config_.distance(embedding(neighbor), embedding(*it), dim);
                if (d > max_dist) {
                    max_dist = d;
                    to_prune = it;
                }
            }

            e_conn.erase(to_prune);
        }

        // ep <- W: continue from the nearest element found at this layer.
        if (!candidates.empty()) {
            ep = nodes_[min_element(candidates.begin(), candidates.end(), compare_lowest_first)
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

    const float ep_dist = config_.distance(query, embedding(ep), dim);

    max_queue.emplace(ep_dist, ep->id);
    candidates.emplace(ep_dist, ep->id);
    seen.insert(ep->id);

    float df = ep_dist; // distance of the furthest element in W

    while (!candidates.empty()) {
        const auto [dc, closest] = candidates.top(); // extract nearest element from candidates to q

        if (dc > df) {
            break;
        }
        candidates.pop();

        for (Node* nb : nodes_.at(closest)->neighbors[level]) {
            if (!seen.insert(nb->id).second) {
                continue;
            }

            const float nd = config_.distance(query, embedding(nb), dim);

            if (nd < df || max_queue.size() < ef) {
                candidates.emplace(nd, nb->id);
                max_queue.emplace(nd, nb->id);

                if (max_queue.size() > ef) {
                    max_queue.pop();
                }
                df = max_queue.top().first;
            }
        }
    }

    vector<Candidate> result;
    result.reserve(max_queue.size());
    while (!max_queue.empty()) {
        result.push_back(max_queue.top());
        max_queue.pop();
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
    const size_t ef = std::max(config_.ef_search, k);
    const auto layer_results = search_layer(query, ep, ef, 0, dim);

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

    // Remove the node from the graph
    for (const auto& [other_id, other] : nodes_) {
        if (other_id == id) {
            continue;
        }
        for (auto& list : other->neighbors) {
            list.erase(std::remove(list.begin(), list.end(), node), list.end());
        }
    }

    // remove the node from the index and deallocate its memory
    nodes_.erase(node_it);
    delete node;
}

vector<Candidate> HNSWIndex::select_neighbors_simple(vector<Candidate> candidates, size_t M) const {
    if (candidates.size() > M) {
        nth_element(candidates.begin(),
                    candidates.begin() + static_cast<std::vector<Candidate>::difference_type>(M),
                    candidates.end(),
                    compare_lowest_first);
        candidates.resize(M);
    }

    // Only sort the M survivors.
    sort(candidates.begin(), candidates.end(), compare_lowest_first);
    return candidates;
}

std::vector<Candidate> HNSWIndex::select_neighbors_heuristic(const float* query,
                                                             const std::vector<Candidate>& C,
                                                             std::size_t M,
                                                             std::size_t lc,
                                                             std::size_t dim,
                                                             bool extend_candidates,
                                                             bool keep_pruned_connections) const {

    // W: working set of candidates, as a min-heap ordered by distance to q.
    priority_queue<Candidate, vector<Candidate>, greater<>> W;
    unordered_set<size_t> in_w;

    for (const auto& c : C) {
        if (in_w.insert(c.second).second) {
            W.push(c);
        }
    }

    // extendCandidates: pull in each candidate's neighbors at this layer too.
    if (extend_candidates) {
        for (const auto& c : C) {
            auto it = nodes_.find(c.second);
            if (it == nodes_.end()) {
                continue;
            }
            for (Node* c_adj : it->second->neighbors[lc]) {
                if (in_w.insert(c_adj->id).second) {
                    const float d = config_.distance(query, embedding(c_adj), dim);
                    W.emplace(d, c_adj->id);
                }
            }
        }
    }

    vector<Candidate> R;
    R.reserve(M);
    vector<Candidate> Wd; // discarded candidates, kept sorted-on-demand below

    while (!W.empty() && R.size() < M) {
        const Candidate e = W.top();
        W.pop();

        const float* e_vec = embedding(nodes_.at(e.second));

        // Diversity test: keep e only if it's closer to q than to every
        // element already selected into R (this is what "closer to q
        // compared to any element from R" actually encodes).
        bool keep = true;
        for (const auto& r : R) {
            const float dist_e_r = config_.distance(e_vec, embedding(nodes_.at(r.second)), dim);
            if (dist_e_r < e.first) {
                keep = false;
                break;
            }
        }

        if (keep) {
            R.push_back(e);
        } else if (keep_pruned_connections) {
            Wd.push_back(e);
        }
    }

    // keepPrunedConnections: top up R from the discarded set, nearest first.
    if (keep_pruned_connections && R.size() < M && !Wd.empty()) {
        sort(Wd.begin(), Wd.end(), compare_lowest_first);
        size_t idx = 0;
        while (R.size() < M && idx < Wd.size()) {
            R.push_back(Wd[idx++]);
        }
    }

    return R;
}

std::size_t HNSWIndex::size() const {
    return nodes_.size();
}

const float* HNSWIndex::embedding(const Node* node) const {
    return embeddings_.data() + node->embedding_offset;
}

HNSWIndex::~HNSWIndex() {
    for (const auto& [id, node] : nodes_) {
        delete node;
    }
}