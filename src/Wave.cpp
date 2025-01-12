#include "Wave.hpp"
#include <ppl.h>

using namespace DirectX;

Wave::Wave(int m, int n, float dx, float dt, float speed, float damping)
{
    rowCount = m;
    columnCount = n;

    vertexCount = m * n; 
    triangleCount = (m - 1) * (n - 1) * 2;

    _timeStep = dt;
    _spatialStep = dx;

    float d = damping * dt + 2.f;
    float e = (speed * speed) * (dt * dt) / (dx * dx);
    mk1 = (damping * dt - 2.f) / d;
    mk2 = (4.f - 8.f * e) / d;
    mk3 = (2.f * e) / d;

    _prevSolution.resize(m * n);
    _currSolution.resize(m * n);
    _normals.resize(m * n);
    _tangentX.resize(m * n);

    float halfWidth = (n - 1) * dx * .5f;
    float halfDepth = (m - 1) * dx * .5f;
    for (int i = 0; i < m; ++i)
    {
        float z = halfDepth - i * dx;
        for (int j = 0; j < n; ++j)
        {
            float x = -halfWidth + j * dx;

            _prevSolution[i * n + j] = XMFLOAT3(x, 0.f, z);
            _currSolution[i * n + j] = XMFLOAT3(x, 0.f, z);
            _normals[i * n + j] = XMFLOAT3(0.f, 1.f, 0.f);
            _tangentX[i * n + j] = XMFLOAT3(1.f, 0.f, 0.f); 
        }
    }
}

void Wave::update(float dt)
{
    static float t = 0;

    t += dt;

    if (t >= _timeStep)
    {
        concurrency::parallel_for(1, rowCount - 1, [this](int i) {
            for (int i = 0; i < rowCount - 1; ++i)
            {
                for (int j = 0; j < columnCount - 1; ++j)
                {
                    _prevSolution[i * columnCount + j].y = 
                        mk1 * _prevSolution[i * columnCount + j].y +
                        mk2 * _currSolution[i * columnCount + j].y +
                        mk3 * (_currSolution[(i + 1) * columnCount + j].y +
                               _currSolution[(i - 1) * columnCount + j].y +
                               _currSolution[i * columnCount + j + 1].y +
                               _currSolution[i * columnCount + j - 1].y);
                }
            }
        });

        std::swap(_prevSolution, _currSolution);

        t = 0.f;

        concurrency::parallel_for(1, rowCount - 1, [this](int i) {
            for (int i = 0; i < rowCount - 1; ++i)
            {
                for (int j = 0; j < columnCount - 1; ++j)
                {
                    float l = _currSolution[i*columnCount+j-1].y;
				    float r = _currSolution[i*columnCount+j+1].y;
				    float t = _currSolution[(i-1)*columnCount+j].y;
				    float b = _currSolution[(i+1)*columnCount+j].y;
				    _normals[i*columnCount+j].x = -r+l;
				    _normals[i*columnCount+j].y = 2.0f*_spatialStep;
				    _normals[i*columnCount+j].z = b-t;

				    XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&_normals[i*columnCount+j]));
				    XMStoreFloat3(&_normals[i*columnCount+j], n);

				    _tangentX[i*columnCount+j] = XMFLOAT3(2.0f*_spatialStep, r-l, 0.0f);
				    XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&_tangentX[i*columnCount+j]));
				    XMStoreFloat3(&_tangentX[i*columnCount+j], T);
                }
            }
        });
    }
}

void Wave::disturb(int i, int j, float magnitude)
{
    assert(i > 1 && i < rowCount - 2);
    assert(j > 1 && j < columnCount - 2);

    float halfMag = .5f * magnitude;

    _currSolution[i * columnCount + j].y += magnitude;
    _currSolution[i * columnCount + j + 1].y += halfMag;
    _currSolution[i * columnCount + j - 1].y += halfMag;
    _currSolution[(i + 1) * columnCount + j].y += halfMag;
    _currSolution[(i - 1) * columnCount + j].y += halfMag;
}