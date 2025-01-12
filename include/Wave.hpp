#pragma once

class Wave
{
public:
    Wave() = default;
    Wave(int m, int n, float dx, float dt, float speed, float damping);
    ~Wave() = default;

    int rowCount = 0;
    int columnCount = 0;
    int vertexCount = 0;
    int triangleCount = 0;
    float width = 0.f;
    float depth = 0.f;

    const auto& position(int i) const { return _currSolution[i]; }
    const auto& normal(int i) const { return _normals[i]; }
    const auto& tangentX(int i) const { return _normals[i]; }

    void update(float dt);
    void disturb(int i, int j, float magnitude);

private:
    std::vector<DirectX::XMFLOAT3> _prevSolution; 
    std::vector<DirectX::XMFLOAT3> _currSolution; 
    std::vector<DirectX::XMFLOAT3> _normals;
    std::vector<DirectX::XMFLOAT3> _tangentX;

    float mk1 = 0;
    float mk2 = 0;
    float mk3 = 0;

    float _timeStep = 0.f;
    float _spatialStep = 0.f; 
};