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
    config_(std::move(config)),
    rng_(42) {

    if (config_.dim <= 0)
        throw invalid_argument("Vector dimension must be greater than 0.");

    if (config_.M <= 0)
        throw invalid_argument("M must be greater than 0.");

    config_.Mmax0 = 2 * config_.M;
    config_.Mmax = config_.M;
    config_.mL_ = 1.0 / log(std::max((int)config_.M, 2));

    if (config_.ef_search <= 0)
        throw invalid_argument("ef_search must be greater than 0.");

    if (config_.ef_construction <= 0)
        throw invalid_argument("ef_construction must be greater than 0.");
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

    if (idToIdx_.find(id) != idToIdx_.end()) {
        throw runtime_error("Node with ID " + std::to_string(id) + " already exists.");
    }

    const size_t idx = nodes_.size();
    const size_t l = random_level();
    Node* node = new Node{id, idx, l, 0, std::vector<std::vector<Node*>>(l + 1)};
    idToIdx_[id] = idx;

    nodes_.push_back(node);
    this->embeddings_.insert(embeddings_.end(), vec, vec + dim);
    const float* node_embedding = embedding(node);

    if (entry_point_ == nullptr) {
        entry_point_ = node;
        return;
    }

    // Phase 1: greedy descent with ef=1.
    Node* ep = entry_point_;
    const size_t L = entry_point_->level; // top layer of the graph, *before* this insertion

    for (size_t lc = L; lc > l; lc--) {
        auto candidates = search_layer(node_embedding, ep, 1, lc, dim);
        if (!candidates.empty()) {
            ep = nodes_[candidates[0].idx];
        }
    }

    // Phase 2: real search + insertion
    for (size_t lc = min(L, l) + 1; lc-- > 0;) {
        auto candidates = search_layer(node_embedding, ep, config_.ef_construction, lc, dim);
        // auto neighbors = select_neighbors_simple(candidates, config_.M);
        auto neighbors = select_neighbors_heuristic(node_embedding,
                                                    candidates,
                                                    config_.M,
                                                    lc,
                                                    dim,
                                                    config_.extend_candidates,
                                                    config_.keep_pruned_connections);

        // Add bidirectional connections from neighbors to q at layer lc
        node->neighbors[lc].reserve(neighbors.size());
        for (const Neighbor& neighbor : neighbors) {
            // edge: new node -> neighbor
            node->neighbors[lc].push_back(nodes_[neighbor.idx]);

            // edge: neighbor -> new node
            nodes_[neighbor.idx]->neighbors[lc].push_back(node);
        }

        // Shrink connections of each neighbor if it now exceeds Mmax at this layer.
        //
        // We only ever add ONE reverse edge per neighbor here, and every list is
        // already <= cap going into this call (the invariant is maintained by this
        // same code path on every insertion), so a list can be at most cap+1 long
        // at this point. That means "select the M nearest" reduces to "drop the
        // single farthest element"
        const size_t cap = lc == 0 ? config_.Mmax0 : config_.Mmax;
        for (const Neighbor& neighbor : neighbors) {
            Node* nb = nodes_[neighbor.idx];
            auto& e_conn = nb->neighbors[lc];
            if (e_conn.size() <= cap) {
                continue;
            }

            // find the index of the farthest neighbor to prune
            erase_farthest(e_conn, node);
        }

        // ep <- W: continue from the nearest element found at this layer.
        if (!candidates.empty()) {
            ep = nodes_[candidates.front().idx];
        }
    }

    // Update the global entry point if the new node is at a higher level than the previous entry
    // point.
    if (l > L) {
        entry_point_ = node;
    }
}

void HNSWIndex::erase_farthest(vector<Node*>& nodes, const Node* reference) {
    auto to_prune = nodes.begin();
    float max_dist =
        config_.distance(embedding(reference), embedding(*to_prune), this->config_.dim);
    for (auto it = nodes.begin() + 1; it != nodes.end(); it++) {
        const float d = config_.distance(embedding(reference), embedding(*it), this->config_.dim);
        if (d > max_dist) {
            max_dist = d;
            to_prune = it;
        }
    }

    nodes.erase(to_prune);
}

void HNSWIndex::add_batch(const size_t* ids,
                          const float* const* vectors,
                          size_t count,
                          size_t dim) {
    for (size_t i = 0; i < count; ++i) {
        add(ids[i], vectors[i], dim);
    }
}

