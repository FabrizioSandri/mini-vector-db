#include <pybind11/native_enum.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "index.hpp"

namespace py = pybind11;

enum Distance { L2, COSINE };

class PyHNSWIndex : private HNSWIndex {
    public:
        PyHNSWIndex(std::size_t dim,
                    std::size_t M = 16,
                    std::size_t ef_construction = 200,
                    std::size_t ef_search = 50,
                    std::size_t max_level = 16,
                    Distance distance = L2) :
            HNSWIndex(HNSWConfig{.dim = dim,
                                 .M = M,
                                 .ef_construction = ef_construction,
                                 .ef_search = ef_search,
                                 .max_level = max_level,
                                 .distance = (distance == L2) ? l2_distance : cosine_distance}) {
        }

        void add(std::size_t id, py::array_t<float> array, std::size_t dim) {
            return HNSWIndex::add(id, array.data(), dim);
        }

        std::vector<Candidate> search(py::array_t<float> query, std::size_t k) const {
            return HNSWIndex::search(query.data(), k);
        }
};

PYBIND11_MODULE(_core, m) {
    m.doc() = "Mini Vector DB core module";

    py::native_enum<Distance>(m, "Distance", "enum.Enum")
        .value("L2", Distance::L2)
        .value("COSINE", Distance::COSINE)
        .export_values()
        .finalize();

    py::class_<PyHNSWIndex>(m, "HNSWIndex")
        .def(py::init<std::size_t, std::size_t, std::size_t, std::size_t, std::size_t, Distance>(),
             py::arg("dim"),
             py::arg("M") = 16,
             py::arg("ef_construction") = 200,
             py::arg("ef_search") = 50,
             py::arg("max_level") = 16,
             py::arg("distance") = L2)
        .def("add", &PyHNSWIndex::add, py::arg("id"), py::arg("vector"), py::arg("dim"))
        .def("search", &PyHNSWIndex::search, py::arg("query"), py::arg("k"));
}
