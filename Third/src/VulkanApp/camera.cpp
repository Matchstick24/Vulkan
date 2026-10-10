// INCLUDES //

#include "camera.h"
#include <iostream>

// MAIN //

void Camera::Set_Projection(const glm::mat4& projection) 
{
    Projection = projection;
}

void Camera::Set_Window(GLFWwindow* window)
{
    Window = window;
    glfwGetCursorPos(Window, &mouseX, &mouseY);
    glfwSetWindowUserPointer(window, this);
}

void Camera::Step(float dt)
{
    // Movement //

    float CurrentSpeed = Speed;

    if (glfwGetKey(Window, (int)KeyMap::Move_Faster) == GLFW_PRESS)
        CurrentSpeed = Fast_Speed;

    glm::vec3 forward_vec = Math::GetForwardVector(Transform);
    glm::vec3 up_vec = Math::GetUpVector(Transform);
    glm::vec3 right_vec = Math::GetRightVector(Transform);

    glm::vec3 move_dir = glm::vec3(0.0f, 0.0f, 0.0f);

    if (glfwGetKey(Window, (int)KeyMap::Move_Forward) == GLFW_PRESS)
    {
        move_dir = forward_vec;
    } else if (glfwGetKey(Window, (int)KeyMap::Move_Backward) == GLFW_PRESS)
    {
        move_dir -= forward_vec;
    }
    
    if (glfwGetKey(Window, (int)KeyMap::Move_Right) == GLFW_PRESS)
    {
        move_dir += right_vec;
    } else if (glfwGetKey(Window, (int)KeyMap::Move_Left) == GLFW_PRESS)
    {
        move_dir -= right_vec;
    }

    if (glfwGetKey(Window, (int)KeyMap::Move_Up) == GLFW_PRESS)
    {
        move_dir += up_vec;
    } else if (glfwGetKey(Window, (int)KeyMap::Move_Down) == GLFW_PRESS)
    {
        move_dir -= up_vec;
    }

    if (glm::length(move_dir) >= 0.0001f) // is moving //
        Position += glm::normalize(move_dir) * CurrentSpeed * dt;
        
    // Rotation //

    double lastX = mouseX;
    double lastY = mouseY;

    glfwGetCursorPos(Window, &mouseX, &mouseY);

    if (glfwGetMouseButton(Window, (int)KeyMap::Mouse_Move) == GLFW_PRESS)
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        double deltaX = mouseX - lastX;
        double deltaY = mouseY - lastY;

        Yaw   -= deltaX * Mouse_Sensitivity * dt;
        Pitch -= deltaY * Mouse_Sensitivity * dt;
        Pitch = glm::clamp(Pitch, glm::radians(-89.0f), glm::radians(89.0f));

        glm::quat qPitch = glm::angleAxis(Pitch, glm::vec3(1, 0, 0));
        glm::quat qYaw   = glm::angleAxis(Yaw,   glm::vec3(0, 1, 0));

        Rotation = qYaw * qPitch;
        Rotation = glm::normalize(Rotation);

    } else
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        // Rotation using arrow keys //

        if (glfwGetKey(Window, (int)KeyMap::Rotate_Left) == GLFW_PRESS)
        {
            Yaw += Key_Rotation_Speed * dt;
        } else if (glfwGetKey(Window, (int)KeyMap::Rotate_Right) == GLFW_PRESS)
        {
            Yaw -= Key_Rotation_Speed * dt;
        }

        if (glfwGetKey(Window, (int)KeyMap::Rotate_Up) == GLFW_PRESS)
        {
            Pitch += Key_Rotation_Speed * dt; 
        } else if (glfwGetKey(Window, (int)KeyMap::Rotate_Down) == GLFW_PRESS)
        {
            Pitch -= Key_Rotation_Speed * dt;
        }   

        Pitch = glm::clamp(Pitch, glm::radians(-89.0f), glm::radians(89.0f));

        glm::quat qPitch = glm::angleAxis(Pitch, glm::vec3(1, 0, 0));
        glm::quat qYaw   = glm::angleAxis(Yaw,   glm::vec3(0, 1, 0));

        Rotation = qYaw * qPitch;
        Rotation = glm::normalize(Rotation);
    }

    // End //

    Transform = glm::translate(glm::mat4(1.0f), Position) * glm::toMat4(Rotation);
    View = glm::inverse(Transform);
}