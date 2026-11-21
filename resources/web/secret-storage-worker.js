// Owns the IndexedDB connection and Web Crypto key for this runtime.
let memory;
const decoder = new TextDecoder();
const bytes = (pointer, size) => new Uint8Array(memory.buffer, pointer, size);
const text = (pointer, size) => decoder.decode(bytes(pointer, size).slice());
const encoder = new TextEncoder();
const maxSecretSize = 64 * 1024; // Matches SecretStorage::kMaxValueSize.
const secretTagSize = 16;
let secretStore;

function openSecretDatabase() {
    return new Promise((resolve, reject) => {
        const request = indexedDB.open("reflex.secret-storage", 1);
        let blocked = false;
        request.onupgradeneeded = () => {
            request.result.createObjectStore("keys");
            request.result.createObjectStore("entries");
        };
        request.onblocked = () => {
            blocked = true;
            reject(new Error("Secret storage database is blocked"));
        };
        request.onerror = () => reject(request.error);
        request.onsuccess = () => {
            if (blocked) request.result.close();
            else resolve(request.result);
        };
    });
}

function secretTransaction(db, name, mode, run) {
    return new Promise((resolve, reject) => {
        const transaction = db.transaction(name, mode, { durability: "strict" });
        let result;
        transaction.oncomplete = () => resolve(result);
        transaction.onabort = () => reject(transaction.error ?? new Error("Secret storage transaction aborted"));
        const request = run(transaction.objectStore(name));
        request.onsuccess = () => { result = request.result; };
    });
}

async function loadSecretKey(db) {
    const keyID = "aes-gcm-v1";
    const read = () => secretTransaction(db, "keys", "readonly", store => store.get(keyID));
    let key = await read();
    if (key === undefined) {
        const candidate = await crypto.subtle.generateKey({ name: "AES-GCM", length: 256 }, false, ["encrypt", "decrypt"]);
        try {
            await secretTransaction(db, "keys", "readwrite", store => store.add(candidate, keyID));
            key = candidate;
        } catch (error) {
            //If another tab initialized the database first, use its committed key.
            if (error.name !== "ConstraintError") throw error;
            key = await read();
        }
    }
    if (!(key instanceof CryptoKey) || key.extractable || key.algorithm.name !== "AES-GCM" || key.algorithm.length !== 256)
        throw new Error("Invalid stored encryption key");
    return key;
}

function getSecretStore() {
    if (!secretStore) {
        const opening = openSecretDatabase().then(async db => {
            const close = () => {
                db.close();
                if (secretStore === opening) secretStore = undefined;
            };
            db.onversionchange = close;
            db.onclose = close;
            try { return { db, key: await loadSecretKey(db) }; }
            catch (error) { close(); throw error; }
        }).catch(error => {
            if (secretStore === opening) secretStore = undefined;
            throw error;
        });
        secretStore = opening;
    }
    return secretStore;
}

async function executeSecretRequest(op, pointer) {
    if (!globalThis.isSecureContext || !globalThis.crypto?.subtle || !globalThis.indexedDB) return -1;
    try {
        const { db, key } = await getSecretStore();
        if (op === 0) return 0;
        //wasm32 request: entry ID pointer and size, then data pointer and size.
        const fields = new Uint32Array(memory.buffer, pointer, 4);
        const id = text(fields[0], fields[1]);
        const buffer = fields[2], size = fields[3];
        const additionalData = encoder.encode(id);
        if (op === 1) {
            if (size > maxSecretSize) return -3;
            const iv = crypto.getRandomValues(new Uint8Array(12));
            const plaintext = bytes(buffer, size).slice();
            let ciphertext;
            try {
                ciphertext = await crypto.subtle.encrypt({ name: "AES-GCM", iv, additionalData }, key, plaintext);
            } finally { plaintext.fill(0); }
            await secretTransaction(db, "entries", "readwrite", store => store.put({ iv, ciphertext }, id));
            return 0;
        }
        if (op === 2) {
            const record = await secretTransaction(db, "entries", "readonly", store => store.get(id));
            if (record === undefined) return -2;
            if (!(record?.iv instanceof Uint8Array) || record.iv.length !== 12 || !(record.ciphertext instanceof ArrayBuffer)) return -1;
            if (record.ciphertext.byteLength > Math.min(size, maxSecretSize) + secretTagSize) return -3;
            const plaintext = new Uint8Array(await crypto.subtle.decrypt({ name: "AES-GCM", iv: record.iv, additionalData }, key, record.ciphertext));
            try {
                bytes(buffer, plaintext.length).set(plaintext);
                return plaintext.length;
            } finally { plaintext.fill(0); }
        }
        if (op !== 3) return -1;
        const removed = await secretTransaction(db, "entries", "readwrite", store => {
            const request = store.getKey(id);
            request.addEventListener("success", () => { if (request.result !== undefined) store.delete(id); });
            return request;
        });
        return removed === undefined ? -2 : 0;
    } catch (error) {
        if (error?.name === "SecurityError" || error?.name === "NotAllowedError") return -4;
        return -1;
    }
}

async function handleRequest(port, request) {
    if (request.type === "shutdown") {
        if (secretStore) (await secretStore.catch(() => undefined))?.db.close();
        port.close();
        self.close();
        return;
    }
    const result = await executeSecretRequest(request.operation, request.pointer);
    port.postMessage({ id: request.id, result });
}

self.onmessage = ({ data }) => {
    memory = data.memory;
    const port = data.port;
    const reportFailure = () => self.postMessage({ type: "secret-error" });
    port.onmessage = ({ data: request }) => handleRequest(port, request).catch(reportFailure);
    port.onmessageerror = reportFailure;
};
