#pragma once
// ============================================================
//  sensors.hpp
//
//  Defines VehicleState — the single shared data structure
//  that flows between the sensor task and the display task.
//
//  WHAT YOU LEARN:
//  - struct: a container grouping related data together
//  - enum class: a type-safe way to represent a fixed set
//    of states — here, which screen is currently showing
//  - Separation of concerns: this file owns the DATA.
//    display.cpp owns the RENDERING. Neither knows about
//    the other's internals — they only share this struct.
//
//  IN A REAL CAR:
//  These values would be read from the CAN Bus via an
//  MCP2515 transceiver. The struct stays identical —
//  only the source of the data changes.
// ============================================================

#include <cstdint>

// ── Diagnostic Trouble Code (SAE J2012) ───────────────────
struct DTC {
    const char* code;         // e.g. "P0300"
    const char* description;  // e.g. "Random misfire"
};

// ── All vehicle data in one place ─────────────────────────
struct VehicleState {

    // Sensor values
    float rpm        = 800.0f;   // engine RPM
    float speed_kmh  = 0.0f;     // vehicle speed
    float coolant_c  = 85.0f;    // coolant temperature °C
    float throttle   = 0.0f;     // throttle position 0–100%

    // Fault state
    bool  has_fault  = false;
    int8_t fault_idx = -1;       // index into FAULT_TABLE, -1 = none

    // Screen state machine
    // enum class = scoped enum — Screen::RPM instead of just RPM
    // This prevents name collisions across files
    enum class Screen : uint8_t {
        RPM = 0,
        SPEED,
        TEMP,
        FAULTS,
        COUNT       // not a real screen — used to wrap the counter
    } screen = Screen::RPM;
};

// ── Fault table (defined in sensors.cpp) ──────────────────
// extern = "this exists, but it's defined in another .cpp file"
extern const DTC FAULT_TABLE[4];

// ── Function declaration ───────────────────────────────────
void sensors_update(VehicleState& state);
void sensors_cycle_screen(VehicleState& state);
void sensors_inject_fault(VehicleState& state);
