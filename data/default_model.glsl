// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#resolution(200,200)
#view(dir=(-25 deg, 140 deg), pos=(0,0,35))
#camera(yfov=10deg, center=(100,100))
#sun(size=3 deg, dir=(40 deg, -90 deg), color=(1,1,0.8), intensity=500)

float cylinder(vec3 p, float r, float height)
{
    vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(r, height);
    return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float sphere(vec3 p, float r)
{
    return length(p) - r;
}

float box(vec3 p, vec3 b)
{
    return max(abs(p.x) - b.x, max(abs(p.y) - b.y, abs(p.z) - b.z));
}

vec4 material(vec3 p, float matIndex)
{
    if (matIndex == MATERIAL0)
        return vec4(0.6,0.1,0.1,1.0);
    else
        return vec4(1.0,1.0,1.0,1.0);
}

vec2 model(vec3 p)
{
    float L = 0.8; // Lipschitz constant
    float d = sphere(p, 1.0);
    d = max(d, -sphere(p - vec3(0.0,0.0,1.0), 0.5));
    d = min(d, sphere(p - vec3(2.0,1.0,0.0), 0.9));
    float m = MATERIAL1;
    float d_roof = box(p - vec3(0.0,3.0 + 0.5*sin(0.3*p.z + 0.5*p.x),2.0), vec3(4.0,0.1,4.0));
    vec3 q = p;
    q.y += 0.2;
    q.x = mod(q.x + 0.7, 1.4) - 0.7;
    q.z = mod(q.z + 0.7, 1.4) - 0.7;
    d_roof = max(d_roof, -(length(q.xz) - 0.4));
    if (d_roof < d)
    {
        d = d_roof;
        m = MATERIAL1;
    }
    float d_ground = box(p + vec3(0.0,0.3,0.0), vec3(4.0,0.1,4.0));
    if (d_ground < d)
    {
        d = d_ground;
        m = MATERIAL0;
    }
    return vec2(d*L, m);
}
