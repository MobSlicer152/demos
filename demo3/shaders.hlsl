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

enum class Scene
{
	LoopingTheRooms,
	FountainStab,
	FountainStart,
	FountainGrow,
	FountainSmoke,
};

Scene scene;
float aspect;
float time;
float speed;
float brightness;
float fountainBrightness;
float fountainSize;
int smokeLayers;

struct Particle
{
    float2 position;
    float size;
    int layer;
};

StructuredBuffer<Particle> particles : register(t1);

float SmoothMin(float d1, float d2, float k)
{
    float h = exp(-k * d1) + exp(-k * d2);
    return -log(h) / k;
}

float Circle(float2 pos, float2 c, float r)
{
    return length(pos - c) - r;
}

float SDF(float2 ray, float2 aspect, int layer)
{
    uint particleCount;
    uint particleStride;
    particles.GetDimensions(particleCount, particleStride);

    float k = particleCount + 1;
    float s = 1e5;
    for (int i = 0; i < particleCount; i++)
    {
        if (particles[i].layer == layer) {
            float2 pos = particles[i].position * aspect;
            float size = particles[i].size;
            s = SmoothMin(s, Circle(ray, pos, size), k);
        }
    }

    return s;
}

bool Smoke(out float4 color, float2 ray, float2 aspect)
{
    for (int i = 0; i < smokeLayers; i++)
    {
        float sdf = SDF(ray, aspect, i);
        float inv = 1.0 / sdf;
        if (sdf < 0.015)
        {
            color = float4(0.0, 0.0, 0.0, 1.0);
            if (inv > 1.0)
            {
                color = float4(1.0, 1.0, 1.0, 1.0);
            }
            
            return true;
        }
    }

    return false;
}

bool Parabola(out float4 color, float2 ray, float scale, float a, float border, float2 vertex, float4 inside, float4 edge)
{
    scale = 1.0 / scale;
    
    float dx = -(ray.x - vertex.x) * scale;
    float dy = -(ray.y - vertex.y) * scale;
    float d = -(dx * dx - 4 * a * dy);
    if (d > 0.0)
    {
        color = inside;
        if (d < border)
        {
            color = edge;
        }

        return true;
    }

    return false;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 aspectScale = float2(aspect, 1.0);
    float2 scaledUv = input.uv * aspectScale;
    float2 ray = (input.uv * 2.0 - 1.0) * aspectScale;

    float4 outColor;

    switch (scene)
    {
    case Scene::FountainGrow:
    {
        float4 fountainInner = float4(fountainBrightness, fountainBrightness, fountainBrightness, 1.0);
        if (Parabola(outColor, ray, 0.1, fountainSize, 0.5, float2(0.0, 0.5), fountainInner, 1.0))
        {
            return outColor;
        }
        break;
    }
    case Scene::FountainSmoke:
    {
        if (Smoke(outColor, ray, aspectScale))
        {
            return outColor;
        }
        break;
    }
    }
    
    return fBM((ray - float2(time * speed, 0)), 8, 2, 0.75) + clamp(brightness - 0.2, 0, 1);
}
