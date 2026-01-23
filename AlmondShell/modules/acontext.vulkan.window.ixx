module;

import aengine.input;

export module acontext.vulkan.window;

import acontext.vulkan.context;

namespace VulkanCube {

#if defined(ALMOND_VULKAN_STANDALONE)
    inline void Application::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (app) {
            app->processMouseInput(xpos, ypos);
        }
    }
#endif

    inline void Application::processMouseInput(double xpos, double ypos) {
        if (firstMouse) {
            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);
            firstMouse = false;
            return;
        }
        float xOffset = static_cast<float>(xpos) - lastX;
        // To invert Y movement, use lastY - ypos instead:
        float yOffset = lastY - static_cast<float>(ypos);
        // Remove Y inversion for natural mouse control
        yOffset = static_cast<float>(ypos) - lastY;

        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);

        Camera::processMouse(cam, xOffset, yOffset);
    }

    inline void Application::updateCamera(float deltaTime) {
        // Process WASD keyboard input for camera movement (engine input)
        if (almondnamespace::input::is_key_held(almondnamespace::input::Key::W))
            Camera::processKeyboard(cam, Camera::Direction::Forward, deltaTime);
        if (almondnamespace::input::is_key_held(almondnamespace::input::Key::S))
            Camera::processKeyboard(cam, Camera::Direction::Backward, deltaTime);
        if (almondnamespace::input::is_key_held(almondnamespace::input::Key::A))
            Camera::processKeyboard(cam, Camera::Direction::Left, deltaTime);
        if (almondnamespace::input::is_key_held(almondnamespace::input::Key::D))
            Camera::processKeyboard(cam, Camera::Direction::Right, deltaTime);
    }

} // namespace VulkanCube
