#ifndef _FFB_COMMANDER_SETUP_H_
#define _FFB_COMMANDER_SETUP_H_

#ifdef __cplusplus
#include "ffb_commander.h"

extern ffb::Commander ffb_commander;

extern "C" {
#endif

// Load saved parameters and register commands before starting worker tasks.
void ffb_commander_setup(void);

// Runtime force-feedback tuning, loaded from flash or initialized from FFB_config.h.
float ffb_commander_constant_gain(void);
float ffb_commander_damping_gain(void);
float ffb_commander_damping_dr2_gain(void);
// Convert Commander total turns to the single-sided range used by FOC and USB.
float ffb_commander_wheel_half_range_rad(void);

#ifdef __cplusplus
}
#endif

#endif
