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
