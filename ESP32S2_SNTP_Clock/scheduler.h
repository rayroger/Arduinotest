/**
 * scheduler.h
 *
 * A lightweight task scheduler for Arduino / ESP32.
 *
 * Supports two kinds of tasks:
 *
 *   TASK_DAILY     – fires once per day at a specific local time (hour:minute).
 *   TASK_INTERVAL  – fires repeatedly every N milliseconds (millis-based).
 *
 * Usage
 * -----
 *   // Register tasks once (e.g. in setup()):
 *   _scheduler.addTask("Noon report", 12, 0, []() { Serial.println("Noon!"); });
 *   _scheduler.addIntervalTask("Watch winder", 20000UL, winderCycle);
 *
 *   // Drive the scheduler every loop() iteration:
 *   _scheduler.run();
 */

#pragma once
#include <Arduino.h>
#include <functional>
#include <vector>

// ── Task type ─────────────────────────────────────────────────────────────────

enum TaskType { TASK_DAILY, TASK_INTERVAL };

// ── ScheduledTask ─────────────────────────────────────────────────────────────

struct ScheduledTask {
    String                name;        ///< Human-readable label (shown on the web portal)
    TaskType              type;        ///< TASK_DAILY or TASK_INTERVAL
    uint8_t               hour;        ///< TASK_DAILY: trigger hour   in local time (0–23)
    uint8_t               minute;      ///< TASK_DAILY: trigger minute in local time (0–59)
    uint32_t              intervalMs;  ///< TASK_INTERVAL: milliseconds between firings
    uint32_t              lastFireMs;  ///< TASK_INTERVAL: millis() timestamp of last firing
    std::function<void()> callback;    ///< Function invoked when the task fires
};

// ── TaskScheduler ─────────────────────────────────────────────────────────────

class TaskScheduler {
public:
    /**
     * Register a new daily task.
     *
     * @param name    Display name (shown on the web portal).
     * @param hour    Trigger hour   in local time (0–23).
     * @param minute  Trigger minute in local time (0–59).
     * @param cb      Callback invoked when the trigger time is reached.
     */
    void addTask(const String &name, uint8_t hour, uint8_t minute,
                 std::function<void()> cb) {
        _tasks.push_back({name, TASK_DAILY, hour, minute, 0u, 0u, cb});
    }

    /**
     * Register a new interval task.
     *
     * @param name        Display name (shown on the web portal).
     * @param intervalMs  Milliseconds between firings.
     * @param cb          Callback invoked on each firing.
     */
    void addIntervalTask(const String &name, uint32_t intervalMs,
                         std::function<void()> cb) {
        _tasks.push_back({name, TASK_INTERVAL, 0, 0, intervalMs, millis(), cb});
    }

    /**
     * Check whether any task should fire and execute it.
     * Must be called from loop().
     *
     * - TASK_INTERVAL tasks fire as soon as intervalMs has elapsed since the
     *   previous firing (millis-based, sub-minute accuracy).
     * - TASK_DAILY tasks fire at most once per minute when the current local
     *   time matches their hour:minute.
     */
    void run() {
        uint32_t now = millis();

        // ── Interval tasks ────────────────────────────────────────────────────
        for (auto &task : _tasks) {
            if (task.type == TASK_INTERVAL &&
                now - task.lastFireMs >= task.intervalMs) {
                // Advance by intervalMs (not reset to now) to prevent drift
                // when run() is called infrequently.
                // Unsigned subtraction handles millis() 49-day rollover correctly.
                task.lastFireMs += task.intervalMs;
                Serial.printf("[Scheduler] Running: %s\n", task.name.c_str());
                task.callback();
            }
        }

        // ── Daily tasks ───────────────────────────────────────────────────────
        struct tm t;
        if (!getLocalTime(&t)) return;

        // Build a per-minute token so each daily task fires exactly once per minute.
        uint32_t token = (uint32_t)t.tm_hour * 60u + (uint32_t)t.tm_min;
        if (token == _lastToken) return;
        _lastToken = token;

        for (const auto &task : _tasks) {
            if (task.type == TASK_DAILY &&
                task.hour   == (uint8_t)t.tm_hour &&
                task.minute == (uint8_t)t.tm_min) {
                Serial.printf("[Scheduler] Running: %s\n", task.name.c_str());
                task.callback();
            }
        }
    }

    /** Read-only access to the registered task list (used by the web portal). */
    const std::vector<ScheduledTask>& tasks() const { return _tasks; }

private:
    std::vector<ScheduledTask> _tasks;
    uint32_t                   _lastToken = 0xFFFFFFFFu;
};

// ── Global singleton shared across all headers in this sketch ─────────────────
static TaskScheduler _scheduler;
