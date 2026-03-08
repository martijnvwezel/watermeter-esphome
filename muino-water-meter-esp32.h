#pragma once

// Muino Water Meter ESP32 Code
// This code is designed for an ESP32-based water meter sensor using light sensors to detect flow.
// It includes calibration options, flow rate and consumption calculations, and a debug mode for troubleshooting.
//
// Written by:
//  - Martijn van Wezel (@martijnvwezel)
//  - Arjan Mels (@arjanmels)

#include "esphome.h"

// Get the name of the current calibration state for logging purposes
const char* get_current_calibration_state_name();

// Process the sensor readings, update the calibration state, and calculate consumption and flow rate based on the detected patterns and fractions.
void process_readings();

// Reset the calibration to the initial state.
void reset_calibration_internal();


// Constants for calibration

// Smoothing weight for min/max calibration, between 0 and 1. Higher values give more weight to new readings, lower values give more weight to historical values.
constexpr float SENSOR_SMOOTHING_WEIGHT                        = 0.1f;
// Hysteresis band for signal state changes, as a fraction of the range between min and max.
constexpr float SENSOR_SMOOTHING_BAND                          = 0.1f;
// Range for the duration match when calibrating fractions, as a fraction of the expected duration.
constexpr float DURATION_MATCH_DEVIATION                       = 0.1f;
// Smoothing weight for flow rate calculation, between 0 and 1. Higher values give more weight to new readings, lower values give more weight to historical values.
constexpr float FLOW_RATE_SMOOTHING_WEIGHT                     = 0.3f;
// Smoothing weight for fraction calibration, between 0 and 1. Higher values give more weight to new readings, lower values give more weight to historical values.
constexpr float FRACTION_SMOOTHING_WEIGHT                      = 0.3f;
// Multiplier for expected phase duration during preliminary fraction calibration, to allow more time for calibration before patterns are fully known.
constexpr float PRELIMINARY_EXPECTED_PHASE_DURATION_MULTIPLIER = 4.0f;

// Constant for converting milliseconds to minutes.
constexpr float MILLIS_IN_MINUTE = 60000.0f;

