#include "Geometry.hpp"

using namespace DX;
using namespace DirectX;

Geometry::Mesh Geometry::createBox(float width, float height, float depth, uint32_t numSubdivisions)
{
    Mesh mesh;

    	Vertex v[24];

	float w2 = 0.5f*width;
	float h2 = 0.5f*height;
	float d2 = 0.5f*depth;
    
	// Fill in the front face vertex data.
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the back face vertex data.
	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face vertex data.
	v[8]  = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9]  = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face vertex data.
	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	mesh.vertices.assign(&v[0], &v[24]);
 
	//
	// Create the indices.
	//

	uint32_t i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7]  = 5; i[8]  = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] =  9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	mesh.indices.assign(&i[0], &i[36]);

    // Put a cap on the number of subdivisions.
    numSubdivisions = std::min<uint32_t>(numSubdivisions, 6u);

    for(uint32_t i = 0; i < numSubdivisions; ++i)
        subdivide(mesh);

    return mesh;
}

Geometry::Mesh Geometry::createSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
{
    Mesh mesh;

    Vertex topVertex(0.f, radius, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f);
    Vertex bottomVertex(0.f, -radius, 0.f, 0.f, -1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f);

    mesh.vertices.push_back(topVertex);

    float phiStep = XM_PI / stackCount;
    float thetaStep = 2.f * XM_PI / sliceCount;

    for (uint32_t i = 1; i <= stackCount - 1; ++i)
    {
        float phi = i * phiStep;
        for (uint32_t j = 0; j <= sliceCount; ++j)
        {
            float theta = j * thetaStep;
            Vertex v;
            v.pos.x = radius * sinf(phi) * cosf(theta);
            v.pos.y = radius * cosf(phi);
            v.pos.z = radius * sinf(phi) * sinf(theta);
            v.tangentU.x = -radius * sinf(phi) * sinf(theta);
            v.tangentU.y = 0.f; 
            v.tangentU.z = radius * sinf(phi) * cosf(theta);

            auto P = XMLoadFloat3(&v.pos);
            XMStoreFloat3(&v.pos, XMVector3Normalize(P));
            auto T = XMLoadFloat3(&v.tangentU);
            XMStoreFloat3(&v.tangentU, XMVector3Normalize(T));

            v.texc.x = theta / XM_2PI;
            v.texc.y = phi / XM_PI;

            mesh.vertices.push_back(v);
        }
    }
    mesh.vertices.push_back(bottomVertex);

    for (uint32_t i = 0; i <= sliceCount; ++i)
    {
        mesh.indices.push_back(0);
        mesh.indices.push_back(i + 1);
        mesh.indices.push_back(i);
    }

    uint32_t baseIndex = 1;
    uint32_t ringVertexCount = sliceCount + 1;
    for (uint32_t i = 0; i < sliceCount; ++i)
    {
        for (uint32_t j = 0; j < sliceCount; ++j)
        {
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    auto southPoleIndex = static_cast<uint32_t>(mesh.indices.size()) - 1;
    baseIndex = southPoleIndex - ringVertexCount;
    for (uint32_t i = 0; i < sliceCount; ++i)
    {
        mesh.indices.push_back(southPoleIndex);
        mesh.indices.push_back(baseIndex + i);
        mesh.indices.push_back(baseIndex + i + 1);
    }

    return mesh;
}

void Geometry::subdivide(Mesh& mesh)
{
    Mesh inputCopy = mesh;
    mesh.vertices.resize(0);
    mesh.indices.resize(0);

    uint32_t numTris = inputCopy.indices.size() / 3;
    for (uint32_t i = 0; i < numTris; ++i)
    {
        Vertex v0 = inputCopy.vertices[inputCopy.indices[i * 3 + 0]];
        Vertex v1 = inputCopy.vertices[inputCopy.indices[i * 3 + 1]];
        Vertex v2 = inputCopy.vertices[inputCopy.indices[i * 3 + 2]];

        Vertex m0 = midPoint(v0, v1);
        Vertex m1 = midPoint(v1, v2);
        Vertex m2 = midPoint(v0, v2);

        mesh.vertices.push_back(v0); 
        mesh.vertices.push_back(v1); 
        mesh.vertices.push_back(v2); 
        mesh.vertices.push_back(m0); 
        mesh.vertices.push_back(m1); 
        mesh.vertices.push_back(m2); 

        mesh.indices.push_back(i * 6 + 0);
        mesh.indices.push_back(i * 6 + 3);
        mesh.indices.push_back(i * 6 + 5);
        mesh.indices.push_back(i * 6 + 3);
        mesh.indices.push_back(i * 6 + 4);
        mesh.indices.push_back(i * 6 + 5);
        mesh.indices.push_back(i * 6 + 5);
        mesh.indices.push_back(i * 6 + 4);
        mesh.indices.push_back(i * 6 + 2);
        mesh.indices.push_back(i * 6 + 3);
        mesh.indices.push_back(i * 6 + 1);
        mesh.indices.push_back(i * 6 + 4);
    }
}

Geometry::Vertex Geometry::midPoint(const Vertex& v0, const Vertex& v1)
{
    Vertex vertex;

    auto p0 = XMLoadFloat3(&v0.pos);
    auto p1 = XMLoadFloat3(&v1.pos);
    auto n0 = XMLoadFloat3(&v0.normal);
    auto n1 = XMLoadFloat3(&v1.normal);
    auto tan0 = XMLoadFloat3(&v0.tangentU);
    auto tan1 = XMLoadFloat3(&v1.tangentU);
    auto tex0 = XMLoadFloat2(&v0.texc);
    auto tex1 = XMLoadFloat2(&v1.texc);

    auto pos = 0.5f * (p1 + p1);
    auto normal = XMVector3Normalize(0.5f * (n0 + n1));
    auto tangent = XMVector3Normalize(0.5f * (tan0 + tan1));
    auto tex = 0.5f * (tex0 + tex1);

    XMStoreFloat3(&vertex.pos, pos);
    XMStoreFloat3(&vertex.normal, normal);
    XMStoreFloat3(&vertex.tangentU, tangent);
    XMStoreFloat3(&vertex.pos, pos);

    return vertex;
}

Geometry::Mesh Geometry::createGeoSphere(float radius, uint32_t numSubdivisions)
{
    Mesh mesh;

    numSubdivisions = std::min<uint32_t>(numSubdivisions, 6U);

    constexpr float X = 0.525731f;
    constexpr float Z = 0.850651f;

    XMFLOAT3 pos[12] = 
    {
		XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),  
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),    
		XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X), 
		XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),    
		XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f), 
		XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
	};

    uint32_t K[60] =
    {
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,    
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,    
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0, 
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7 
	};

    mesh.vertices.resize(12);
    mesh.indices.assign(&K[0], &K[60]);

    for (uint32_t i = 0; i < 12; ++i)
        mesh.vertices[i].pos = pos[i];
    for (uint32_t i = 0; i < numSubdivisions; ++i)
        subdivide(mesh);

    for (uint32_t i = 0; i < mesh.vertices.size(); ++i)
    {
        auto n = XMVector3Normalize(XMLoadFloat3(&mesh.vertices[i].pos));
        auto p = radius * n;
        XMStoreFloat3(&mesh.vertices[i].pos, p);
        XMStoreFloat3(&mesh.vertices[i].normal, n);

        float theta = atan2f(mesh.vertices[i].pos.z, mesh.vertices[i].pos.x);
        if (theta < 0.f)
            theta += XM_2PI;
        float phi = acosf(mesh.vertices[i].pos.y / radius);

        mesh.vertices[i].texc.x = theta / XM_2PI;
        mesh.vertices[i].texc.y = phi / XM_PI;

        mesh.vertices[i].tangentU.x = -radius * sinf(phi) * sinf(theta);
        mesh.vertices[i].tangentU.y = 0.f; 
        mesh.vertices[i].tangentU.z = radius * sinf(phi) * cosf(theta);

        auto T = XMLoadFloat3(&mesh.vertices[i].tangentU);
        XMStoreFloat3(&mesh.vertices[i].tangentU, XMVector3Normalize(T));
    }

    return mesh;
}

