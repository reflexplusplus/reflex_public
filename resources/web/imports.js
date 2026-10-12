import { createLibcImports } from "./libc.js";
import { createThreadImports } from "./threads.js";
import { createIOImports } from "./io.js";

// Explicit imports for the Clang backend. No generated runtime helpers.
export function createImports(memory, systemID = 1n, onAssertion = console.error, timeOrigin = performance.timeOrigin) {
    // Normalize each worker's local clock to the shared process origin.
    const timeOffset = performance.timeOrigin - timeOrigin;
    const decoder = new TextDecoder("utf-8");
    const text = (pointer, length) => {
        // Create the view at access time, so shared-memory growth is observed.
        return decoder.decode(new Uint8Array(memory.buffer, pointer >>> 0, length >>> 0).slice());
    };
    return {
        env: { memory },
        wasi: createThreadImports(),
        reflex_io: createIOImports(memory),
        wasi_snapshot_preview1: createLibcImports(memory, undefined, timeOrigin),
        reflex_host: {
            assertion: (pointer, length) => onAssertion(text(pointer, length)),
            systemID: () => systemID,
            log: (pointer, length) => console.log(text(pointer, length)),
            warn: (pointer, length) => console.warn(text(pointer, length)),
            error: (pointer, length) => console.error(text(pointer, length)),
            waitAudio: (pointer, generation) => Atomics.wait(new Int32Array(memory.buffer, pointer, 1), 0, generation),
            wakeAudio: pointer => Atomics.notify(new Int32Array(memory.buffer, pointer, 1), 0),
            performanceTime: () => (timeOffset + performance.now()) / 1000,
        },
    };
}
