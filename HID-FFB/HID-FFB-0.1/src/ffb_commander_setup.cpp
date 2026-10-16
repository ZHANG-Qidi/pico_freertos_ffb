#include "ffb_commander_setup.h"

#include <Arduino.h>
#include <stddef.h>
#include <string.h>

#include "FFB_config.h"
#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "pico/flash.h"

static constexpr float k_pi = 3.14159265358979323846f;

static float g_constant_gain = CONFIG_FFB_CONSTANT_FORCE_GAIN;
static float g_damping_gain = CONFIG_FFB_DAMPING_GAIN;
static float g_damping_dr2_gain = CONFIG_FFB_DAMPING_DR2_GAIN;
// Total lock-to-lock turns; the control interface converts to half-range radians.
static float g_wheel_range_turns = CONFIG_FFB_WHEEL_HALF_RANGE_RAD / k_pi;

ffb::Commander ffb_commander(SerialUSB);

// Two independent sectors retain the previous configuration during a save.
static constexpr uint32_t k_slot_offsets[] = {FFB_PARAMETER_FLASH_OFFSET, FFB_PARAMETER_FLASH_OFFSET + FLASH_SECTOR_SIZE};
static constexpr uint32_t k_record_magic = 0x46464250;
static constexpr uint32_t k_record_version = 1;

struct SavedParameters {
    uint32_t magic;
    uint32_t version;
    uint32_t sequence;
    float constant_gain;
    float damping_gain;
    float damping_dr2_gain;
    float wheel_range_turns;
    uint32_t checksum;
};

static_assert(sizeof(SavedParameters) == 32, "Parameter record layout changed; update the version");
static_assert(sizeof(SavedParameters) <= FLASH_PAGE_SIZE, "Parameter record must fit in one page");
static_assert(FFB_PARAMETER_FLASH_OFFSET % FLASH_SECTOR_SIZE == 0, "Parameter slots must be sector aligned");
static_assert(FFB_PARAMETER_FLASH_OFFSET + 2 * FLASH_SECTOR_SIZE == PICO_FLASH_SIZE_BYTES, "Parameter slots must occupy the last two flash sectors");

static int g_saved_slot = -1;
static uint32_t g_saved_sequence = 0;

static uint32_t parameter_checksum(const SavedParameters &record) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&record);
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0; i < offsetof(SavedParameters, checksum); ++i) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
        }
    }
    return ~crc;
}

static const uint8_t *slot_contents(int slot) { return reinterpret_cast<const uint8_t *>(XIP_BASE + k_slot_offsets[slot]); }

static bool read_record(int slot, SavedParameters &record) {
    memcpy(&record, slot_contents(slot), sizeof(record));
    return record.magic == k_record_magic && record.version == k_record_version && record.checksum == parameter_checksum(record);
}

static void load_parameters(void) {
    SavedParameters records[2];
    const bool valid_a = read_record(0, records[0]);
    const bool valid_b = read_record(1, records[1]);
    if (!valid_a && !valid_b) return;  // Retain FFB_config.h defaults.

    // Unsigned subtraction also handles sequence wraparound.
    const uint32_t difference = records[1].sequence - records[0].sequence;
    const int slot = !valid_a ? 1 : (valid_b && difference != 0 && difference < 0x80000000u ? 1 : 0);
    const SavedParameters &record = records[slot];
    g_constant_gain = record.constant_gain;
    g_damping_gain = record.damping_gain;
    g_damping_dr2_gain = record.damping_dr2_gain;
    g_wheel_range_turns = record.wheel_range_turns;
    g_saved_slot = slot;
    g_saved_sequence = record.sequence;
}

struct FlashWrite {
    uint32_t offset;
    uint8_t page[FLASH_PAGE_SIZE];
};

// flash_safe_execute() excludes flash access by interrupts during these calls.
static void write_parameter_page(void *argument) {
    const FlashWrite *write = static_cast<const FlashWrite *>(argument);
    flash_range_erase(write->offset, FLASH_SECTOR_SIZE);
    flash_range_program(write->offset, write->page, FLASH_PAGE_SIZE);
}

static bool save_parameters(void) {
    const int slot = g_saved_slot == 0 ? 1 : 0;
    SavedParameters record{};
    record.magic = k_record_magic;
    record.version = k_record_version;
    record.sequence = g_saved_sequence + 1;
    record.constant_gain = g_constant_gain;
    record.damping_gain = g_damping_gain;
    record.damping_dr2_gain = g_damping_dr2_gain;
    record.wheel_range_turns = g_wheel_range_turns;
    record.checksum = parameter_checksum(record);

    FlashWrite write;
    write.offset = k_slot_offsets[slot];
    memset(write.page, 0xff, sizeof(write.page));
    memcpy(write.page, &record, sizeof(record));
    if (flash_safe_execute(write_parameter_page, &write, 1000) != PICO_OK) return false;

    SavedParameters verified;
    if (!read_record(slot, verified) || memcmp(write.page, slot_contents(slot), FLASH_PAGE_SIZE) != 0) return false;
    g_saved_slot = slot;
    g_saved_sequence = record.sequence;
    return true;
}

static void doConstantGain(char *cmd) { ffb_commander.scalar(&g_constant_gain, cmd); }
static void doDampingGain(char *cmd) { ffb_commander.scalar(&g_damping_gain, cmd); }
static void doDampingDr2Gain(char *cmd) { ffb_commander.scalar(&g_damping_dr2_gain, cmd); }
static void doWheelRange(char *cmd) { ffb_commander.scalar(&g_wheel_range_turns, cmd); }
static void doLoadDefaults(char *) {
    g_constant_gain = CONFIG_FFB_CONSTANT_FORCE_GAIN;
    g_damping_gain = CONFIG_FFB_DAMPING_GAIN;
    g_damping_dr2_gain = CONFIG_FFB_DAMPING_DR2_GAIN;
    g_wheel_range_turns = CONFIG_FFB_WHEEL_HALF_RANGE_RAD / k_pi;
    if (ffb_commander.verbose != ffb::nothing) {
        ffb_commander.com_port->println(F("defaults loaded"));
    }
}

static void doSave(char *) {
    // Saving is requested while the motor is idle; FOC may pause during flash access.
    const bool saved = save_parameters();
    if (ffb_commander.verbose != ffb::nothing) {
        ffb_commander.com_port->println(saved ? F("saved") : F("err"));
    }
}

void ffb_commander_setup(void) {
    load_parameters();
    SerialUSB.begin(CONFIG_FFB_COMMANDER_BAUD_RATE);
    ffb_commander.add('C', doConstantGain, "constant force gain");
    ffb_commander.add('D', doDampingGain, "damping gain");
    ffb_commander.add('R', doDampingDr2Gain, "DR2 damping gain");
    ffb_commander.add('A', doWheelRange, "wheel range (turns)");
    ffb_commander.add('L', doLoadDefaults, "load defaults");
    ffb_commander.add('S', doSave, "save parameters");
}

float ffb_commander_constant_gain(void) { return g_constant_gain; }
float ffb_commander_damping_gain(void) { return g_damping_gain; }
float ffb_commander_damping_dr2_gain(void) { return g_damping_dr2_gain; }
float ffb_commander_wheel_half_range_rad(void) { return g_wheel_range_turns * k_pi; }
