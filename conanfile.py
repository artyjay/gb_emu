from conans import ConanFile, CMake

class GBConan(ConanFile):
	settings			= "os", "compiler", "build_type", "arch"
	requires			= "glfw/3.2.1@bincrafters/stable", "gtest/1.8.0@bincrafters/stable"
	generators			= "cmake"

	def imports(self):
		self.copy("*.dll", dst="bin", src="bin")
		self.copy("*.so", dst="bin", src="bin")