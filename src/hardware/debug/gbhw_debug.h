#pragma once

/*----------------------------------------------------------------------------*/

#include <gbhw.h>
#include <cpu.h>
#include <gpu.h>
#include <mmu.h>
#include <registers.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/

HWPublicAPI gbhw_errorcode_t gbhw_get_cpu(gbhw_context_t ctx, gbhw::CPU** cpu);
HWPublicAPI gbhw_errorcode_t gbhw_get_gpu(gbhw_context_t ctx, gbhw::GPU** gpu);
HWPublicAPI gbhw_errorcode_t gbhw_get_mmu(gbhw_context_t ctx, gbhw::MMU** mmu);
HWPublicAPI gbhw_errorcode_t gbhw_get_registers(gbhw_context_t ctx, gbhw::Registers** registers);
HWPublicAPI gbhw_errorcode_t gbhw_disasm(gbhw_context_t ctx, gbhw::Address address, gbhw::InstructionList& instructions, int32_t instructionCount);

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif