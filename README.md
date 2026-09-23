```
cmake -S . -B build/cpp --fresh \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build/cpp
```

```
uv run bash build.sh
```
