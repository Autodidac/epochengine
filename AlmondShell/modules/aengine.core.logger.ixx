/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗    *
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗   *
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║   *
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║   *
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝   *
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝    *
 *                                                            *
 *   This file is part of the Almond Project.                 *
 *   AlmondShell - Modular C++ Framework                      *
 *                                                            *
 *   SPDX-License-Identifier: LicenseRef-MIT-NoSell           *
 *                                                            *
 *   Provided "AS IS", without warranty of any kind.          *
 *   Use permitted for Non-Commercial Purposes ONLY,          *
 *   without prior commercial licensing agreement.            *
 *                                                            *
 *   Redistribution Allowed with This Notice and              *
 *   LICENSE file. No obligation to disclose modifications.   *
 *                                                            *
 *   See LICENSE file for full terms.                         *
 *                                                            *
 **************************************************************/
module;

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>

export module aengine.core.logger;

import aengine.core.time;

export namespace almondnamespace
{
    namespace logger
    {
        enum class LogLevel
        {
            INFO,
            WARN,
            ALMOND_ERROR
        };

        class Logger
        {
        public:
            Logger(const std::string& filename, LogLevel level = LogLevel::INFO)
                : logFile(filename, std::ios::app), logFileName(filename), logLevel(level)
            {
                std::cout << "Attempting to open log file: " << filename << std::endl;

                std::filesystem::path logPath(filename);
                const auto parentPath = logPath.parent_path();
                if (!parentPath.empty() && !std::filesystem::exists(parentPath))
                {
                    throw std::runtime_error("Directory for log file does not exist: " + parentPath.string());
                }

                if (!logFile.is_open()) {
                    std::cerr << "Failed to open log file: " << filename << std::endl;
                    throw std::runtime_error("Could not open log file: " + filename);
                }
            }

            ~Logger()
            {
                if (logFile.is_open())
                {
                    logFile.close();
                }
            }

            inline static Logger& GetInstance(const std::string& logFileName)
            {
                static Logger instance(logFileName);
                return instance;
            }

            void log(const std::string& message, LogLevel level = LogLevel::INFO)
            {
                std::lock_guard<std::mutex> lock(mutex);

                if (level >= logLevel) {
                    logFile << timing::getCurrentTimeString() << " [" << logLevelToString(level) << "] - " << message << std::endl;
                }
            }

            std::string getLogFileName() const
            {
                return logFileName;
            }

        private:
            std::ofstream logFile;
            std::mutex mutex;
            std::string logFileName;
            LogLevel logLevel;

            std::string logLevelToString(LogLevel level) const
            {
                switch (level) {
                case LogLevel::INFO: return "INFO";
                case LogLevel::WARN: return "WARN";
                case LogLevel::ALMOND_ERROR: return "ERROR";
                default: return "UNKNOWN";
                }
            }
        };
    }
} // namespace almondnamespace
