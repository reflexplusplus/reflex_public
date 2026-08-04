let gAudioContext = null;

let libReflex = 
{
	jsConsoleLog: function(string) 
	{
		console.log(UTF8ToString(string));
	},
	
	jsConsoleError: function(string) 
	{
		console.error(UTF8ToString(string));
	},

	jsInitSystem: function() 
	{
		console.log("jsInitSystem");

		try
		{
			const reflexOnUnload = Module._reflexOnUnload;

			if (typeof reflexOnUnload !== 'function') throw("reflexOnUnload");

			window.addEventListener('unload', function () 
			{
                reflexOnUnload();
            });


			const reflexOnAnimationClock = Module._reflexOnAnimationClock;

			if (typeof reflexOnAnimationClock !== 'function') throw("reflexOnAnimationClock");

			function frameLoop() 
			{
				reflexOnAnimationClock();
				
				window.reflexAnimationFrameID = requestAnimationFrame(frameLoop);
			}
					
			window.reflexAnimationFrameID = requestAnimationFrame(frameLoop);


			//const reflexProcessAudio= Module._reflexProcessAudio;

			//if (typeof reflexProcessAudio !== 'function') throw("reflexProcessAudio");
		}
		catch(error)
		{
			console.error("jsInitSystem undefined: ", error);
		}
	},

	jsGetPerformanceTimer: function()
	{
		return performance.now() * 0.001;
	},

	jsDeinitSystem: function() 
	{
		console.log("jsDeinitSystem");

		cancelAnimationFrame(window.reflexAnimationFrameID);
	},

	jsDeleteLocalStorage: function(keyPtr)
	{
		const key = UTF8ToString(keyPtr);
		
		localStorage.removeItem(key);
	},

	jsWriteLocalStorage: function(keyPtr, dataPtr, length)
	{
		const key = UTF8ToString(keyPtr);

		const data = new Uint8Array(Module.HEAPU8.buffer, dataPtr, length);	

		localStorage.setItem(key, btoa(String.fromCharCode(...data)));
	},

	jsQueryLocalStorage: function(keyPtr)
	{
		const key = UTF8ToString(keyPtr);
		
		const hex = localStorage.getItem(key);

		return hex ? 1 : 0;
	},
	
	jsReadLocalStorage: function(keyPtr, outPtrToPtr, outLenPtr)
	{
		const key = UTF8ToString(keyPtr);

		const base64 = localStorage.getItem(key);

		let len = 0;

		let outPtr = 0;
		
		if (base64)
		{
			const binaryString = atob(base64);

			len = binaryString.length;

			outPtr = _malloc(len);

			const region = new Uint8Array(Module.HEAPU8.buffer, outPtr, len);

			for (let i = 0; i < len; ++i) region[i] = binaryString.charCodeAt(i);
		}

		setValue(outPtrToPtr, outPtr, '*');
		setValue(outLenPtr, len, 'i32');
	},
	
	jsStartWebAudio: async function()
	{
		console.log("jsStartWebAudio");

		let module = Module;

		if (typeof module === 'undefined') throw("no module");

		const reflexProcessAudio= Module._reflexProcessAudio;

		if (typeof reflexProcessAudio !== 'function') throw("reflexProcessAudio");

		gAudioContext = new AudioContext();

		await gAudioContext.audioWorklet.addModule("reflex_audioworklet.js?v2");

		// Create the custom processor and pass the WASM module

		const processorOptions = 
		{
			module: Module,
			callback: reflexProcessAudio // Pass the `Module` to the AudioWorkletProcessor
		};

		const node = new AudioWorkletNode(gAudioContext, "ReflexAudioWorklet", 
		{
			processorOptions: processorOptions // Pass options to the AudioWorklet
		});
	
		node.connect(gAudioContext.destination);
	},

	jsStopWebAudio: function()
	{
		if (gAudioContext) 
		{
			try 
			{
				gAudioContext.close().then(() => 
				{
					console.log("WebAudio stopped.");
				}).catch(error => 
				{
					console.error("Error closing AudioContext:", error);
				});
	
				gAudioContext = null;
			} 
			catch (error) 
			{
				console.error("Error stopping WebAudio:", error);
			}
		}

		gAudioContext = null;
	},

	jsWebAudioGetSampleRate: function()
	{
		return gAudioContext.sampleRate;
	},

	jsWebAudioGetCurrentBufferSize: function()
	{
		//AudioWorkletProcessor is always 128
		
		return 128;
	}
};

addToLibrary(libReflex);