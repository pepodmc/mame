// license:BSD-3-Clause
// copyright-holders:David Haywood
/***************************************************************************

    xavix.cpp

    The dies for these are marked

    SSD 97 PA7270-107 (only seen on Ping Pong)
    SSD 98 PA7351-107
    SSD 98 PL7351-181

    6502 with custom opcodes
    integrated gfx / sound

    special notes

    0x0000-0x7fff seems to be a 'low bus' area, it is always the same regardless
                  of banking
    0x8000-0xffff is a banked area with individual code and data banks
                  0x00ff contains the DATA bank, set manually in code
                  0x00fe appears to be the current CODE bank, set with either the
                         custom opcodes, or manually (if running from lowbus only?)

***************************************************************************/

#include "emu.h"
#include "xavix.h"
#include "xavixd.h"

DEFINE_DEVICE_TYPE(XAVIX, xavix_device, "xavix", "XaviX (SSD 97 / SSD 98)")

xavix_device::xavix_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	m6502_device(mconfig, XAVIX, tag, owner, clock),
	XPC(0),
	m_special_data_config("specialdata", ENDIANNESS_LITTLE, 8, 24)
{
	program_config.m_addr_width = 24;
	program_config.m_logaddr_width = 24;
	sprogram_config.m_addr_width = 24;
	sprogram_config.m_logaddr_width = 24;
	m_special_data_config.m_addr_width = 24;
	m_special_data_config.m_logaddr_width = 24;

}

xavix_device::xavix_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	m6502_device(mconfig, type, tag, owner, clock),
	XPC(0),
	m_special_data_config("specialdata", ENDIANNESS_LITTLE, 8, 24)
{
	program_config.m_addr_width = 24;
	program_config.m_logaddr_width = 24;
	sprogram_config.m_addr_width = 24;
	sprogram_config.m_logaddr_width = 24;
	m_special_data_config.m_addr_width = 24;
	m_special_data_config.m_logaddr_width = 24;
}



std::unique_ptr<util::disasm_interface> xavix_device::create_disassembler()
{
	return std::make_unique<xavix_disassembler>();
}


offs_t xavix_device::pc_to_external(u16 pc)
{
	return adr_with_codebank(pc);
}

void xavix_device::device_start()
{
	if(cache_disabled)
		mintf = std::make_unique<mi_xavix_nd>(this);
	else
		mintf = std::make_unique<mi_xavix_normal>(this);

	// bind delegates
	m_vector_callback.bind_relative_to(*owner());

	init();

	m_special_data_space = &space(4);

	state_add(XAVIX_DATABANK, "DATBNK", m_databank).callimport().formatstr("%2s");;
	state_add(XAVIX_CODEBANK, "CODBNK", m_codebank).callimport().formatstr("%2s");
}

void xavix_device::device_reset()
{
	set_codebank(0);
	set_databank(0);
	m6502_device::device_reset();
}

uint8_t xavix_device::read_special(uint16_t adr)
{
	return m_special_data_space->read_byte(adr | (m_databank<<16));
}

xavix_device::mi_xavix_normal::mi_xavix_normal(xavix_device *_base)
{
	base = _base;
}


uint8_t xavix_device::mi_xavix_normal::read(uint16_t adr)
{
	if (adr == 0xfe)
		return base->m_codebank;
	else if (adr == 0xff)
		return base->m_databank;

	return program->read_byte((base->get_databank() << 16) | adr);
}

uint8_t xavix_device::mi_xavix_normal::read_sync(uint16_t adr)
{
	return scache->read_byte(base->adr_with_codebank(adr));
}

uint8_t xavix_device::mi_xavix_normal::read_arg(uint16_t adr)
{
	return cache->read_byte(base->adr_with_codebank(adr));
}

void xavix_device::mi_xavix_normal::write(uint16_t adr, uint8_t val)
{
	if (adr == 0xfe)
	{
		base->m_codebank = val;
		return;
	}
	else if (adr == 0xff)
	{
		base->m_databank = val;
		return;
	}

	program->write_byte((base->get_databank() << 16) | adr, val);
}

xavix_device::mi_xavix_nd::mi_xavix_nd(xavix_device *_base) : mi_xavix_normal(_base)
{
}

uint8_t xavix_device::mi_xavix_nd::read_sync(uint16_t adr)
{
	return sprogram->read_byte(base->adr_with_codebank(adr));
}

uint8_t xavix_device::mi_xavix_nd::read_arg(uint16_t adr)
{
	return program->read_byte(base->adr_with_codebank(adr));
}

void xavix_device::set_codebank(uint8_t bank)
{
//  space().write_byte(0xfe, bank);
	m_codebank = bank;
}

inline uint8_t xavix_device::get_codebank()
{
//  return space().read_byte(0xfe);
	return m_codebank;
}

inline void xavix_device::set_databank(uint8_t bank)
{
//  space().write_byte(0xff, bank);
	m_databank = bank;
}

inline uint8_t xavix_device::get_databank()
{
//  return space().read_byte(0xff);
	return m_databank;
}

device_memory_interface::space_config_vector xavix_device::memory_space_config() const
{
	if(has_configured_map(AS_OPCODES))
		return space_config_vector {
			std::make_pair(AS_PROGRAM, &program_config),
			std::make_pair(AS_OPCODES, &sprogram_config),
			std::make_pair(4, &m_special_data_config)
		};
	else
		return space_config_vector {
			std::make_pair(AS_PROGRAM, &program_config),
			std::make_pair(4, &m_special_data_config)
		};
}



void xavix_device::state_import(const device_state_entry &entry)
{
	m6502_device::state_import(entry);

	switch(entry.index())
	{
	case XAVIX_DATABANK:
		break;
	case XAVIX_CODEBANK:
		break;
	}
}

void xavix_device::state_string_export(const device_state_entry &entry, std::string &str) const
{
	m6502_device::state_string_export(entry, str);

	switch(entry.index())
	{
	case XAVIX_DATABANK:
		str = string_format("%02x", m_databank);
		break;
	case XAVIX_CODEBANK:
		str = string_format("%02x", m_codebank);
		break;
	}
}




#include "cpu/m6502/xavix.hxx"
