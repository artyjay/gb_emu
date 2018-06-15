#pragma once

#include "gbhw_types.h"

namespace gbhw
{
	class MMU;
	class MBC
	{
	public:
		MBC(MMU* mmu);
		virtual ~MBC();
		virtual bool HandleWrite(const Address& address, Byte value);

	protected:
		MMU* m_mmu;
	};

	MBC* CreateMBC(MMU* mmu, CartridgeType::Type cartridgeType);
}
