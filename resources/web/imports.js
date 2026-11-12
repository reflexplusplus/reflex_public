import { createLibcImports } from "./libc.js?r1-dev";
import { createThreadImports } from "./threads.js?r1-dev";
import { createIOImports } from "./io.js?r1-dev";

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
            openURL: (pointer, length) =>
            {
                const result = new Int32Array(new SharedArrayBuffer(4));
                self.postMessage({ type: "open-url", url: text(pointer, length), result: result.buffer });
                while (Atomics.load(result, 0) === 0) Atomics.wait(result, 0, 0);
                return Atomics.load(result, 0) > 0 ? 1 : 0;
            },
            assertion: (pointer, length) => onAssertion(text(pointer, length)),
            systemID: () => systemID,
            log: (pointer, length) => console.log(text(pointer, length)),
            warn: (pointer, length) => console.warn(text(pointer, length)),
            error: (pointer, length) => console.error(text(pointer, length)),
            waitAudio: (pointer, generation) => Atomics.wait(new Int32Array(memory.buffer, pointer, 1), 0, generation),
            wakeAudio: pointer => Atomics.notify(new Int32Array(memory.buffer, pointer, 1), 0),
            requestMidi: () => self.postMessage({ type: "midi-request" }),
            enableMidi: (token, enable) => self.postMessage({ type: "midi-enable", token, enable: Boolean(enable) }),
            sendMidi: packed => self.postMessage({ type: "midi-output", packed: packed >>> 0 }),
            resumeAudio: () => self.postMessage({ type: "audio-resume" }),
            performanceTime: () => (timeOffset + performance.now()) / 1000,
        },
    };
}
