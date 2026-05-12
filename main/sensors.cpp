// ============================================================
//  sensors.cpp — Simulated vehicle sensor data
//
//  WHAT YOU LEARN:
//  - static variables inside a function keep their value
//    between calls (like a persistent local variable)
//  - constrain pattern: keeping a value within min/max bounds
//  - rand() for sensor noise simulation
//
//  WHY SIMULATE?
//  In a real car you would read these from the CAN Bus.
//  Simulating lets you focus on the embedded architecture
//  without needing a vehicle or CAN transceiver.
//  The code structure is identical either way.
// ============================================================

#include "sensors.hpp"
#include "config.hpp"
#include "esp_timer.h"
#include <cstdlib>

// ── Fault code table ───────────────────────────────────────
// const = read-only, stored in flash (not precious RAM)
const DTC FAULT_TABLE[4] = {
    { "P0300", "Random misfire"  },
    { "P0113", "IAT sensor high" },
    { "P0171", "System lean B1"  },
    { "P0420", "Catalyst eff B1" },
};

// ── sensors_update ────────────────────────────────────────
// Called every 50ms from the sensor task.
// Updates all simulated sensor values in VehicleState.
void sensors_update(VehicleState& state)
{
    // ── Rate limiting ─────────────────────────────────────
    // static means s_last_us persists between calls.
    // We only update if enough time has passed.
    static int64_t s_last_us = 0;
    int64_t now_us = esp_timer_get_time();
    if ((now_us - s_last_us) < (SENSOR_INTERVAL_MS * 1000LL)) return;
    s_last_us = now_us;

    // ── Simulate RPM ──────────────────────────────────────
    // RPM target sweeps between ~750 and ~4200 like city driving
    static float s_rpm_target = 850.0f;
    static int   s_rpm_dir    = 1;

    s_rpm_target += s_rpm_dir * 15.0f;
    if (s_rpm_target > 4200.0f) s_rpm_dir = -1;
    if (s_rpm_target <  750.0f) s_rpm_dir =  1;

    // Add sensor noise: real sensors are never perfectly stable
    float noise = ((float)(rand() % 61) - 30.0f);
    state.rpm = s_rpm_target + noise;
    if (state.rpm < 0.0f)    state.rpm = 0.0f;
    if (state.rpm > 8000.0f) state.rpm = 8000.0f;

    // ── Simulate speed ────────────────────────────────────
    // Proportional to RPM above idle
    state.speed_kmh = (state.rpm > 900.0f)
                      ? (state.rpm - 900.0f) * 0.033f
                      : 0.0f;
    if (state.speed_kmh > 200.0f) state.speed_kmh = 200.0f;

    // ── Simulate throttle position ────────────────────────
    state.throttle = (state.rpm - 750.0f) / 75.0f;
    if (state.throttle < 0.0f)   state.throttle = 0.0f;
    if (state.throttle > 100.0f) state.throttle = 100.0f;

    // ── Simulate coolant temperature ──────────────────────
    static float s_temp_dir = 0.05f;
    state.coolant_c += s_temp_dir;
    if (state.coolant_c > 97.0f)  s_temp_dir = -0.05f;
    if (state.coolant_c < 82.0f)  s_temp_dir =  0.05f;

    // Active fault P0300 (misfire) causes simulated overheating
    if (state.has_fault && state.fault_idx == 0)
        state.coolant_c += 0.2f;
    if (state.coolant_c > 130.0f) state.coolant_c = 130.0f;
}

// ── sensors_cycle_screen ──────────────────────────────────
// Advances to the next screen in the state machine.
// Called automatically every SCREEN_CYCLE_MS from main.
void sensors_cycle_screen(VehicleState& state)
{
    uint8_t next = (static_cast<uint8_t>(state.screen) + 1)
                   % static_cast<uint8_t>(VehicleState::Screen::COUNT);
    state.screen = static_cast<VehicleState::Screen>(next);
}

// ── sensors_inject_fault ──────────────────────────────────
// Injects a random DTC after 30 seconds to demo the fault screen.
void sensors_inject_fault(VehicleState& state)
{
    if (!state.has_fault) {
        state.has_fault  = true;
        state.fault_idx  = static_cast<int8_t>(rand() % 4);
        state.screen     = VehicleState::Screen::FAULTS;
    }
}
