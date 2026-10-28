#include "foc_setup.h"

#include "SimpleFOC.h"

// magnetic sensor
#if defined(CONFIG_FFB_MagneticSensorSPI)
// magnetic sensor instance - SPI
MagneticSensorSPI g_sensor = MagneticSensorSPI(AS5147_SPI, SPI_MASTER_CS_IO);
#endif
#if defined(CONFIG_FFB_MagneticSensorIIC)
// magnetic sensor instance - I2C
MagneticSensorI2C g_sensor = MagneticSensorI2C(AS5600_I2C);
#endif
// BLDC motor & driver instance
BLDCMotor g_motor = BLDCMotor(BLDC_MOTOR_PP);
static BLDCDriver3PWM s_driver = BLDCDriver3PWM(MOTOR_U, MOTOR_V, MOTOR_W, MOTOR_EN);
void foc_setup(void) {
    // initialise magnetic sensor hardware
    g_sensor.init();
    // link the motor to the sensor
    g_motor.linkSensor(&g_sensor);
    // power supply voltage
    s_driver.voltage_power_supply = VOLTAGE_POWER;
    s_driver.voltage_limit = VOLTAGE_LIMIT;
    s_driver.init();
    g_motor.linkDriver(&s_driver);
    // aligning voltage
    g_motor.voltage_sensor_align = VOLTAGE_SENSOR_ALIGN;
    // choose FOC modulation (optional)
    g_motor.foc_modulation = FOCModulationType::SVPWM;
    // set motion control loop to be used
    g_motor.controller = MotionControlType::torque;
    // set torque control loop to be used
    g_motor.torque_controller = TorqueControlType::voltage;
    // use monitoring with serial
    SerialUSB.begin(COMMANDER_BAUD_RATE);
    // comment out if not needed
    g_motor.useMonitoring(SerialUSB);
    // initialize motor
    g_motor.init();
    // align sensor and start FOC
    g_motor.initFOC();
    SerialUSB.println(F("Motor ready."));
    delay(1000);
}
