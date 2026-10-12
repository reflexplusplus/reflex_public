// Page-side loader. Reflex application execution takes place in the worker.
const PAGE_BYTES = 65536;
import { createThreadManager } from "./threads.js";
import { attachInput } from "./input.js";
import { createIOManager } from "./io.js";

function workerError(message) {
    const error = new Error(message);
    // Preserve the worker's Wasm stack instead of the page-side wrapping stack.
    error.stack = message;
    return error;
}

function pages(bytes, name) {
    if (!Number.isSafeInteger(bytes) || bytes <= 0 || bytes % PAGE_BYTES !== 0)
        throw new RangeError(`${name} must be a positive multiple of 64 KiB`);
    return bytes / PAGE_BYTES;
}

function readSystemID() {
    const key = "reflex.systemID";
    try {
        const stored = localStorage.getItem(key);
        if (/^[0-9a-f]{16}$/i.test(stored || "") && BigInt(`0x${stored}`) !== 0n)
            return BigInt(`0x${stored}`);
    } catch { /* Storage may be unavailable; retain a process-local identity. */ }
    const words = crypto.getRandomValues(new Uint32Array(2));
    const id = ((BigInt(words[0]) << 32n) | BigInt(words[1])) || 1n;
    try { localStorage.setItem(key, id.toString(16).padStart(16, "0")); }
    catch { console.warn("Reflex system ID cannot persist because browser storage is unavailable"); }
    return id;
}

async function downloadModule(response, onProgress) {
    if (!onProgress) return response.arrayBuffer();
    if (!response.body) {
        onProgress({ loaded: 0, total: null });
        const bytes = await response.arrayBuffer();
        onProgress({ loaded: bytes.byteLength, total: bytes.byteLength, done: true });
        return bytes;
    }
    const length = Number(response.headers.get("Content-Length"));
    // Compressed transfer sizes do not describe the decoded stream's size.
    const decodedLength = Number(response.headers.get("X-Uncompressed-Content-Length"));
    const total = decodedLength > 0 ? decodedLength :
        length > 0 && !response.headers.get("Content-Encoding") ? length : null;
    const reader = response.body.getReader();
    const chunks = [];
    let loaded = 0;
    onProgress({ loaded, total });
    try {
        for (;;) {
            const { done, value } = await reader.read();
            if (done) break;
            chunks.push(value);
            loaded += value.byteLength;
            onProgress({ loaded, total });
        }
    } finally { reader.releaseLock(); }
    const bytes = new Uint8Array(loaded);
    let offset = 0;
    for (const chunk of chunks) { bytes.set(chunk, offset); offset += chunk.byteLength; }
    onProgress({ loaded, total: loaded, done: true });
    return bytes.buffer;
}

