import { expect, test } from 'vitest';

import { HNSWIndex, Distance } from '../../wasm/mvdb.mjs';

test('typed module exports and index operations', () => {
    const metric = Distance.COSINE;
    const index = new HNSWIndex(2, 16, 100, 50, 16, metric);

    index.add(1, new Float32Array([1, 0]));
    index.add(2, [0, 1]);
    expect(index.size()).toBe(2);

    const results = index.search(new Float32Array([1, 0]), 2);
    expect(results[0]?.id).toBe(1);
    expect(results[0]?.distance).toBeCloseTo(0, 6);
    expect(results[1]?.id).toBe(2);
    
    index.remove(1);
    expect(index.size()).toBe(1);
});
