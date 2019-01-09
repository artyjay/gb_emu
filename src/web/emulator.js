function createDataPointer(data)
{
	var nDataBytes = data.byteLength;
	var dataPtr = Module._malloc(nDataBytes);
	var dataHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, nDataBytes);
	dataHeap.set(new Uint8Array(data.buffer));
	return dataHeap;
};

function freeDataPointer(data)
{
	Module._free(data.byteOffset);
};

export default class Emulator
{
	constructor(options)
	{
		this._handle = null;
		this._canvas = null;
		this._romdata = null;
		this._screenw = 0;
		this._screenh = 0;
	}

	step(timestamp)
	{
		if(this._handle !== null)
		{
			var res = Module._gbhw_step(this._handle, 0);	// V-Sync stepping.

			if(res != 0)
			{
				console.log("Stepping produced error: " + res);
			}

			var screendata = Module._gbhw_get_screen_web(this._handle);
			var screen = new Uint8ClampedArray(Module.HEAPU8.buffer, screendata, this._screenw * this._screenh * 4);
			var imageData = this._ctx.getImageData(0, 0, this._screenw, this._screenh);
			imageData.data.set(screen);
			this._ctx.putImageData(imageData, 0, 0);
		}
	}

	load_rom(data)
	{
		if(this._handle !== null)
		{
			Module._gbhw_destroy(this._handle);
			this._handle = null;
		}

		if(this._romdata !== null)
		{
			freeDataPointer(this._romdata);
		}

		// Get data byte size, allocate memory on Emscripten heap, and get pointer
		this._romdata = createDataPointer(data);

		this._handle = Module._gbhw_create_web(this._romdata.byteOffset, this._romdata.length);
		// @todo: Check handle is valid.

		this._screenw = Module._gbhw_get_screen_resolution_width(this._handle);
		this._screenh = Module._gbhw_get_screen_resolution_height(this._handle);

		this._canvas = document.getElementById("screen");
		this._canvas.width = this._screenw;
		this._canvas.height = this._screenh;
		this._ctx = this._canvas.getContext("2d");
	}
}