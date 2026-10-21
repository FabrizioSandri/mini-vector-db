// tests/ts/test.ts
import { HNSWIndex, Distance } from "../../wasm/mvdb.mjs";

// TypeScript validates that these are all numbers and enums matching your constructor
const index = new HNSWIndex(2, 16, 100, 50, 16, Distance.COSINE); 

console.log("HNSWIndex instance created:", index);
console.log("HNSWIndex instance created with size:", index.size());

// Add dummy vector to test API typing validation
const vector = new Float32Array([1.0, 2.0]);
index.add(1, vector);
console.log("Size after adding item:", index.size());

const results = index.search(vector, 1);
console.log("Search results:", results);