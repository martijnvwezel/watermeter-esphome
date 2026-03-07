#pragma once
#include "esphome.h"

// Get the name of the current calibration state for logging purposes
const char* get_current_calibration_state_name();

// Process the sensor readings, update the calibration state, and calculate consumption and flow rate based on the detected patterns and fractions.
void process_readings();

// Reset the calibration to the initial state.
void reset_calibration_internal();

// Enums and structs for calibration state, signal state, and phase information
enum CalibrationState { UNINITIALIZED, UNCALIBRATED, PRELIMINARY_RANGE, STATES, RANGE, PATTERNS, PRELIMINARY_FRACTIONS, FRACTIONS };

enum SignalState { UNKNOWN, HIGH, FALLING, LOW, RISING };

struct PhaseInfo {
    uint32_t millis;
    uint32_t prev_millis;
    uint32_t prev_prev_millis;
    int      calibration_count;
};

// Global variables for calibration and state tracking
static struct PhaseInfo phase_info[6]           = {};
static uint32_t         last_phase_change       = 0;
static uint32_t         expected_phase_duration = 0;
static uint32_t         flow_start_time         = 0;
static int              edge_phase              = 0;
static SignalState      ah = UNKNOWN, bh = UNKNOWN, ch = UNKNOWN;
static int              total_crossings = 0;
static bool             all_extremes_known;
static bool             all_patterns_known;
static bool             all_fractions_known;
static CalibrationState calibrated = UNINITIALIZED;

/// Get the name of the current calibration state for logging purposes
///
/// @return the name of the calibration state, or nullptr if the state is invalid
const char* get_current_calibration_state_name() {
    switch (calibrated) {
        case UNINITIALIZED: return "Uninitialized";
        case UNCALIBRATED: return "Uncalibrated";
        case PRELIMINARY_RANGE: return "Preliminary Range Calibrated";
        case RANGE: return "Range Calibrated";
        case STATES: return "States Calibrated";
        case PATTERNS: return "Patterns Calibrated";
        case PRELIMINARY_FRACTIONS: return "Preliminary Fractions Calibrated";
        case FRACTIONS: return "Fully Calibrated";
    }
    return nullptr;
}

/// Update the calibration state and publish it to the sensor, only if the new state is higher than the current state.
///
/// @param new_state the new calibration state to update to
/// @returns void
void update_calibration_state(CalibrationState new_state) {
    if (calibrated < new_state || new_state == UNINITIALIZED) {
        calibrated = new_state;
        id(calibration_state).update();
        ESP_LOGD("log2csv", "calibration_state:%d", new_state);
        ESP_LOGI("calibration", get_current_calibration_state_name());
    }
};

/// Calculate the minimum average value based on the given parameters.
///
/// @param _min The current minimum value.
/// @param _max The current maximum value.
/// @param y The new value to consider.
/// @param alpha_cor The correction factor.
/// @return The updated minimum average value.
float min_average(float _min, float _max, float y, float alpha_cor) {
    if (_min == 0)
        return y;
    else if (y < _min + 0.10 * (_max - _min))
        return (1.0f - alpha_cor) * _min + alpha_cor * y;
    else
        return _min;
};

/// Calculate the maximum average value based on the given parameters.
///
/// @param _min The current minimum value.
/// @param _max The current maximum value.
/// @param y The new value to consider.
/// @param alpha_cor The correction factor.
/// @return The updated maximum average value.
float max_average(float _min, float _max, float y, float alpha_cor) {
    if (_max == 0)
        return y;
    else if (y > _max - 0.10 * (_max - _min))
        return (1.0f - alpha_cor) * _max + alpha_cor * y;
    else
        return _max;
};

