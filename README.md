```
cmake -S . -B build --fresh \
  -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build
```
