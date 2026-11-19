// license:BSD-3-Clause
// copyright-holders:Vortex research contributors

#include "emu.h"

#include "cpu/adsp2100/adsp2100.h"

#include <array>

namespace {

class vortex_exec_state : public driver_device
{
public:
	vortex_exec_state(machine_config const &mconfig, device_type type, char const *tag)
		: driver_device(mconfig, type, tag)
		, m_dsp(*this, "dsp")
		, m_boot_pm(*this, "boot_pm")
		, m_dmram(*this, "dmram")
	{
	}

	void vortex_exec(machine_config &config);

protected:
	virtual void machine_reset() override;
	virtual void machine_start() override;

private:
	void program_map(address_map &map);
	void data_map(address_map &map);
	u32 external_pm_r(offs_t offset);
	void external_pm_w(offs_t offset, u32 data);
	u16 external_dm_r(offs_t offset);
	void external_dm_w(offs_t offset, u16 data);
	u16 reserved_dm_r(offs_t offset);
	void reserved_dm_w(offs_t offset, u16 data);
	u16 control_dm_r(offs_t offset);
	void control_dm_w(offs_t offset, u16 data);
	u32 current_pc() const { return m_dsp->state_int(STATE_GENPCBASE); }

	required_device<adsp2105_device> m_dsp;
	required_shared_ptr<u32> m_boot_pm;
	required_shared_ptr<u16> m_dmram;
	std::array<u16, 0x400> m_mmr;
};


void vortex_exec_state::program_map(address_map &map)
{
	map(0x0000, 0x03ff).ram().share("boot_pm");
	map(0x0400, 0x3fff).rw(FUNC(vortex_exec_state::external_pm_r), FUNC(vortex_exec_state::external_pm_w));
}


void vortex_exec_state::data_map(address_map &map)
{
	map(0x0000, 0x37ff).rw(FUNC(vortex_exec_state::external_dm_r), FUNC(vortex_exec_state::external_dm_w));
	map(0x3800, 0x39ff).ram().share("dmram");
	map(0x3a00, 0x3bff).rw(FUNC(vortex_exec_state::reserved_dm_r), FUNC(vortex_exec_state::reserved_dm_w));
	map(0x3c00, 0x3fff).rw(FUNC(vortex_exec_state::control_dm_r), FUNC(vortex_exec_state::control_dm_w));
}


void vortex_exec_state::machine_reset()
{
	auto const *rom = memregion("firmware")->base();
	if (memregion("firmware")->bytes() != 0x10000 || rom[3] > 0x7f)
		fatalerror("Vortex execution harness: invalid or truncated firmware boot page");

	// The ADSP-2105 core resets PC to zero but does not perform its hardware
	// auto-boot DMA.  This uses MAME's own unpacker to model the reset transfer.
	m_dsp->load_boot_data(const_cast<u8 *>(rom), m_boot_pm);
	m_mmr.fill(0xffff);
	m_mmr[0x3ffe - 0x3c00] = 0x7fff; // DWAIT0-4 reset to seven wait states; bit 15 reads zero.

	logerror("Vortex harness: booted %u PM words from ROM file offset 0x0000; reset PC=%04X\n",
		(unsigned(rom[3]) + 1) * 8, current_pc());
}


void vortex_exec_state::machine_start()
{
	save_item(NAME(m_mmr));
}


u32 vortex_exec_state::external_pm_r(offs_t offset)
{
	logerror("UNMAPPED PM read PC=%04X address=%04X -> FFFFFF\n", current_pc(), 0x0400 + offset);
	return 0x00ffffff;
}


void vortex_exec_state::external_pm_w(offs_t offset, u32 data)
{
	logerror("UNMAPPED PM write PC=%04X address=%04X data=%06X ignored\n", current_pc(), 0x0400 + offset, data & 0x00ffffff);
}


u16 vortex_exec_state::external_dm_r(offs_t offset)
{
	logerror("UNMAPPED external DM read PC=%04X address=%04X -> FFFF\n", current_pc(), unsigned(offset));
	return 0xffff;
}


void vortex_exec_state::external_dm_w(offs_t offset, u16 data)
{
	logerror("UNMAPPED external DM write PC=%04X address=%04X data=%04X ignored\n", current_pc(), unsigned(offset), data);
}


u16 vortex_exec_state::reserved_dm_r(offs_t offset)
{
	logerror("UNMAPPED reserved DM read PC=%04X address=%04X -> FFFF\n", current_pc(), 0x3a00 + offset);
	return 0xffff;
}


void vortex_exec_state::reserved_dm_w(offs_t offset, u16 data)
{
	logerror("UNMAPPED reserved DM write PC=%04X address=%04X data=%04X ignored\n", current_pc(), 0x3a00 + offset, data);
}


u16 vortex_exec_state::control_dm_r(offs_t offset)
{
	unsigned const address = 0x3c00 + offset;
	switch (address)
	{
	case 0x3fff: case 0x3ffe: case 0x3ffd: case 0x3ffc: case 0x3ffb:
	case 0x3ff2: case 0x3ff1: case 0x3ff0: case 0x3fef:
		return m_mmr[offset];
	default:
		logerror("UNMAPPED ADSP control-register read PC=%04X address=%04X -> FFFF\n", current_pc(), address);
		return 0xffff;
	}
}


void vortex_exec_state::control_dm_w(offs_t offset, u16 data)
{
	unsigned const address = 0x3c00 + offset;
	switch (address)
	{
	case 0x3fff: case 0x3ffe: case 0x3ffd: case 0x3ffc: case 0x3ffb:
	case 0x3ff2: case 0x3ff1: case 0x3ff0: case 0x3fef:
		m_mmr[offset] = data;
		logerror("ADSP MMR write PC=%04X address=%04X data=%04X (register latch; device effects not modeled)\n", current_pc(), address, data);
		break;
	default:
		logerror("UNMAPPED ADSP control-register write PC=%04X address=%04X data=%04X ignored\n", current_pc(), address, data);
		break;
	}
}


void vortex_exec_state::vortex_exec(machine_config &config)
{
	ADSP2105(config, m_dsp, 10_MHz_XTAL); // service-manual frequency; board clock source not yet modeled
	m_dsp->set_addrmap(AS_PROGRAM, &vortex_exec_state::program_map);
	m_dsp->set_addrmap(AS_DATA, &vortex_exec_state::data_map);
}


static INPUT_PORTS_START(vortex_exec)
INPUT_PORTS_END


ROM_START(vortexexec)
	ROM_REGION(0x10000, "firmware", 0)
	ROM_LOAD("vortex.bin", 0x0000, 0x10000, CRC(ec595f22) SHA1(c50543fbb67fc8406072f448a7a6b601ea63aa01))
ROM_END


} // anonymous namespace


COMP(2026, vortexexec, 0, 0, vortex_exec, vortex_exec, vortex_exec_state, empty_init, "TROLLfx research", "Lexicon Vortex ADSP-2105 execution harness", MACHINE_NO_SOUND | MACHINE_NOT_WORKING)
