// Web MIDI belongs to the page, while Reflex processing runs in workers.
export function createMIDIManager(worker, onError = console.warn) {
    let access, requesting = false, stopped = false, nextToken = 1;
    const ports = new Map();
    const report = error => onError(new Error(`Web MIDI: ${error.message || error}`));
    const publish = () => {
        if (stopped) return;
        worker.postMessage({ type: "midi-ports", ports: [...ports.values()]
            .filter(item => item.port.state === "connected")
            .map(item => ({ token: item.token, output: item.port.type === "output",
                name: item.port.name || item.port.manufacturer || "MIDI", enabled: item.desired })) });
    };
    const refresh = () => {
        if (stopped) return;
        for (const port of [...access.inputs.values(), ...access.outputs.values()]) {
            const key = `${port.type}:${port.id}`;
            let item = ports.get(key);
            if (!item) {
                item = { token: nextToken++, port, enabled: false, desired: false, serial: 0, operation: Promise.resolve() };
                ports.set(key, item);
            }
            item.port = port;
        }
        for (const item of ports.values()) {
            const port = item.port;
            if (port.state !== "connected") {
                const wasActive = item.enabled || item.desired;
                item.enabled = item.desired = false;
                ++item.serial;
                if (wasActive) port.close().catch(report);
                if (port.type === "input") port.onmidimessage = null;
            }
        }
        publish();
    };
    const requestAccess = async () => {
        if (stopped || access || requesting) return;
        if (typeof navigator.requestMIDIAccess !== "function") {
            report(new Error("MIDI access is unavailable in this browser context"));
            return;
        }
        requesting = true;
        try {
            access = await navigator.requestMIDIAccess({ sysex: false });
        } catch (error) {
            if (!stopped) report(error);
            return;
        } finally {
            requesting = false;
        }
        if (stopped) return;
        access.onstatechange = refresh;
        refresh();
    };
    const enable = data => {
        const item = [...ports.values()].find(item => item.token === data.token);
        if (!item || stopped) return;
        const serial = ++item.serial;
        item.desired = Boolean(data.enable);
        item.operation = item.operation.then(async () => {
            if (serial !== item.serial || stopped) return;
            item.enabled = false;
            const port = item.port;
            if (port.type === "input") port.onmidimessage = null;
            try {
                if (item.desired) {
                    await port.open();
                    if (stopped) { await port.close(); return; }
                    if (serial !== item.serial) return;
                    item.enabled = port.state === "connected";
                    if (item.enabled && port.type === "input") port.onmidimessage = event => {
                        const bytes = event.data;
                        // Reflex's event payload supports short MIDI messages only.
                        if (!item.enabled || !item.desired || bytes.length < 1 || bytes.length > 3 || bytes[0] === 0xf0) return;
                        let packed = 0;
                        for (let i = 0; i < bytes.length; ++i) packed |= bytes[i] << (8 * i);
                        worker.postMessage({ type: "midi-input", token: item.token, packed: packed >>> 0 });
                    };
                } else await port.close();
            } catch (error) {
                if (serial === item.serial) item.enabled = item.desired = false;
                report(error);
            }
            if (serial === item.serial) publish();
        });
        return item.operation;
    };
    const send = packed => {
        const status = packed & 255;
        // Web MIDI requires the actual message length, without zero padding.
        const length = status < 0x80 ? 0 : status < 0xf0 ?
            ((status & 0xe0) === 0xc0 ? 2 : 3) :
            status === 0xf1 || status === 0xf3 ? 2 : status === 0xf2 ? 3 :
            [0xf6, 0xf8, 0xfa, 0xfb, 0xfc, 0xfe, 0xff].includes(status) ? 1 : 0;
        if (!length) return;
        const bytes = Array.from({ length }, (_, i) => (packed >>> (8 * i)) & 255);
        for (const item of ports.values()) {
            if (!item.enabled || !item.desired || item.port.type !== "output" || item.port.state !== "connected") continue;
            try { item.port.send(bytes); } catch (error) { report(error); }
        }
    };
    return {
        handle: ({ data }) => {
            if (data.type === "midi-request") { void requestAccess(); return true; }
            if (data.type === "midi-enable") { void enable(data); return true; }
            if (data.type === "midi-output") { send(data.packed); return true; }
            return false;
        },
        stop: () => {
            stopped = true;
            if (access) access.onstatechange = null;
            for (const item of ports.values()) {
                ++item.serial;
                item.enabled = item.desired = false;
                if (item.port.type === "input") item.port.onmidimessage = null;
                item.port.close().catch(report);
            }
        },
    };
}