vector<Neighbor> HNSWIndex::search_layer(const float* query,
                                         Node* ep,
                                         size_t ef,
                                         size_t level,
                                         size_t dim) const {

    priority_queue<Neighbor, vector<Neighbor>, NeighborMinHeapCompare>
        candidates; // Min-priority queue C
    priority_queue<Neighbor, vector<Neighbor>, NeighborMaxHeapCompare>
        max_queue; // Max-priority queue W

    const float ep_dist = config_.distance(query, embedding(ep), dim);

    max_queue.emplace(ep->idx, ep_dist);
    candidates.emplace(ep->idx, ep_dist);
    std::uint64_t generation = ++visit_counter_; // used to mark visited nodes
    ep->visit_generation = generation;           // mark the entry point as visited

    float df = ep_dist; // distance of the furthest element in W

    while (!candidates.empty()) {
        const Neighbor closest = candidates.top(); // extract nearest element from candidates to q
        if (closest.dist > df) {
            break;
        }
        candidates.pop();

        for (Node* nb : nodes_[closest.idx]->neighbors[level]) {
            // skip already visited nodes in this search generation
            if (nb->visit_generation == generation) {
                continue;
            }
            nb->visit_generation = generation;

            const float nd = config_.distance(query, embedding(nb), dim);
            if (nd < df || max_queue.size() < ef) {
                candidates.emplace(nb->idx, nd);
                max_queue.emplace(nb->idx, nd);

                if (max_queue.size() > ef) {
                    max_queue.pop();
                }
                df = max_queue.top().dist;
            }
        }
    }

    vector<Neighbor> result;
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

    if (k == 0 || nodes_.empty()) {
        return {};
    }

    Node* ep = entry_point_;
    const size_t L = entry_point_->level;

    // K-NN-SEARCH: for lc <- L ... 1, greedy descent with ef=1.
    for (size_t lc = L; lc > 0; --lc) {
        auto layer_candidates = search_layer(query, ep, 1, lc, dim);
        if (!layer_candidates.empty()) {
            ep = nodes_[layer_candidates[0].idx];
        }
    }

    // Final search at layer 0 must start from the descended entry point `ep`,
    // not the original global entry point.
    const size_t ef = std::max(config_.ef_search, k);
    const auto layer_results = search_layer(query, ep, ef, 0, dim);

    std::vector<Candidate> results;
    results.reserve(min(k, layer_results.size()));
    for (const auto& [idx, distance] : layer_results) {
        if (results.size() == k) {
            break;
        }
        results.push_back({nodes_[idx]->id, distance});
    }
    return results;
}

void HNSWIndex::remove(size_t id) {
    auto node_it = idToIdx_.find(id);
    if (node_it == idToIdx_.end()) {
        throw runtime_error("Node with ID " + std::to_string(id) + " does not exist.");
    }
    const std::size_t idx = node_it->second;
    Node* node = nodes_[idx];

    // if the node was the entry point and there are other nodes, update the entry point, by picking
    // the node with the highest level among the remaining nodes
    if (nodes_.size() > 1 && idx == entry_point_->idx) {
        Node* best = nullptr;
        for (Node* n : nodes_) {
            if (n->idx == idx)
                continue;
            if (!best || n->level > best->level)
                best = n;
        }
        entry_point_ = best;
    }

    // Remove the node from the graph
    for (Node* other : nodes_) {
        if (other->idx == idx) {
            continue;
        }
        for (auto& list : other->neighbors) {
            list.erase(std::remove(list.begin(), list.end(), node), list.end());
        }
    }

    Node* last = nodes_.back();
    if (last != node) {
        std::copy_n(embeddings_.data() + last->idx * config_.dim,
                    config_.dim,
                    embeddings_.data() + idx * config_.dim);
        nodes_[idx] = last;
        idToIdx_[last->id] = idx;
        last->idx = idx;
    }
    nodes_.pop_back();
    embeddings_.resize(embeddings_.size() - config_.dim);
    idToIdx_.erase(id);
    delete node;

    if (nodes_.empty()) {
        entry_point_ = nullptr;
    }
}

vector<Neighbor> HNSWIndex::select_neighbors_simple(vector<Neighbor> candidates, size_t M) const {
    if (candidates.size() > M) {
        nth_element(candidates.begin(),
                    candidates.begin() + static_cast<std::vector<Neighbor>::difference_type>(M),
                    candidates.end(),
                    compare_lowest_first);
        candidates.resize(M);
    }

    // Only sort the M survivors.
    sort(candidates.begin(), candidates.end(), compare_lowest_first);
    return candidates;
}

std::vector<Neighbor> HNSWIndex::select_neighbors_heuristic(const float* query,
                                                            const std::vector<Neighbor>& C,
                                                            std::size_t M,
                                                            std::size_t lc,
                                                            std::size_t dim,
                                                            bool extend_candidates,
                                                            bool keep_pruned_connections) const {

    // W: working set of candidates, as a min-heap ordered by distance to q.
    priority_queue<Neighbor, vector<Neighbor>, NeighborMinHeapCompare> W;

    if (!extend_candidates) {
        for (const Neighbor& c : C) {
            W.push(c);
        }
    } else {
        unordered_set<size_t> in_w;
        in_w.reserve(C.size());
        for (const Neighbor& c : C) {
            in_w.insert(c.idx);
            W.push(c);
        }

        // extendCandidates: pull in each candidate's neighbors at this layer too.
        for (const Neighbor& c : C) {
            Node* c_node = nodes_[c.idx];
            for (Node* c_adj : c_node->neighbors[lc]) {
                if (in_w.insert(c_adj->idx).second) {
                    const float d = config_.distance(query, embedding(c_adj), dim);
                    W.emplace(c_adj->idx, d);
                }
            }
        }
    }

    vector<Neighbor> R;
    R.reserve(M);
    vector<Neighbor> Wd; // discarded candidates, kept sorted-on-demand below

    while (!W.empty() && R.size() < M) {
        const Neighbor e = W.top();
        W.pop();

        const float* e_vec = embedding(nodes_[e.idx]);

        // Diversity test: keep e only if it's closer to q than to every
        // element already selected into R (this is what "closer to q
        // compared to any element from R" actually encodes).
        bool keep = true;
        for (const Neighbor& r : R) {
            const float dist_e_r = config_.distance(e_vec, embedding(nodes_[r.idx]), dim);
            if (dist_e_r < e.dist) {
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
    return embeddings_.data() + node->idx * config_.dim;
}

HNSWIndex::~HNSWIndex() {
    for (const Node* node : nodes_) {
        delete node;
    }
}