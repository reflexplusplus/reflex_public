// Async browser APIs live here; the calling Reflex worker waits on the reply.
let memory;
let nextHandle = 1;
const files = new Map();
const directories = new Map();
const responses = new Map();
const decoder = new TextDecoder();
const text = (p, n) => decoder.decode(new Uint8Array(memory.buffer, p, n).slice());
const bytes = (p, n) => new Uint8Array(memory.buffer, p, n);
// Include the path for diagnosis without exposing URL credentials/query values.
const diagnosticURL = value => {
    const url = new URL(value);
    return url.origin + url.pathname + (url.search ? "?[query omitted]" : "");
};
async function parent(path, create = false) {
    const parts = path.split("/").filter(Boolean);
    if (parts.some(part => part === "." || part === "..")) throw new Error("Invalid OPFS path");
    let directory = await navigator.storage.getDirectory();
    const name = parts.pop();
    for (const part of parts) directory = await directory.getDirectoryHandle(part, { create });
    return { directory, name };
}
async function directory(path, create = false) {
    let result = await navigator.storage.getDirectory();
    for (const part of path.split("/").filter(Boolean)) {
        if (part === "." || part === "..") throw new Error("Invalid OPFS path");
        result = await result.getDirectoryHandle(part, { create });
    }
    return result;
}
async function execute(data) {
    const { operation: op, handle, pointer: p, length: n, offset } = data;
    const file = files.get(handle);
    const at = Number(offset);
    if (!Number.isSafeInteger(at) || at < 0) throw new Error("Invalid file offset");
    if (op === 20) {
        const fields = new Uint32Array(memory.buffer, p, 10);
        const method = text(fields[0], fields[1]);
        const url = text(fields[2], fields[3]);
        const headers = new Headers();
        for (const line of text(fields[4], fields[5]).split("\r\n")) {
            const colon = line.indexOf(":");
            if (colon > 0) headers.append(line.slice(0, colon), line.slice(colon + 1).trim());
        }
        const body = fields[7] ? bytes(fields[6], fields[7]).slice() : undefined;
        const connectTimeout = fields[8], readTimeout = fields[9];
        const controller = new AbortController();
        const timer = connectTimeout ? setTimeout(() => controller.abort(), connectTimeout) : undefined;
        try {
            self.postMessage({ type: "http-diagnostic", message: `${method} ${diagnosticURL(url)}` });
            const response = await fetch(url, { method, headers, body, signal: controller.signal });
            self.postMessage({ type: "http-diagnostic", message: `${method} ${diagnosticURL(url)} -> ${response.status}` +
                (response.redirected ? ` (redirected to ${diagnosticURL(response.url)})` : "") });
            const responseHeaders = new TextEncoder().encode(Array.from(response.headers, ([key, value]) => `${key}: ${value}\r\n`).join(""));
            const id = nextHandle++;
            responses.set(id, { status: response.status, headers: responseHeaders, reader: response.body?.getReader(), controller, readTimeout, remainder: new Uint8Array() });
            return id;
        } catch (error) {
            controller.abort();
            const reason = error.name === "AbortError" ? "connection timed out or was aborted" :
                `${error.message}; check server CORS headers, TLS and connectivity`;
            throw new Error(`${method} ${diagnosticURL(url)} failed: ${reason}`);
        }
        finally { clearTimeout(timer); }
    }
    if (op >= 21 && op <= 25) {
        const response = responses.get(handle);
        if (op === 21) return response.status;
        if (op === 23) return response.headers.length;
        if (op === 22) { const part = response.headers.subarray(at, at + n); bytes(p, part.length).set(part); return part.length; }
        if (op === 25) { response.controller.abort(); responses.delete(handle); return 0; }
        if (!n || !response.reader) return 0;
        if (!response.remainder.length) {
            let timer;
            try {
                const item = await (response.readTimeout ? Promise.race([
                    response.reader.read(),
                    new Promise((resolve, reject) => { timer = setTimeout(() => { response.controller.abort(); reject(new Error("HTTP read timeout")); }, response.readTimeout); }),
                ]) : response.reader.read());
                if (item.done) return 0;
                response.remainder = item.value;
            } finally { clearTimeout(timer); }
        }
        const size = Math.min(n, response.remainder.length);
        bytes(p, size).set(response.remainder.subarray(0, size));
        response.remainder = response.remainder.subarray(size);
        return size;
    }
    if (op === 0) {
        const { directory, name } = await parent(text(p, n));
        const entry = await directory.getFileHandle(name, { create: handle !== 0 });
        const access = await entry.createSyncAccessHandle({ mode: handle === 0 ? "read-only" : "readwrite" });
        if (handle === 1) access.truncate(0);
        const id = nextHandle++;
        files.set(id, { access, writable: handle !== 0 });
        return id;
    }
    if (op === 1) { try { if (file.writable) file.access.flush(); } finally { file.access.close(); files.delete(handle); } return 0; }
    if (op === 2) return file.access.getSize();
    if (op === 3 || op === 4) {
        if (op === 4 && !file.writable) return -1;
        let total = 0;
        while (total < n) {
            const size = Math.min(65536, n - total);
            const buffer = op === 3 ? new Uint8Array(size) : bytes(p + total, size).slice();
            const done = file.access[op === 3 ? "read" : "write"](buffer, { at: at + total });
            if (op === 3) bytes(p + total, done).set(buffer.subarray(0, done));
            total += done;
            if (done < size) break;
        }
        return total;
    }
    if (op === 5) { if (!file.writable) return -1; file.access.truncate(at); return 0; }
    if (op === 6) { file.access.flush(); return 0; }
    if (op === 7) { await directory(text(p, n), true); return 0; }
    if (op === 8 || op === 9) {
        try {
            const path = text(p, n);
            if (!path.split("/").filter(Boolean).length) return 1;
            const { directory, name } = await parent(path);
            if (op === 9) await directory.getDirectoryHandle(name);
            else { try { await directory.getFileHandle(name); } catch { await directory.getDirectoryHandle(name); } }
            return 1;
        } catch { return 0; }
    }
    if (op === 10) { const { directory, name } = await parent(text(p, n)); await directory.removeEntry(name); return 0; }
    if (op === 12 || op === 13) {
        const { directory, name } = await parent(text(p, n));
        const entry = await directory.getFileHandle(name);
        const value = await entry.getFile();
        return op === 12 ? value.size : Math.floor(value.lastModified / 1000);
    }
    if (op === 14) {
        const [from, to] = text(p, n).split("\0");
        const source = await parent(from);
        const target = await parent(to);
        const entry = await source.directory.getFileHandle(source.name);
        if (typeof entry.move !== "function") return -1;
        await entry.move(target.directory, target.name);
        return 0;
    }
    if (op === 15) {
        const entries = [];
        for await (const [name, entry] of (await directory(text(p, n))).entries())
            if (handle || !name.startsWith(".")) entries.push({ name, directory: entry.kind === "directory" });
        const id = nextHandle++;
        directories.set(id, entries[Symbol.iterator]());
        return id;
    }
    if (op === 16) {
        const item = directories.get(handle).next();
        if (item.done) return 0;
        const encoded = new TextEncoder().encode(item.value.name);
        if (encoded.length > n) return -1;
        bytes(p, encoded.length).set(encoded);
        return BigInt(encoded.length) | (item.value.directory ? 1n << 32n : 0n);
    }
    if (op === 17) { directories.delete(handle); return 0; }
    return -1;
}
self.onmessage = async ({ data }) => {
    if (data.type === "initialize") { memory = data.memory; return; }
    if (data.type !== "io") return;
    let result = -1n;
    try { result = BigInt(await execute(data)); }
    catch (error) {
        if (data.operation >= 20 && data.operation <= 25)
            self.postMessage({ type: "http-diagnostic", message: error.message });
        else console.debug("Reflex I/O:", error.message);
    }
    new DataView(data.reply).setBigInt64(8, result, true);
    const state = new Int32Array(data.reply);
    Atomics.store(state, 0, 1);
    Atomics.notify(state, 0);
    self.postMessage({ type: "io-complete", requestId: data.requestId });
};
