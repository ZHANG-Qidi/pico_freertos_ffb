#include "ffb_loop.h"

#include "esp_log.h"
#include "ffb_commander_setup.h"
#include "ffb_setup.h"
static const char *s_tag = "ffb_loop";
static float s_constant_force;
static float s_damper;
void ffb_output(float *constant_force, float *damper) {
    *constant_force = s_constant_force;
    const float minimum = ffb_commander_min_damping();
    *damper = s_damper < minimum ? minimum : s_damper;
}
void ffb_loop(void) {
    for (int i = 0; i < FFB_EFFECT_COUNT; i++) {
        if (g_effect_pool[i].allocated == BLOCK_FREE) {
            continue;
        }
        if (g_effect_pool[i].operation_report.operation == EFFECT_STOP) {
            continue;
        }
        // float duration_ms = g_effect_pool[i].effect_report.duration * 0.1f;
        // float trigger_repeat_interval_ms = g_effect_pool[i].effect_report.trigger_repeat_interval * 0.1f;
        // float sample_period_ms = g_effect_pool[i].effect_report.sample_period * 0.1f;
        // float direction_deg = g_effect_pool[i].effect_report.direction[0] / 255.0f * 360.0f;
        float gain_effect_scale = g_effect_pool[i].effect_report.gain / GAIN_EFFECT_LOGICAL_MAX;
        float gain_device_scale = g_gain_device / GAIN_DEVICE_LOGICAL_MAX;
        switch (g_effect_pool[i].effect_report.type) {
            case ET_CONSTANT: {
                int16_t magnitude = g_effect_pool[i].constant_force_report.magnitude;
                float constant_force = magnitude * gain_effect_scale * gain_device_scale / CONSTANT_MANITUDE_MAX * ffb_commander_constant_gain();
                s_constant_force = constant_force;
                // ESP_LOGI(s_tag, "ET_CONSTANT: %f", s_constant_force);
                break;
            }
            case ET_DAMPER: {
                float damper = (float)g_effect_pool[i].condition_report.positive_coefficient / (float)g_effect_pool[i].condition_report.positive_saturation * ffb_commander_damping_gain();
                s_damper = damper;
                // ESP_LOGI(s_tag, "ET_DAMPER: %f", s_damper);
                break;
            }
            case ET_DAMPER_DR2: {
                float damper = (float)g_effect_pool[i].condition_report.positive_coefficient / (float)g_effect_pool[i].condition_report.positive_saturation * ffb_commander_damping_dr2_gain();
                s_damper = damper;
                // ESP_LOGI(s_tag, "ET_DAMPER: %f", s_damper);
                break;
            }
            default: {
                ESP_LOGI(s_tag, "Unimplemented Effect: %d", g_effect_pool[i].effect_report.type);
                break;
            }
        }
    }
}
