#pragma once

// INCLUDES //

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// MAIN //

namespace Math
{

glm::vec3 GetForwardVector(glm::mat4 transform);
glm::vec3 GetUpVector(glm::mat4 transform);
glm::vec3 GetRightVector(glm::mat4 transform);

glm::vec3 GetPosition(glm::mat4 transform);

};