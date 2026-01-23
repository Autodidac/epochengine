module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

export module acontext.vulkan.camera;



namespace Camera {

    enum class Direction { Forward, Backward, Left, Right };

    struct State {
        glm::vec3 Position;
        glm::vec3 Front;
        glm::vec3 Up;
        glm::vec3 Right;
        glm::vec3 WorldUp;
        float Yaw;
        float Pitch;
        float MovementSpeed;
        float MouseSensitivity;
    };

    inline State create(const glm::vec3& position, const glm::vec3& up, float yaw, float pitch) {
        State state{};
        state.Position = position;
        state.WorldUp = up;
        state.Yaw = yaw;
        state.Pitch = pitch;
        state.MovementSpeed = 2.5f;
        state.MouseSensitivity = 0.1f;
        // Calculate initial Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        state.Front = glm::normalize(front);
        // Calculate Right and Up vectors
        state.Right = glm::normalize(glm::cross(state.Front, state.WorldUp));
        state.Up = glm::normalize(glm::cross(state.Right, state.Front));
        return state;
    }

    inline void processKeyboard(State& state, Direction direction, float deltaTime) {
        float velocity = state.MovementSpeed * deltaTime;
        if (direction == Direction::Forward)  state.Position += state.Front * velocity;
        if (direction == Direction::Backward) state.Position -= state.Front * velocity;
        if (direction == Direction::Left)     state.Position -= state.Right * velocity;
        if (direction == Direction::Right)    state.Position += state.Right * velocity;
    }

    inline void processMouse(State& state, float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= state.MouseSensitivity;
        yoffset *= state.MouseSensitivity;
        state.Yaw += xoffset;
        state.Pitch += yoffset;
        if (constrainPitch) {
            if (state.Pitch > 89.0f)  state.Pitch = 89.0f;
            if (state.Pitch < -89.0f) state.Pitch = -89.0f;
        }
        // Update Front, Right and Up vectors using the updated Euler angles
        glm::vec3 front;
        front.x = cos(glm::radians(state.Yaw)) * cos(glm::radians(state.Pitch));
        front.y = sin(glm::radians(state.Pitch));
        front.z = sin(glm::radians(state.Yaw)) * cos(glm::radians(state.Pitch));
        state.Front = glm::normalize(front);
        state.Right = glm::normalize(glm::cross(state.Front, state.WorldUp));
        state.Up = glm::normalize(glm::cross(state.Right, state.Front));
    }

    inline glm::mat4 getViewMatrix(const State& state) {
        return glm::lookAt(state.Position, state.Position + state.Front, state.Up);
    }

} // namespace Camera
