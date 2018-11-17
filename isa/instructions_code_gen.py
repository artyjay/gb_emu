import sys

## ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
## Helpers
## ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
FLAG_BEHAVIOUR_RESET 	= 0
FLAG_BEHAVIOUR_SET 		= 1
FLAG_BEHAVIOUR_IGNORED 	= 2
FLAG_BEHAVIOUR_PER_FN	= 3

## ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
## Instruction
## ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class Instruction():
	def __init__(self, opcode, extended_opcode, assembly, args, bytesize, cycles, flags):
		assert(len(flags) == 4)
		assert(len(cycles) > 0 and len(cycles) <= 2)

		self._opcode 			= opcode
		self._extended_opcode 	= extended_opcode
		self._assembly 			= assembly
		self._args 				= args
		self._bytesize 			= bytesize
		self._cycles 			= cycles
		self._flags 			= flags

	def _get_opcode_cpp(self):
		return "{0:>4}, {1:>4}".format(self._opcode, self._extended_opcode)

	def _get_byte_size_cpp(self):
		return str(self._bytesize)

	def _get_cycles_cpp(self):
		cpp_strs = ["0", "0"]

		for index,cycle in enumerate(self._cycles):
			cpp_strs[index] = str(cycle)
			
		return "{0:>2}, {1:>2}".format(cpp_strs[0], cpp_strs[1])

	def _get_flag_behaviour_cpp(self):
		cpp_strs = [ "", "", "", "" ]

		for index,flag in enumerate(self._flags):
			if flag == "0":
				cpp_strs[index] = "RFB::Reset"
			elif flag == "1":
				cpp_strs[index] = "RFB::Set"
			elif flag == "-":
				cpp_strs[index] = "RFB::Unmodified"
			else:
				cpp_strs[index] = "RFB::PerFunction"

			if index != 3:
				cpp_strs[index] = "{0},".format(cpp_strs[index])

		return "{0:17} {1:17} {2:17} {3:17}".format(cpp_strs[0], cpp_strs[1], cpp_strs[2], cpp_strs[3])

	def _get_args_cpp(self):
		cpp_strs = [ "RTD::None", "RTD::None" ]

		for index,arg in enumerate(self._args):
			if arg == "A":
				cpp_strs[index] = "RTD::A"
			elif arg == "B":
				cpp_strs[index] = "RTD::B"
			elif arg == "C":
				cpp_strs[index] = "RTD::C"
			elif arg == "D":
				cpp_strs[index] = "RTD::D"
			elif arg == "E":
				cpp_strs[index] = "RTD::E"
			elif arg == "F":
				cpp_strs[index] = "RTD::F"
			elif arg == "H":
				cpp_strs[index] = "RTD::H"
			elif arg == "L":
				cpp_strs[index] = "RTD::L"

			elif arg == "AF":
				cpp_strs[index] = "RTD::AF"
			elif arg == "BC":
				cpp_strs[index] = "RTD::BC"
			elif arg == "DE":
				cpp_strs[index] = "RTD::DE"
			elif arg == "HL":
				cpp_strs[index] = "RTD::HL"

			elif arg == "SP":
				cpp_strs[index] = "RTD::SP"
			elif arg == "PC":
				cpp_strs[index] = "RTD::PC"

			elif arg == "d8":
				cpp_strs[index] = "RTD::Imm8"
			elif arg == "r8":
				cpp_strs[index] = "RTD::SImm8"
			elif arg == "d16":
				cpp_strs[index] = "RTD::Imm16"
			elif arg == "a8":
				cpp_strs[index] = "RTD::Addr8"
			elif arg == "a16":
				cpp_strs[index] = "RTD::Addr16"

			elif arg == "(C)":
				cpp_strs[index] = "RTD::AddrC"
			elif arg == "(BC)":
				cpp_strs[index] = "RTD::AddrBC"
			elif arg == "(DE)":
				cpp_strs[index] = "RTD::AddrDE"
			elif arg == "(HL)":
				cpp_strs[index] = "RTD::AddrHL"


			elif arg == "Z":
				cpp_strs[index] = "RTD::FlagZ"
			elif arg == "NZ":
				cpp_strs[index] = "RTD::FlagNZ"
			elif arg == "CF":
				cpp_strs[index] = "RTD::FlagC"
			elif arg == "NC":
				cpp_strs[index] = "RTD::FlagNC"

			elif arg == "0":
				cpp_strs[index] = "RTD::Zero"
			elif arg == "1":
				cpp_strs[index] = "RTD::One"
			elif arg == "2":
				cpp_strs[index] = "RTD::Two"
			elif arg == "3":
				cpp_strs[index] = "RTD::Three"
			elif arg == "4":
				cpp_strs[index] = "RTD::Four"
			elif arg == "5":
				cpp_strs[index] = "RTD::Five"
			elif arg == "6":
				cpp_strs[index] = "RTD::Six"
			elif arg == "7":
				cpp_strs[index] = "RTD::Seven"
			elif arg == "8":
				cpp_strs[index] = "RTD::Eight"
			elif arg == "9":
				cpp_strs[index] = "RTD::Nine"

			elif arg == "00H" or arg == "08H" or arg == "10H" or arg == "18H" or arg == "20H" or arg == "28H" or arg == "30H" or arg == "38H" or arg == "CB":
				cpp_strs[index] = "RTD::None"

			else:
				print "ERROR: Unknown register arg type: {0}".format(arg)
				sys.exit(0)

		cpp_strs[0] = "{0},".format(cpp_strs[0])

		return "{0:12} {1:11}".format(cpp_strs[0], cpp_strs[1])

				#d8  means immediate 8 bit data
		#d16 means immediate 16 bit data
		#a8  means 8 bit unsigned data, which are added to $FF00 in certain instructions (replacement for missing IN and OUT instructions)
		#a16 means 16 bit address
		#r8  means 8 bit signed data, which are added to program counter

	def _get_assembly_cpp(self):
		if len(self._args) > 0:
			return "{0} {1}".format(self._assembly, ",".join(self._args))

		return self._assembly

	def get_cpp_str(self):
		return "({0}, {1}, {2}, {3}, {4}, \"{5}\")".format(self._get_opcode_cpp(), self._get_byte_size_cpp(), self._get_cycles_cpp(), self._get_flag_behaviour_cpp(), self._get_args_cpp(), self._get_assembly_cpp())


## ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
## Generator
## ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class Generator():
	def __init__(self, input_filename, output_filename, is_extended_opcodes):
		self._input_filename = input_filename
		self._output_filename = output_filename
		self._is_extended_opcodes = is_extended_opcodes
		self._instructions = []

		self._parse_input()
		self._write_output()

	def _parse_input(self):

		print "Parsing input: {0}".format(self._input_filename)

		with open(self._input_filename, "r") as input_file:
			linec = 0

			for line in input_file:
				line_split = line.split()

				## Flags
				flags = line_split[-4:]
				line_split = line_split[:-4]

				## Cycles
				cycles = map(int, line_split[-1:][0].split("/"))
				line_split = line_split[:-1]

				## Bytesize
				bytesize = int(line_split[-1:][0])
				line_split = line_split[:-1]

				## Instruction
				instruction_str = line_split[0]
				line_split = line_split[1:]

				## Args
				if len(line_split) == 1:
					args = line_split[0].split(",")
				elif len(line_split) != 0:
					print "ERROR, should be 1 or 0 splits remaining"
					sys.exit(0)
				else:
					args = []

				if self._is_extended_opcodes:
					opcode = "0xCB"
					extended_opcode = str(hex((linec)))
				else:
					opcode = str(hex((linec)))
					extended_opcode = "0x0"

				instruction = Instruction(opcode, extended_opcode, instruction_str, args, bytesize, cycles, flags)

				self._instructions.append(instruction)			
				linec += 1

	def _write_output(self):

		print "Writing output: {0}".format(self._output_filename)
		
		with open(self._output_filename, "w") as output_file:

			output_file.write("#pragma once\n\n")

			output_file.write("#include \"cpu.h\"\n\n")

			output_file.write("namespace gbhw\n{\n")

			if self._is_extended_opcodes:
				output_file.write("\tinline void initialise_instructions_ext(Instruction* instructions)\n")
			else:
				output_file.write("\tinline void initialise_instructions(Instruction* instructions)\n")

			output_file.write("\t{\n")

			for index,instruction in enumerate(self._instructions):
				instruction_str = instruction.get_cpp_str()

				if self._is_extended_opcodes:
					opcode = instruction._extended_opcode
				else:
					opcode = instruction._opcode

				output_file.write("\t\tinstructions[{0:>4}].set{1};\n".format(opcode, instruction_str))

			output_file.write("\t}\n} // gbhw")
	
Generator("instructions.txt", "../src/hardware/private/instructions.h", False);
Generator("instructions_extended.txt", "../src/hardware/private/instructions_extended.h", True);