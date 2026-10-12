import { createImports } from "./imports.js";
import { createGLImports } from "./gl.js";

let instance;
let started = false;
let stopped = false;
let assertionMessage;
let pendingWindow;
let pendingFit;
let pendingChanges = false;

function requireFunction(exports, name) {
    if (typeof exports[name] !== "function") throw new Error(`Missing Wasm export: ${name}`);
    return exports[name];
}

function reportFailure(error) {
    const description = String(error);
    const stack = String(error.stack || "");
    // Firefox stacks omit the error message; include it explicitly.
    const failure = stack.startsWith(description) ? stack : `${description}\n${stack}`;
    const message = assertionMessage ? `Assertion failed:\n${assertionMessage}\n\n${failure}` : failure;
    assertionMessage = undefined;
    self.postMessage({ type: "error", message });
}

self.onmessage = async ({ data }) => {
    try {
        if (data.type === "select-window") {
            // Apply the latest selection alongside its redraw, not between frames.
            pendingWindow = data.id;
            pendingChanges = true;
            return;
        }
        if (data.type === "benchmark-threads") {
            const count = data.count;
            if (!Number.isSafeInteger(count) || count < 1 || count > 100) throw new Error("Thread benchmark count must be 1..100");
            const pointer = requireFunction(instance.exports, "malloc")(count * 3 * 8);
            if (!pointer) throw new Error("Thread benchmark allocation failed");
            try {
                const result = requireFunction(instance.exports, "reflexBenchmarkThreads")(pointer, count);
                if (result) throw new Error(`Thread benchmark failed: ${result}`);
                const samples = Array.from(new Float64Array(self.reflexMemory.buffer, pointer, count * 3));
                self.postMessage({ type: "threads-benchmarked", samples });
            } finally { instance.exports.free(pointer); }
            return;
        }
        if (data.type === "audio-configure") {
            const pointer = requireFunction(instance.exports, "reflexConfigureAudio")(data.sampleRate);
            self.postMessage({ type: "audio-configured", pointer });
            return;
        }
        if (data.type === "verify") {
            const result = requireFunction(instance.exports, "reflexVerifySystem")(Number(self.location.port) || 80);
            self.postMessage({ type: "verified", result });
            return;
        }
        if (data.type === "fit") {
            // Resize bursts must not queue expensive redraws for obsolete sizes.
            pendingFit = data;
            pendingChanges = true;
            return;
        }
        if (data.type === "density") {
            requireFunction(instance.exports, "reflexSetPixelDensity")(data.density);
            return;
        }
        if (data.type === "input") {
            if (instance && typeof instance.exports.reflexInputEvent === "function")
                instance.exports.reflexInputEvent(data.kind, data.a, data.b, data.modifiers, data.repeat ? 1 : 0);
            return;
        }
        if (data.type !== "start") return;
        if (started) throw new Error("Reflex application worker was already started");
        started = true;
        const imports = createImports(data.memory, data.systemID, text => { assertionMessage = text; }, data.timeOrigin);
        imports.reflex_gl = createGLImports(data.memory, data.canvas, () => instance.exports);
        imports.reflex_host.setTitle = (p, n) => {
            const text = new TextDecoder().decode(new Uint8Array(data.memory.buffer, p, n).slice());
            self.postMessage({ type: "title", text });
        };
        // Order matches System::MouseCursor; invalid values fall back to arrow.
        const cursors = ["none", "default", "wait", "move", "ew-resize", "ns-resize",
            "nwse-resize", "nesw-resize", "pointer", "grabbing", "text", "not-allowed", "zoom-in"];
        imports.reflex_host.setCursor = value => {
            self.postMessage({ type: "cursor", cursor: cursors[value] ?? "default" });
        };
        let inFrame = false;
        let pendingSize;
        imports.reflex_host.resizeCanvas = (width, height, density) => {
            if (data.canvas.width !== width * density) data.canvas.width = width * density;
            if (data.canvas.height !== height * density) data.canvas.height = height * density;
            const size = { type: "size", width, height };
            if (inFrame) pendingSize = size;
            else self.postMessage(size);
        };
        imports.reflex_host.windowUpdate = (id, p, n, flags) => {
            const title = new TextDecoder().decode(new Uint8Array(data.memory.buffer, p, n).slice());
            self.postMessage({ type: "window", id, title, alive: Boolean(flags & 1), selected: Boolean(flags & 2) });
        };
        instance = await WebAssembly.instantiate(data.module, imports);
        if (stopped) return;
        const exports = instance.exports;
        const initialize = requireFunction(exports, "reflexRuntimeInitialize");
        const start = requireFunction(exports, "reflexStartApplication");
        const clock = requireFunction(exports, "reflexOnAnimationClock");
        if (initialize() !== 1) throw new Error("Reflex runtime was already initialized");

        // Retain the canvas for the forthcoming browser Renderer backend.
        self.reflexCanvas = data.canvas;
        self.reflexMemory = data.memory;
        requireFunction(exports, "reflexSetPixelDensity")(data.density);
        start();
        if (typeof requestAnimationFrame !== "function") throw new Error("Worker animation frames are unavailable");
        const frame = () => {
            try {
                inFrame = true;
                if (pendingChanges) {
                    pendingChanges = false;
                    if (pendingWindow !== undefined) {
                        const id = pendingWindow;
                        pendingWindow = undefined;
                        requireFunction(exports, "reflexSelectWindow")(id);
                    }
                    if (pendingFit) {
                        const { fit, width, height } = pendingFit;
                        pendingFit = undefined;
                        requireFunction(exports, "reflexFitViewport")(fit ? 1 : 0, width, height);
                    }
                }
                clock();
                // Publish only the final dimensions after the new window has drawn.
                if (pendingSize) {
                    self.postMessage(pendingSize);
                    pendingSize = undefined;
                }
                if (!stopped) requestAnimationFrame(frame);
            } catch (error) {
                stopped = true;
                reportFailure(error);
            } finally {
                inFrame = false;
            }
        };
        requestAnimationFrame(frame);
        self.postMessage({ type: "ready" });
    } catch (error) {
        stopped = true;
        reportFailure(error);
    }
};
