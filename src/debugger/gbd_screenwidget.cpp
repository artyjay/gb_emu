#include "gbd_log.h"
#include "gbd_screenwidget.h"

#include <QPainter>
#include <QMouseEvent>

namespace gbd
{
	ScreenWidget::ScreenWidget(QWidget * parent)
		: QWidget(parent)
		, m_hardware(nullptr)
		, m_image(160, 144, QImage::Format::Format_RGBX8888)
		, m_bShowTilemapGrid(false)
		, m_scaleX(2.0f)
		, m_scaleY(2.0f)
	{
		setMouseTracking(true);
		m_gridPen.setColor(QColor(100, 100, 100, 100));
	}

	ScreenWidget::~ScreenWidget()
	{
	}

	void ScreenWidget::SetHardware(gbhw::Hardware* hardware)
	{
		m_hardware = hardware;
	}

	void ScreenWidget::SetShowTilemapGrid(bool bShow)
	{
		m_bShowTilemapGrid = bShow;
		repaint();
	}

	void ScreenWidget::paintEvent(QPaintEvent* evt)
	{
		QPainter painter(this);

		// Update the image
		if(m_hardware)
		{
			const gbhw::Byte* screenData = m_hardware->GetGPU().GetScreenData();
			QRgb* destData = (QRgb*)m_image.scanLine(0);

			for (uint32_t y = 0; y < gbhw::GPU::kScreenHeight; ++y)
			{
				for (uint32_t x = 0; x < gbhw::GPU::kScreenWidth; ++x)
				{
					gbhw::Byte hwpixel = (3 - *screenData) * 85;
					*destData = qRgb(hwpixel, hwpixel, hwpixel);
					destData++;
					screenData++;
				}
			}
		}

		// Paint the widget contents.
		painter.scale(m_scaleX, m_scaleY);

		// Screen image
		painter.drawImage(QPoint(0, 0), m_image);

		// Tile map grid.
		if (m_bShowTilemapGrid)
		{
			painter.setPen(m_gridPen);

			gbhw::CPU& cpu = m_hardware->GetCPU();
			gbhw::Byte scrollX = cpu.ReadIO(gbhw::HWRegs::ScrollX);
			gbhw::Byte scrollY = cpu.ReadIO(gbhw::HWRegs::ScrollY);

			int lineX = -(scrollX % 8);
			int lineY = -(scrollY % 8);
			
			// Draw horizontal lines
			for (uint32_t y = 0; y < 19; ++y)
			{
				painter.drawLine(0, lineY, 160, lineY);
				lineY += 8;
			}

			// Draw vertical lines
			for (uint32_t x = 0; x < 21; ++x)
			{
				painter.drawLine(lineX, 0, lineX, 144);
				lineX += 8;
			}
		}
	}

	void ScreenWidget::mouseMoveEvent(QMouseEvent* evt)
	{
		gbhw::CPU& cpu = m_hardware->GetCPU();
		gbhw::MMU& mmu = cpu.GetMMU();

		gbhw::Byte scrollX = cpu.ReadIO(gbhw::HWRegs::ScrollX);
		gbhw::Byte scrollY = cpu.ReadIO(gbhw::HWRegs::ScrollY);

		int posX = evt->pos().x();
		int posY = evt->pos().y();

		int tileX = (posX + scrollX) / (8 * m_scaleX);
		int tileY = (posY + scrollY) / (8 * m_scaleY);

		gbhw::Address tilemapAddress = gbhw::HWLCDC::GetBGTileMapAddress(mmu.ReadByte(gbhw::HWRegs::LCDC));
		gbhw::Byte tileIndex = mmu.ReadByte(tilemapAddress + (tileY * 32) + tileX);
		QString str;

		if (gbhw::HWLCDC::GetTilePatternIndex(mmu.ReadByte(gbhw::HWRegs::LCDC)) == 0)
		{
			int32_t unsignedTileIndex = tileIndex ^ 0x80;
			int32_t signedTileIndex = static_cast<int32_t>((gbhw::SByte)(tileIndex));

// 			if (signedTileIndex >= 128)
// 			{
// 				signedTileIndex -= 256;
// 			}

			str = QString::asprintf("Tile: %d, %d - Index: %d (%d)", tileX, tileY, signedTileIndex, unsignedTileIndex);
		}
		else
		{
			str = QString::asprintf("Tile: %d, %d - Index: %d", tileX, tileY, tileIndex);
		}

		emit UpdateCoordText(str);
	}
}