// aglfw.h
#pragma once

#include "aengineconfig.h"// <GLFW/glfw3.h>

#include "acontextwindow.h"

#include <functional>
#include <stdexcept>

namespace almondnamespace::glfw {
    // GLFW-specific WindowContext implementation
    struct GLFWWindowContext {
        using ResizeCallback = std::function<void(int, int)>;

        // Function to create a GLFW window
        static inline GLFWwindow* create(int width, int height, const char* title) {
            if (!glfwInit()) {
                throw std::runtime_error("Failed to initialize GLFW.");
            }
            // These are the Magic that makes WinMain and GLFW work as the same window
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL context
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);  // Hide the window initially
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);  // Remove decorations if needed

            auto* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
            if (!window) {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window.");
            }
#ifdef _WIN32
            contextwindow::WindowContext::setWindowHandle(glfwGetWin32Window(window));
#endif
            glfwMakeContextCurrent(window);
            return window;
        }

        // Function to destroy a GLFW window
        static inline void destroy(GLFWwindow* window) {
            if (window) {
                glfwDestroyWindow(static_cast<GLFWwindow*>(window));
            }
            glfwTerminate();
        }

        // Function to poll GLFW events
        static inline void poll_events(void* window) {
            glfwPollEvents();
        }

        // Function to check if the GLFW window should close
        static inline bool should_close(GLFWwindow* window) {
            return glfwWindowShouldClose(static_cast<GLFWwindow*>(window));
        }

        // Function to set a GLFW resize callback
        static inline void set_resize_callback(GLFWwindow* window, ResizeCallback callback) {
            glfwSetWindowSizeCallback(static_cast<GLFWwindow*>(window), [](GLFWwindow* w, int width, int height) {
                auto* cb = static_cast<ResizeCallback*>(glfwGetWindowUserPointer(w));
                (*cb)(width, height);
                });
            glfwSetWindowUserPointer(static_cast<GLFWwindow*>(window), new ResizeCallback(callback));
        }

        // Function to get the native window handle from GLFW
        static inline void* get_native_handle(GLFWwindow* window) {
#ifdef _WIN32
            return contextwindow::WindowContext::getWindowHandle();
#else
            return window; // Fallback for non-Windows platforms
#endif
        }
    };
} // namespace almond::glfw