// INCLUDES //

#include "math.h"

// MAIN //

glm::vec3 Math::GetForwardVector(glm::mat4 transform)
{
    return glm::normalize(-glm::vec3(transform[2]));
}

glm::vec3 Math::GetUpVector(glm::mat4 transform)
{
    return glm::normalize(glm::vec3(transform[1]));
}

glm::vec3 Math::GetRightVector(glm::mat4 transform)
{
    return glm::normalize(glm::vec3(transform[0]));
}

glm::vec3 Math::GetPosition(glm::mat4 transform)
{
    return glm::vec3(transform[3]);
}