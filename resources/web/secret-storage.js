// The page owns the dedicated worker. Wasm callers send pointers and wait for a reply.
export function createSecretStorageManager(memory, enabled) {
    let current;
    let nextHandle = 1;
    let nextRequest = 1;

    function reply(pointer, result) {
        const view = new DataView(memory.buffer, pointer, 16);
        view.setBigInt64(8, BigInt(result), true);
        const state = new Int32Array(view.buffer, pointer, 1);
        Atomics.store(state, 0, 1);
        Atomics.notify(state, 0);
    }

    function fail(session) {
        if (current !== session) return;
        current = undefined;
        session.worker.terminate();
        session.port.close();
        for (const request of session.pending.values()) reply(request.reply, -1);
        session.pending.clear();
    }

    function stopIfIdle(session) {
        if (session.handles.size || session.pending.size) return;
        current = undefined;
        try { session.port.postMessage({ type: "shutdown" }); }
        catch { session.worker.terminate(); session.port.close(); }
    }

    function start() {
        const { port1, port2 } = new MessageChannel();
        let worker;
        try {
            worker = new Worker(new URL("./secret-storage-worker.js?r5-dev", import.meta.url), { type: "module" });
            worker.postMessage({ memory, port: port2 }, [port2]);
        } catch (error) {
            worker?.terminate();
            port1.close();
            port2.close();
            throw error;
        }
        const session = { worker, port: port1, handles: new Set(), pending: new Map() };
        port1.onmessage = ({ data }) => {
            if (current !== session) return;
            const request = session.pending.get(data.id);
            if (!request || !Number.isSafeInteger(data.result)) {
                fail(session);
                return;
            }
            session.pending.delete(data.id);
            let result = data.result;
            if (request.operation === 0 && result === 0) {
                result = nextHandle++;
                session.handles.add(result);
            }
            reply(request.reply, result);
            stopIfIdle(session);
        };
        port1.onmessageerror = () => fail(session);
        worker.onmessage = () => fail(session);
        worker.onmessageerror = () => fail(session);
        worker.onerror = event => { event.preventDefault(); fail(session); };
        current = session;
        return session;
    }

    return {
        handle: ({ data }) => {
            if (data.type !== "secret-request") return false;
            const { operation, handle, pointer, reply: address } = data;
            if (!enabled) {
                reply(address, -1);
                return true;
            }
            let session = current;
            if (operation === 0) {
                try { session ??= start(); }
                catch { reply(address, -1); return true; }
            } else if (!session?.handles.has(handle)) {
                reply(address, -1);
                return true;
            }
            if (operation === 4) {
                session.handles.delete(handle);
                stopIfIdle(session);
                reply(address, 0);
                return true;
            }
            const id = nextRequest++;
            session.pending.set(id, { operation, reply: address });
            try { session.port.postMessage({ id, operation, pointer }); }
            catch { fail(session); }
            return true;
        },
        stop: () => { if (current) fail(current); },
    };
}
