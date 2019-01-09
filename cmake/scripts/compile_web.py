import argparse
import os
import subprocess
import sys

log_tag_width	= 20
log_tag_fmt		= "%-" + str(log_tag_width) + "s| %s"

def log_msg(tag, msg):
	if tag == None:
		out = msg
	else:
		out = log_tag_fmt % (tag, msg)
	print(out)

def execute_cmdline(cmd, working_dir):
	p = subprocess.Popen(cmd, shell=True, cwd=working_dir)
	res = p.wait()
	(out, err) = p.communicate()

	if (res != 0):
		log_msg("Error", "Cmd execution failed")
		log_msg(None, "")
		log_msg("Error - Cmd", cmd)
		log_msg("Error - Res", "%d" % (res))
		log_msg("Error - Out", out)
		log_msg("Error - Err", err)

	return (res, "Empty" if (out == None) or (len(out) == 0) else out.rstrip(), "Empty" if (err == None) or (len(err) == 0) else err.rstrip())

def main():
	argParser = argparse.ArgumentParser(description="Build the web based emulator using Emscripten via Ninja")
	argParser.add_argument("--build-debug",		default=False, action="store_true",		help="Specify building the debug binaries")

	args = argParser.parse_args()

	cur_path			= os.getcwd()
	source_path			= os.path.abspath(os.path.join(os.path.dirname(__file__), "../../"))
	source_path_rel		= os.path.relpath(source_path, cur_path)
	toolchain_path		= os.path.abspath(os.path.join(source_path, "cmake/toolchains/emscripten/emscripten.cmake"))
	cmake_cache_path	= os.path.abspath(os.path.join(cur_path, "CMakeCache.txt"))

	build_debug			= args.build_debug

	log_msg(None, "")
	log_msg("Source", source_path)
	log_msg("Relative", source_path_rel)
	log_msg("Debug", build_debug)
	log_msg("Toolchain", toolchain_path)
	log_msg("CMake cache", cmake_cache_path)
	log_msg(None, "")

	if not os.path.exists(toolchain_path):
		log_msg("Error", "CMake toolchain could not be found")
		return -1

	if os.path.exists(cmake_cache_path):
		log_msg("Cleaning", "Deleting current cmake cache file")
		os.remove(cmake_cache_path)

	generate_cmdline = "cmake -DCMAKE_TOOLCHAIN_FILE=\"{0}\" -DCMAKE_BUILD_TYPE={1} -DEMSCRIPTEN_GENERATE_BITCODE_STATIC_LIBRARIES=ON -GNinja {2}".format(toolchain_path, "Debug" if build_debug else "Release", source_path_rel)
	log_msg("CMake command", generate_cmdline)
	res = execute_cmdline(generate_cmdline, cur_path)

	if res[0] != 0:
		log_msg("Error", "Failed to execute CMake command")
		return res[0]

	build_cmdline = "cmake --build ."
	log_msg("Build command", build_cmdline)
	res = execute_cmdline(build_cmdline, cur_path)

	if res[0] != 0:
		log_msg("Error", "Failed to build")

	return res[0]

if __name__ == "__main__":
	sys.exit(main())