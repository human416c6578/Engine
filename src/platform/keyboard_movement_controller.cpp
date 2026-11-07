#include "engine/platform/keyboard_movement_controller.hpp"

#include <limits>
#include <cmath>
#include <imgui/imgui.h>

namespace se
{

    void KeyboardMovementController::moveInPlaneXZ(
        SEInputSystem &input, float dt, TransformComponent& transform)
    {
        glm::vec3 rotate{ 0 };
        double posX, posY;
        double diffX, diffY;
        /*
        if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS)
          rotate.y += 1.f;
        if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS)
          rotate.y -= 1.f;
        if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS)
          rotate.x += 1.f;
        if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS)
          rotate.x -= 1.f;
        */

        ImGuiIO& io = ImGui::GetIO();

        if (!io.WantCaptureMouse && input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_1)) {
            input.setCursorCaptured(true);
            captured = true;
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

        input.getMousePosition(posY, posX);
        if (oldX == 0.f || oldY == 0.f)
        {
            oldX -= posX;
            oldY = posY;
        }
        diffX = std::abs(posX - oldX);
        diffY = std::abs(posY - oldY);

        if (diffX < 50.f && diffY < 50.f)
        {
            rotate.x -= posX - oldX;
            rotate.y += posY - oldY;
        }


        // Handle rotation
        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
        {
            transform.rotation += lookSpeed * dt * rotate;
        }

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
