export function createIOImports(memory) {
    return {
        request: (operation, handle, pointer, length, offset) => {
            const reply = new SharedArrayBuffer(16);
            const state = new Int32Array(reply);
            self.postMessage({ type: "io", operation, handle, pointer: pointer >>> 0, length: length >>> 0, offset, reply });
            while (Atomics.load(state, 0) === 0) Atomics.wait(state, 0, 0);
            return new DataView(reply).getBigInt64(8, true);
        },
    };
}

export function createIOManager(memory, onError = console.error) {
    const worker = new Worker(new URL("./io-worker.js", import.meta.url), { type: "module" });
    const pending = new Map();
    let nextRequest = 1;
    let failed = false;
    const finish = reply => { new DataView(reply).setBigInt64(8, -1n, true); Atomics.store(new Int32Array(reply), 0, 1); Atomics.notify(new Int32Array(reply), 0); };
    worker.postMessage({ type: "initialize", memory });
    worker.onmessage = ({ data }) => {
        if (data.type === "io-complete") pending.delete(data.requestId);
        else if (data.type === "http-diagnostic") console.warn("Reflex HTTP:", data.message);
    };
    worker.onerror = event => { failed = true; for (const reply of pending.values()) finish(reply); pending.clear(); onError(new Error(event.message)); };
    return {
        handle: ({ data }) => {
            if (data.type !== "io") return false;
            if (failed) finish(data.reply);
            else { const requestId = nextRequest++; pending.set(requestId, data.reply); worker.postMessage({ ...data, requestId }); }
            return true;
        },
        stop: () => { failed = true; for (const reply of pending.values()) finish(reply); pending.clear(); worker.terminate(); },
    };
}
