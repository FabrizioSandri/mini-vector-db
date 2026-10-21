.PHONY: python cmake wasm

python:
	uv sync

cpp:
	cmake -S . -B build/cpp -DMVDB_BUILD_EXAMPLES=ON
	cmake --build build/cpp

wasm:
	emcmake cmake -S . -B build/wasm -DMVDB_BUILD_WASM=ON -DMVDB_BUILD_EXAMPLES=OFF
	cmake --build build/wasm
