#include "gbd_screenwindow.h"
#include "gbd_log.h"

using namespace gbhw;

namespace gbd
{
	ScreenWindow::ScreenWindow(QWidget* parent, gbhw_context_t hardware)
		: QWidget(parent, Qt::Window)
		, m_hardware(hardware)
	{
		m_ui.setupUi(this);

		// Screen tab
		QObject::connect(m_ui.m_screen, &ScreenWidget::UpdateCoordText, m_ui.m_tileCoord, &QLabel::setText);
		QObject::connect(m_ui.m_showTileGrid, &QCheckBox::toggled, this, &ScreenWindow::OnShowTileGrid);
		m_ui.m_screen->SetHardware(m_hardware);

		// Tile RAM tab
// 		QObject::connect(m_ui.m_pattern0, &TilePatternWidget::OnTileCoordinateChanged, m_ui.m_pattern0Coord, &QLabel::setText);
// 		QObject::connect(m_ui.m_pattern0, &TilePatternWidget::OnHighlightedTileChanged, m_ui.m_patternZoom, &TilePatternZoomWidget::SetZoomedTile);
// 		QObject::connect(m_ui.m_pattern0, &TilePatternWidget::OnTileLocked, m_ui.m_patternZoom, &TilePatternZoomWidget::SetLockedTile);
// 		QObject::connect(m_ui.m_pattern1, &TilePatternWidget::OnTileCoordinateChanged, m_ui.m_pattern1Coord, &QLabel::setText);
// 		QObject::connect(m_ui.m_pattern1, &TilePatternWidget::OnHighlightedTileChanged, m_ui.m_patternZoom, &TilePatternZoomWidget::SetZoomedTile);
// 		QObject::connect(m_ui.m_pattern1, &TilePatternWidget::OnTileLocked, m_ui.m_patternZoom, &TilePatternZoomWidget::SetLockedTile);

		m_ui.m_tileDataBank0->initialise(m_hardware, 0);
		m_ui.m_tileDataBank1->initialise(m_hardware, 1);

		// Tile Map tab
		m_ui.m_tileMap0->initialise(m_hardware, 0);
		m_ui.m_tileMap1->initialise(m_hardware, 1);
		QObject::connect(m_ui.m_tileMap0, &TileMapWidget::on_focus_change, this, &ScreenWindow::on_tilemap_focus_change);

		m_ui.m_bgPalette->initialise(m_hardware, GPUPalette::BG);
		QObject::connect(m_ui.m_bgPalette, &PaletteWidget::on_focus_change, this, &ScreenWindow::on_palette_focus_change);

		// Sprites tab

	}

	ScreenWindow::~ScreenWindow()
	{
	}

	void ScreenWindow::OnShowTileGrid(bool bChecked)
	{
		m_ui.m_screen->SetShowTilemapGrid(bChecked);
	}

	void ScreenWindow::on_palette_focus_change(PaletteFocusArgs args)
	{
		const uint32_t r = (args.value & 0x1f);
		const uint32_t g = ((args.value >> 5) & 0x1f);
		const uint32_t b = ((args.value >> 10) & 0x1f);

		m_ui.m_palColV->setText(QString::asprintf("0x%04x", args.value));
		m_ui.m_palColR->setText(QString::asprintf("0x%02x", r));
		m_ui.m_palColG->setText(QString::asprintf("0x%02x", g));
		m_ui.m_palColB->setText(QString::asprintf("0x%02x", b));
		m_ui.m_palColRScaled->setText(QString::asprintf("%u", args.r_scaled));
		m_ui.m_palColGScaled->setText(QString::asprintf("%u", args.g_scaled));
		m_ui.m_palColBScaled->setText(QString::asprintf("%u", args.b_scaled));
	}

	void ScreenWindow::on_tilemap_focus_change(TileMapFocusArgs args)
	{
		if(!m_hardware)
			return;

		GPU* gpu = nullptr;
		if(gbhw_get_gpu(m_hardware, &gpu) != e_success)
			return;

		const GPUTileRam* ram = gpu->get_tile_ram();
		const Byte tileIndex = ram->tileMap[args.map][args.tile];

		// @todo:



	}
}