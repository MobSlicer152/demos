float2 Hash(float2 p)
{
    p = float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)));
    return -1.0 + 2.0 * frac(sin(p) * 43758.5453123);
}

float Perlin(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);

    return lerp(lerp(dot(Hash(i + float2(0.0, 0.0)), f - float2(0.0, 0.0)),
                     dot(Hash(i + float2(1.0, 0.0)), f - float2(1.0, 0.0)), u.x),
                lerp(dot(Hash(i + float2(0.0, 1.0)), f - float2(0.0, 1.0)),
                     dot(Hash(i + float2(1.0, 1.0)), f - float2(1.0, 1.0)), u.x), u.y);
}

float fBM(float2 p, int octaves, float lacunarity, float gain)
{
    float a = 0.5;
    float freq = 1;
    float result = 0;
    for (int i = 0; i < octaves; i++)
    {
        result += a * Perlin(freq * p);
        freq *= lacunarity;
        a *= gain;
    }

    return result;
}

