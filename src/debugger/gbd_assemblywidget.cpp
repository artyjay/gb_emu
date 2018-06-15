#include "gbd_assemblywidget.h"

namespace
{
	QString ArgValuesToString(const gbhw::Instruction& instruction, gbhw::Hardware* hardware)
	{
		QString res;

		gbhw::RTD::Type	argTypes[2] = { instruction.GetArgType(0), instruction.GetArgType(1) };
		uint32_t argCount = 0;

		if (argTypes[1] != gbhw::RTD::None)
		{
			argCount = 2;
		}
		else if (argTypes[0] != gbhw::RTD::None)
		{
			argCount = 1;
		}

		const auto& regs = hardware->GetCPU().GetRegisters();
		auto& mmu = hardware->GetCPU().GetMMU();

		QString argstr;
		gbhw::Address immoffset = 1;

		if (instruction.GetOpcode() == 0xCB)
		{
			immoffset = 2;
		}

		for (uint32_t argIndex = 0; argIndex < argCount; ++argIndex)
		{
			switch (argTypes[argIndex])
			{
				case gbhw::RTD::A: argstr = QString::asprintf("0x%02x", regs.a); break;
				case gbhw::RTD::B: argstr = QString::asprintf("0x%02x", regs.b); break;
				case gbhw::RTD::C: argstr = QString::asprintf("0x%02x", regs.c); break;
				case gbhw::RTD::D: argstr = QString::asprintf("0x%02x", regs.d); break;
				case gbhw::RTD::E: argstr = QString::asprintf("0x%02x", regs.e); break;
				case gbhw::RTD::F: argstr = QString::asprintf("0x%02x", regs.f); break;
				case gbhw::RTD::H: argstr = QString::asprintf("0x%02x", regs.h); break;
				case gbhw::RTD::L: argstr = QString::asprintf("0x%02x", regs.l); break;
				case gbhw::RTD::AF: argstr = QString::asprintf("0x%04x", regs.af); break;
				case gbhw::RTD::BC: argstr = QString::asprintf("0x%04x", regs.bc); break;
				case gbhw::RTD::DE: argstr = QString::asprintf("0x%04x", regs.de); break;
				case gbhw::RTD::HL: argstr = QString::asprintf("0x%04x", regs.hl); break;
				case gbhw::RTD::SP: argstr = QString::asprintf("0x%04x", regs.sp); break;
				case gbhw::RTD::PC: argstr = QString::asprintf("0x%04x", regs.pc); break;
				case gbhw::RTD::Imm8:
				{
					argstr = QString::asprintf("0x%02x", mmu.ReadByte(instruction.GetAddress() + immoffset));
					immoffset += 1;
					break;
				}
				case gbhw::RTD::SImm8:
				{
					gbhw::Byte byte = mmu.ReadByte(instruction.GetAddress() + immoffset);
					int32_t signedByte = 0;

					if ((byte & 0x80) == 0x80)
					{
						gbhw::Byte newbyte = ~(byte - 1);
						signedByte -= newbyte;
					}
					else
					{
						signedByte = byte;
					}

					gbhw::Byte opcode = instruction.GetOpcode();
					if (opcode == 0x18 || opcode == 0x20 || opcode == 0x28 || opcode == 0x30 || opcode == 0x38)
					{
						gbhw::Address targetAddress = static_cast<gbhw::Address>(static_cast<int32_t>(instruction.GetAddress() + instruction.GetByteSize()) + signedByte);
						argstr = QString::asprintf("0x%02x (%d) [0x%04x]", byte, signedByte, targetAddress);
					}
					else
					{
						argstr = QString::asprintf("0x%02x (%d)", byte, signedByte);
					}
					immoffset += 1;
					break;
				}
				case gbhw::RTD::Imm16:
				{
					argstr = QString::asprintf("0x%04x", mmu.ReadWord(instruction.GetAddress() + immoffset));
					immoffset += 2;
					break;
				}
				case gbhw::RTD::Addr8:
				{
					gbhw::Byte imm8 = mmu.ReadByte(instruction.GetAddress() + immoffset);
					immoffset += 2;
					argstr = QString::asprintf("0xFF00 + 0x%02x [0x%02x]", imm8, mmu.ReadByte(0xFF00 + imm8));
					break;
				}
				case gbhw::RTD::Addr16:
				{
					gbhw::Address imm16 = mmu.ReadWord(instruction.GetAddress() + immoffset);
					immoffset += 2;
					argstr = QString::asprintf("0x%04x [0x%04x]", imm16, mmu.ReadByte(imm16));
					break;
				}
				case gbhw::RTD::AddrC: argstr = QString::asprintf("0xFF00 + 0x%02x [0x%04x]", regs.c, mmu.ReadWord(0xFF00 + regs.c)); break;
				case gbhw::RTD::AddrBC: argstr = QString::asprintf("0x%04x [0x%04x]", regs.bc, mmu.ReadWord(regs.bc)); break;
				case gbhw::RTD::AddrDE: argstr = QString::asprintf("0x%04x [0x%04x]", regs.de, mmu.ReadWord(regs.de)); break;
				case gbhw::RTD::AddrHL: argstr = QString::asprintf("0x%04x [0x%04x]", regs.hl, mmu.ReadWord(regs.hl)); break;
				case gbhw::RTD::FlagZ: argstr = QString::asprintf("Zero: %s", regs.IsFlagSet(gbhw::RF::Zero) ? "true" : "false"); break;
				case gbhw::RTD::FlagNZ: argstr = QString::asprintf("Not-Zero: %s", regs.IsFlagSet(gbhw::RF::Zero) ? "false" : "true"); break;
				case gbhw::RTD::FlagC: argstr = QString::asprintf("Zero: %s", regs.IsFlagSet(gbhw::RF::Carry) ? "true" : "false"); break;
				case gbhw::RTD::FlagNC: argstr = QString::asprintf("Zero: %s", regs.IsFlagSet(gbhw::RF::Carry) ? "false" : "true"); break;
				case gbhw::RTD::Zero: argstr = QString("0"); break;
				case gbhw::RTD::One: argstr = QString("1"); break;
				case gbhw::RTD::Two: argstr = QString("2"); break;
				case gbhw::RTD::Three: argstr = QString("3"); break;
				case gbhw::RTD::Four: argstr = QString("4"); break;
				case gbhw::RTD::Five: argstr = QString("5"); break;
				case gbhw::RTD::Six: argstr = QString("6"); break;
				case gbhw::RTD::Seven: argstr = QString("7"); break;
				case gbhw::RTD::Eight: argstr = QString("8"); break;
				case gbhw::RTD::Nine: argstr = QString("9"); break;
				default: argstr = "unknown"; break;
			}

			if (argCount == 1 || argIndex == 0)
			{
				res = QString::asprintf("%-22s", argstr.toStdString().c_str());
			}
			else
			{
				res = QString::asprintf("%-22s, %-22s", res.toStdString().c_str(), argstr.toStdString().c_str());
			}
		}

		return res;
	}

