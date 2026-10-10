#pragma once

// INCLUDES //

#include "math.h"
#include <GLFW/glfw3.h>

// SETTINGS //

constexpr float Speed = 5.0f;
constexpr float Fast_Speed = 100.0f;

constexpr float Mouse_Sensitivity = 0.045f;
constexpr float Key_Rotation_Speed = 2.0f; // rotating using arrow keys and not the mouse //

enum class KeyMap 
{
    Move_Faster = GLFW_KEY_LEFT_SHIFT,
    Move_Forward = GLFW_KEY_W,
    Move_Backward = GLFW_KEY_S,
    Move_Left = GLFW_KEY_A,
    Move_Right = GLFW_KEY_D,
    Move_Up = GLFW_KEY_E,
    Move_Down = GLFW_KEY_Q,
    Mouse_Move = GLFW_MOUSE_BUTTON_RIGHT,
    Rotate_Left = GLFW_KEY_LEFT,
    Rotate_Right = GLFW_KEY_RIGHT,
    Rotate_Up = GLFW_KEY_UP,
    Rotate_Down = GLFW_KEY_DOWN
};

// MAIN //

class Camera
{
    public:

        glm::mat4 View = glm::mat4(1.0f); // inverse of transform for rendering, read only //
        glm::mat4 Projection;

        glm::vec3 Position = glm::vec3(0.0f);
        glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::mat4 Transform = glm::mat4(1.0f); // use to find direction and etc, read only //

        void Step(float dt);
        void Set_Projection(const glm::mat4& projection);
        void Set_Window(GLFWwindow* window);
        
    private:

        float Pitch = 0.0f;
        float Yaw   = 0.0f;

        double mouseX = 0.0f;
        double mouseY = 0.0f;

        GLFWwindow* Window;
};