// INCLUDES //

#include "mesh_import.h"
#include <unordered_map>

namespace Mesh_Import
{

// HASH FUNCTIONS //

struct TupleHash 
{
    std::size_t operator() (const std::tuple<int,int,int>& t) const 
    {
        auto hash_1 = std::hash<int>()(std::get<0>(t));
        auto hash_2 = std::hash<int>()(std::get<1>(t));
        auto hash_3 = std::hash<int>()(std::get<2>(t));
       
        return hash_1 ^ (hash_2 << 1) ^ (hash_3 << 2);
    }
};

// MAIN //

void SetMeshData(const char* file, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices)
{
    // RESET CURRENT MESH_DATA //

    vertices.clear();
    indices.clear();

    // LOADING FROM FILE //

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    tinyobj::ObjReaderConfig config;
    config.triangulate = true; 

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file);

    if (!warn.empty()) std::cout << "WARN: " << warn << std::endl;
    if (!err.empty()) std::cerr << "ERROR: " << err << std::endl;
    if (!ret) exit(1);

    // SETTING THE MESH DATA //

    std::unordered_map<std::tuple<int,int,int>, unsigned int, TupleHash> uniqueVertices;

    for (const auto& shape : shapes) 
    {
        size_t index_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) 
        {
            int fv = shape.mesh.num_face_vertices[f]; 

            for (int v = 0; v < fv; v++) 
            {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                auto key = std::make_tuple(idx.vertex_index,
                                        idx.texcoord_index,
                                        idx.normal_index);

                if (uniqueVertices.count(key) == 0)
                {

                    tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index    ];
                    tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                    tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

                    tinyobj::real_t uv_x = attrib.texcoords[2 * idx.texcoord_index + 0];
                    tinyobj::real_t uv_y = attrib.texcoords[2 * idx.texcoord_index + 1];

                    Vertex vertice = 
                    {
                        {vx, vy, vz},
                        {uv_x, uv_y, 1.0f},
                    };

                    uint16_t newIndex = static_cast<uint16_t>(vertices.size());
                    uniqueVertices[key] = newIndex;
                    vertices.push_back(vertice);
                }

                indices.push_back(uniqueVertices[key]);
            }

            index_offset += fv;
        }
    }
}

} // Mesh_Import