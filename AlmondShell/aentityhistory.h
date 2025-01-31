#pragma once

#include "alogger.h"
#include "arobusttime.h"

#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>
#include <string>
#include <chrono>

namespace almondnamespace {

    class HistoryManager {
    public:
        // Constructor takes RobustTime and Logger for management
        HistoryManager(const std::string& logFile, RobustTime& timeSystem)
            : logger(logFile, timeSystem, LogLevel::INFO), timeSystem(timeSystem) {
        }

        void saveState(int entityId, float x, float y) {
            std::lock_guard<std::mutex> lock(mutex);
            if (history.size() >= MAX_HISTORY_SIZE) {
                history.pop_front(); // Remove the oldest state
            }
            auto currentTime = timeSystem.getCurrentTimeString();
            history.emplace_back(x, y, currentTime); // Save the state
            logger.log("Entity " + std::to_string(entityId) + " state saved: (" +
                std::to_string(x) + ", " + std::to_string(y) + ") at " + currentTime);
        }

        bool rewind(int entityId, float& x, float& y) {
            std::lock_guard<std::mutex> lock(mutex);
            if (!history.empty()) {
                auto lastState = history.back();
                x = lastState.x;
                y = lastState.y;
                history.pop_back();
                logger.log("Entity " + std::to_string(entityId) +
                    " rewound to: (" + std::to_string(x) + ", " + std::to_string(y) + ") at " + lastState.timeStamp);
                return true;
            }
            logger.log("No history to rewind for Entity " + std::to_string(entityId));
            return false;
        }

    private:
        static constexpr size_t MAX_HISTORY_SIZE = 100;

        struct State {
            float x, y;
            std::string timeStamp;
            State(float x, float y, const std::string& timeStamp) : x(x), y(y), timeStamp(timeStamp) {}
        };

        std::deque<State> history;
        std::mutex mutex;
        Logger logger;
        RobustTime& timeSystem;
    };

} // namespace almond
