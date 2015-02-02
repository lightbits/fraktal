#version 150

in vec3 ray_origin;
in vec3 ray_direction;
in vec2 texel;
uniform float time;
out vec4 out_color;

#define EPSILON 0.001
#define STEPS 256
#define Z_NEAR 0.1
#define Z_FAR 15.0
const vec3 LIGHT_POS = vec3(1.0);
const float LIGHT_RAD = 0.2;

vec3 SampleSky(vec3 d)
{
    return vec3(1.0, 0.85, 0.82) * d.y;
}

float Scene(vec3 p)
{
    float sphere0 = length(p) - 0.5;
    float sphere1 = length(p - vec3(0.2, 0.4, 0.0)) - 0.25;
    float sphere2 = length(p - LIGHT_POS) - LIGHT_RAD;
    float ground = p.y + 0.1;
    float wall = p.z + 0.3 + 0.3 * p.x;
    return min(min(sphere0, min(sphere1, sphere2)), ground);
    // return min(min(min(sphere0, sphere1), ground), wall);
}

vec3 Normal(vec3 p)
{
    vec2 e = vec2(EPSILON, 0.0);
    return normalize(vec3(
                     Scene(p + e.xyy) - Scene(p - e.xyy),
                     Scene(p + e.yxy) - Scene(p - e.yxy),
                     Scene(p + e.yyx) - Scene(p - e.yyx))
    );
}

void 
Trace(vec3 origin, vec3 dir, 
      out vec3 hit_point, out float travel, out float nearest)
{
    hit_point = origin;
    travel = 0.0;
    nearest = Z_FAR;
    for (int i = 0; i < STEPS; i++)
    {
        float s = Scene(hit_point);
        travel += s;
        hit_point += s * dir;
        nearest = min(nearest, s);
        if (s < EPSILON)
            break;
        if (travel > Z_FAR)
            break;
    }
}

vec2 seed = texel * (float(time) + 1.0);

vec2 Noise2f() {
    seed += vec2(-1, 1);
    // implementation based on: lumina.sourceforge.net/Tutorials/Noise.html
    return vec2(fract(sin(dot(seed.xy, vec2(12.9898, 78.233))) * 43758.5453),
        fract(cos(dot(seed.xy, vec2(4.898, 7.23))) * 23421.631));
}

// See http://lolengine.net/blog/2013/09/21/picking-orthogonal-vector-combing-coconuts
vec3 Ortho(vec3 v)
{
    return abs(v.x) > abs(v.z) ? vec3(-v.y, v.x, 0.0)
                               : vec3(0.0, -v.z, v.y);
}

#define PI_HALF 1.57079632679
#define PI 3.14159265359
#define TWO_PI 6.28318530718
vec3 CosineWeightedSample(vec3 normal)
{
    vec3 tangent = normalize(Ortho(normal));
    vec3 bitangent = normalize(cross(normal, tangent));
    vec2 r = Noise2f();
    r.x *= TWO_PI;
    r.y = pow(r.y, 1.0 / 2.0);
    float oneminus = sqrt(1.0 - r.y * r.y);
    return cos(r.x) * oneminus * tangent + 
           sin(r.x) * oneminus * bitangent + 
           r.y * normal;
}

vec3 ComputeLight(vec3 hit, vec3 from)
{
    vec3 normal = Normal(hit);
    vec3 dir = CosineWeightedSample(normal);
    vec3 origin = hit + normal * 2.0 * EPSILON;

    float travel, nearest;
    Trace(origin, dir, hit, travel, nearest);

    if (nearest > EPSILON)
        return SampleSky(dir);

    return vec3(0.0);
}

void main()
{
    vec3 dir = normalize(ray_direction);
    vec3 origin = ray_origin;
    vec3 hit_point;
    float travel;
    float nearest;
    Trace(origin, dir, hit_point, travel, nearest);

    if (nearest < EPSILON)
    {
        out_color.rgb = ComputeLight(hit_point, dir);
    }
    else
    {
        out_color.rgb = vec3(0.0);
    }
    out_color.a = 1.0;
}
