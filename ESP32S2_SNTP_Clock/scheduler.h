/**
 * scheduler.h
 *
 * A lightweight, time-based daily task scheduler for Arduino / ESP32.
 *
 * Each registered task has a human-readable name, a daily trigger time
 * (hour + minute in local time), and a callback function.  The scheduler
 * guarantees that each task fires at most once per minute, regardless of how
 * frequently run() is called from loop().
 *
 * Usage
 * -----
 *   // Register tasks once (e.g. in setup()):
 *   _scheduler.addTask("Noon report", 12, 0, []() { Serial.println("Noon!"); });
 *   _scheduler.addTask("Log time",     7, 30, printLocalTime);
 *
 *   // Drive the scheduler every loop() iteration:
 *   _scheduler.run();
 */

#pragma once
#include <Arduino.h>
#include <functional>
#include <vector>

// ── ScheduledTask ─────────────────────────────────────────────────────────────

struct ScheduledTask {
    String                name;     ///< Human-readable label (shown on the web portal)
    uint8_t               hour;     ///< Trigger hour   in local time (0–23)
    uint8_t               minute;   ///< Trigger minute in local time (0–59)
    std::function<void()> callback; ///< Function invoked at the scheduled time
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
        _tasks.push_back({name, hour, minute, cb});
    }

    /**
     * Check whether any task should fire and execute it.
     * Must be called from loop().  Each task fires at most once per minute.
     */
    void run() {
        struct tm t;
        if (!getLocalTime(&t)) return;

        // Build a per-minute token so each task fires exactly once per minute.
        uint32_t token = (uint32_t)t.tm_hour * 60u + (uint32_t)t.tm_min;
        if (token == _lastToken) return;
        _lastToken = token;

        for (const auto &task : _tasks) {
            if (task.hour   == (uint8_t)t.tm_hour &&
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
