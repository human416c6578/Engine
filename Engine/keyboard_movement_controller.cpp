#include "keyboard_movement_controller.hpp"

#include <limits>
#include <imgui/imgui.h>

namespace se
{

    void KeyboardMovementController::moveInPlaneXZ(
        GLFWwindow* window, float dt, SEGameObject& gameObject)
    {
        static bool captured = false;
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

        if (!io.WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            captured = true;
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            captured = false;
        }
        if (!captured)
        {
            return;
        }

        glfwGetCursorPos(window, &posY, &posX);
        if (oldX == 0.f || oldY == 0.f)
        {
            oldX -= posX;
            oldY = posY;
        }
        diffX = abs(posX - oldX);
        diffY = abs(posY - oldY);

        if (diffX < 50.f && diffY < 50.f)
        {
            rotate.x -= posX - oldX;
            rotate.y += posY - oldY;
        }


        auto transform = gameObject.getTransform();

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
        if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS)
            moveDir += forwardDir;
        if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS)
            moveDir -= forwardDir;
        if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS)
            moveDir += rightDir;
        if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS)
            moveDir -= rightDir;
        if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS)
            moveDir += upDir;
        if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS)
            moveDir -= upDir;

        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            moveSpeed = 6.f;
        else
            moveSpeed = 3.0f;

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
        {
            transform.translation += moveSpeed * dt * glm::normalize(moveDir);
        }

        // Write back the modified transform
        gameObject.setTransform(transform);

        oldX = posX;
        oldY = posY;

    }
}