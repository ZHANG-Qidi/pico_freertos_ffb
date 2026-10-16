#include "ffb_commander_setup.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "FFB_config.h"

static constexpr float k_pi = 3.14159265358979323846f;

static float g_constant_gain = CONFIG_FFB_CONSTANT_FORCE_GAIN;
static float g_damping_gain = CONFIG_FFB_DAMPING_GAIN;
static float g_damping_dr2_gain = CONFIG_FFB_DAMPING_DR2_GAIN;
// Total lock-to-lock turns; the control interface converts to half-range radians.
static float g_wheel_range_turns = CONFIG_FFB_WHEEL_HALF_RANGE_RAD / k_pi;

ffb::Commander ffb_commander(SerialUSB);

static constexpr int k_parameter_address = 0;

struct Parameters {
    float constant_gain;
    float damping_gain;
    float damping_dr2_gain;
    float wheel_range_turns;
};

static void load_parameters(void) {
    bool blank = true;
    for (size_t i = 0; i < sizeof(Parameters); ++i) {
        if (EEPROM.read(k_parameter_address + i) != 0xff) {
            blank = false;
            break;
        }
    }
    if (blank) return;  // Retain FFB_config.h defaults on first use.

    Parameters parameters;
    EEPROM.get(k_parameter_address, parameters);
    g_constant_gain = parameters.constant_gain;
    g_damping_gain = parameters.damping_gain;
    g_damping_dr2_gain = parameters.damping_dr2_gain;
    g_wheel_range_turns = parameters.wheel_range_turns;
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
    const Parameters parameters{g_constant_gain, g_damping_gain, g_damping_dr2_gain, g_wheel_range_turns};
    EEPROM.put(k_parameter_address, parameters);
    if (ffb_commander.verbose != ffb::nothing) {
        ffb_commander.com_port->println(F("saved"));
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
