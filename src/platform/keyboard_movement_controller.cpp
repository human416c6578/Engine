#include "engine/platform/keyboard_movement_controller.hpp"
#include "engine/platform/se_input_system.hpp"
#include "engine/scene/se_gameobject.hpp"

#include <glm/gtc/constants.hpp>

#include <limits>
#include <cmath>
#include <imgui/imgui.h>

namespace se
{

    void KeyboardMovementController::moveInPlaneXZ(
        SEInputSystem &input, float dt, TransformComponent& transform)
    {
        double posX, posY;
        ImGuiIO& io = ImGui::GetIO();

        if (!captured && !io.WantCaptureMouse && input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_1)) {
            input.setCursorCaptured(true);
            captured = true;
            // Cursor capture may change coordinates. Start a fresh baseline.
            input.getMousePosition(oldX, oldY);
        }

        if (input.isKeyPressed(GLFW_KEY_ESCAPE))
        {
            input.setCursorCaptured(false);
            captured = false;
        }
        if (!captured)
        {
            return;
        }

        input.getMousePosition(posX, posY);
        // Mouse displacement already spans the frame interval. Multiplying by
        // dt again would increase sensitivity at low FPS. Preserve axis signs.
        transform.rotation.x -= lookSpeed * static_cast<float>(posY - oldY);
        transform.rotation.y += lookSpeed * static_cast<float>(posX - oldX);

        // Limit pitch values between about +/- 90 degrees
        transform.rotation.x = glm::clamp(
            transform.rotation.x,
            -glm::half_pi<float>(),
            glm::half_pi<float>()
        );

        transform.rotation.y = glm::mod(transform.rotation.y, glm::two_pi<float>());

        float yaw = transform.rotation.y;
        float pitch = transform.rotation.x;
        const glm::vec3 forwardDir{ sin(yaw), -sin(pitch), cos(yaw) };
        const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
        const glm::vec3 upDir{ 0.f, -1.f, 0.f };

        glm::vec3 moveDir{ 0.f };
        if (input.isKeyPressed(keys.moveForward))
            moveDir += forwardDir;
        if (input.isKeyPressed(keys.moveBackward))
            moveDir -= forwardDir;
        if (input.isKeyPressed(keys.moveRight))
            moveDir += rightDir;
        if (input.isKeyPressed(keys.moveLeft))
            moveDir -= rightDir;
        if (input.isKeyPressed(keys.moveUp))
            moveDir += upDir;
        if (input.isKeyPressed(keys.moveDown))
            moveDir -= upDir;

        if (input.isKeyPressed(GLFW_KEY_LEFT_CONTROL))
            moveSpeed = 6.f;
        else
            moveSpeed = 3.0f;

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
        {
            transform.translation += moveSpeed * dt * glm::normalize(moveDir);
        }

        oldX = posX;
        oldY = posY;

    }
}
