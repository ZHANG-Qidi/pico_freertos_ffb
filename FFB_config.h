#ifndef _FFB_CONFIG_H_
#define _FFB_CONFIG_H_

// Force-feedback device and motor-control tuning. These values are independent
// of the MCU peripheral and GPIO assignments in Arduino_config.h.

#define CONFIG_FFB_PI 3.14159265358979323846f

// Motor and sensor setup.
#define CONFIG_FFB_MOTOR_POLE_PAIRS 7
#define CONFIG_FFB_POWER_SUPPLY_VOLTAGE 9.0f
#define CONFIG_FFB_MOTOR_VOLTAGE_LIMIT 6.0f
#define CONFIG_FFB_SENSOR_ALIGN_VOLTAGE 1.0f
#define CONFIG_FFB_COMMANDER_BAUD_RATE 115200
#define CONFIG_FFB_MagneticSensorSPI
// #define CONFIG_FFB_MagneticSensorIIC

// Control-loop timing and steering travel.
#define CONFIG_FFB_FOC_LOOP_PERIOD_US 200.0f
#define CONFIG_FFB_DAMPING_MAX_VELOCITY_RAD_S (6.0f * CONFIG_FFB_PI)
#define CONFIG_FFB_WHEEL_RANGE_DEG 540.0f
#define CONFIG_FFB_WHEEL_HALF_RANGE_RAD (CONFIG_FFB_WHEEL_RANGE_DEG * CONFIG_FFB_PI / 360.0f)
#define CONFIG_FFB_STOP_ZONE_RAD (10.0f / 180.0f * CONFIG_FFB_PI)
#define CONFIG_FFB_STOP_FORCE 0.66f

// Force-feedback output tuning.
#define CONFIG_FFB_CONSTANT_FORCE_GAIN 0.5f
#define CONFIG_FFB_DAMPING_GAIN 0.5f
#define CONFIG_FFB_DAMPING_GAIN_DR2 0.5f
#define CONFIG_FFB_DAMPING_MIN 0.05f

#endif
