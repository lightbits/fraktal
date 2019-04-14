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

vec2 rotate(vec2 p, float a) {
    return cos(a)*p + sin(a)*vec2(p.y, -p.x);
}

float smin(float a, float b, float r) {
    float e = max(r - abs(a - b), 0);
    return min(a, b) - e*e*0.25/r;
}

vec2 model(vec3 p)
{
    float d = sphere(p, 0.5);

    // grooves
    vec3 q = p;
    float t = q.y + 0.5;
    q.xz = rotate(q.xz, -smoothstep(0.0,0.5,0.5*t)*0.7);
    float a = atan(q.z, q.x);
    d -= 0.03*sin(14.0*a);
    d *= 0.5; // Lipschitz factor

    // Top
    p.y -= 0.5;
    d = smin(d, cylinder(p, 0.2, 1.0), 0.3);
    p.y -= 1.0;
    d = smin(d, cylinder(p, 0.3, 0.01), 0.1);
    d = max(d, -cylinder(p, 0.18, 0.5));

    float d_floor = box(vec3(p.x, p.y + 2.2, p.z), vec3(2.0,0.1,2.0));
    if (d_floor < d)
        return vec2(d_floor, MATERIAL0);
    else
        return vec2(d, MATERIAL1);
}
