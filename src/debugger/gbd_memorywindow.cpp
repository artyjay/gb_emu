#include "gbd_memorywindow.h"
#include "gbd_log.h"

using namespace gbhw;

namespace
{
	const int32_t kMemoryDisplayWindowSize = 16;
}

namespace gbd
{
	MemoryWindow::MemoryWindow(QWidget* parent, gbhw_context_t hardware)
		: QWidget(parent, Qt::Window)
		, m_hardware(hardware)
		, m_enteredAddress(0)
	{
		m_ui.setupUi(this);

		// connect text entering
		//QLineEdit
		QObject::connect(m_ui.m_address, &QLineEdit::textChanged, this, &MemoryWindow::OnAddressChanged);

		// Create lines.
		for(int32_t i = 0; i < kMemoryDisplayWindowSize + 1; ++i)
		{
			QListWidgetItem* lineItem = new QListWidgetItem();

			if(i == 0)
			{
				lineItem->setText("        0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F");
			}
			else
			{
				lineItem->setText("");
			}

			m_lines.push_back(lineItem);
			m_ui.m_memory->addItem(lineItem);
		}
	}

	MemoryWindow::~MemoryWindow()
	{
	}

	void MemoryWindow::OnAddressChanged(const QString& str)
	{
		// Parse the string, expecting hex address of 4 characters 0-9,A-F
		if(str.length() != 4)
		{
			return;
		}

		bool bOk = false;
		m_enteredAddress = static_cast<gbhw::Address>(str.toUInt(&bOk, 16));

		UpdateView();
	}

	void MemoryWindow::UpdateView()
	{
		gbhw::Address address = m_enteredAddress;

		//Message("Entered address [%s], at address: %d\n", str.toStdString().c_str(), address);
		gbhw::Byte lineBytes[16];

		MMU* mmu;
		if(gbhw_get_mmu(m_hardware, &mmu) != e_success)
		{
			return;
		}

		// Update view.
		for (int32_t i = 0; i < kMemoryDisplayWindowSize; ++i)
		{
			for (int32_t lineByte = 0; lineByte < 16; ++lineByte)
			{
				lineBytes[lineByte] = mmu->ReadByte(address + lineByte);
			}

			QString lineText = QString::asprintf("0x%04x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x",
				address,
				lineBytes[0],
				lineBytes[1],
				lineBytes[2],
				lineBytes[3],
				lineBytes[4],
				lineBytes[5],
				lineBytes[6],
				lineBytes[7],
				lineBytes[8],
				lineBytes[9],
				lineBytes[10],
				lineBytes[11],
				lineBytes[12],
				lineBytes[13],
				lineBytes[14],
				lineBytes[15]);

			m_lines[i + 1]->setText(lineText);

			address += 16;
		}
	}
}