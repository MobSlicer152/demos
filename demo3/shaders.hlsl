#include "shaders/noise.hlsl"

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

PSInput VSMain(uint vertexId : SV_VertexID, float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;
    
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    result.position = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    result.uv = uv;
    result.color = color;

    return result;
}

float aspect;
float time;
float speed;

struct Particle
{
    float2 position;
    float size;
};

StructuredBuffer<Particle> particles : register(t1);

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 aspectScale = float2(aspect, 1.0);
    float2 scaledUv = input.uv * aspectScale;
    float2 ray = (input.uv * 2.0 - 1.0) * aspectScale;
    uint particleCount;
    uint particleStride;
    particles.GetDimensions(particleCount, particleStride);
    for (uint i = 0; i < particleCount; i++)
    {
        float dist = length(abs(ray - particles[i].position * aspectScale)) / particles[i].size;
        float borderDist = 1.0 - dist;
        if (dist < 1.0)
        {
            if (borderDist < 0.05)
            {
                return float4(1.0, 1.0, 1.0, 1.0);
            }
            else
            {
                return float4(0.0, 0.0, 0.0, 1.0);
            }
        }
    }
    
    return fBM((ray - float2(time * speed, 0)), 8, 2, 0.5);
}
