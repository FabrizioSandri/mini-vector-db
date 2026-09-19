```
cmake -S . -B build/cpp --fresh \
  -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build/cpp
```

```
uv run bash build.sh
```
