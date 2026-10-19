#include "ffb_commander_setup.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "FFB_config.h"

static constexpr float s_pi = 3.14159265358979323846f;

static float s_constant_force_gain = CONFIG_FFB_CONSTANT_FORCE_GAIN;
static float s_damping_gain = CONFIG_FFB_DAMPING_GAIN;
static float s_damping_dr2_gain = CONFIG_FFB_DAMPING_DR2_GAIN;
static float s_min_damping = CONFIG_FFB_MIN_DAMPING;
static float s_stop_force = CONFIG_FFB_STOP_FORCE;
// Total lock-to-lock turns; the control interface converts to half-range radians.
static float s_wheel_range_turns = CONFIG_FFB_WHEEL_HALF_RANGE_RAD / s_pi;

ffb::Commander g_ffb_commander(SerialUSB);

static constexpr int s_parameter_address = 0;

struct Parameters {
    float constant_force_gain;
    float damping_gain;
    float damping_dr2_gain;
    float min_damping;
    float stop_force;
    float wheel_range_turns;
};

static bool load_parameters(void) {
    bool blank = true;
    for (size_t i = 0; i < sizeof(Parameters); ++i) {
        if (EEPROM.read(s_parameter_address + i) != 0xff) {
            blank = false;
            break;
        }
    }
    if (blank) return false;  // Retain current values when EEPROM is empty.

    Parameters parameters;
    EEPROM.get(s_parameter_address, parameters);
    s_constant_force_gain = parameters.constant_force_gain;
    s_damping_gain = parameters.damping_gain;
    s_damping_dr2_gain = parameters.damping_dr2_gain;
    s_min_damping = parameters.min_damping;
    s_stop_force = parameters.stop_force;
    s_wheel_range_turns = parameters.wheel_range_turns;
    return true;
}

static void doConstantGain(char *cmd) { g_ffb_commander.scalar(&s_constant_force_gain, cmd); }
static void doDampingGain(char *cmd) { g_ffb_commander.scalar(&s_damping_gain, cmd); }
static void doDampingDr2Gain(char *cmd) { g_ffb_commander.scalar(&s_damping_dr2_gain, cmd); }
static void doMinDamping(char *cmd) { g_ffb_commander.scalar(&s_min_damping, cmd); }
static void doEndstopForce(char *cmd) { g_ffb_commander.scalar(&s_stop_force, cmd); }
static void doWheelRange(char *cmd) { g_ffb_commander.scalar(&s_wheel_range_turns, cmd); }
static void doLoad(char *) {
    const bool loaded = load_parameters();
    if (g_ffb_commander.verbose != ffb::nothing) {
        g_ffb_commander.com_port->println(loaded ? F("loaded") : F("empty"));
    }
}
static void doLoadDefaults(char *) {
    s_constant_force_gain = CONFIG_FFB_CONSTANT_FORCE_GAIN;
    s_damping_gain = CONFIG_FFB_DAMPING_GAIN;
    s_damping_dr2_gain = CONFIG_FFB_DAMPING_DR2_GAIN;
    s_min_damping = CONFIG_FFB_MIN_DAMPING;
    s_stop_force = CONFIG_FFB_STOP_FORCE;
    s_wheel_range_turns = CONFIG_FFB_WHEEL_HALF_RANGE_RAD / s_pi;
    if (g_ffb_commander.verbose != ffb::nothing) {
        g_ffb_commander.com_port->println(F("defaults loaded"));
    }
}
static void doSave(char *) {
    const Parameters parameters{s_constant_force_gain, s_damping_gain, s_damping_dr2_gain, s_min_damping, s_stop_force, s_wheel_range_turns};
    EEPROM.put(s_parameter_address, parameters);
    if (g_ffb_commander.verbose != ffb::nothing) {
        g_ffb_commander.com_port->println(F("saved"));
    }
}

void ffb_commander_setup(void) {
    load_parameters();
    SerialUSB.begin(CONFIG_FFB_COMMANDER_BAUD_RATE);
    g_ffb_commander.add('C', doConstantGain, "constant force gain");
    g_ffb_commander.add('D', doDampingGain, "damping gain");
    g_ffb_commander.add('R', doDampingDr2Gain, "DR2 damping gain");
    g_ffb_commander.add('M', doMinDamping, "minimum damping");
    g_ffb_commander.add('E', doEndstopForce, "endstop force");
    g_ffb_commander.add('A', doWheelRange, "wheel range (turns)");
    g_ffb_commander.add('L', doLoad, "load parameters");
    g_ffb_commander.add('F', doLoadDefaults, "load defaults");
    g_ffb_commander.add('S', doSave, "save parameters");
}

float ffb_commander_constant_gain(void) { return s_constant_force_gain; }
float ffb_commander_damping_gain(void) { return s_damping_gain; }
float ffb_commander_damping_dr2_gain(void) { return s_damping_dr2_gain; }
float ffb_commander_min_damping(void) { return s_min_damping; }
float ffb_commander_endstop_force(void) { return s_stop_force; }
float ffb_commander_wheel_half_range_rad(void) { return s_wheel_range_turns * s_pi; }
