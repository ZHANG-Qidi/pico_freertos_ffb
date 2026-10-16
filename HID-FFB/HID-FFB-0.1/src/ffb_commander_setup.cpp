#include "ffb_commander_setup.h"

#include <Arduino.h>

#include "FFB_config.h"

static constexpr float k_pi = 3.14159265358979323846f;

static float g_constant_gain = CONFIG_FFB_CONSTANT_FORCE_GAIN;
static float g_damping_gain = CONFIG_FFB_DAMPING_GAIN;
static float g_damping_dr2_gain = CONFIG_FFB_DAMPING_DR2_GAIN;
// Total lock-to-lock turns; the control interface converts to half-range radians.
static float g_wheel_range_turns = CONFIG_FFB_WHEEL_HALF_RANGE_RAD / k_pi;

ffb::Commander ffb_commander(SerialUSB);

static void doConstantGain(char *cmd) { ffb_commander.scalar(&g_constant_gain, cmd); }
static void doDampingGain(char *cmd) { ffb_commander.scalar(&g_damping_gain, cmd); }
static void doDampingDr2Gain(char *cmd) { ffb_commander.scalar(&g_damping_dr2_gain, cmd); }
static void doWheelRange(char *cmd) { ffb_commander.scalar(&g_wheel_range_turns, cmd); }

void ffb_commander_setup(void) {
    SerialUSB.begin(CONFIG_FFB_COMMANDER_BAUD_RATE);
    ffb_commander.add('C', doConstantGain, "constant force gain");
    ffb_commander.add('D', doDampingGain, "damping gain");
    ffb_commander.add('R', doDampingDr2Gain, "DR2 damping gain");
    ffb_commander.add('A', doWheelRange, "wheel range (turns)");
}

float ffb_commander_constant_gain(void) { return g_constant_gain; }
float ffb_commander_damping_gain(void) { return g_damping_gain; }
float ffb_commander_damping_dr2_gain(void) { return g_damping_dr2_gain; }
float ffb_commander_wheel_half_range_rad(void) { return g_wheel_range_turns * k_pi; }