// Constants for phase and pattern detection
constexpr int   PHASE_COUNT                                    = 6;
constexpr int   PATTERN_BIT_A                                  = 0b100;
constexpr int   PATTERN_BIT_B                                  = 0b010;
constexpr int   PATTERN_BIT_C                                  = 0b001;

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
static struct PhaseInfo phase_info[PHASE_COUNT]      = {};
static uint32_t         last_phase_change            = 0;
static uint32_t         expected_phase_duration      = 0;
static uint32_t         flow_start_time              = 0;
static float            consumption_since_flow_start = 0.0f;
static int              edge_phase                   = 0;
static SignalState      ah = UNKNOWN, bh = UNKNOWN, ch = UNKNOWN;
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
    else if (y < _min + SENSOR_SMOOTHING_BAND * (_max - _min))
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
    else if (y > _max - SENSOR_SMOOTHING_BAND * (_max - _min))
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
template<typename TNumber, typename TValue>
void set_value(TNumber& number, TValue value) {
    auto call = number.make_call();
    call.set_value(value);
    call.perform();
}

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
    edge_phase                   = 0;
    last_phase_change            = 0;
    expected_phase_duration      = 0;
    flow_start_time              = 0;
    consumption_since_flow_start = 0.0f;

    ah = bh = ch = UNKNOWN;

    for (int i = 0; i < PHASE_COUNT; ++i) {
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

/// Wait until the API is ready.
///
/// @return True if the API is ready, false otherwise.
bool wait_until_api_ready() {
    static int initial_counter = 0;
    if (!WAIT_FOR_API_CONNECTION) {
        return true;
    }

    if (!api_is_connected()) {
        initial_counter = 0;
        return false;
    }

    if (initial_counter < 100) {
        ++initial_counter;
        return false;
    }

    return true;
}

/// Initialize the calibration flags based on the current sensor states, and update the calibration state accordingly.
void initialize_calibration_flags() {
    all_extremes_known  = id(min_a).state > 0.0f && id(max_a).state > 0.0f && id(min_b).state > 0.0f && id(max_b).state > 0.0f && id(min_c).state > 0.0f && id(max_c).state > 0.0f;
    all_patterns_known  = true;
    all_fractions_known = true;

    for (int i = 0; i < PHASE_COUNT; ++i) {
        all_patterns_known  = all_patterns_known && get_pattern(i) >= 0;
        all_fractions_known = all_fractions_known && get_fraction(i) >= 0.0f;
    }

    ESP_LOGD("log2csv", "all_extremes_known:%d all_patterns_known:%d all_fractions_known:%d", all_extremes_known, all_patterns_known, all_fractions_known);
    update_calibration_state(all_extremes_known ? PRELIMINARY_RANGE : UNCALIBRATED);
}

/// Calibrate the range (min/max)of the sensor values.
///
/// @param a The new value for sensor A.
/// @param b The new value for sensor B.
/// @param c The new value for sensor C.
void calibrate_range(float a, float b, float c) {
    id(min_a).state = min_average(id(min_a).state, id(max_a).state, a, SENSOR_SMOOTHING_WEIGHT);
    id(min_b).state = min_average(id(min_b).state, id(max_b).state, b, SENSOR_SMOOTHING_WEIGHT);
    id(min_c).state = min_average(id(min_c).state, id(max_c).state, c, SENSOR_SMOOTHING_WEIGHT);
    id(max_a).state = max_average(id(min_a).state, id(max_a).state, a, SENSOR_SMOOTHING_WEIGHT);
    id(max_b).state = max_average(id(min_b).state, id(max_b).state, b, SENSOR_SMOOTHING_WEIGHT);
    id(max_c).state = max_average(id(min_c).state, id(max_c).state, c, SENSOR_SMOOTHING_WEIGHT);

    ESP_LOGD("log2csv", "a_min:%f b_min:%f c_min:%f a_max:%f b_max:%f c_max:%f", id(min_a).state, id(min_b).state, id(min_c).state, id(max_a).state, id(max_b).state, id(max_c).state);

    if (id(max_a).state - id(min_a).state <= MIN_RANGE || id(max_b).state - id(min_b).state <= MIN_RANGE || id(max_c).state - id(min_c).state <= MIN_RANGE) {
        return;
    }

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

    if (!changed) {
        return;
    }

    set_value(id(min_a), id(min_a).state);
    set_value(id(min_b), id(min_b).state);
    set_value(id(min_c), id(min_c).state);
    set_value(id(max_a), id(max_a).state);
    set_value(id(max_b), id(max_b).state);
    set_value(id(max_c), id(max_c).state);
}

/// @brief Determine the pattern from the given signal states.
///
/// @param sa The state of signal A.
/// @param sb The state of signal B.
/// @param sc The state of signal C.
/// @return The pattern represented by the signal states, or -1 if any state is unknown.
int pattern_from_states(SignalState sa, SignalState sb, SignalState sc) {
    if (sa == UNKNOWN || sb == UNKNOWN || sc == UNKNOWN) {
        return -1;
    }
    return (sa == HIGH || sa == RISING ? PATTERN_BIT_A : 0) | (sb == HIGH || sb == RISING ? PATTERN_BIT_B : 0) | (sc == HIGH || sc == RISING ? PATTERN_BIT_C : 0);
}

/// Count the number of crossings between two patterns.
///
/// @param prev_pattern The previous pattern.
/// @param pattern The current pattern.
/// @return The number of crossings between the two patterns.
int count_crossings(int prev_pattern, int pattern) {
    int crossings = 0;
    if ((pattern & PATTERN_BIT_A) != (prev_pattern & PATTERN_BIT_A))
        ++crossings;
    if ((pattern & PATTERN_BIT_B) != (prev_pattern & PATTERN_BIT_B))
        ++crossings;
    if ((pattern & PATTERN_BIT_C) != (prev_pattern & PATTERN_BIT_C))
        ++crossings;
    return crossings;
}

/// Update the calibration state based on the known data about extremes, patterns, and fractions.
void update_calibration_state_from_known_data() {
    if (all_extremes_known) {
        if (all_patterns_known) {
            update_calibration_state(all_fractions_known ? FRACTIONS : PATTERNS);
        } else {
            update_calibration_state(RANGE);
        }
    } else {
        update_calibration_state(STATES);
    }
}

/// Rotate the phase tables to the left by the given number of positions.
///
/// @param shift The number of positions to rotate the phase tables to the left.
void rotate_phase_tables_left(int shift) {
    PhaseInfo tmp_phase[PHASE_COUNT];
    int       tmp_pattern[PHASE_COUNT];
    float     tmp_fraction[PHASE_COUNT];

    for (int i = 0; i < PHASE_COUNT; ++i) {
        tmp_phase[i]    = phase_info[(shift + i) % PHASE_COUNT];
        tmp_pattern[i]  = get_pattern((shift + i) % PHASE_COUNT);
        tmp_fraction[i] = get_fraction((shift + i) % PHASE_COUNT);
    }

    for (int i = 0; i < PHASE_COUNT; ++i) {
        phase_info[i] = tmp_phase[i];
        set_pattern(i, tmp_pattern[i]);
        set_fraction(i, tmp_fraction[i]);
    }
}

/// Update the pattern calibration based on the known patterns, and rotate the phase tables if necessary to align with the detected patterns.
void update_pattern_calibration() {
    bool missing_pattern = false;
    for (int i = 0; i < PHASE_COUNT; ++i) {
        missing_pattern = missing_pattern || get_pattern(i) < 0;
    }
    if (missing_pattern) {
        return;
    }

    int min_idx = 0;
    for (int i = 1; i < PHASE_COUNT; ++i) {
        bool better = false;
        for (int j = 0; j < PHASE_COUNT; ++j) {
            const int pi = get_pattern((i + j) % PHASE_COUNT);
            const int pm = get_pattern((min_idx + j) % PHASE_COUNT);
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

    if (min_idx != 0) {
        rotate_phase_tables_left(min_idx);
        edge_phase = (edge_phase - min_idx + PHASE_COUNT) % PHASE_COUNT;
    }

    update_calibration_state(PATTERNS);
}

/// Align the edge phase to the detected pattern. 
/// If the detected pattern does not match the expected pattern for the current edge phase, 
/// search for the phase that matches the detected pattern and has a valid previous pattern, and update the edge phase accordingly. 
/// If no matching phase is found, log an error message.
/// 
/// @param pattern The detected pattern.
/// @param prev_pattern The previous pattern.
void align_edge_phase_to_pattern(int pattern, int prev_pattern) {
    if (calibrated < PATTERNS || pattern == get_pattern(edge_phase)) {
        return;
    }

    const int old_edge_phase = edge_phase;
    for (int i = 0; i < PHASE_COUNT; ++i) {
        if (get_pattern(i) == pattern && (get_pattern((i - 1 + PHASE_COUNT) % PHASE_COUNT) < 0 || get_pattern((i - 1 + PHASE_COUNT) % PHASE_COUNT) == prev_pattern)) {
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

/// Update the fraction calibration based on the detected patterns and durations.
/// 
/// @param now The current time in milliseconds.
void update_fraction_calibration(uint32_t now) {
    auto& ci = phase_info[edge_phase];
    auto& pi = phase_info[(edge_phase - 1 + PHASE_COUNT) % PHASE_COUNT];

    ci.prev_prev_millis = ci.prev_millis;
    ci.prev_millis      = ci.millis;
    ci.millis           = now;

    const uint32_t cdur      = ci.millis - ci.prev_millis;
    const uint32_t pdur      = pi.millis - pi.prev_millis;
    const uint32_t prev_cdur = ci.prev_millis - ci.prev_prev_millis;

    if (ci.prev_prev_millis > 0 && pi.prev_millis > 0 && cdur >= pdur * (1.0f-DURATION_MATCH_DEVIATION) && cdur <= pdur * (1.0f+DURATION_MATCH_DEVIATION) && cdur >= prev_cdur * (1.0f-DURATION_MATCH_DEVIATION) && cdur <= prev_cdur * (1.0f+DURATION_MATCH_DEVIATION)) {
        const float new_fraction = LITERS_PER_ROTATION * (ci.millis - pi.millis) / cdur;
        set_fraction(edge_phase, get_fraction(edge_phase) < 0 ? new_fraction : ((1.0f-FRACTION_SMOOTHING_WEIGHT) * get_fraction(edge_phase) + FRACTION_SMOOTHING_WEIGHT * new_fraction));
        ci.calibration_count++;

        ESP_LOGD("log2csv", "duration:%d prev_duration:%d fraction:%f calibration_count:%d", cdur, pdur, get_fraction(edge_phase), ci.calibration_count);
    }

    bool preliminarily_calibrated = true;
    bool fully_calibrated         = true;
    for (int i = 0; i < PHASE_COUNT; ++i) {
        preliminarily_calibrated = preliminarily_calibrated && phase_info[i].calibration_count > 0;
        fully_calibrated         = fully_calibrated && phase_info[i].calibration_count >= CALIBRATION_FRACTION_COUNT;
    }

    if (preliminarily_calibrated) {
        float total_liters = 0.0f;
        for (int i = 0; i < PHASE_COUNT; ++i) {
            total_liters += get_fraction(i);
        }

        float total_liters2 = 0.0f;
        for (int i = 0; i < PHASE_COUNT; ++i) {
            set_fraction(i, get_fraction(i) * LITERS_PER_ROTATION / total_liters);
            total_liters2 += get_fraction(i);
        }

        set_fraction(PHASE_COUNT - 1, get_fraction(PHASE_COUNT - 1) + (LITERS_PER_ROTATION - total_liters2));
        ESP_LOGD("log2csv", "total_liters:%f total_liters2:%f", total_liters, total_liters2);
        update_calibration_state(fully_calibrated ? FRACTIONS : PRELIMINARY_FRACTIONS);
    }

    for (int i = 0; i < PHASE_COUNT; ++i) {
        ESP_LOGD("log2csv", "pattern%d:%u fraction%d:%f calibration_count%d:%d millis%d:%u prev_millis%d:%u dur%d:%u", i, get_pattern(i), i, get_fraction(i), i, phase_info[i].calibration_count, i, phase_info[i].millis, i, phase_info[i].prev_millis, i, phase_info[i].millis - phase_info[i].prev_millis);
    }
}

/// Calculate the consumption and flow based on the current phase and update the relevant states.
///
/// @param now The current time in milliseconds.
void calculate_consumption_and_flow(uint32_t now) {
    const float liters_for_phase = calibrated < PRELIMINARY_FRACTIONS ? LITERS_PER_ROTATION / PHASE_COUNT : get_fraction(edge_phase);

    id(global_consumption_lifetime) += liters_for_phase;
    id(consumption_lifetime).publish_state(id(global_consumption_lifetime));
    id(consumption_lifetime_number).publish_state(id(global_consumption_lifetime));

    id(consumption_since_restart).publish_state(id(consumption_since_restart).state + liters_for_phase);
    id(consumption_current).publish_state(id(consumption_current).state + liters_for_phase);

    consumption_since_flow_start += liters_for_phase;

    float flow_rate = 0.0f;
    if (flow_start_time > 0) {
        if (liters_for_phase > 0.0f) {
            flow_rate = liters_for_phase / (now - last_phase_change) * MILLIS_IN_MINUTE; // L/min
            id(flow_rate_now).publish_state(id(flow_rate_now).state <= 0.0f ? flow_rate : ((1.0f-FLOW_RATE_SMOOTHING_WEIGHT) * id(flow_rate_now).state + FLOW_RATE_SMOOTHING_WEIGHT * flow_rate));
        }

        id(flow_rate_current).publish_state(consumption_since_flow_start / (now - flow_start_time) * MILLIS_IN_MINUTE); // L/min
    }

    if (flow_start_time == 0) {
        flow_start_time              = now;
        consumption_since_flow_start = 0.0f;
    }

    if (liters_for_phase > 0.0f) {
        const float next_liters_for_phase = calibrated < PRELIMINARY_FRACTIONS ? LITERS_PER_ROTATION / PHASE_COUNT : get_fraction((edge_phase + 1) % PHASE_COUNT);
        expected_phase_duration           = (now - last_phase_change) / liters_for_phase * next_liters_for_phase;

        if (calibrated < PRELIMINARY_FRACTIONS) {
            expected_phase_duration *= PRELIMINARY_EXPECTED_PHASE_DURATION_MULTIPLIER;
        }
        ESP_LOGD("log2csv", "expected_phase_duration:%d liters_for_phase:%f next_liters_for_phase:%f", expected_phase_duration, liters_for_phase, next_liters_for_phase);
    }

    last_phase_change = now;

    ESP_LOGD("log2csv", "last_phase_change:%d liters_for_phase:%f flow_rate_inst:%f", last_phase_change, liters_for_phase, flow_rate);
    ESP_LOGD("log2csv", "consumption_lifetime:%f consumption_since_restart:%f consumption_current:%f consumption_previous:%f flow_rate_now:%f flow_rate_current:%f flow_rate_previous:%f", id(global_consumption_lifetime), id(consumption_since_restart).state, id(consumption_current).state, id(consumption_previous).state, id(flow_rate_now).state, id(flow_rate_current).state, id(flow_rate_previous).state);
}

/// Reset the flow if last edge was too long ago and there is currently a flow.
///
/// @param now The current time in milliseconds.
void maybe_reset_flow(uint32_t now) {
    if (id(flow_rate_now).state <= 0.0f && id(consumption_current).state <= 0.0f && id(flow_rate_current).state <= 0.0f) {
        return;
    }

    const uint32_t reset_time     = static_cast<uint32_t>((now - last_phase_change) * RESET_FRACTION);
    const uint32_t max_reset_time = std::min(expected_phase_duration, static_cast<uint32_t>(MAX_RESET_DURATION));
    ESP_LOGD("log2csv", "reset_time:%u reset_threshold:%u", reset_time, max_reset_time);

    if (reset_time <= max_reset_time) {
        return;
    }

    if (id(consumption_current).state != 0.0f) {
        id(consumption_previous).publish_state(id(consumption_current).state);
        id(flow_rate_previous).publish_state(id(flow_rate_current).state);
    }

    id(consumption_current).publish_state(0.0f);
    id(flow_rate_now).publish_state(0.0f);
    id(flow_rate_current).publish_state(0.0f);

    flow_start_time              = 0;
    consumption_since_flow_start = 0.0f;

    ESP_LOGD("log2csv", "flow_reset:1");
    ESP_LOGD("log2csv", "consumption_lifetime:%f consumption_since_restart:%f consumption_current:%f consumption_previous:%f flow_rate_now:%f flow_rate_current:%f flow_rate_previous:%f", id(global_consumption_lifetime), id(consumption_since_restart).state, id(consumption_current).state, id(consumption_previous).state, id(flow_rate_now).state, id(flow_rate_current).state, id(flow_rate_previous).state);
}

/// Update the pattern and count the number of crossings.
///
/// @param a The current value of sensor A.
/// @param b The current value of sensor B.
/// @param c The current value of sensor C.
/// @param pattern The current pattern.
/// @param prev_pattern The previous pattern.
/// @return The number of crossings detected.
int update_pattern_and_count_crossings(float a, float b, float c, int& pattern, int& prev_pattern) {
    prev_pattern = pattern_from_states(ah, bh, ch);

    ah = update_state(ah, a);
    bh = update_state(bh, b);
    ch = update_state(ch, c);

    pattern = pattern_from_states(ah, bh, ch);
    if (pattern < 0) {
        return -1;
    }

    if (prev_pattern < 0) {
        prev_pattern = pattern;
    }

    const int crossings = count_crossings(prev_pattern, pattern);
    if (crossings > 0) {
        ESP_LOGD("log2csv", "pattern:%u prev_pattern:%u crossings:%u", pattern, prev_pattern, crossings);
    }

    return crossings;
}

/// Handle a crossing event, updating the edge phase and performing calibration and consumption calculations.
///
/// @param now The current time in milliseconds.
/// @param pattern The current pattern.
/// @param prev_pattern The previous pattern.
void handle_crossing(uint32_t now, int pattern, int prev_pattern) {
    edge_phase = (edge_phase + 1) % PHASE_COUNT;
    ESP_LOGD("log2csv", "edge_phase:%d", edge_phase);

    if (calibrated < PATTERNS) {
        if (!all_patterns_known) {
            set_pattern(edge_phase, pattern);
        }
        if (calibrated == PATTERNS - 1) {
            update_pattern_calibration();
            if (calibrated < PATTERNS) {
                return;
            }
        }
    }

    align_edge_phase_to_pattern(pattern, prev_pattern);
    
    if (calibrated == PRELIMINARY_FRACTIONS - 1 || calibrated == FRACTIONS - 1) {
        update_fraction_calibration(now);
    }
    
    calculate_consumption_and_flow(now);
}

/// Publish a debug JSON string containing the most relevant state.
void publish_debug_json() {
    if (!id(debug_mode).state) {
        id(debug_json).publish_state("");
        return;
    }

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

    ESP_LOGD("debug_json", "Publishing debug json: %s", json);
    id(debug_json).publish_state(json);
}

/// Process the sensor readings, update the calibration state, and calculate consumption and flow rate based on the detected patterns and fractions.
void process_readings() {
    if (!wait_until_api_ready()) {
        return;
    }

    const uint32_t now = millis();

    float a = id(light_sensor_a_diff).state;
    float b = id(light_sensor_b_diff).state;
    float c = id(light_sensor_c_diff).state;

    ESP_LOGD("log2csv", "al:%f bl:%f cl:%f ad:%f bd:%f cd:%f a:%f b:%f c:%f now:%d", id(light_sensor_a_light).state, id(light_sensor_b_light).state, id(light_sensor_c_light).state, id(light_sensor_a_dark).state, id(light_sensor_b_dark).state, id(light_sensor_c_dark).state, a, b, c, now);

    if (isnan(a) || isnan(b) || isnan(c)) {
        ESP_LOGD("log2csv", "Skipping reading due to NaN value(s): a:%f b:%f c:%f", a, b, c);
        publish_debug_json();
        return;
    }

    if (calibrated == UNINITIALIZED) {
        initialize_calibration_flags();
    }

    if (!all_extremes_known && calibrated < RANGE) {
        calibrate_range(a, b, c);
    }

    if (calibrated < PRELIMINARY_RANGE) {
        publish_debug_json();
        return;
    }

    // normalize to -1...1 based on min/max
    a = 2.0f * (a - id(min_a).state) / (id(max_a).state - id(min_a).state) - 1.0f;
    b = 2.0f * (b - id(min_b).state) / (id(max_b).state - id(min_b).state) - 1.0f;
    c = 2.0f * (c - id(min_c).state) / (id(max_c).state - id(min_c).state) - 1.0f;

    ESP_LOGD("log2csv", "scaled_a:%f scaled_b:%f scaled_c:%f", a, b, c);

    // Update the pattern based on the current signal states and count the number of crossings that occurred since the last reading.
    int pattern, prev_pattern;
    int crossings = update_pattern_and_count_crossings(a, b, c, pattern, prev_pattern);
    if (crossings < 0) {
        publish_debug_json();
        return;
    }

    // Update the calibration state based on the known data.
    update_calibration_state_from_known_data();
   
    // Handle each crossing that occurred since the last reading.
    // Updating the edge phase, finalizing pattern calibration if necessary, aligning the edge phase to the detected pattern, 
    // updating fraction calibration if necessary, and applying consumption and flow calculations.
    for (int i = 0; i < crossings; ++i) {
        handle_crossing(now, pattern, prev_pattern);
    }

    maybe_reset_flow(now);
    publish_debug_json();
}