#ifndef SVFMESH_H
#define SVFMESH_H

#include <cmath>
#include <vector>

struct Float3
{
    float x, y, z;
    
    Float3() = default;
	
	static float Dot(Float3 const &a, Float3 const &b)
    {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }

    static Float3 Up()
    {
        return { 0.0f, 1.0f, 0.0f };
    }

    static Float3 Right()
    {
        return { 1.0f, 0.0f, 0.0f };
    }
    
    static Float3 Normalize(Float3 const & n)
    {
        float const div = 1.0f / std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
        return { n.x * div, n.y * div, n.z * div };
    }
    
    static Float3 Cross(Float3 const & a, Float3 const & b)
    {
        Float3 result;
        result.x = a.y*b.z - a.z*b.y;
        result.y = a.z*b.x - a.x*b.z;
        result.z = a.x*b.y - a.y*b.x;
        return result;
    }

    
    Float3 operator-(Float3 const & rhs) const
    {
        return { x - rhs.x, y - rhs.y, z - rhs.z };
    }
    
    Float3 & operator+=(Float3 const & rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
        this->z += rhs.z;
        return *this;
    }
    
    Float3 operator / (float divisor) const
    {
        divisor = 1.0f / divisor;
        return {x*divisor, y*divisor, z*divisor};
    }
};

struct Float2
{
    Float2() = default;
    
    float x, y;
};

/**
 * SVFMeshVertex represents a container for data defining 3-D object
 *
 * A 3D object consist of positions, normals and texture coordinates 
 *
 */
struct SVFMeshVertex
{
    SVFMeshVertex() = default;
    
    Float3 pos;
    Float3 normal;
    Float2 uv;
};

struct MeshRequestInputData {
    uint32_t requestedIndexCount = 0;
    uint32_t requestedVertexCount = 0;
    uint32_t textureWidth;
    uint32_t textureHeight;
};

struct SVFMesh {

    std::vector<SVFMeshVertex> m_vertexData;
    std::vector<uint32_t> m_indexData;
	void* m_gpuTexture;
};


#endif // ! SVFMESH_H