#include "gbd_screenwindow.h"
#include "gbd_log.h"

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

		// Sprites tab

	}

	ScreenWindow::~ScreenWindow()
	{
	}

	void ScreenWindow::OnShowTileGrid(bool bChecked)
	{
		m_ui.m_screen->SetShowTilemapGrid(bChecked);
	}
}