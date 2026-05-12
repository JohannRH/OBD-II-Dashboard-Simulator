#pragma once
// ============================================================
//  config.hpp
//
//  ALL hardware assignments and timing constants live here.
//  This is standard practice in embedded/automotive projects:
//  one file to change if the hardware ever changes.
//  In AUTOSAR this is called the "ECU Configuration" layer.
// ============================================================

#include "hal/gpio_types.h"
#include "soc/gpio_num.h"

// ── I2C pins (OLED display) ────────────────────────────────
static constexpr gpio_num_t PIN_SDA   = GPIO_NUM_6;
static constexpr gpio_num_t PIN_SCL   = GPIO_NUM_7;
static constexpr uint32_t   I2C_FREQ  = 400000;
static constexpr uint8_t    OLED_ADDR = 0x3C;

// ── OLED geometry ──────────────────────────────────────────
static constexpr uint8_t OLED_WIDTH  = 128;
static constexpr uint8_t OLED_HEIGHT =  64;

// ── Timing ─────────────────────────────────────────────────
static constexpr uint32_t SENSOR_INTERVAL_MS  =  50;
static constexpr uint32_t DISPLAY_INTERVAL_MS = 100;
static constexpr uint32_t SCREEN_CYCLE_MS     = 6000;
static constexpr uint32_t FAULT_INJECT_MS     = 30000;
