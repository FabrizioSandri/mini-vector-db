import createMVDB from "../build/mvdb-core.mjs";

const Module = await createMVDB();

export const { HNSWIndex, Distance } = Module;
export default Module;
