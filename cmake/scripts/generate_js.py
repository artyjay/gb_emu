#!/usr/bin/python
import argparse
import os
import sys

log_tag_width	= 20
log_tag_fmt		= "%-" + str(log_tag_width) + "s| %s"

def log_msg(tag, msg):
	if tag == None:
		out = msg
	else:
		out = log_tag_fmt % (tag, msg)
	print(out)

def compile(input_file, output_file, functions):
	## Bootstap emscripten, either user has explicitly declared
	## emscripten they want to use through env-var. If not let's
	## use the generated emscripten config.
	EMSCRIPTEN_ROOT = os.environ.get("EMSCRIPTEN_ROOT")
	if not EMSCRIPTEN_ROOT:
		exec(open(os.path.expanduser('~/.emscripten'), 'r').read())
	sys.path.append(EMSCRIPTEN_ROOT)
	import tools.shared as emscripten

	exported_functions = "EXPORTED_FUNCTIONS=[%s]" % (", ".join("\"%s\"" % t for t in functions))

	print("exported functions: {}".format(exported_functions))

	emcc_args = [
		'-s', exported_functions,
		'--memory-init-file', '0',
		'--llvm-opts', '3',
		'--llvm-lto', '3',
		'-O3',
		'-s', 'NO_EXIT_RUNTIME=1',
		'-s', 'NO_FILESYSTEM=0',
		'-s', 'USE_PTHREADS=0',
		'-s', 'DEMANGLE_SUPPORT=1',
		'-s', 'ASSERTIONS=1',
		'-s', 'DOUBLE_MODE=0',
		'-s', 'PRECISE_I64_MATH=0',
		'-s', 'SIMD=0',
		'-s', 'AGGRESSIVE_VARIABLE_ELIMINATION=0',
		'-s', 'ALIASING_FUNCTION_POINTERS=1',
		'-s', 'DISABLE_EXCEPTION_CATCHING=1',
		'-s', 'WASM=0',
		'-s', 'TOTAL_MEMORY=16777216'
		]

	print("emcc args: {}".format(emcc_args))

	print('emcc {} -> {}'.format(input_file, output_file))
	return emscripten.Building.emcc(input_file, emcc_args, output_file)

def main():
	argParser = argparse.ArgumentParser(description="Compile emscripten byte-code into raw javascript")
	argParser.add_argument("-i", "--input")
	argParser.add_argument("-o", "--output")
	argParser.add_argument("-f", "--functions")

	args = argParser.parse_args()

	input_file = args.input
	output_file = args.output
	functions = args.functions

	if len(functions) == 0:
		log_msg("Error", "No functions specified, must be a comma-delimited list of functions you want exposing")
		return -1

	functions = functions.split(";")

	log_msg(None, "")
	log_msg("Input file", input_file)
	log_msg("Output file", output_file)

	for f in functions:
		log_msg("Export function", f)

	log_msg(None, "")

	if not os.path.exists(input_file):
		log_msg("Error", "Input file does not exist: {0}".format(input_file))
		return -1

	if os.path.exists(output_file):
		log_msg("Cleaning", "Deleting current file: {0}".format(output_file))
		os.remove(output_file)

	return compile(input_file, output_file, functions)

if __name__ == "__main__":
	sys.exit(main())