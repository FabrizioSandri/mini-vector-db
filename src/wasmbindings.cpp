#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "index.hpp"

enum class Distance { L2, COSINE };

class WasmHNSWIndex {
private:
    std::unique_ptr<HNSWIndex> index_;

public:
    WasmHNSWIndex(std::size_t dim,
                  std::size_t M,
                  std::size_t ef_construction,
                  std::size_t ef_search,
                  std::size_t max_level,
                  Distance distance) :
        index_(std::make_unique<HNSWIndex>(HNSWConfig{
            .dim = dim,
            .M = M,
            .ef_construction = ef_construction,
            .ef_search = ef_search,
            .max_level = max_level,
            .distance = (distance == Distance::L2) ? l2_distance : cosine_distance,
            .Mmax0 = 2 * M,
            .Mmax = M,
            .mL_ = 1.0 / log(std::max((int)M, 2)),
        })) {
    }

    void add(std::size_t id, const emscripten::val& vec) {
        const auto v = emscripten::convertJSArrayToNumberVector<float>(vec);
        index_->add(id, v.data(), v.size());
    }

    void remove(std::size_t id) {
        index_->remove(id);
    }

    std::size_t size() const {
        return index_->size();
    }

    // returns [{id, distance}, ...]
    emscripten::val search(const emscripten::val& query, std::size_t k) const {
        const auto q = emscripten::convertJSArrayToNumberVector<float>(query);
        const auto res = index_->search(q.data(), k, q.size());
        emscripten::val out = emscripten::val::array();
        for (const auto& [id, dist] : res) {
            emscripten::val o = emscripten::val::object();
            o.set("id", id);
            o.set("distance", dist);
            out.call<void>("push", o);
        }
        return out;
    }
};

EMSCRIPTEN_BINDINGS(mvdb) {
    emscripten::enum_<Distance>("Distance")
        .value("L2", Distance::L2)
        .value("COSINE", Distance::COSINE);

    emscripten::class_<WasmHNSWIndex>("HNSWIndex")
        .constructor<std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, Distance>()
        .function("add", &WasmHNSWIndex::add)
        .function("remove", &WasmHNSWIndex::remove)
        .function("size", &WasmHNSWIndex::size)
        .function("search", &WasmHNSWIndex::search);
}