	QString GenerateInstructionString(const gbhw::Instruction& instruction, gbhw::Hardware* hardware)
	{
		QString text;
		QString args = ArgValuesToString(instruction, hardware);

		if (instruction.GetOpcode() == 0xCB)
		{
			text = QString::asprintf("0x%04x : 0x%02x : 0x%02x : %2u/%2u : %2u : %2u : %-12s : %s", instruction.GetAddress(), instruction.GetOpcode(), instruction.GetExtended(), instruction.GetCycles(true), instruction.GetCycles(false), instruction.GetByteSize(), 0, instruction.GetAssembly(), args.toStdString().c_str());
		}
		else
		{
			text = QString::asprintf("0x%04x : 0x%02x : ---- : %2u/%2u : %2u : %2u : %-12s : %s", instruction.GetAddress(), instruction.GetOpcode(), instruction.GetCycles(true), instruction.GetCycles(false), instruction.GetByteSize(), 0, instruction.GetAssembly(), args.toStdString().c_str());
		}

		return text;
	}

	const int32_t kInstructionDisplayWindowSize = 38;

	struct AssemblyData
	{
		enum Enum
		{
			Index = Qt::UserRole,
			Address,
			Breakpoint
		};
	};
}

namespace gbd
{
	AssemblyWidget::AssemblyWidget(QWidget* parent)
		: QListWidget(parent)
		, m_hardware(nullptr)
	{
		m_brushBreakpoint.setColor(Qt::red);

		m_instructions.reserve(kInstructionDisplayWindowSize);

		// Create assembly widgets
		for (int32_t i = 0; i < kInstructionDisplayWindowSize; ++i)
		{
			QListWidgetItem* lineItem = new QListWidgetItem();
			lineItem->setText("");
			lineItem->setData(AssemblyData::Index, QVariant(i));
			lineItem->setData(AssemblyData::Address, QVariant(-1));
			lineItem->setData(AssemblyData::Breakpoint, QVariant(false));

			m_lines.push_back(lineItem);
			addItem(lineItem);
		}
	}

