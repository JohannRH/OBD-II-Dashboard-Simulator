#pragma once
// ============================================================
//  display.hpp
//
//  Public interface for the display module.
//  Only two functions exposed — init once, update forever.
//  The rendering details are hidden inside display.cpp.
//  This is called "encapsulation" — a core C++ principle.
// ============================================================

#include "sensors.hpp"

void display_init();
void display_update(const VehicleState& state);
