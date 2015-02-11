#version 150

uniform float aspect;
uniform float tan_fov_h;
uniform vec2 pixel_size;
uniform mat4 view;

in vec2 texel;
uniform float time;
uniform sampler2D tex_sky;
out vec4 out_color;

#define EPSILON 0.001
#define STEPS 256
#define Z_NEAR 0.1
#define Z_FAR 100.0
#define PI_HALF 1.57079632679
#define PI 3.14159265359
#define TWO_PI 6.28318530718

vec3 SampleSky(vec3 dir)
{
    float u = atan(dir.x, dir.z);
    float v = asin(dir.y);
    u /= TWO_PI;
    v /= PI;
    v *= -1.0;
    v += 0.5;
    u += 0.5;
    return texture(tex_sky, vec2(u, v)).rgb;
}

// http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float udRoundBox(vec3 p, vec3 b, float r)
{
    return length(max(abs(p)-b,0.0))-r;
}

float sdSphere(vec3 p, float r)
{
    return length(p) - r;
}

float opIntersect(float d1, float d2)
{
    return max(d1, d2);
}

float opSubtract(float d1, float d2)
{
    return max(d1, -d2);
}

float sdBox( vec3 p, vec3 b )
{
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) +
         length(max(d,0.0));
}

vec3 RotateY(vec3 v, float t)
{
    float cost = cos(t); float sint = sin(t);
    return vec3(v.x * cost + v.z * sint, v.y, -v.x * sint + v.z * cost);
}

vec3 RotateX(vec3 v, float t)
{
    float cost = cos(t); float sint = sin(t);
    return vec3(v.x, v.y * cost - v.z * sint, v.y * sint + v.z * cost);
}

float Mystery(vec3 p)
{
    float db = sdSphere(p, 1.5);
    p.xyz = mod(p.xyz, vec3(0.25)) - vec3(0.125);
    return opSubtract(db, sdSphere(p, 0.15));

    // p = RotateY(p, 0.01 * time);
    // float d1 = sdSphere(p, 0.8);
    // float d2 = sdBox(p, vec3(1.0, 0.4, 0.4));
    // float d3 = sdBox(p, vec3(0.1, 0.1, 2.0));
    // return min(d3, opSubtract(d1, d2));
}

float sdFloor(vec3 p)
{
    return sdSphere(p - vec3(0.0, -10000.0, 0.0), 9999.5);
}

float sdPillars(vec3 p)
{
    float height = 0.4 * exp(-dot(p, p) * 0.01);
    vec2 modrad = vec2(0.5);
    p.xz = mod(p.xz + modrad, 2.0 * modrad) - modrad;
    p = RotateY(p, 0.0 * time);
    return udRoundBox(p - vec3(0.0, -0.25, 0.0), vec3(0.1, height, 0.1), 0.05);
}

float Scene(vec3 p)
{
    float d1 = sdFloor(p);
    float d2 = sdPillars(p);
    float d3 = Mystery(p - vec3(0.0, 0.5, 0.0));
    return min(d1, min(d2, d3));
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

bool 
Trace(vec3 origin, vec3 dir, out vec3 hit_point)
{
    float t = 0.0;
    hit_point = origin;
    for (int i = 0; i < STEPS; i++)
    {
        float s = Scene(hit_point);
        t += s;
        hit_point += s * dir;
        if (s <= EPSILON)
            return true;
        if (t > Z_FAR)
            break;
    }
    return false;
}

vec2 seed = (texel) * (time + 1.0);
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

vec3 ConeSample(vec3 dir, float extent)
{
    vec3 tangent = normalize(Ortho(dir));
    vec3 bitangent = normalize(cross(dir, tangent));
    vec2 r = Noise2f();
    r.x *= TWO_PI;
    r.y *= 1.0 - r.y * extent;
    float oneminus = sqrt(1.0 - r.y * r.y);
    return cos(r.x) * oneminus * tangent + 
           sin(r.x) * oneminus * bitangent + 
           r.y * dir;
}

vec3 ComputeLight(vec3 hit, vec3 from)
{
    vec3 normal = Normal(hit);
    vec3 dir = CosineWeightedSample(normal);
    vec3 origin = hit + normal * 2.0 * EPSILON;

    if (!Trace(origin, dir, hit))
    {
        return SampleSky(dir);
    }

    return vec3(0.0);
}

vec2 SampleDisk()
{
    vec2 r = Noise2f();
    r.x *= TWO_PI;
    r.y = sqrt(r.y);
    float x = r.y * cos(r.x);
    float y = r.y * sin(r.y);
    return vec2(x, y);
}

void main()
{
    // Perturb texel to get anti-aliasing (for free! yay!)
    vec2 sample = texel;
    sample += (-1.0 + 2.0 * Noise2f()) * 0.5 * pixel_size;
    sample *= -1.0; // Flip before passing through lens

    float lens_radius = 0.05; // Affects field of depth
    float focal_distance = 3.0; // Affects where objects are in focus
    vec3 film = vec3(sample.x * aspect, sample.y, 0.0);
    vec3 lens_centre = vec3(0.0, 0.0, -1.0 / tan_fov_h);
    vec3 dir = normalize(lens_centre - film);
    float t = -focal_distance / dir.z;
    vec3 focus = lens_centre + dir * t;

    vec3 lens = lens_centre + lens_radius * vec3(SampleDisk(), 0.0);
    dir = normalize(focus - lens);
    vec3 origin = lens;

    origin = (view * vec4(origin, 1.0)).xyz;
    dir = normalize((view * vec4(dir, 0.0)).xyz);

    // Transform image plane coordinates via view-space matrix
    // vec3 film = vec3(sample.x * aspect, sample.y, -1.0 / tan_fov_h);
    // vec3 origin = (view[3]).xyz;
    // vec3 dir = normalize((view * vec4(film, 1.0)).xyz - origin);

    vec3 hit_point;
    if (Trace(origin, dir, hit_point))
    {
        out_color.rgb = ComputeLight(hit_point, dir);
    }
    else
    {
        out_color.rgb = SampleSky(dir);
    }

    // Visualize region in focus by a redline
    focus = (view * vec4(focus, 1.0)).xyz;
    vec3 focus_line = vec3(focus.x, -0.5, focus.z); // Project onto floor
    if (length(hit_point - focus_line) <= 0.3)
        out_color.r = 1.0;

    out_color.a = 1.0;
}