Geometry::Mesh Geometry::createCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount)
{
    Mesh mesh;

    // build stack
    float stackHeight = height / stackCount;
    float radiusStep  = (topRadius - bottomRadius) / stackCount;
    float dTheta      = 2.0f * DirectX::XM_PI / sliceCount;

    uint32_t ringCount = stackCount + 1;
    for (uint32_t i = 0; i < ringCount; ++i)
    {
        float y = -0.5f * height + i * stackHeight;
        float r = bottomRadius + i * radiusStep;

        for (uint32_t j = 0; j <= sliceCount; ++j)
        {
            Vertex vertex;

            float c = cosf(j * dTheta);
            float s = sinf(j * dTheta);

            vertex.tangentU = XMFLOAT3(-s, 0.f, c);

            float dr = bottomRadius - topRadius;
            XMFLOAT3 bitangent(dr * c, -height, dr * s);

            XMVECTOR T = XMLoadFloat3(&vertex.tangentU);
            XMVECTOR B = XMLoadFloat3(&bitangent);
            XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
            XMStoreFloat3(&vertex.normal, N);

            mesh.vertices.push_back(vertex);
        }

        auto ringVertexCount = sliceCount + 1;
        for (uint32_t i = 0; i < stackCount; ++i)
        {
            for (uint32_t j = 0; j < sliceCount; ++j)
            {
                mesh.indices.push_back(i * ringVertexCount + j);
                mesh.indices.push_back((i + 1) * ringVertexCount + j);
                mesh.indices.push_back((i + 1) * ringVertexCount + j + 1);
                mesh.indices.push_back(i * ringVertexCount + j);
                mesh.indices.push_back((i + 1) * ringVertexCount + j + 1);
                mesh.indices.push_back(i * ringVertexCount + j + 1);
            }
        }
    }

    // build top cap
    auto baseIndex = mesh.vertices.size();
    float y = 0.5f * height;
    Vertex vertex;
    vertex.normal = { 0.f, 1.f,0.f };
    vertex.tangentU = { 1.f, 0.f, 0.f };
    for (uint32_t i = 0; i < sliceCount; ++i)
    {
        float x = topRadius * cosf(i * dTheta);
        float z = topRadius * sinf(i * dTheta);
        float u = x / height + 0.5f;
        float v = z / height + 0.5f;
        vertex.pos = { x, y, z };
        vertex.texc = { u, v };
        mesh.vertices.push_back(vertex);
    }

    vertex.pos = { 0.f, y, 0.f };
    vertex.texc = { 0.5f, 0.5f };

    auto centerIndex = mesh.vertices.size() - 1;
    for (uint32_t i = 0; i < sliceCount; ++i)
    {
        mesh.indices.push_back(centerIndex);
        mesh.indices.push_back(baseIndex + i + 1);
        mesh.indices.push_back(baseIndex + i);
    }

    // build bottom cap
    baseIndex = mesh.vertices.size();
    y = -0.5f * height;
    vertex.normal = { 0.f, -1.f, 0.f };
    vertex.tangentU = { 1.f, 0.f, 0.f };
    for (uint32_t i = 0; i <= sliceCount; ++i)
    {
        float x = bottomRadius * cosf(i * dTheta);
        float z = bottomRadius * sinf(i * dTheta);
        float u = x / height + 0.5f;
        float v = z / height + 0.5f;
        vertex.pos = { x, y, z };
        vertex.texc = { u, v };
        mesh.vertices.push_back(vertex);
    }
    vertex.pos = { 0.f, y, 0.f };
    vertex.normal = { 0.f, 0.f, -1.f };
    vertex.texc = { 0.5f, 0.5f };
    mesh.vertices.push_back(vertex);

    centerIndex = mesh.vertices.size() - 1;
    for (uint32_t i = 0; i < sliceCount; ++i)
    {
        mesh.indices.push_back(centerIndex);
        mesh.indices.push_back(baseIndex + i);
        mesh.indices.push_back(baseIndex + i + 1);
    }

    return mesh;
}