export async function startReflex({ moduleUrl, canvas, initialBytes, maximumBytes, onError = console.error, onProgress }) {
    if (!globalThis.crossOriginIsolated)
        throw new Error("Reflex shared memory requires cross-origin isolation (COOP/COEP)");
    const initial = pages(initialBytes, "initialBytes");
    const maximum = pages(maximumBytes, "maximumBytes");
    if (initial > maximum) throw new RangeError("Initial memory exceeds maximum memory");
    if (!canvas || typeof canvas.transferControlToOffscreen !== "function")
        throw new Error("Reflex requires a canvas with OffscreenCanvas support");
    const response = await fetch(moduleUrl);
    if (!response.ok) throw new Error(`Wasm download failed: ${response.status}`);
    const module = await WebAssembly.compile(await downloadModule(response, onProgress));
    const audioSupported = WebAssembly.Module.exports(module).some(item => item.name === "reflexConfigureAudio");
    const memory = new WebAssembly.Memory({ initial, maximum, shared: true });
    const timeOrigin = performance.timeOrigin + performance.now();
    const worker = new Worker(new URL("./application-worker.js", import.meta.url), { type: "module" });
    // Runtime-owned control also supports custom/older project scaffolds.
    const windowSelector = document.createElement("select");
    windowSelector.setAttribute("aria-label", "Window");
    windowSelector.hidden = true;
    const header = document.querySelector("header");
    if (header) header.prepend(windowSelector);
    else canvas.before(windowSelector);
    windowSelector.onchange = () => {
        worker.postMessage({ type: "select-window", id: Number(windowSelector.value) });
        canvas.focus();
    };
    const updateWindow = data => {
        let option = Array.from(windowSelector.options).find(item => Number(item.value) === data.id);
        if (!data.alive) { option?.remove(); }
        else {
            if (!option) { option = document.createElement("option"); option.value = String(data.id); windowSelector.append(option); }
            option.textContent = data.title || `Window ${data.id}`;
            if (data.selected) windowSelector.value = String(data.id);
        }
        windowSelector.hidden = windowSelector.options.length < 2;
    };
    const io = createIOManager(memory, onError);
    const systemID = readSystemID();
    const threads = createThreadManager(module, memory, onError, event => io.handle(event), systemID, timeOrigin);
    let detachInput = () => {};
    let detachDensity = () => {};
    let detachFit = () => {};
    // Keep Reflex's integer density and compensate CSS sizing so each backing
    // pixel maps to one physical display pixel, including fractional browser DPR.
    const readPixelRatio = () => window.devicePixelRatio || 1;
    let pixelRatio = readPixelRatio();
    let density = Math.max(1, Math.ceil(pixelRatio));
    let logicalSize;
    let refreshFit = () => {};
    const cssScale = () => density / pixelRatio;
    const setSize = ({ width, height }) => {
        logicalSize = { width, height };
        canvas.style.width = `${width * cssScale()}px`;
        canvas.style.height = `${height * cssScale()}px`;
        canvas.dataset.logicalWidth = width;
        canvas.dataset.logicalHeight = height;
    };
    let verification;
    let threadBenchmark;
    let audioConfiguration;
    let audioContext, audioNode;
    try {
        const offscreen = canvas.transferControlToOffscreen();
        await new Promise((resolve, reject) => {
            const fail = message => {
                detachInput();
                threads.stop();
                io.stop();
                worker.terminate();
                reject(workerError(message));
            };
            worker.onerror = event => fail(event.message || "Reflex worker failed");
            worker.onmessage = event => {
                if (threads.handle(event) || io.handle(event)) return;
                const { data } = event;
                if (data.type === "size") setSize(data);
                if (data.type === "window") updateWindow(data);
                if (data.type === "title") document.title = data.text;
                if (data.type === "cursor") canvas.style.cursor = data.cursor;
                else if (data.type === "ready") resolve();
                else if (data.type === "error") fail(data.message);
            };
            worker.postMessage({ type: "start", module, memory, canvas: offscreen, density, systemID, timeOrigin }, [offscreen]);
        });
        const reportError = error => {
            if (threadBenchmark) { threadBenchmark.reject(error); threadBenchmark = undefined; }
            if (verification) { verification.reject(error); verification = undefined; }
            if (audioConfiguration) { audioConfiguration.reject(error); audioConfiguration = undefined; }
            onError(error);
        };
        worker.onerror = event => reportError(new Error(event.message || "Reflex worker failed"));
        worker.onmessage = event => {
            if (threads.handle(event) || io.handle(event)) return;
            const { data } = event;
            if (data.type === "threads-benchmarked" && threadBenchmark) { threadBenchmark.resolve(data.samples); threadBenchmark = undefined; }
            if (data.type === "size") setSize(data);
            if (data.type === "window") updateWindow(data);
            if (data.type === "title") document.title = data.text;
            if (data.type === "cursor") canvas.style.cursor = data.cursor;
            if (data.type === "error") reportError(workerError(data.message));
            if (data.type === "audio-configured" && audioConfiguration) { audioConfiguration.resolve(data.pointer); audioConfiguration = undefined; }
            if (data.type === "verified" && verification) { verification.resolve(data.result); verification = undefined; }
        };
        detachInput = attachInput(canvas, worker);
        let media;
        const watchDensity = () => {
            if (media) media.removeEventListener("change", updateDensity);
            media = matchMedia(`(resolution: ${window.devicePixelRatio || 1}dppx)`);
            media.addEventListener("change", updateDensity);
        };
        const updateDensity = () => {
            pixelRatio = readPixelRatio();
            const next = Math.max(1, Math.ceil(pixelRatio));
            if (next !== density) { density = next; worker.postMessage({ type: "density", density }); }
            if (logicalSize) setSize(logicalSize);
            refreshFit();
            watchDensity();
        };
        watchDensity();
        window.addEventListener("resize", updateDensity);
        detachDensity = () => { media.removeEventListener("change", updateDensity); window.removeEventListener("resize", updateDensity); };
        updateDensity();
        let fit = false, fitElement, lastFitSize = "";
        const updateFit = () => {
            const rect = canvas.getBoundingClientRect();
            const width = Math.max(1, Math.floor((fitElement ? fitElement.clientWidth : window.innerWidth - rect.left) / cssScale()));
            const height = Math.max(1, Math.floor((fitElement ? fitElement.clientHeight : window.innerHeight - rect.top) / cssScale()));
            const signature = `${fit}:${width}:${height}`;
            if (signature === lastFitSize) return;
            lastFitSize = signature;
            worker.postMessage({ type: "fit", fit, width, height });
        };
        const resizeFit = () => { if (fit) updateFit(); };
        refreshFit = resizeFit;
        window.addEventListener("resize", resizeFit);
        // Header/status changes can alter the available height independently
        // of browser resize. Deduplication prevents canvas-size feedback loops.
        const observer = new ResizeObserver(() => { if (fit) updateFit(); });
        observer.observe(canvas.parentElement);
        detachFit = () => { window.removeEventListener("resize", resizeFit); observer.disconnect(); };
        return {
            memory,
            benchmarkThreads: (count = 20) => {
                if (!WebAssembly.Module.exports(module).some(item => item.name === "reflexBenchmarkThreads"))
                    return Promise.reject(new Error("Thread benchmark requires the Test Project"));
                if (threadBenchmark) return Promise.reject(new Error("Thread benchmark already running"));
                if (!Number.isSafeInteger(count) || count < 1 || count > 100)
                    return Promise.reject(new RangeError("Thread benchmark count must be 1..100"));
                return new Promise((resolve, reject) => { threadBenchmark = { resolve, reject }; worker.postMessage({ type: "benchmark-threads", count }); });
            },
            audioSupported,
            setFit: (value, element) => { fit = Boolean(value); fitElement = element; observer.disconnect(); observer.observe(fitElement || canvas.parentElement); updateFit(); },
            runAudio: async (onState = () => {}, onStats = () => {}) => {
                if (!audioSupported) throw new Error("This Reflex application does not include audio output");
                // Invoke from the DOM click handler: resume before any await.
                if (!audioContext) audioContext = new AudioContext({ latencyHint: "interactive" });
                const resumed = audioContext.resume();
                audioContext.onstatechange = () => onState(audioContext.state);
                await resumed;
                if (!audioNode) {
                    await audioContext.audioWorklet.addModule(new URL("./audio-worklet.js", import.meta.url));
                    const pointer = await new Promise((resolve, reject) => {
                        audioConfiguration = { resolve, reject };
                        worker.postMessage({ type: "audio-configure", sampleRate: audioContext.sampleRate });
                    });
                    audioNode = new AudioWorkletNode(audioContext, "reflex-output", {
                        numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2],
                        processorOptions: { memory, pointer },
                    });
                    audioNode.port.onmessage = event => onStats(event.data);
                    audioNode.onprocessorerror = () => { onState("failed"); reportError(new Error("AudioWorklet processor failed")); };
                    audioNode.connect(audioContext.destination);
                }
                onState(audioContext.state);
            },
            pauseAudio: () => audioContext ? audioContext.suspend() : Promise.resolve(),
            verify: () => {
                if (verification) return Promise.reject(new Error("Verification already running"));
                return new Promise((resolve, reject) => { verification = { resolve, reject }; worker.postMessage({ type: "verify" }); });
            },
        };
    } catch (error) {
        windowSelector.remove();
        detachFit();
        detachDensity();
        detachInput();
        threads.stop();
        io.stop();
        worker.terminate();
        throw error;
    }
}
