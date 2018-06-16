from conans import ConanFile, CMake

class GBConan(ConanFile):
	settings			= "os", "compiler", "build_type", "arch"
	requires			= "sdl2/2.0.8@bincrafters/stable", "gtest/1.8.0@bincrafters/stable", "Qt/5.11@bincrafters/stable"
	generators			= "cmake"
	default_options		= "Qt:shared=True"

	def imports(self):
		self.copy("*.dll", dst="bin", src="bin")
		self.copy("*.so", dst="bin", src="bin")