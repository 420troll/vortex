// Minimal header shim for compiling MAME's unmodified ADSP disassembler.
#pragma once

#include <cstdint>
#include <ostream>

#include "osdcomm.h"
#include "util/disasmintf.h"
#include "util/strformat.h"

using u32 = util::disasm_interface::u32;
using u8 = util::disasm_interface::u8;
using u16 = util::disasm_interface::u16;
using u64 = util::disasm_interface::u64;
using offs_t = util::disasm_interface::offs_t;

template <typename T>
constexpr unsigned BIT(T value, unsigned bit)
{
	return (static_cast<std::uint64_t>(value) >> bit) & 1;
}

template <typename T>
constexpr unsigned BIT(T value, unsigned bit, unsigned width)
{
	return (static_cast<std::uint64_t>(value) >> bit) & ((std::uint64_t(1) << width) - 1);
}
