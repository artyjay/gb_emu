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

function key_down(event)
{
	console.log("Key down");

	if(event.code == "Space" && event.target === document.body)
	{
		// @todo: 
	}
}

function key_up(event)
{
	// @todo:
	console.log("Key up");
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