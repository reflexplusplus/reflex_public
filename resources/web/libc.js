// Minimal WASI ABI imports used by the compiled C/C++ support libraries.
// Reflex file/network services use their own System bridge, not these calls.
export function createLibcImports(memory, write = (fd, text) => (fd === 2 ? console.error : console.log)(text), timeOrigin = performance.timeOrigin) {
    const data = () => new DataView(memory.buffer);
    const decoder = new TextDecoder();
    const sleeper = new Int32Array(new SharedArrayBuffer(4));
    const timeOffset = performance.timeOrigin - timeOrigin;
    const monotonicTime = () => timeOffset + performance.now();
    return {
        environ_sizes_get: (count, bytes) => { data().setUint32(count, 0, true); data().setUint32(bytes, 0, true); return 0; },
        environ_get: () => 0,
        args_sizes_get: (count, bytes) => { data().setUint32(count, 0, true); data().setUint32(bytes, 0, true); return 0; },
        args_get: () => 0,
        clock_time_get: (clock, precision, result) => {
            if (clock !== 0 && clock !== 1) return 28;
            const milliseconds = clock === 0 ? Date.now() : monotonicTime();
            data().setBigUint64(result, BigInt(Math.floor(milliseconds * 1000000)), true);
            return 0;
        },
        clock_res_get: (clock, result) => { data().setBigUint64(result, 1000000n, true); return clock <= 1 ? 0 : 28; },
        random_get: (pointer, length) => {
            for (let offset = 0; offset < length; offset += 65536)
                crypto.getRandomValues(new Uint8Array(memory.buffer, pointer + offset, Math.min(65536, length - offset)));
            return 0;
        },
        fd_write: (fd, vectors, count, written) => {
            if (fd !== 1 && fd !== 2) return 8;
            let total = 0;
            for (let i = 0; i < count; ++i) {
                const pointer = data().getUint32(vectors + i * 8, true);
                const length = data().getUint32(vectors + i * 8 + 4, true);
                write(fd, decoder.decode(new Uint8Array(memory.buffer, pointer, length).slice()));
                total += length;
            }
            data().setUint32(written, total, true);
            return 0;
        },
        fd_fdstat_get: (fd, pointer) => {
            if (fd < 0 || fd > 2) return 8;
            new Uint8Array(memory.buffer, pointer, 24).fill(0);
            data().setUint8(pointer, 2); // character device
            data().setBigUint64(pointer + 8, fd === 0 ? 2n : 64n, true);
            return 0;
        },
        fd_close: fd => fd >= 0 && fd <= 2 ? 0 : 8,
        fd_seek: () => 70, // ESPIPE: console streams are not seekable.
        fd_read: () => 52, // ENOSYS: no implicit stdin filesystem.
        fd_prestat_get: () => 8,
        fd_prestat_dir_name: () => 8,
        sched_yield: () => 0,
        poll_oneoff: (subscriptions, events, count, returned) => {
            if (!count) return 28;
            const pending = [];
            for (let i = 0; i < count; ++i) {
                const p = subscriptions + i * 48;
                const type = data().getUint8(p + 8);
                const clock = type === 0 ? data().getUint32(p + 16, true) : -1;
                const timeout = type === 0 ? Number(data().getBigUint64(p + 24, true)) / 1e6 : 0;
                const absolute = type === 0 && (data().getUint16(p + 40, true) & 1);
                const now = clock === 0 ? Date.now() : monotonicTime();
                pending.push({ userdata: data().getBigUint64(p, true), type,
                    error: type !== 0 ? 52 : clock > 1 ? 28 : 0,
                    deadline: performance.now() + (absolute ? Math.max(0, timeout - now) : timeout) });
            }
            const deadline = Math.min(...pending.map(item => item.error ? performance.now() : item.deadline));
            while (performance.now() < deadline) Atomics.wait(sleeper, 0, 0, deadline - performance.now());
            let n = 0;
            for (const item of pending) {
                if (!item.error && item.deadline > performance.now()) continue;
                const p = events + n++ * 32;
                new Uint8Array(memory.buffer, p, 32).fill(0);
                data().setBigUint64(p, item.userdata, true);
                data().setUint16(p + 8, item.error, true);
                data().setUint8(p + 10, item.type);
            }
            data().setUint32(returned, n, true);
            return 0;
        },
        proc_exit: code => { throw new Error(`C runtime exited with status ${code}`); },
    };
}
