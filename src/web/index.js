import Emulator from "./emulator.js";

let emulator = null;

let step = function(timestamp)
{
	emulator.step()
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
	console.log(InputButton);
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
	if(event.target === document.body)
	{
		var k = map_key(event.code);
		console.log("key: " + k);
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

function load_rom()
{
	var input, file, reader;

	if (typeof window.FileReader !== 'function')
	{
		bodyAppend("p", "The file API isn't supported on this browser yet.");
		return;
	}

	input = document.getElementById('rominput');

	if (!input.files)
	{
		// @todo: better error mechanism on the page than this.
		bodyAppend("p", "This browser doesn't seem to support the `files` property of file inputs.");
	}
	else if (!input.files[0])
	{
		bodyAppend("p", "Please select a file before clicking 'Load'");
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

function bodyAppend(tagName, innerHTML)
{
	var elm;

	elm = document.createElement(tagName);
	elm.innerHTML = innerHTML;
	document.body.appendChild(elm);
}