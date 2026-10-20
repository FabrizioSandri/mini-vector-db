export enum Distance {
    L2,
    Cosine
}

export type SearchResult = { id: number; distance: number }

export class HNSWIndex {
    constructor(dim: number, M: number, ef_construction: number, efSearch: number, maxLevel: number, distance: Distance);
    add(id: number, vec: Float32Array): void;
    remove(id: number): void;
    size(): number;
    search(query: Float32Array, k: number): SearchResult[];
}