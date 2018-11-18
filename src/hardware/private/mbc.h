#pragma once

#include "context.h"

namespace gbhw
{
	//--------------------------------------------------------------------------

	class MBC
	{
	public:
		MBC(MMU_ptr mmu);
		virtual ~MBC();
		virtual bool write(const Address& address, Byte value);

		static MBC* create(MMU_ptr mmu, CartridgeType::Type cartridge);

	protected:
		MMU_ptr m_mmu;
	};

	//--------------------------------------------------------------------------
}
