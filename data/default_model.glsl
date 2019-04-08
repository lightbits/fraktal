#resolution(200,200)
// #view(dir=(-30 deg, 20deg), pos=(0,0,20))
// #camera(yfov=10deg, center=(100,100))
#camera(yfov=50deg, center=(100,100))
#material0(roughness=0.1)
#material1(albedo=(0.7,0.3,0.2), roughness=0.1)
#sun(size=30 deg, dir=(60 deg, 30 deg))

float sphere(vec3 p, float r)
{
    return length(p) - r;
}

float box(vec3 p, vec3 b)
{
    return max(abs(p.x) - b.x, max(abs(p.y) - b.y, abs(p.z) - b.z));
}

vec2 model(vec3 p)
{
    float d = sphere(p, 1.0);
    d = max(d, -sphere(p - vec3(0.0,0.0,1.0), 0.5));
    d = min(d, sphere(p - vec3(2.0,1.0,0.0), 0.9));
    float m = MATERIAL1;
    float d_roof = box(p - vec3(0.0,3.0,2.0), vec3(4.0,0.1,4.0));
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
    return vec2(d, m);
}
