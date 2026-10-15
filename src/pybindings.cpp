#include <pybind11/native_enum.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "index.hpp"

namespace py = pybind11;

enum class Distance { L2, COSINE };

class PyHNSWIndex : private HNSWIndex {
public:
    PyHNSWIndex(std::size_t dim,
                std::size_t M = 16,
                std::size_t ef_construction = 100,
                std::size_t ef_search = 50,
                std::size_t max_level = 16,
                Distance distance = Distance::L2) :
        HNSWIndex(
            HNSWConfig{.dim = dim,
                       .M = M,
                       .ef_construction = ef_construction,
                       .ef_search = ef_search,
                       .max_level = max_level,
                       .distance = (distance == Distance::L2) ? l2_distance : cosine_distance}) {
    }

    void add(std::size_t id, py::array_t<float, py::array::c_style | py::array::forcecast> array) {
        if (array.ndim() != 1) {
            throw py::value_error("array must be a 1-dimensional NumPy array");
        }
        if (array.shape(0) <= 0) {
            throw py::value_error("array length cannot be non-positive");
        }

        return HNSWIndex::add(id, array.data(), static_cast<std::size_t>(array.shape(0)));
    }

    void add_batch(py::array_t<std::size_t, py::array::c_style | py::array::forcecast> ids,
                   py::array_t<float, py::array::c_style | py::array::forcecast> arrays) {
        if (ids.ndim() != 1) {
            throw py::value_error("ids must be a 1-dimensional NumPy array");
        }
        if (arrays.ndim() != 2) {
            throw py::value_error("vectors must be a 2-dimensional NumPy array");
        }
        if (ids.shape(0) != arrays.shape(0)) {
            throw py::value_error("ids and vectors must contain the same number of items");
        }

        std::vector<const float*> vector_ptrs(arrays.shape(0));
        for (std::size_t i = 0; i < vector_ptrs.size(); ++i) {
            vector_ptrs[i] = arrays.data() + i * arrays.shape(1);
        }

        return HNSWIndex::add_batch(ids.data(),
                                    vector_ptrs.data(),
                                    arrays.shape(0),
                                    arrays.shape(1));
    }

    void remove(std::size_t id) {
        return HNSWIndex::remove(id);
    }

    std::size_t size() const {
        return HNSWIndex::size();
    }

    std::vector<Candidate>
    search(py::array_t<float, py::array::c_style | py::array::forcecast> query,
           std::size_t k) const {
        if (query.ndim() != 1) {
            throw py::value_error("query must be a 1-dimensional NumPy array");
        }
        if (query.shape(0) < 0) {
            throw py::value_error("query length cannot be negative");
        }
        return HNSWIndex::search(query.data(), k, static_cast<std::size_t>(query.shape(0)));
    }
};

PYBIND11_MODULE(_core, m) {
    m.doc() = "MVDB core module";

    py::native_enum<Distance>(m, "Distance", "enum.Enum")
        .value("L2", Distance::L2)
        .value("COSINE", Distance::COSINE)
        .export_values()
        .finalize();

    py::class_<PyHNSWIndex>(m, "HNSWIndex")
        .def(py::init<std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, Distance>(),
             py::arg("dim"),
             py::arg("M") = 16,
             py::arg("ef_construction") = 100,
             py::arg("ef_search") = 50,
             py::arg("max_level") = 16,
             py::arg("distance") = Distance::L2)
        .def("add", &PyHNSWIndex::add, py::arg("id"), py::arg("vector"))
        .def("add_batch", &PyHNSWIndex::add_batch, py::arg("ids"), py::arg("vectors"))
        .def("remove", &PyHNSWIndex::remove, py::arg("id"))
        .def("size", &PyHNSWIndex::size)
        .def("search", &PyHNSWIndex::search, py::arg("query"), py::arg("k"));
}