	AssemblyWidget::~AssemblyWidget()
	{
	}

	void AssemblyWidget::SetHardware(gbhw::Hardware* hardware)
	{
		m_hardware = hardware;

		//AddBreakpoint(0xDEF8);
	}

	void AssemblyWidget::UpdateView()
	{
		gbhw::Address currentPC = m_hardware->GetCPU().GetRegisters().pc;

		QListWidgetItem* foundItem = nullptr;

		for(auto& lineItem : m_lines)
		{
			//item->data(Qt::UserRole + 1).toBool();
			gbhw::Address itemAddress = static_cast<gbhw::Address>(lineItem->data(AssemblyData::Address).toInt());

			if(itemAddress == currentPC)
			{
				foundItem = lineItem;
				break;
			}
		}

		if(foundItem)
		{
			// If we're already visible then let's just select that item.
			setCurrentItem(foundItem);
		}
		else
		{
			// Refresh view entirely.
			m_instructions.clear();
			m_hardware->GetRom().GetInstructions(currentPC, m_instructions, kInstructionDisplayWindowSize);

			for(int32_t i = 0; i < kInstructionDisplayWindowSize; ++i)
			{
				gbhw::Address instructionAddress = m_instructions[i].GetAddress();

				m_lines[i]->setData(AssemblyData::Address, QVariant(instructionAddress));

				bool bIsBreakpointed = false;

				for(auto& bp : m_breakpoints)
				{
					if(bp == instructionAddress)
					{
						bIsBreakpointed = true;
					}
				}

				if(bIsBreakpointed)
				{
					m_lines[i]->setForeground(m_brushBreakpoint);
				}
				else
				{
					m_lines[i]->setForeground(m_brushDefault);
				}

				m_lines[i]->setData(AssemblyData::Breakpoint, QVariant(bIsBreakpointed));	// clear break point colouring?
			}

			setCurrentItem(m_lines[0]);
		}

		// Update all the strings in view.
		for (int32_t i = 0; i < kInstructionDisplayWindowSize; ++i)
		{
			m_lines[i]->setText(GenerateInstructionString(m_instructions[i], m_hardware));
		}
	}

	void AssemblyWidget::ToggleBreakpoint()
	{
		auto items = selectedItems();

		if (items.size() > 1)
		{
			return;
		}
		else if (items.size() == 1)
		{
			QListWidgetItem* item = items[0];

			gbhw::Address instructionaddress = static_cast<gbhw::Address>(item->data(AssemblyData::Address).toUInt());
			bool breakpoint = item->data(AssemblyData::Breakpoint).toBool();
			item->setData(AssemblyData::Breakpoint, QVariant(!breakpoint));

			if (!breakpoint)
			{
				item->setForeground(m_brushBreakpoint);
				m_breakpoints.push_back(instructionaddress);
				m_hardware->GetCPU().BreakpointSet(instructionaddress);
			}
			else
			{
				item->setForeground(m_brushDefault);
				m_breakpoints.erase(std::remove(m_breakpoints.begin(), m_breakpoints.end(), instructionaddress));
				m_hardware->GetCPU().BreakpointRemove(instructionaddress);
			}
		}
	}

	void AssemblyWidget::AddBreakpoint(gbhw::Address address)
	{
		m_breakpoints.push_back(address);
		m_hardware->GetCPU().BreakpointSet(address);
	}
}