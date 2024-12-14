#pragma once

#include <vector>
#include <unordered_map>

namespace DX
{
    struct SubMeshGeometry
    {
        SubMeshGeometry() = default;
        SubMeshGeometry(size_t i, size_t s, size_t b) :
            indexCount(i),
            startIndexLocation(s),
            baseVertexLocation(b) {}

        uint32_t indexCount = 0;
        uint32_t startIndexLocation = 0;
        int32_t  baseVertexLocation = 0; 
    };

    struct MeshGeometry
    {
        std::string name;

        Microsoft::WRL::ComPtr<ID3DBlob> vertexBufferCPU;
        Microsoft::WRL::ComPtr<ID3DBlob> indexBufferCPU;

        Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferGPU;
        Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferGPU;

        Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploader;
        Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploader;

        uint32_t vertexByteStride   = 0;
        uint32_t vertexBufferByteSize = 0;
        DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;
        uint32_t indexBufferByteSize  = 0;

        std::unordered_map<std::string, SubMeshGeometry> drawArgs;

        auto VertexBufferView() const 
        {
            D3D12_VERTEX_BUFFER_VIEW view;
            view.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
            view.StrideInBytes = vertexByteStride;
            view.SizeInBytes = vertexBufferByteSize;
            return view;
        }
        auto IndexBufferView() const
        {
            D3D12_INDEX_BUFFER_VIEW view;
            view.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
            view.Format = indexFormat;
            view.SizeInBytes = indexBufferByteSize;
            return view;
        }

        void disposeUploaders()
        {
            vertexBufferUploader = nullptr;
            indexBufferUploader  = nullptr;
        }
    };

    class Geometry
    {
    public:
        Geometry()                           = delete;
        ~Geometry()                          = delete;
        Geometry(const Geometry&)            = delete;
        Geometry(Geometry&&)                 = delete;
        Geometry& operator=(const Geometry&) = delete;
        Geometry& operator=(Geometry&&)      = delete;

        struct Vertex
        {
            Vertex() = default;
            Vertex(float px, float py, float pz,
                   float nx, float ny, float nz,
                   float tx, float ty, float tz,
                   float u,  float v) :
                   pos(px, py, pz),
                   normal(nx, ny, nz),
                   tangentU(tx, ty, tz),
                   texc(u, v) {}

            DirectX::XMFLOAT3 pos;
            DirectX::XMFLOAT3 normal;
            DirectX::XMFLOAT3 tangentU;
            DirectX::XMFLOAT2 texc;
        };

        struct Mesh
        {
            std::vector<Vertex>   vertices;
            std::vector<uint32_t> indices;   

            std::vector<uint16_t>& getIndices16()
            {
                if (m_indices16.empty())
                {
                    m_indices16.resize(indices.size());
                    for (size_t i = 0; i < indices.size(); ++i)
                        m_indices16[i] = static_cast<uint16_t>(indices[i]);
                }
                return m_indices16;
            }
        private:
            std::vector<uint16_t> m_indices16;
        };

        // Creates a box centerred at the origin with the given dimensions,
        // where each face has m rows and columns of vertices.
        static Mesh createBox(float width, float height, float depth, uint32_t numSubdivisions);

        // Creates a sphere centerred at the origin with the given radius.
        // The slices and stacks parameters control the degree of tessellation.
        static Mesh createSphere(float radius, uint32_t sliceCount, uint32_t stackCount);

        // Creates a geosphere cenetered at the origin with the given radius.
        // The depth controls the level of tessellation.
        static Mesh createGeoSphere(float radius, uint32_t numSubdivisions);

        // Creates a cylinder parallel to the y-axis, and centered about the origin.
        // The bottom and top radius can vary to form various cone shapes rather than true cylinders.
        // The slices and stacks parameters control the degree of tessellation.
        static Mesh createCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount);

        // Creates an mxn grid in the xz-plane with m rows and n columns,
        // centered at the origin with the specified width and depth.
        static Mesh createGrid(float width, float depth, uint32_t m, uint32_t n);

        // Creates a quad aligned with the screen.
        // This is useful for postprocessing and screen effects.
        static Mesh createQuad(float x, float y, float w, float h, float depth);

    private:
        static void subdivide(Mesh& mesh);
        static Vertex midPoint(const Vertex& v0, const Vertex& v1);
    };
}