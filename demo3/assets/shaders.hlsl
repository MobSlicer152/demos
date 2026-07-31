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

enum class Scene : int
{
	LoopingTheRooms,
	FountainStab,
	FountainStart,
	FountainGrow,
	FountainSmoke,
    Ending
};

enum class Pose : int
{
	Jump,
	Stab
};

Scene scene;
float aspect;
float elapsed;
float progress;
float2 ellipsePos;
float2 ellipseSize;
float brightness;
float fountainBrightness;
float fountainSize;
Pose pose;
float2 characterPos;

struct Particle
{
    float2 pos;
    float size;
    int layer;
    float alpha;
};

StructuredBuffer<Particle> particles : register(t1);
SamplerState texSampler : register(s2);
Texture2DArray<float4> textures : register(t2);
Texture2DArray<float4> character : register(t3);

float SmoothMin(float d1, float d2, float k)
{
    float h = exp(-k * d1) + exp(-k * d2);
    return -log(h) / k;
}

float Circle(float2 pos, float2 c, float r)
{
    return length(pos - c) - r;
}

float Box(float2 p, float2 bottomLeft, float2 topRight)
{
    float2 center = 0.5 * (bottomLeft + topRight);
    float2 extents = 0.5 * (topRight - bottomLeft);
    float2 d = abs(p -  center) - extents;
    return length(max(d, float2(0, 0))) + min(max(d.x, d.y), 0.0);
}

float SDF(float2 ray, float2 aspect, int layer)
{
    uint particleCount;
    uint particleStride;
    particles.GetDimensions(particleCount, particleStride);

    float k = particleCount;
    float s = 1e5;
    for (int i = 0; i < particleCount; i++)
    {
        if (particles[i].layer == layer && particles[i].size > 0.02) {
            float2 pos = particles[i].pos * aspect;
            float size = particles[i].size;
            s = SmoothMin(s, Circle(ray, pos, size), k);
        }
    }

    return s;
}

bool Smoke(out float4 color, float2 ray, float2 aspect)
{
    int smokeLayers = 1;
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

bool Ellipse(float2 ray)
{
    float2 dist = ray - ellipsePos;
    float dx = dist.x;
    float dy = dist.y;
    float rx = ellipseSize.x;
    float ry = ellipseSize.y;

    return (ry > 0 && rx > 0) && ((dx * dx) / (rx * rx) + (dy * dy) / (ry * ry) < 1);
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

float2 rotate(float2 p, float deg) {
    float r = length(p);
    float angle = atan2(p.y, p.x);
    float radians = angle + deg / 360.0 * (6.282);
    return float2(r * cos(radians), r * sin(radians));
}

float combine( float d1, float d2 ) { return min(d1,d2); }
float difference( float d1, float d2 ) { return max(d1,-d2); }
float intersect( float d1, float d2 ) { return max(d1,d2); }

float mixer() {
    return (sin(progress * 1.5) + 1.0) * 0.5;
}

bool Heart(out float4 color, float2 ray, float2 aspect)
{
    float2 p = ray * float2(1.0, -1.0) + float2(0.0, -0.15);
    // Mirror -x half plane into +x
    p.x =  -abs(p.x);

    float upperRadius = 0.2;
    float lowerRadius = 0.55;
    
    float upperHalf =  Circle(p, float2(-upperRadius,0), upperRadius);
    
    // Slightly increased extents for subtractive box to avoid small artifacts
    upperHalf = difference(upperHalf, Box(p, float2(-upperRadius * 2.1, -upperRadius * 1.1), float2(0.1, 0)));
    
    float lowerHalf = Circle(p, float2(-upperRadius * 2.0 + lowerRadius, 0), lowerRadius);
    lowerHalf = difference(lowerHalf, Box(p, float2(-upperRadius * 2.1, 0), float2(lowerRadius, lowerRadius * 1.1)));
    
    float combined = combine(upperHalf, lowerHalf);
    if (combined < 0.01)
    {
        color = float4(1.0, 0.0, 0.0, 1.0);
        return true;
    }

    return false;
}

float4 Stars(float2 ray)
{
    int starCount = 0;
    float scaleFactor = 1.0;
    if (scene == Scene::FountainStart)
    {
        starCount = 1;
        scaleFactor = 0.5;
    }
    else if (scene == Scene::FountainGrow)
    {
        starCount = -exp(progress - 3) + 8;
        scaleFactor = elapsed;
    }
    else
    {
        return 0.0;
    }

    float4 color = 0.0;
    for (int i = 0; i < starCount; i++)
    {
        float scaledI = starCount > 1 ? (float)i / starCount : 0.1;
        float factor = (scaledI) * (fmod(scaleFactor, scaledI) + 0.5) * 0.5;
        float opacity = factor;
        float2 scale = float2(4.0, 0.5) * factor;
        float2 center = float2(0.0, 0.5 + scale.y);
        if (abs(ray.x - center.x) < scale.x && abs(ray.y - center.y) < scale.y)
        {
            float2 uv = ((ray - center) / scale * 0.5 + 0.5);
            color += textures.Sample(texSampler, float3(uv, 0.0)) * opacity;
        }
    }

    return color;
}

bool Character(out float4 outColor, float2 ray)
{
    float2 scale = float2(0.3, 0.3);
    float2 center = characterPos;
    if (abs(ray.x - center.x) < scale.x && abs(ray.y - center.y) < scale.y)
    {
        float2 uv = ((ray - center) / scale * 0.5 + 0.5);
        float4 color = character.Sample(texSampler, float3(uv, (int)pose));
        outColor = 0.0;
        if (length(color - float4(1.0, 0.0, 1.0, 1.0)) != 0)
        {
            outColor = color;
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
    bool hit = false;

    float4 characterColor;
    bool characterHit = Character(characterColor, ray);
    bool characterContrib = characterHit && characterColor.w > 0.0;

    switch (scene)
    {
    case Scene::FountainStab:
    {
        if (characterContrib)
        {
            return characterColor;
        }
        break;
    }
    case Scene::FountainStart:
    {
        if (Ellipse(ray))
        {
            hit = true;
            outColor = float4(1.0, 1.0, 1.0, 1.0);
            if (characterContrib)
            {
                outColor -= length(characterColor) / 4;
            }
        }
        break;
    }
    case Scene::FountainGrow:
    {
        float4 fountainInner = float4(fountainBrightness, fountainBrightness, fountainBrightness, 1.0);
        hit = Parabola(outColor, ray, 0.1, fountainSize, 0.5, float2(0.0, 0.5), fountainInner, 1.0);
        if (characterContrib)
        {
            outColor += length(characterColor) / 4 * (fountainBrightness > 0.5 ? -1.0 : 1.0);
        }
        break;
    }
    case Scene::FountainSmoke:
    {
        hit = Smoke(outColor, ray, aspectScale);
        break;
    }
    case Scene::Ending:
    {
        if (Heart(outColor, ray, aspectScale))
        {
            return outColor;
        }
        return 0.0;
    }
    default:
        break;
    }

    if (hit)
    {
        return outColor;
    }
    if (characterContrib)
    {
        return characterColor;
    }

    float4 stars = Stars(ray);

    float noise = fBM((ray + float2(0, elapsed)), 16, 2, 0.75);
    if (brightness > 0.0f)
    {
        if (noise > 0.01f)
        {
            noise += brightness;
        }
        noise += 0.2 * brightness;
        noise = clamp(noise, 0, 0.9);
    }

    return stars + noise;
}
