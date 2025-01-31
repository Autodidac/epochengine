#pragma once

#include "alogger.h"
#include "arobusttime.h"
#include "aentityhistory.h"

#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>
#include <string>
#include <chrono>
#define _CRT_SECURE_NO_WARNINGS
#include <ctime>

namespace almondnamespace {
    class Entity {
    public:
        // Default constructor
        Entity(const std::string& logFile, RobustTime& m_timeSystem)
            : id(0), posX(0.0f), posY(0.0f), historyManager(logFile, m_timeSystem), logger(logFile, m_timeSystem, almondnamespace::LogLevel::INFO), m_timeSystem(m_timeSystem) {
            if (!&m_timeSystem) {
                throw std::invalid_argument("timeSystem cannot be nullptr");
            }
            logger.log("Entity created with ID: " + std::to_string(id) + " at " + currentTime());
        }

        Entity(int id, float x, float y, const std::string& logFileName, RobustTime& m_timeSystem)
            : id(id), posX(x), posY(y), historyManager(logFileName, m_timeSystem), logger(logFileName, m_timeSystem, almondnamespace::LogLevel::INFO), m_timeSystem(m_timeSystem) {
            if (!&m_timeSystem) {
                throw std::invalid_argument("timeSystem cannot be nullptr");
            }
            logger.log("Entity created with ID: " + std::to_string(id) + " at " + currentTime());
        }


        // Print entity's position
        void printPosition() const {
            std::cout << "Entity " << id << " Position: (" << posX << ", " << posY << ")\n";
        }

        int getId() const { return id; }

        // Move entity and log the state
        void move(float deltaX, float deltaY) {
            saveState(); // Save the current state before moving
            posX += deltaX;
            posY += deltaY;
            logger.log("Entity " + std::to_string(id) + " moved to: (" + std::to_string(posX) + ", " + std::to_string(posY) + ") at " + currentTime());
        }

        // Rewind to the last state
        void rewind() {
            if (historyManager.rewind(id, posX, posY)) {
                logger.log("Entity " + std::to_string(id) + " rewound to: (" + std::to_string(posX) + ", " + std::to_string(posY) + ") at " + currentTime());
            }
            else {
                logger.log("No previous state to rewind to for Entity " + std::to_string(id) + " at " + currentTime());
            }
        }

        // Clone method: creates a new instance with the same ID and position
        std::unique_ptr<almondnamespace::Entity> clone() const {
            return std::make_unique<almondnamespace::Entity>(id, posX, posY, logger.getLogFileName(), m_timeSystem);
        }

    private:
        int id = 0;
        float posX;
        float posY;
        almondnamespace::HistoryManager historyManager; // Manages the history of states
        almondnamespace::Logger logger;         // Logger for the entity
        const almondnamespace::RobustTime& m_timeSystem = {};        // Time system reference
        std::tm tm;

        void saveState() {
            historyManager.saveState(id, posX, posY); // Save the current position before changing it
        }

        std::string currentTime() const {
            auto now = std::chrono::system_clock::now();
            auto timeT = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
            if (localtime_s(&tm, &timeT) != 0) {
                throw std::runtime_error("Failed to convert time to local time");
            }
            char buffer[80];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
            return std::string(buffer);
        }

    };
} // namespace almond

