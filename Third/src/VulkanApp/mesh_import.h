#pragma once

// INCLUDES //

#include <iostream>
#include "tiny_obj_loader/tiny_obj_loader.h"

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtx/quaternion.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// MAIN //

namespace Mesh_Import // THIS FILE IS TEMP //
{

// STRUCTS //

struct Vertex
{
    glm::vec3 pos; 
    glm::vec3 color;
};

// FUNCTIONS //

void SetMeshData(const char* file, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices);

} // MESH_IMPORT