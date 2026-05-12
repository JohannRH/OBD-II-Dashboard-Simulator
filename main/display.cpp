// ============================================================
//  display.cpp — OLED rendering engine
//
//  WHAT YOU LEARN:
//  - I2C in practice: sda/scl wires, slave address 0x3C
//  - Framebuffer pattern: draw everything to RAM first,
//    then flush to the display in one shot (no flicker)
//  - switch/case dispatch — the state machine in action
//  - snprintf: safe way to build strings in C/C++
//    (never use sprintf — it can overflow the buffer)
//
//  THE FOUR SCREENS:
//  1. RPM     — engine speed + throttle bar
//  2. SPEED   — vehicle speed in km/h
//  3. TEMP    — coolant temperature + overheat warning
//  4. FAULTS  — active DTC or "no faults" message
// ============================================================

#include "display.hpp"
#include "config.hpp"
#include "ssd1306.h"
// #include "font8x8_basic.h"
#include <cstdio>
#include <cstring>

// The SSD1306 driver uses a global device handle
static SSD1306_t s_dev;

// Reusable string buffer for building text on screen
// 32 chars is enough for any line on a 128px wide display
static char s_buf[32];

// ── draw_bar ──────────────────────────────────────────────
// Draws a horizontal progress bar using raw pixel columns.
// page = which 8-pixel row (0–7), x = start column,
// width = total bar width in pixels, fill = 0.0 to 1.0
static void draw_bar(uint8_t page, uint8_t x, uint8_t width, float fill)
{
    uint8_t filled = (uint8_t)(fill * width);
    if (filled > width) filled = width;

    for (uint8_t col = x; col < x + width; col++) {
        // 0x3C = 00111100 — lights the middle 4 pixels of the 8-pixel column
        uint8_t pattern = (col < x + filled) ? 0x3C : 0x00;
        ssd1306_display_image(&s_dev, page, col, &pattern, 1);
    }
}

// ── Screen 1: RPM ─────────────────────────────────────────
static void draw_rpm(const VehicleState& s)
{
    ssd1306_clear_screen(&s_dev, false);

    ssd1306_display_text(&s_dev, 0, "ENGINE RPM", 11, false);

    // Large RPM value positioned at top-left, allowing space for other elements
    snprintf(s_buf, sizeof(s_buf), "%4d", (int)s.rpm);
    ssd1306_display_text_x3(&s_dev, 2, s_buf, strlen(s_buf), false);

    // Move units and other elements to avoid overlap with large text
    ssd1306_display_text(&s_dev, 5, "RPM", 3, false);

    // Throttle bar moved down to avoid overlap with large RPM text
    ssd1306_display_text(&s_dev, 7, "TPS:", 4, false);
    draw_bar(7, 32, 80, s.throttle / 100.0f);

    // Fault warning indicator top-right
    if (s.has_fault)
        ssd1306_display_text(&s_dev, 0, "!", 1, true);  // true = inverted

    ssd1306_show_buffer(&s_dev);
}

// ── Screen 2: Speed ───────────────────────────────────────
static void draw_speed(const VehicleState& s)
{
    ssd1306_clear_screen(&s_dev, false);

    ssd1306_display_text(&s_dev, 0, "VEHICLE SPEED", 12, false);

    // Large speed value positioned to avoid overlap
    snprintf(s_buf, sizeof(s_buf), "%4d", (int)s.speed_kmh);
    ssd1306_display_text_x3(&s_dev, 2, s_buf, strlen(s_buf), false);

    // Move units down to avoid overlap
    ssd1306_display_text(&s_dev, 5, "km/h", 4, false);

    // RPM mini bar moved to bottom to avoid overlap
    ssd1306_display_text(&s_dev, 7, "RPM:", 4, false);
    draw_bar(7, 32, 80, s.rpm / 8000.0f);

    if (s.has_fault)
        ssd1306_display_text(&s_dev, 0, "!", 1, true);

    ssd1306_show_buffer(&s_dev);
}

// ── Screen 3: Temperature ─────────────────────────────────
static void draw_temp(const VehicleState& s)
{
    ssd1306_clear_screen(&s_dev, false);

    ssd1306_display_text(&s_dev, 0, "COOLANT TEMP", 12, false);

    // Large temp value positioned to avoid overlap
    snprintf(s_buf, sizeof(s_buf), "%4d", (int)s.coolant_c);
    ssd1306_display_text_x3(&s_dev, 2, s_buf, strlen(s_buf), false);

    // Move units down to avoid overlap
    ssd1306_display_text(&s_dev, 5, "deg C", 5, false);

    // Overheat warning - shorter text to avoid overlap
    bool overtemp = (s.coolant_c > 105.0f);
    if (overtemp)
        ssd1306_display_text(&s_dev, 7, "!! OVERHEAT !!", 14, true);
    else
        ssd1306_display_text(&s_dev, 7, "TEMP OK", 7, false);

    if (s.has_fault)
        ssd1306_display_text(&s_dev, 0, "!", 1, true);

    ssd1306_show_buffer(&s_dev);
}

// ── Screen 4: Fault codes ─────────────────────────────────
static void draw_faults(const VehicleState& s)
{
    ssd1306_clear_screen(&s_dev, false);

    ssd1306_display_text(&s_dev, 0, " FAULT CODES ", 13, true); // inverted title

    if (!s.has_fault || s.fault_idx < 0) {
        ssd1306_display_text(&s_dev, 2, " No faults stored", 17, false);
        ssd1306_display_text(&s_dev, 4, " System OK", 10, false);
    } else {
        const DTC& f = FAULT_TABLE[s.fault_idx];

        // Show the code in large text but positioned to avoid overlap
        ssd1306_display_text_x3(&s_dev, 2, f.code, strlen(f.code), false);

        // Show description at bottom to avoid overlap
        ssd1306_display_text(&s_dev, 6, f.description, strlen(f.description), false);
        ssd1306_display_text(&s_dev, 7, "CHECK ENGINE", 12, false);
    }

    ssd1306_show_buffer(&s_dev);
}

// ── Startup splash screen ─────────────────────────────────
static void draw_splash()
{
    ssd1306_clear_screen(&s_dev, false);
    ssd1306_display_text(&s_dev, 1, " OBD-II  SIM  ", 14, true);
    ssd1306_display_text(&s_dev, 3, " ESP32-C3 + IDF", 15, false);
    ssd1306_display_text(&s_dev, 5, "  C++ / RTOS  ", 14, false);
    ssd1306_display_text(&s_dev, 7, "  Initializing", 14, false);
    ssd1306_show_buffer(&s_dev);
}

// ── Public: display_init ──────────────────────────────────
void display_init()
{
    i2c_master_init(&s_dev,
                    (int16_t)PIN_SDA,
                    (int16_t)PIN_SCL,
                    -1);   // -1 = no reset pin

    ssd1306_init(&s_dev, OLED_WIDTH, OLED_HEIGHT);
    draw_splash();
}

// ── Public: display_update ────────────────────────────────
// Called every 100ms from the display task.
// Dispatches to the correct screen renderer based on state.
// This is the state machine dispatch — the switch IS the state machine.
void display_update(const VehicleState& state)
{
    using S = VehicleState::Screen;

    switch (state.screen) {
        case S::RPM:    draw_rpm(state);    break;
        case S::SPEED:  draw_speed(state);  break;
        case S::TEMP:   draw_temp(state);   break;
        case S::FAULTS: draw_faults(state); break;
        default:        draw_rpm(state);    break;
    }
}
