// ============================================================
//  main.cpp — Application entry point
//
//  WHAT YOU LEARN:
//  - app_main(): ESP-IDF's entry point (replaces main())
//  - FreeRTOS xTaskCreate(): launching concurrent tasks
//  - vTaskDelay(): yielding the CPU without blocking
//  - esp_timer_get_time(): microsecond timestamp
//  - ESP_LOGI(): structured logging (better than printf)
//
//  TWO-TASK ARCHITECTURE:
//
//  task_sensors  (priority 5, every 20ms)
//  ├── updates simulated RPM, speed, temp, throttle
//  ├── cycles the screen every 3 seconds automatically
//  └── injects a fault code at 30 seconds
//
//  task_display  (priority 4, every 100ms)
//  └── reads VehicleState and redraws the OLED
//
//  WHY SEPARATE TASKS?
//  If we did everything in one loop, a slow I2C write to
//  the OLED (which takes ~10ms) would delay the sensor
//  update. Separate tasks mean each runs at its own pace.
//  This is exactly how automotive ECUs are structured:
//  fast tasks for control, slow tasks for display/logging.
//
//  NOTE ON THREAD SAFETY:
//  Both tasks share g_state. For a learning project this
//  is fine. In production you would protect it with a
//  FreeRTOS mutex: xSemaphoreCreateMutex().
// ============================================================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <cstdlib>

#include "config.hpp"
#include "sensors.hpp"
#include "display.hpp"

static const char* TAG = "OBD2";

// ── Shared vehicle state ───────────────────────────────────
// Both tasks read and write this struct.
static VehicleState g_state;

// ── Task 1: Sensor simulation + screen cycling ────────────
static void task_sensors(void* /*arg*/)
{
    ESP_LOGI(TAG, "Sensor task started");

    int64_t last_cycle_us  = esp_timer_get_time();
    int64_t last_fault_us  = esp_timer_get_time();

    for (;;) {
        // Update simulated sensor values
        sensors_update(g_state);

        int64_t now = esp_timer_get_time();

        // Auto-cycle screen every SCREEN_CYCLE_MS
        if ((now - last_cycle_us) >= (SCREEN_CYCLE_MS * 1000LL)) {
            last_cycle_us = now;
            sensors_cycle_screen(g_state);
            ESP_LOGI(TAG, "Screen: %d  RPM: %.0f  Speed: %.1f  Temp: %.1f",
                     (int)g_state.screen,
                     g_state.rpm,
                     g_state.speed_kmh,
                     g_state.coolant_c);
        }

        // Inject a fault code after FAULT_INJECT_MS (30 seconds)
        if (!g_state.has_fault &&
            (now - last_fault_us) >= (FAULT_INJECT_MS * 1000LL)) {
            sensors_inject_fault(g_state);
            ESP_LOGW(TAG, "Fault injected: %s - %s",
                     FAULT_TABLE[g_state.fault_idx].code,
                     FAULT_TABLE[g_state.fault_idx].description);
        }

        // Yield for 20ms — this is NOT a blocking delay.
        // vTaskDelay tells FreeRTOS "I'm done for now, run other tasks"
        // The scheduler wakes this task back up after 20ms.
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ── Task 2: Display rendering ─────────────────────────────
static void task_display(void* /*arg*/)
{
    ESP_LOGI(TAG, "Display task started");

    for (;;) {
        display_update(g_state);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ── app_main ──────────────────────────────────────────────
// This is where the ESP32 starts after booting.
// Think of it as main() — but for ESP-IDF.
extern "C" void app_main()
{
    ESP_LOGI(TAG, "==============================");
    ESP_LOGI(TAG, " OBD-II Simulator booting...");
    ESP_LOGI(TAG, " ESP32-C3 | ESP-IDF | C++17  ");
    ESP_LOGI(TAG, "==============================");

    // Seed random number generator using hardware timer noise
    srand((unsigned)esp_timer_get_time());

    // Initialise the OLED — shows splash screen immediately
    display_init();
    ESP_LOGI(TAG, "OLED initialised at I2C address 0x%02X", OLED_ADDR);

    // Brief pause so the splash screen is readable
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Launching FreeRTOS tasks...");

    // Launch sensor task
    // Arguments: function, name, stack size, arg, priority, handle
    xTaskCreate(task_sensors,
                "sensors",
                4096,       // 4KB stack
                nullptr,
                5,          // priority 5 — higher than display
                nullptr);

    // Launch display task
    xTaskCreate(task_display,
                "display",
                4096,
                nullptr,
                4,          // priority 4
                nullptr);

    ESP_LOGI(TAG, "Tasks running. Screens cycle every 3s. Fault at 30s.");

    // app_main can return — FreeRTOS scheduler keeps tasks alive
}
