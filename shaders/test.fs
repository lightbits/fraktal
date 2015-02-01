#version 150

in vec3 ray_origin;
in vec3 ray_direction;
out vec4 out_color;

#define EPSILON 0.01
#define STEPS 128
#define Z_FAR 0.5

float sphere(vec3 p, float r)
{
    return length(p) - r;
}

float ground(vec3 p)
{
    return p.y + 0.1;
}

float scene(vec3 p)
{
    float sphere0 = length(p) - 0.5;
    float sphere1 = length(p - vec3(0.2, 0.4, 0.0)) - 0.25;
    float ground = p.y + 0.1;
    float wall = p.z + 0.3 + 0.3 * p.x;
    return min(min(min(sphere0, sphere1), ground), wall);
}

vec3 normal(vec3 p)
{
    vec2 e = vec2(EPSILON, 0.0);
    return normalize(vec3(
                     scene(p + e.xyy) - scene(p - e.xyy),
                     scene(p + e.yxy) - scene(p - e.yxy),
                     scene(p + e.yyx) - scene(p - e.yyx))
    );
}

void main()
{
    vec3 rd = normalize(ray_direction);
    vec3 p = ray_origin;
    float t = 0.0;
    float d = 1.0;
    for (int i = 0; i < STEPS; i++)
    {
        d = scene(p);
        t += d;
        p += d * rd;
        if (d < EPSILON)
            break;

        if (t > 5.0)
            break;
    }

    if (d < EPSILON)
    {
        vec3 n = normal(p);
        out_color.rgb = (vec3(0.5) + 0.5 * n);
    }
    else
    {
        out_color.rgb = vec3(0.0);
    }
    out_color.a = 1.0;
}
