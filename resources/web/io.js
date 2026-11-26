export function createIOManager(memory, onError) {
    const worker = new Worker(new URL("./io-worker.js?r6-dev", import.meta.url), { type: "module" });
    let nextPortId = 1;
    worker.postMessage({ type: "initialize", memory });
    worker.onmessage = ({ data }) => {
        if (data.type === "http-diagnostic") console.warn("Reflex HTTP:", data.message);
        if (data.type === "io-fatal") onError(new Error(`Reflex I/O worker: ${data.message}`));
    };
    worker.onerror = event => onError(new Error(event.message || "Reflex I/O worker failed"));
    worker.onmessageerror = () => onError(new Error("Could not decode a message from the Reflex I/O worker"));
    return {
        connect: (caller, message, transfer = []) => {
            const id = nextPortId++;
            const { port1, port2 } = new MessageChannel();
            caller.postMessage({ ...message, ioPort: port1 }, [...transfer, port1]);
            worker.postMessage({ type: "connect", id, port: port2 }, [port2]);
            return () => worker.postMessage({ type: "disconnect", id });
        },
        stop: () => worker.terminate(),
    };
}
