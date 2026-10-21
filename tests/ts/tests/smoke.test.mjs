import { expect, test } from 'vitest';

// Tests consume the generated WASM package at the repository root.
// eslint-disable-next-line import/no-relative-parent-imports
import { Distance, HNSWIndex } from '../../../wasm/mvdb.mjs';

test.each([
    ['L2', Distance.L2, 1],
    ['COSINE', Distance.COSINE, 0],
])('%s: add, search and remove', (_name, metric, expectedDistance) => {
    const index = new HNSWIndex(2, 16, 100, 50, 16, metric);
    expect(index.size()).toBe(0);
    expect(index.search([1, 0], 1)).toEqual([]);
    index.add(10, new Float32Array([2, 0]));
    index.add(20, [0, 1]);
    expect(index.size()).toBe(2);
    const results = index.search(new Float32Array([1, 0]), 10);
    expect(Array.isArray(results)).toBe(true);
    expect(results.map(({ id }) => id)).toEqual([10, 20]);
    expect(results[0].distance).toBeCloseTo(expectedDistance, 6);
    expect(results[0].distance).toBeLessThanOrEqual(results[1].distance);
    expect(index.search([1, 0], 0)).toEqual([]);
    index.remove(10);
    expect(index.size()).toBe(1);
    expect(index.search([1, 0], 1)[0].id).toBe(20);
    index.remove(20);
    expect(index.size()).toBe(0);
    expect(index.search([1, 0], 1)).toEqual([]);
    index.add(30, [1, 0]);
    expect(index.search([1, 0], 1)[0].id).toBe(30);
});

test('core validation becomes native JavaScript Errors', () => {
    const index = new HNSWIndex(2, 16, 100, 50, 16, Distance.L2);
    expect(() => index.add(1, [1])).toThrowError(/dimension/i);
    expect(() => index.search([1], 1)).toThrowError(/dimension/i);
    expect(index.size()).toBe(0);
    index.add(1, [1, 2]);
    expect(() => index.add(1, [1, 2])).toThrowError(/already exists/);
    expect(() => index.remove(999)).toThrowError(/does not exist/);
    expect(index.size()).toBe(1);
    expect(index.search([1, 2], 1)).toEqual([{ id: 1, distance: 0 }]);
});
