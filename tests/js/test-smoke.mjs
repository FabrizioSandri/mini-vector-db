import { HNSWIndex, Distance } from "../../wasm/mvdb.mjs";

const index = new HNSWIndex(2, 16, 100, 50, 16, Distance.L2); // Example usage of the HNSWIndex class
console.log("HNSWIndex instance created:", index);
console.log("HNSWIndex instance created with size:", index.size());
