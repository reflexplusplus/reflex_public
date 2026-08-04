class ReflexAudioWorklet extends AudioWorkletProcessor 
{
	constructor(options) 
	{
		super();

		this.wasm = options.processorOptions.module; // Get the WASM module
		this.reflexProcessAudio = options.processorOptions.callback;

		const bufferPtr = wasm._get_wasm_buffer();
		this.wasmBuffer = new Float32Array(wasm.memory.buffer, bufferPtr, 128); // 128 samples buffer
	}

	process(inputs, outputs) 
	{
		if (!this.reflexProcessAudio || inputs.length === 0 || inputs[0].length === 0) return true;

		const input = inputs[0][0];  // First input channel
		const output = outputs[0][0]; // First output channel

		// Instead of copying, we just reference the same memory
		this.wasmBuffer.set(input);  // Map JS buffer to WASM memory
		
		this.reflexProcessAudio(this.wasmBuffer.byteOffset, this.wasmBuffer.length);
		
		output.set(this.wasmBuffer); // Use the processed buffer directly

		return true;
	}
}

registerProcessor("ReflexAudioWorklet", ReflexAudioWorklet);