/// Get the fraction value for the given index.
///
/// @param i The index of the fraction.
/// @return The value of the fraction.
float get_fraction(int i) {
    switch (i) {
        case 0: return id(fraction_0).state;
        case 1: return id(fraction_1).state;
        case 2: return id(fraction_2).state;
        case 3: return id(fraction_3).state;
        case 4: return id(fraction_4).state;
        case 5: return id(fraction_5).state;
        default: return id(fraction_0).state; // should never happen
    }
};

// Workaround to save template number value changes to flash, since publish_state doesn't work for template numbers.
#define set_value(id, value) \
    do { \
        auto call = id.make_call(); \
        call.set_value(value); \
        call.perform(); \
    } while (0)

/// Set the fraction value for the given index.
/// @param i The index of the fraction.
/// @param value The value to set for the fraction.
void set_fraction(int i, float value) {
    switch (i) {
        case 0: set_value(id(fraction_0), value); break;
        case 1: set_value(id(fraction_1), value); break;
        case 2: set_value(id(fraction_2), value); break;
        case 3: set_value(id(fraction_3), value); break;
        case 4: set_value(id(fraction_4), value); break;
        case 5: set_value(id(fraction_5), value); break;
        default: set_value(id(fraction_0), value); break; // should never happen
    }
};

/// Get the pattern value for the given index.
/// @param i The index of the pattern.
/// @return The value of the pattern.
int get_pattern(int i) {
    switch (i) {
        case 0: return id(pattern_0).state;
        case 1: return id(pattern_1).state;
        case 2: return id(pattern_2).state;
        case 3: return id(pattern_3).state;
        case 4: return id(pattern_4).state;
        case 5: return id(pattern_5).state;
        default: return id(pattern_0).state; // should never happen
    }
};

/// Set the pattern value for the given index.
///
/// @param i The index of the pattern.
/// @param value The value to set for the pattern.
void set_pattern(int i, int value) {
    switch (i) {
        case 0: set_value(id(pattern_0), value); break;
        case 1: set_value(id(pattern_1), value); break;
        case 2: set_value(id(pattern_2), value); break;
        case 3: set_value(id(pattern_3), value); break;
        case 4: set_value(id(pattern_4), value); break;
        case 5: set_value(id(pattern_5), value); break;
        default: set_value(id(pattern_0), value); break; // should never happen
    }
};

/// Reset the calibration to the initial state, and publish the reset state to all relevant sensors.
void reset_calibration_internal() {
    ESP_LOGI("calibration", "Resetting calibration to initial state.");
    edge_phase              = 0;
    last_phase_change       = 0;
    expected_phase_duration = 0;
    flow_start_time         = 0;
    total_crossings         = 0;

    ah = bh = ch = UNKNOWN;

    for (int i = 0; i < 6; ++i) {
        set_pattern(i, -1);
        set_fraction(i, -0.001f);
        phase_info[i] = {};
    }

    set_value(id(min_a), 0);
    set_value(id(min_b), 0);
    set_value(id(min_c), 0);
    set_value(id(max_a), 0);
    set_value(id(max_b), 0);
    set_value(id(max_c), 0);

    id(consumption_since_restart).publish_state(0);
    id(consumption_current).publish_state(0);
    id(flow_rate_current).publish_state(0);

    update_calibration_state(UNINITIALIZED);
};

/// Update the state of a signal based on its current state and a new value, using a hysteresis band to prevent rapid state changes due to noise.
///
/// @param current The current state of the signal.
/// @param val The new value to update the state with.
/// @return The updated state of the signal.
SignalState update_state(SignalState current, float val) {
    switch (current) {
        case HIGH: return val > 0.0f ? HIGH : FALLING;
        case LOW: return val < 0.0f ? LOW : RISING;
        case RISING: return val > HYSTERESIS_BAND ? HIGH : RISING;
        case FALLING: return val < -HYSTERESIS_BAND ? LOW : FALLING;
        case UNKNOWN: return val > HYSTERESIS_BAND ? HIGH : (val < -HYSTERESIS_BAND ? LOW : UNKNOWN);
        default: return UNKNOWN;
    }
};