Geometry::Mesh Geometry::createGrid(float width, float depth, uint32_t m, uint32_t n)
{
    Mesh mesh;

	uint32_t vertexCount = m*n;
	uint32_t faceCount   = (m-1)*(n-1)*2;

	//
	// Create the vertices.
	//

	float halfWidth = 0.5f*width;
	float halfDepth = 0.5f*depth;

	float dx = width / (n-1);
	float dz = depth / (m-1);

	float du = 1.0f / (n-1);
	float dv = 1.0f / (m-1);

	mesh.vertices.resize(vertexCount);
	for(uint32_t i = 0; i < m; ++i)
	{
		float z = halfDepth - i*dz;
		for(uint32_t j = 0; j < n; ++j)
		{
			float x = -halfWidth + j*dx;

			mesh.vertices[i*n+j].pos = XMFLOAT3(x, 0.0f, z);
			mesh.vertices[i*n+j].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			mesh.vertices[i*n+j].tangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// Stretch texture over grid.
			mesh.vertices[i*n+j].texc.x = j*du;
			mesh.vertices[i*n+j].texc.y = i*dv;
		}
	}
 
    //
	// Create the indices.
	//

	mesh.indices.resize(faceCount*3); // 3 indices per face

	// Iterate over each quad and compute indices.
	uint32_t k = 0;
	for(uint32_t i = 0; i < m-1; ++i)
	{
		for(uint32_t j = 0; j < n-1; ++j)
		{
			mesh.indices[k]   = i*n+j;
			mesh.indices[k+1] = i*n+j+1;
			mesh.indices[k+2] = (i+1)*n+j;

			mesh.indices[k+3] = (i+1)*n+j;
			mesh.indices[k+4] = i*n+j+1;
			mesh.indices[k+5] = (i+1)*n+j+1;

			k += 6; // next quad
		}
	}

    return mesh;
}

Geometry::Mesh Geometry::createQuad(float x, float y, float w, float h, float depth)
{
    Mesh mesh;

	mesh.vertices.resize(4);
	mesh.indices.resize(6);

	// Position coordinates specified in NDC space.
	mesh.vertices[0] = Vertex(
        x, y - h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f);

	mesh.vertices[1] = Vertex(
		x, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f);

	mesh.vertices[2] = Vertex(
		x+w, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f);

	mesh.vertices[3] = Vertex(
		x+w, y-h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f);

	mesh.indices[0] = 0;
	mesh.indices[1] = 1;
	mesh.indices[2] = 2;

	mesh.indices[3] = 0;
	mesh.indices[4] = 2;
	mesh.indices[5] = 3;

    return mesh;
}