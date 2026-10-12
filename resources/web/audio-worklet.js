// Browser render callback: no waits, locks, allocation or Wasm initialization.
// Ring layout must match System::Web::AudioApp in instance/audioapp.cpp.
const BLOCK_FRAMES = 512;
const RING_BLOCKS = 4;
class ReflexOutput extends AudioWorkletProcessor {
    constructor(options) {
        super();
        this.memory = options.processorOptions.memory;
        this.pointer = options.processorOptions.pointer;
        this.offset = 0;
        this.ticks = 0;
        this.peak = 0;
        this.refresh();
    }
    refresh() {
        this.buffer = this.memory.buffer;
        this.state = new Int32Array(this.buffer, this.pointer, 4);
        this.samples = new Float32Array(this.buffer, this.pointer + 16, RING_BLOCKS * 2 * BLOCK_FRAMES);
    }
    process(inputs, outputs) {
        if (this.buffer !== this.memory.buffer) this.refresh();
        const output = outputs[0];
        if (!output || !output.length) return true;
        let read = Atomics.load(this.state, 0) >>> 0;
        for (let frame = 0; frame < output[0].length; ++frame) {
            const write = Atomics.load(this.state, 1) >>> 0;
            if (read === write) {
                for (let channel = 0; channel < output.length; ++channel) output[channel][frame] = 0;
                if (frame === 0) Atomics.add(this.state, 2, 1);
                continue;
            }
            for (let channel = 0; channel < output.length; ++channel) {
                const value = channel < 2 ? this.samples[(read % RING_BLOCKS) * 2 * BLOCK_FRAMES + channel * BLOCK_FRAMES + this.offset] : 0;
                output[channel][frame] = value;
                this.peak = Math.max(this.peak, Math.abs(value));
            }
            if (++this.offset === BLOCK_FRAMES) {
                this.offset = 0;
                read = (read + 1) >>> 0;
                Atomics.store(this.state, 0, read);
                Atomics.add(this.state, 3, 1);
                Atomics.notify(this.state, 3);
            }
        }
        // Low-frequency diagnostics outside the inner sample loop.
        if (++this.ticks === 375) {
            this.port.postMessage({ peak: this.peak, blocks: read, underruns: Atomics.load(this.state, 2) });
            this.ticks = 0; this.peak = 0;
        }
        return true;
    }
}
registerProcessor("reflex-output", ReflexOutput);
