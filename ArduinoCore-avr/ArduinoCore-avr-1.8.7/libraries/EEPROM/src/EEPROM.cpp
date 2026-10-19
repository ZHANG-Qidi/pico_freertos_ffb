#include "EEPROM.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "pico/assert.h"
#include "pico/flash.h"

namespace pico_eeprom {

static_assert(PICO_EEPROM_FLASH_OFFSET % FLASH_SECTOR_SIZE == 0, "EEPROM must be sector aligned");
static_assert(PICO_EEPROM_FLASH_OFFSET + FLASH_SECTOR_SIZE == PICO_FLASH_SIZE_BYTES, "EEPROM must occupy the last flash sector");

static uint8_t s_sector[FLASH_SECTOR_SIZE];

static const uint8_t *contents() { return reinterpret_cast<const uint8_t *>(XIP_BASE + PICO_EEPROM_FLASH_OFFSET); }

static bool in_range(int index, size_t count) { return index >= 0 && static_cast<size_t>(index) < FLASH_SECTOR_SIZE && count <= FLASH_SECTOR_SIZE - static_cast<size_t>(index); }

// The sector buffer preserves bytes outside the requested write range.
static void write_sector(void *) {
    flash_range_erase(PICO_EEPROM_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(PICO_EEPROM_FLASH_OFFSET, s_sector, FLASH_SECTOR_SIZE);
}

static void write_bytes(int index, const uint8_t *data, size_t count, bool changed_only) {
    if (!data || !count || !in_range(index, count)) return;
    if (changed_only && memcmp(contents() + index, data, count) == 0) return;
    memcpy(s_sector, contents(), FLASH_SECTOR_SIZE);
    memcpy(s_sector + index, data, count);
    // Like the AVR API, writes are synchronous and have no error return value.
    const int result = flash_safe_execute(write_sector, nullptr, 1000);
    hard_assert(result == PICO_OK);
}

uint8_t read(int index) { return in_range(index, 1) ? contents()[index] : 0xff; }

void write(int index, uint8_t value) { write_bytes(index, &value, 1, false); }

void update(int index, const uint8_t *data, size_t count) { write_bytes(index, data, count, true); }

}  // namespace pico_eeprom
