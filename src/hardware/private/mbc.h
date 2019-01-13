#pragma once

#include "types.h"

namespace gbhw
{
	class MMU;

	//--------------------------------------------------------------------------

	class MBC
	{
	public:
		MBC(MMU* mmu);
		virtual ~MBC();
		virtual bool write(const Address& address, Byte value);

		static MBC* create(MMU* mmu, CartridgeType::Type cartridge);

	protected:
		MMU* m_mmu;
	};

	//--------------------------------------------------------------------------
}
