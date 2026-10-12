import { createImports } from "./imports.js";

let instance;
let assertionMessage;
self.onmessage = async ({ data }) => {
    try {
        if (data.type === "initialize") {
            const imports = createImports(data.memory, data.systemID, text => { assertionMessage = text; }, data.timeOrigin);
            // The module contains renderer imports even when this thread never
            // renders. Keep the System renderer confined to its owning worker.
            for (const item of WebAssembly.Module.imports(data.module)) {
                if (item.module !== "reflex_gl" && item.module !== "reflex_host") continue;
                imports[item.module] ||= {};
                if (!imports[item.module][item.name]) imports[item.module][item.name] = () => {
                    throw new Error(`${item.name} requires the Reflex application/rendering worker`);
                };
            }
            instance = await WebAssembly.instantiate(data.module, imports);
            if (typeof instance.exports.wasi_thread_start !== "function") throw new Error("Missing wasi_thread_start export");
            // No reactor initialization: process constructors run only once.
            self.postMessage({ type: "thread-ready" });
        } else if (data.type === "run") {
            instance.exports.wasi_thread_start(data.id, data.argument);
            self.postMessage({ type: "thread-done" });
            self.close();
        }
    } catch (error) {
        const description = String(error);
        const stack = String(error.stack || "");
        const failure = stack.startsWith(description) ? stack : `${description}\n${stack}`;
        const message = assertionMessage ? `Assertion failed:\n${assertionMessage}\n\n${failure}` : failure;
        assertionMessage = undefined;
        self.postMessage({ type: "error", message });
    }
};
