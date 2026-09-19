```
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=/home/fabri/projects/mini-vector-db/vcpkg_installed/x64-linux

cmake --build build
```

```
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=/home/fabri/projects/mini-vector-db/vcpkg_installed/x64-linux \
  -Dpybind11_DIR="$PWD/build/vcpkg_installed/x64-linux/share/pybind11" \
  -DPython_EXECUTABLE="$PWD/.venv/bin/python"

cmake --build build
```
