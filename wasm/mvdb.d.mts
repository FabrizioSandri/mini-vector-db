export enum Distance {
    L2,
    COSINE
}

export interface SearchResult {
    id: number;
    distance: number;
}

export class HNSWIndex {
    constructor(
        dim: number,
        M: number,
        efConstruction: number,
        efSearch: number,
        maxLevel: number,
        distance: Distance,
    );

    add(id: number, vec: Float32Array | number[]): void;
    remove(id: number): void;
    size(): number;
    search(query: Float32Array | number[], k: number): SearchResult[];
}

declare const Module: {
    HNSWIndex: typeof HNSWIndex;
    Distance: typeof Distance;
};
export default Module;
