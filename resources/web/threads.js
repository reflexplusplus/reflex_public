export function createThreadManager(module, memory, connectIO, onError, handleService, systemID, timeOrigin) {
    let nextId = 2;
    const workers = new Set();
    const workerURL = new URL("./thread-worker.js?r6-dev", import.meta.url);
    let stopped = false;

    function spawn(data, reply) {
        const id = nextId++;
        let worker;
        let disconnectIO;
        const dispose = () => {
            if (!workers.delete(worker)) return;
            worker.terminate();
            disconnectIO?.();
        };
        const fail = error => {
            if (stopped) return;
            dispose();
            reply(-1);
            onError(error);
        };
        try {
            worker = new Worker(workerURL, { type: "module" });
            workers.add(worker);
            disconnectIO = connectIO(worker, { type: "initialize", module, memory, systemID, timeOrigin });
            worker.onmessage = event => {
                if (!workers.has(worker)) return;
                if (handle(event) || handleService(event)) return;
                if (event.data.type === "thread-ready") {
                    try {
                        worker.postMessage({ type: "run", id, argument: data.argument });
                        reply(id);
                    } catch (error) { fail(error); }
                } else if (event.data.type === "thread-done") {
                    dispose();
                } else if (event.data.type === "error") {
                    const error = new Error(event.data.message);
                    error.stack = event.data.message;
                    fail(error);
                }
            };
            worker.onerror = event => fail(new Error(event.message || "Reflex pthread worker failed"));
            worker.onmessageerror = () => fail(new Error("Could not decode a message from a Reflex pthread worker"));
        } catch (error) {
            fail(error);
        }
    }

    const handle = ({ data }) => {
        if (data.type !== "spawn-thread") return false;
        const result = new Int32Array(data.result);
        let replied = false;
        const reply = value => {
            if (replied) return;
            replied = true;
            Atomics.store(result, 1, value);
            Atomics.store(result, 0, 1);
            Atomics.notify(result, 0);
        };
        if (stopped) reply(-1);
        else spawn(data, reply);
        return true;
    };
    return {
        handle,
        stop: () => {
            stopped = true;
            for (const worker of workers) worker.terminate();
            workers.clear();
        },
    };
}
