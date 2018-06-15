#pragma once

namespace gbhw
{
	inline const Byte* GPUTilePattern::GetTileLine(Byte tileIndex, const Byte tileLineY) const
	{
		if (m_bSignedIndex)
		{
			tileIndex ^= 0x80;
		}

		return m_tiles[tileIndex].m_pixels[tileLineY];
	}

	inline void GPUTilePattern::DirtyTile(Byte tileIndex)
	{
		m_tiles[tileIndex].m_bDirty = true;
	}

	inline GPUSpriteData::GPUSpriteData()
	{
		Reset();
	}
	
	inline void GPUSpriteData::Reset()
	{
		m_x			= 0;
		m_y			= 0;
		m_tileIndex = 0;
		m_flags		= 0;
	}

	inline Byte GPUSpriteData::GetX() const
	{
		return m_x;
	}
	
	inline Byte GPUSpriteData::GetY() const
	{
		return m_y;
	}
	
	inline Byte GPUSpriteData::GetTileIndex() const
	{
		return m_tileIndex;
	}
	
	inline Byte GPUSpriteData::GetFlags() const
	{
		return m_flags;
	}

	inline void GPUSpriteData::SetX(Byte x)
	{
		m_x = x;
	}
	
	inline void GPUSpriteData::SetY(Byte y)
	{
		m_y = y;
	}
	
	inline void GPUSpriteData::SetTile(Byte tile)
	{
		m_tileIndex = tile;
	}
	
	inline void GPUSpriteData::SetFlags(Byte flags)
	{
		m_flags = flags;
	}








	inline Byte GPU::GetPaletteColour(const Byte palette, const Byte paletteIndex) const
	{
		return m_paletteData[palette][paletteIndex & 0x03];
	}

	inline void GPU::UpdateScanLineSprites()
	{
		m_scanLineSprites.clear();

		const Byte spriteHeight = HWLCDC::GetSpriteDoubleHeight(m_lcdc) ? 16 : 8;

		for (Byte spriteIndex = 0; spriteIndex < 40; ++spriteIndex)
		{
			GPUSpriteData* sprite = &m_spriteData[spriteIndex];

			const Byte		x			= sprite->GetX();
			const Byte		y			= sprite->GetY();
			const int16_t	yProper		= y - 16;

			if ((x == 0 || x >= 168 || y == 0 || y >= 160) ||												// Visibility check
				(m_currentScanLine < yProper || m_currentScanLine > (yProper + (spriteHeight - 1))))		// Scanline check
			{
				continue;
			}

			// Just stop once we achieve goal.
			m_scanLineSprites.push_back(spriteIndex);

// 			if (m_scanLineSprites.size() == 10)
// 			{
// 				// @todo Priorities based upon position and address.
// 				break;
// 			}
		}

		// Currently just reverse the priorities, such that the left most is drawn last
		// and on-top of everything else.
// 		if (m_scanLineSprites.size() > 0)
// 		{
// 			std::reverse(m_scanLineSprites.begin(), m_scanLineSprites.end());
// 		}
	}

} // gbhw