/// Process the sensor readings, update the calibration state, and calculate consumption and flow rate based on the detected patterns and fractions.
void process_readings() {
    // wait until api is connected and then 5s for debugging purposes
    static int initial_counter = 0;
    if (WAIT_FOR_API_CONNECTION) {
        if (!api_is_connected()) {
            initial_counter = 0;
            return;
        } else if (initial_counter < 100) {
            initial_counter++;
            return;
        }
    }

    uint32_t now = millis();

    // Get signals and normalize them
    float a = id(light_sensor_a_diff).state;
    float b = id(light_sensor_b_diff).state;
    float c = id(light_sensor_c_diff).state;

    ESP_LOGD("log2csv", "al:%f bl:%f cl:%f ad:%f bd:%f cd:%f a:%f b:%f c:%f now:%d", id(light_sensor_a_light).state, id(light_sensor_b_light).state, id(light_sensor_c_light).state, id(light_sensor_a_dark).state, id(light_sensor_b_dark).state, id(light_sensor_c_dark).state, a, b, c, now);

    if (isnan(a) || isnan(b) || isnan(c)) {
        ESP_LOGD("log2csv", "Skipping reading due to NaN value(s): a:%f b:%f c:%f", a, b, c);
        return;
    }

    if (calibrated == UNINITIALIZED) {
        all_extremes_known  = id(min_a).state > 0.0f && id(max_a).state > 0.0f && id(min_b).state > 0.0f && id(max_b).state > 0.0f && id(min_c).state > 0.0f && id(max_c).state > 0.0f;
        all_patterns_known  = true;
        all_fractions_known = true;
        for (int i = 0; i < 6; ++i) {
            all_patterns_known  = all_patterns_known && get_pattern(i) >= 0;
            all_fractions_known = all_fractions_known && get_fraction(i) >= 0.0f;
        }

        ESP_LOGD("log2csv", "all_extremes_known:%d all_patterns_known:%d all_fractions_known:%d", all_extremes_known, all_patterns_known, all_fractions_known);
        if (all_extremes_known) {
            update_calibration_state(PRELIMINARY_RANGE);
        } else {
            update_calibration_state(UNCALIBRATED);
        }
    }

    // Full range calibration requires that the states are calibrated first (as otherwise cannot detect if enough state transitions have occured)
    if (!all_extremes_known && calibrated < RANGE) {
        // correction factor for min/max averaging, adjust as needed
        // 0.1 is reasonable during calibration, if continuous adjustment is needed, suggest using 0.001 after calibration
        float alpha_cor = 0.1f;

        id(min_a).state=min_average(id(min_a).state, id(max_a).state, a, alpha_cor);
        id(min_b).state=min_average(id(min_b).state, id(max_b).state, b, alpha_cor);
        id(min_c).state=min_average(id(min_c).state, id(max_c).state, c, alpha_cor);
        id(max_a).state=max_average(id(min_a).state, id(max_a).state, a, alpha_cor);
        id(max_b).state=max_average(id(min_b).state, id(max_b).state, b, alpha_cor);
        id(max_c).state=max_average(id(min_c).state, id(max_c).state, c, alpha_cor);

        ESP_LOGD("log2csv", "a_min:%f b_min:%f c_min:%f a_max:%f b_max:%f c_max:%f", id(min_a).state, id(min_b).state, id(min_c).state, id(max_a).state, id(max_b).state, id(max_c).state);

        if (id(max_a).state - id(min_a).state > MIN_RANGE && id(max_b).state - id(min_b).state > MIN_RANGE && id(max_c).state - id(min_c).state > MIN_RANGE) {
            bool changed = false;
            if (id(consumption_since_restart).state < CALIBRATION_LITERS) {
                if (calibrated < PRELIMINARY_RANGE) {
                    changed = true;
                    update_calibration_state(PRELIMINARY_RANGE);
                }
            } else {
                changed = true;
                update_calibration_state(RANGE);
            }

            if (changed) {
                set_value(id(min_a), id(min_a).state);
                set_value(id(min_b), id(min_b).state);
                set_value(id(min_c), id(min_c).state);
                set_value(id(max_a), id(max_a).state);
                set_value(id(max_b), id(max_b).state);
                set_value(id(max_c), id(max_c).state);
            }
        }
    }

    // State and pattern calibration requires that the range is calibrated first (as otherwise cannot reliably detect state transitions or patterns)
    if (calibrated < PRELIMINARY_RANGE) {
        return;
    }

    // Remove offset and scale to -1..1 range based on min/max
    a = 2.0f * (a - id(min_a).state) / (id(max_a).state - id(min_a).state) - 1.0f;
    b = 2.0f * (b - id(min_b).state) / (id(max_b).state - id(min_b).state) - 1.0f;
    c = 2.0f * (c - id(min_c).state) / (id(max_c).state - id(min_c).state) - 1.0f;

    ESP_LOGD("log2csv", "scaled_a:%f scaled_b:%f scaled_c:%f", a, b, c);

    bool prev_unknown = ah == UNKNOWN || bh == UNKNOWN || ch == UNKNOWN;
    int  prev_pattern = (ah == HIGH || ah == RISING ? 0b100 : 0) | (bh == HIGH || bh == RISING ? 0b010 : 0) | (ch == HIGH || ch == RISING ? 0b001 : 0);

    ah = update_state(ah, a), bh = update_state(bh, b), ch = update_state(ch, c);

    if (ah == UNKNOWN || bh == UNKNOWN || ch == UNKNOWN) {
        return;
    }
    
    int pattern = (ah == HIGH || ah == RISING ? 0b100 : 0) | (bh == HIGH || bh == RISING ? 0b010 : 0) | (ch == HIGH || ch == RISING ? 0b001 : 0);

    if (prev_unknown) {
        prev_pattern = pattern;
    }
    
    if (all_extremes_known) {
        if (all_patterns_known) {
            if (all_fractions_known) {
                update_calibration_state(FRACTIONS);
            } else {
                update_calibration_state(PATTERNS);
            }
        } else {
            update_calibration_state(RANGE);
        }
    } else {
        update_calibration_state(STATES);
    }


    int crossings;
    if ((pattern & 0b100) != (prev_pattern & 0b100))
        ++crossings;
    if ((pattern & 0b010) != (prev_pattern & 0b010))
        ++crossings;
    if ((pattern & 0b001) != (prev_pattern & 0b001))
        ++crossings;

    if (crossings > 0) {
        ESP_LOGD("log2csv", "pattern:%u prev_pattern:%u crossings:%u", pattern, prev_pattern, crossings);
    }

    for (int i = 0; i < crossings; ++i) {
        edge_phase = (edge_phase + 1) % 6;
        ESP_LOGD("log2csv", "edge_phase:%d", edge_phase, now);

        if (calibrated < PATTERNS) {

            if (!all_patterns_known) {
                set_pattern(edge_phase, pattern);
            }

            if (calibrated == PATTERNS - 1) {

                bool missing_pattern = false;
                for (int i = 0; i < 6; ++i) {
                    missing_pattern = missing_pattern || get_pattern(i) < 0;
                }

                // if not all patterns are detected yet, continue to next crossing
                if (missing_pattern) {
                    continue;
                }

                // shift table so pattern sequence with lowest number goes first, preserve
                // the relative order of the other entries instead of sorting.

                // find the index of the lowest pattern sequence
                int min_idx = 0;
                for (int i = 1; i < 6; ++i) {
                    // compare the full 6-phase sequence starting at `i` with the
                    // sequence starting at `min_idx`. pick the rotation whose
                    // next phases are lexicographically smallest
                    bool better = false;
                    for (int j = 0; j < 6; ++j) {
                        int pi = get_pattern((i + j) % 6);
                        int pm = get_pattern((min_idx + j) % 6);
                        if (pi < pm) {
                            better = true;
                            break;
                        }
                        if (pi > pm)
                            break;
                    }
                    if (better) {
                        min_idx = i;
                    }
                }

                // rotate the array so that the lowest pattern sequence is first, and update the edge_phase accordingly
                if (min_idx != 0) {
                    // rotate the array left by min_idx positions
                    PhaseInfo tmp[6];
                    int       tmp_pattern[6];
                    float     tmp_fraction[6];

                    for (int i = 0; i < 6; ++i) {
                        tmp[i]          = phase_info[(min_idx + i) % 6];
                        tmp_pattern[i]  = get_pattern((min_idx + i) % 6);
                        tmp_fraction[i] = get_fraction((min_idx + i) % 6);
                    }

                    for (int i = 0; i < 6; ++i) {
                        phase_info[i] = tmp[i];
                        set_pattern(i, tmp_pattern[i]);
                        set_fraction(i, tmp_fraction[i]);
                    }

                    // update the edge_phase to match the new ordering
                    edge_phase = (edge_phase - min_idx + 6) % 6;
                }

                update_calibration_state(PATTERNS);
            }
        }

        if (calibrated >= PATTERNS) {
            // The pattern at the detected edge should match the expected pattern for this phase;
            int old_edge_phase = edge_phase;
            int old_pattern    = get_pattern(edge_phase);
            if (pattern != get_pattern(edge_phase)) {
                // If not, try to find the phase that matches the detected pattern and has a previous pattern that matches the previous pattern (if available).
                for (int i = 0; i < 6; ++i) {
                    if (get_pattern(i) == pattern && (get_pattern((i - 1 + 6) % 6) < 0 || get_pattern((i - 1 + 6) % 6) == prev_pattern)) {
                        edge_phase = i;
                        break;
                    }
                }

                if (pattern != get_pattern(edge_phase)) {
                    ESP_LOGE("phase_detection", "Could not update phase from %d, because could not detect pattern %u. Redo calibration!", old_edge_phase, pattern);
                } else {
                    ESP_LOGW("phase_detection", "Updated phase from %d to %d based on detected pattern (%u). If this occurs often, consider recalibrating.", old_edge_phase, edge_phase, pattern);
                }
            }
        }

        if (calibrated == PRELIMINARY_FRACTIONS - 1 || calibrated == FRACTIONS - 1) {
            // update fraction based on duration of current phase compared to previous phase, with some tolerance to filter out outliers.
            // also require that the previous duration is similar to the current duration to prevent wrong fraction updates due to noise or misdetections.

            auto& ci = phase_info[edge_phase];
            auto& pi = phase_info[(edge_phase - 1 + 6) % 6];

            ci.prev_prev_millis = ci.prev_millis;
            ci.prev_millis      = ci.millis;
            ci.millis           = now;

            uint32_t cdur      = ci.millis - ci.prev_millis;
            uint32_t pdur      = pi.millis - pi.prev_millis;
            uint32_t prev_cdur = ci.prev_millis - ci.prev_prev_millis;

            if (ci.prev_prev_millis > 0 && pi.prev_millis > 0 && cdur >= pdur * 0.9f && cdur <= pdur * 1.1f && cdur >= prev_cdur * 0.9f && cdur <= prev_cdur * 1.1f) {
                float new_fraction = LITERS_PER_ROTATION * (ci.millis - pi.millis) / cdur;
                set_fraction(edge_phase, get_fraction(edge_phase) < 0 ? new_fraction : (0.8f * get_fraction(edge_phase) + 0.2f * new_fraction));
                ci.calibration_count++;

                ESP_LOGD("log2csv", "duration:%d prev_duration:%d fraction:%f calibration_count:%d", cdur, pdur, get_fraction(edge_phase), ci.calibration_count);
            }

            bool preliminarily_calibrated = true;
            bool fully_calibrated         = true;
            for (int i = 0; i < 6; ++i) {
                preliminarily_calibrated = preliminarily_calibrated && phase_info[i].calibration_count > 0;
                fully_calibrated         = fully_calibrated && phase_info[i].calibration_count >= CALIBRATION_FRACTION_COUNT;
            }
            if (preliminarily_calibrated) {
                // make certain all phases add up to LITERS_PER_ROTATION, to prevent drift in total consumption over time due to small errors in fraction estimation
                float total_liters = 0.0f;
                for (int i = 0; i < 6; ++i) {
                    total_liters += get_fraction(i);
                }

                float total_liters2 = 0.0f;
                for (int i = 0; i < 6; ++i) {
                    set_fraction(i, get_fraction(i) * LITERS_PER_ROTATION / total_liters);
                    total_liters2 += get_fraction(i);
                }

                // adjust the last fraction to make sure total is exactly LITERS_PER_ROTATION
                set_fraction(5, get_fraction(5) + (LITERS_PER_ROTATION - total_liters2));
                ESP_LOGD("log2csv", "total_liters:%f total_liters2:%f", total_liters, total_liters2);

                if (fully_calibrated) {
                    update_calibration_state(FRACTIONS);
                } else {
                    update_calibration_state(PRELIMINARY_FRACTIONS);
                }
            }

            for (int i = 0; i < 6; ++i) {
                ESP_LOGD("log2csv", "pattern%d:%u fraction%d:%f calibration_count%d:%d millis%d:%u prev_millis%d:%u dur%d:%u", i, get_pattern(i), i, get_fraction(i), i, phase_info[i].calibration_count, i, phase_info[i].millis, i, phase_info[i].prev_millis, i, phase_info[i].millis - phase_info[i].prev_millis);
            }
        }

        float liters_for_phase = calibrated < PRELIMINARY_FRACTIONS ? LITERS_PER_ROTATION / 6.0f : get_fraction(edge_phase);

        id(global_consumption_lifetime) += liters_for_phase;
        id(consumption_lifetime).publish_state(id(global_consumption_lifetime));
        id(consumption_lifetime_number).publish_state(id(global_consumption_lifetime));

        id(consumption_since_restart).publish_state(id(consumption_since_restart).state + liters_for_phase);
        id(consumption_current).publish_state(id(consumption_current).state + liters_for_phase);

        static float consumption_since_flow_start = 0.0f;
        consumption_since_flow_start += liters_for_phase;

        float flow_rate = 0.0f;

        if (flow_start_time > 0) {
            if (liters_for_phase > 0.0f) {
                // calculate flow rate based on the duration of the current phase and the expected liters for this phase, with some smoothing to filter out noise
                flow_rate = liters_for_phase / (now - last_phase_change) * 60000.0f; // L/min
                id(flow_rate_now).publish_state(id(flow_rate_now).state <= 0.0f ? flow_rate : (0.7f * id(flow_rate_now).state + 0.3f * flow_rate));
            }

            // on average only half of the first phase transition belongs to the current flow, so use 0.5 as a correction factor for the first phase to prevent overestimation of flow rate at the start of the flow.
            // for the consumption_current, we want to use the full amount as otherwise the totals do not add up
            id(flow_rate_current).publish_state(consumption_since_flow_start / (now - flow_start_time) * 60000.0f); // L/min
        }

        if (flow_start_time == 0) {
            flow_start_time              = now;
            consumption_since_flow_start = 0.0f;
        }

        if (liters_for_phase > 0.0f) {
            float next_liters_for_phase = calibrated < PRELIMINARY_FRACTIONS ? LITERS_PER_ROTATION / 6.0f : get_fraction((edge_phase + 1) % 6);
            expected_phase_duration     = (now - last_phase_change) / liters_for_phase * next_liters_for_phase;

            // add some tolerance to the expected phase duration during preliminary fraction calibration.
            if (calibrated < PRELIMINARY_FRACTIONS) {
                expected_phase_duration *= 4.0f;
            }
            ESP_LOGD("log2csv", "expected_phase_duration:%d next_liters_for_phase:%f", expected_phase_duration, liters_for_phase, next_liters_for_phase);
        }

        last_phase_change = now;

        ESP_LOGD("log2csv", "last_phase_change:%d liters_for_phase:%f flow_rate_inst:%f", last_phase_change, liters_for_phase, flow_rate);
        ESP_LOGD("log2csv", "consumption_lifetime:%f consumption_since_restart:%f consumption_current:%f consumption_previous:%f flow_rate_now:%f flow_rate_current:%f flow_rate_previous:%f", id(global_consumption_lifetime), id(consumption_since_restart).state, id(consumption_current).state, id(consumption_previous).state, id(flow_rate_now).state, id(flow_rate_current).state, id(flow_rate_previous).state);
    }

    // If the duration is more than the expected phase duration multiplied by the reset fraction then assume the flow has stopped.
    if (id(flow_rate_now).state > 0.0f || id(consumption_current).state > 0.0f || id(flow_rate_current).state > 0.0f) {
        uint32_t reset_time     = (uint32_t)((now - last_phase_change) * RESET_FRACTION);
        uint32_t max_reset_time = std::min(expected_phase_duration, (uint32_t)MAX_RESET_DURATION);
        ESP_LOGD("log2csv", "reset_time:%u reset_threshold:%u", reset_time, max_reset_time);
        if (reset_time > max_reset_time) {
            if (id(consumption_current).state != 0.0f) {
                id(consumption_previous).publish_state(id(consumption_current).state);
                id(flow_rate_previous).publish_state(id(flow_rate_current).state);
            }

            id(consumption_current).publish_state(0.0f);
            id(flow_rate_now).publish_state(0.0f);
            id(flow_rate_current).publish_state(0.0f);

            flow_start_time = 0;
            ESP_LOGD("log2csv", "flow_reset:1");
            ESP_LOGD("log2csv", "consumption_lifetime:%f consumption_since_restart:%f consumption_current:%f consumption_previous:%f flow_rate_now:%f flow_rate_current:%f flow_rate_previous:%f", id(global_consumption_lifetime), id(consumption_since_restart).state, id(consumption_current).state, id(consumption_previous).state, id(flow_rate_now).state, id(flow_rate_current).state, id(flow_rate_previous).state);
        }
    }

    if (id(debugmodus).state) {
        char json[256];

        snprintf(
            json, sizeof(json),
            "{"
            "\"time\":%d,"
            "\"al\":%.3f,\"bl\":%.3f,\"cl\":%.3f,"
            "\"ad\":%.3f,\"bd\":%.3f,\"cd\":%.3f,"
            "\"ami\":%.3f,\"bmi\":%.3f,\"cmi\":%.3f,"
            "\"ama\":%.3f,\"bma\":%.3f,\"cma\":%.3f,"
            "\"phs\":%d,"
            "\"tot\":%.2f,\"res\":%.2f,\"cur\":%.2f,\"prv\":%.2f,"
            "\"flw_cur\":%.2f,\"flw_prv\":%.2f"
            "}",
            millis(), id(light_sensor_a_light).state, id(light_sensor_b_light).state, id(light_sensor_c_light).state, id(light_sensor_a_dark).state, id(light_sensor_b_dark).state, id(light_sensor_c_dark).state, id(min_a).state, id(min_b).state, id(min_c).state, id(max_a).state, id(max_b).state, id(max_c).state, edge_phase, id(global_consumption_lifetime), id(consumption_since_restart).state, id(consumption_current).state, id(consumption_previous).state, id(flow_rate_current).state, id(flow_rate_previous).state);

        ESP_LOGI("debug_json", "Publishing debug json: %s", json);
        id(debug_json).publish_state(json);
    } else {
        id(debug_json).publish_state("");
    }
}