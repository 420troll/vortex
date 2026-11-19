// license:BSD-3-Clause
// Copyright MAMEdev and contributors; wrapper code copyright Vortex research contributors.

#include "emu.h"

#include "cpu/adsp2100/2100dasm.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

constexpr std::size_t rom_size = 0x10000;
constexpr std::size_t page_size = 0x1000;
constexpr std::size_t max_words = 0x400;

class word_buffer final : public util::disasm_interface::data_buffer
{
public:
	std::vector<u32> words;

	u8 r8(offs_t address) const override { return u8(read_word(address)); }
	u16 r16(offs_t address) const override { return u16(read_word(address)); }
	u32 r32(offs_t address) const override { return read_word(address); }
	u64 r64(offs_t address) const override { return read_word(address); }

private:
	u32 read_word(offs_t address) const { return address < words.size() ? words[address] : 0; }
};

} // anonymous namespace


int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "usage: mame-adsp-disassemble ROM_PATH PAGE_OFFSET\n";
		return 2;
	}

	try
	{
		std::size_t const page_offset = std::stoul(argv[2], nullptr, 0);
		if ((page_offset % page_size) || (page_offset + page_size > rom_size))
			throw std::runtime_error("page offset must select a complete aligned 4 KiB page in the 64 KiB image");

		std::ifstream input(argv[1], std::ios::binary);
		std::vector<u8> image(std::istreambuf_iterator<char>(input), {});
		if (!input.eof() && input.fail())
			throw std::runtime_error("could not read ROM image");
		if (image.size() != rom_size)
			throw std::runtime_error("ROM image must be exactly 65536 bytes");

		auto const *page = image.data() + page_offset;
		std::size_t const words_count = (std::size_t(page[3]) + 1) * 8;
		if (words_count > max_words)
			throw std::runtime_error("page length exceeds ADSP-2105 internal program memory");

		word_buffer opcodes;
		opcodes.words.reserve(words_count);
		for (std::size_t index = 0; index < words_count; ++index)
		{
			auto const base = page + index * 4;
			opcodes.words.push_back((u32(base[0]) << 16) | (u32(base[1]) << 8) | u32(base[2]));
		}

		adsp21xx_disassembler disassembler;
		for (std::size_t pc = 0; pc < words_count; ++pc)
		{
			std::ostringstream operation;
			(void)disassembler.disassemble(operation, offs_t(pc), opcodes, opcodes);
			std::cout << "ROM+" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << (page_offset + pc * 4)
				<< " PC=" << std::setw(4) << pc
				<< " " << std::setw(6) << opcodes.words[pc]
				<< "  " << operation.str() << '\n';
		}
		std::cout << "decoded_words=" << std::dec << words_count << '\n';
	}
	catch (std::exception const &error)
	{
		std::cerr << "error: " << error.what() << '\n';
		return 1;
	}

	return 0;
}
