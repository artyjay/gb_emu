#pragma once

#include "context.h"

namespace gbhw
{
	class MBC
	{
	public:
		MBC(MMU_ptr mmu);
		virtual ~MBC();
		virtual bool HandleWrite(const Address& address, Byte value);

	protected:
		MMU_ptr m_mmu;
	};

	MBC* CreateMBC(MMU* mmu, CartridgeType::Type cartridgeType);
}
