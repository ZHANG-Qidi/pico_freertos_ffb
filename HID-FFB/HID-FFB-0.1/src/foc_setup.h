#ifndef _FOC_SETUP_H_
#define _FOC_SETUP_H_
#include "SimpleFOC.h"
#ifdef __cplusplus
extern "C" {
#endif
#define BLDC_MOTOR_PP (7)
#define VOLTAGE_POWER (9.0f)
#define VOLTAGE_LIMIT (6.0f)
#define VOLTAGE_SENSOR_ALIGN (1.0f)
#define COMMANDER_BAUD_RATE (115200)
#define MOTOR_U (10)
#define MOTOR_V (11)
#define MOTOR_W (12)
#define MOTOR_EN (13)
extern BLDCMotor motor;
extern MagneticSensorSPI sensor;
extern void foc_setup(void);
#ifdef __cplusplus
}
#endif
#endif
