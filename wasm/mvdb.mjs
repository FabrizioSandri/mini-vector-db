import createMVDB from "../build/mvdb-core.js";

const Module = await createMVDB();

export const { HNSWIndex, Distance } = Module;
export default Module;
