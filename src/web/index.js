let emulator = null;

let step = function(timestamp)
{
	if(emulator !== null)
	{
		emulator.step()
	}

	window.requestAnimationFrame(step);
}

window.onload = function()
{
	emulator = new Emulator;

	window.onkeydown	= key_down;
	window.onkeyup		= key_up;
	document.getElementById("romload").onclick = load_rom;

	// Enter main.
	window.requestAnimationFrame(step);
}

// ----------------------------------------------------------------------------
// Rom Loading
// ----------------------------------------------------------------------------

function load_rom()
{
	var input, file, reader;

	if (typeof window.FileReader !== 'function')
	{
		body_append("p", "The file API isn't supported on this browser yet.");
		return;
	}

	input = document.getElementById('rominput');

	if (!input.files)
	{
		// @todo: better error mechanism on the page than this.
		body_append("p", "This browser doesn't seem to support the `files` property of file inputs.");
	}
	else if (!input.files[0])
	{
		body_append("p", "Please select a file before clicking 'Load'");
	}
	else
	{
		file = input.files[0];
		reader = new FileReader();
		reader.onload = loaded_rom;
		reader.readAsArrayBuffer(file);
	}

	function loaded_rom()
	{
		let data = new Uint8Array(reader.result);
		emulator.load_rom(data);
	}
}

function body_append(tagName, innerHTML)
{
	var elm;

	elm = document.createElement(tagName);
	elm.innerHTML = innerHTML;
	document.body.appendChild(elm);
}

// ----------------------------------------------------------------------------
// Input handling.
// ----------------------------------------------------------------------------

var InputButton = Object.freeze(
{
	"a" : 0,
	"b" : 1,
	"select" : 2,
	"start" : 3,
	"dpad_right" : 4,
	"dpad_left" : 5,
	"dpad_up" : 6,
	"dpad_down" : 7
});

var ButtonState = Object.freeze(
{
	"pressed" : 0,
	"released" : 1
});

function map_key(code)
{

	switch(code)
	{
		case "ArrowUp": 	return InputButton.dpad_up;
		case "ArrowDown": 	return InputButton.dpad_down;
		case "ArrowLeft": 	return InputButton.dpad_left;
		case "ArrowRight": 	return InputButton.dpad_right;
		case "Enter": 		return InputButton.start;
		case "Backspace": 	return InputButton.select;
		case "Space": 		return InputButton.a;
		case "KeyB": 		return InputButton.b;
		default: break;
	}
	return InputButton.a;
}

function key_down(event)
{
	if(event.target === document.body && emulator !== null)
	{
		var k = map_key(event.code);
		emulator.modify_button_state(k, ButtonState.pressed);
	}
}

function key_up(event)
{
	if(event.target === document.body)
	{
		var k = map_key(event.code);
		emulator.modify_button_state(k, ButtonState.released);
	}
}

// ----------------------------------------------------------------------------
// Emulator class
// ----------------------------------------------------------------------------

class Emulator
{
	constructor(options)
	{
		this._handle = null;
		this._canvas = null;
		this._romdata = null;
		this._screenw = 0;
		this._screenh = 0;
		this._scaling = 4;
	}

	step(timestamp)
	{
		if(this._handle !== null)
		{
			// V-Sync stepping.
			if(Module._gbhw_step(this._handle, 0) != 0)
			{
				console.log("Stepping produced error: " + res);
			}

			// Load in screen to image data.
			var screendata	= Module._gbhw_get_screen_web(this._handle);
			var screen		= new Uint8ClampedArray(Module.HEAPU8.buffer, screendata, this._screenw * this._screenh * 4);
			var imagedata	= this._ctx.getImageData(0, 0, this._screenw, this._screenh);
			imagedata.data.set(screen);

			// Async load up image data into a bitmap that can be drawn scaled.
			// @todo: This causes a decent amount of buffering.
			var ctx			= this._ctx;
			var scaling		= this._scaling;
			createImageBitmap(imagedata).then(function(image)
			{
				ctx.setTransform(scaling, 0, 0, scaling, 0, 0);
				ctx.drawImage(image, 0, 0);
			});
		}
	}

	modify_button_state(button, state)
	{
		if(this._handle !== null)
		{
			Module._gbhw_set_button_state(this._handle, button, state);
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
			this.free_data(this._romdata);
		}

		this._romdata		= this.alloc_data(data);
		this._handle		= Module._gbhw_create_web(this._romdata.byteOffset, this._romdata.length);
		this._screenw		= Module._gbhw_get_screen_resolution_width(this._handle);
		this._screenh		= Module._gbhw_get_screen_resolution_height(this._handle);
		this._canvas		= document.getElementById("screen");
		this._canvas.width	= this._screenw * this._scaling;
		this._canvas.height	= this._screenh * this._scaling;
		this._ctx			= this._canvas.getContext("2d");
	}

	alloc_data(data)
	{
		var ptr = Module._malloc(data.byteLength);
		var heap = new Uint8Array(Module.HEAPU8.buffer, ptr, data.byteLength);
		heap.set(new Uint8Array(data.buffer));
		return heap;
	}

	free_data(data)
	{
		Module._free(data.byteOffset);
	}
}

// ----------------------------------------------------------------------------