// The page owns worker creation; callers can wait inside a Reflex worker.
// The compiled pthread layer owns each thread's Wasm stack and TLS allocation.
export function createThreadImports(post = message => self.postMessage(message)) {
    return {
        "thread-spawn": argument => {
            const result = new Int32Array(new SharedArrayBuffer(8));
            post({ type: "spawn-thread", argument, result: result.buffer });
            while (Atomics.load(result, 0) === 0) Atomics.wait(result, 0, 0);
            return Atomics.load(result, 1);
        },
    };
}

export function createThreadManager(module, memory, onError = console.error, handleService = () => false, systemID = 1n, timeOrigin = performance.timeOrigin) {
    let nextId = 2;
    const workers = new Set();
    const handle = ({ data }) => {
        if (data.type !== "spawn-thread") return false;
        const result = new Int32Array(data.result);
        const reply = value => {
            Atomics.store(result, 1, value);
            Atomics.store(result, 0, 1);
            Atomics.notify(result, 0);
        };
        let worker;
        let replied = false;
        const id = nextId++;
        try {
            worker = new Worker(new URL("./thread-worker.js", import.meta.url), { type: "module" });
            workers.add(worker);
            worker.onmessage = event => {
                if (handle(event) || handleService(event)) return;
                if (event.data.type === "thread-ready") {
                    replied = true;
                    reply(id);
                    worker.postMessage({ type: "run", id, argument: data.argument });
                } else if (event.data.type === "thread-done" || event.data.type === "error") {
                    if (!replied) { replied = true; reply(-1); }
                    if (event.data.type === "error") {
                        const error = new Error(event.data.message);
                        error.stack = event.data.message;
                        onError(error);
                    }
                    workers.delete(worker);
                    worker.terminate();
                }
            };
            worker.onerror = event => {
                if (!replied) { replied = true; reply(-1); }
                workers.delete(worker);
                worker.terminate();
                onError(new Error(event.message));
            };
            worker.postMessage({ type: "initialize", module, memory, systemID, timeOrigin });
        } catch (error) {
            reply(-1);
            if (worker) { workers.delete(worker); worker.terminate(); }
            onError(error);
        }
        return true;
    };
    return { handle, stop: () => { for (const worker of workers) worker.terminate(); workers.clear(); } };
}
