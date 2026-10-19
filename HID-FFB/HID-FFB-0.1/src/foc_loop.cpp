#include "foc_loop.h"

#include "SimpleFOC.h"
#include "esp_log.h"
#include "ffb_commander_setup.h"
#include "ffb_loop.h"
#include "ffb_setup.h"
#include "foc_setup.h"
// static const char *TAG = "foc_loop";
static float s_wheel_rad;
void foc_output(float *wheel_rad) { *wheel_rad = s_wheel_rad; }
void foc_loop(void) {
    // main FOC algorithm function
    // the faster you run this function the better
    // Arduino UNO loop  ~1kHz
    // Bluepill loop ~10kHz
    g_motor.loopFOC();
    float damper;
    float constant_force;
    ffb_output(&constant_force, &damper);
    float damping = damper * g_motor.shaft_velocity / DAMPING_MAX_VELOCITY;
    float torque_ratio = constant_force - damping;
    float angle = g_motor.shaft_angle;
    const float wheel_half = ffb_commander_wheel_half_range_rad();
    if (angle < -wheel_half) {
        float x = (-wheel_half - angle) / STOP_ZONE;
        x = x > 1.0f ? 1.0f : x;
        float stop_force = x * x * ffb_commander_endstop_force();
        torque_ratio += stop_force;
    } else if (angle > wheel_half) {
        float x = (angle - wheel_half) / STOP_ZONE;
        x = x > 1.0f ? 1.0f : x;
        float stop_force = x * x * ffb_commander_endstop_force();
        torque_ratio -= stop_force;
    }
    torque_ratio = torque_ratio > 1.0f ? 1.0f : (torque_ratio < -1.0f ? -1.0f : torque_ratio);
    // voltage set point variable
    float target_voltage = VOLTAGE_LIMIT * torque_ratio;
    // Motion control function
    // current_velocity, position or voltage (defined in motor.controller)
    // this function can be run at much lower frequency than loopFOC() function
    // You can also use motor.move() and set the motor.target in the code
    g_motor.move(target_voltage);
    s_wheel_rad = g_sensor.getAngle() * SENSOR_DIRECTION;
    static int dividerCounter;
    const int dividerFactor = 10.f * 1000.f / FOC_LOOP_PERIOD;
    if (++dividerCounter < dividerFactor) {
        return;
    }
    dividerCounter = 0;
    // ESP_LOGI(TAG, "damper = %f, constant_force = %f", damper, constant_force);
}
