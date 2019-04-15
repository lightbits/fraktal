// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#define EPSILON 0.0001
#define STEPS 512
#define M_PI 3.1415926535897932384626433832795
#define Z_FAR 100.0
#define ZERO (min(iFrame,0))
#define MATERIAL_FLOOR -1.0
#define Z_FAR_VISIBILITY_TEST 10.0

float vmax(vec3 v) {
    return max(max(v.x, v.y), v.z);
}

// Cylinder standing upright on the xz plane
float fCylinder(vec3 p, float r, float height) {
    vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(r, height);
    return min(max(d.x,d.y),0.0) + length(max(d,0.0));
    // return d;
}

// Box: correct distance to corners
float fBox(vec3 p, vec3 b) {
    vec3 d = abs(p) - b;
    return length(max(d, vec3(0))) + vmax(min(d, vec3(0)));
}

vec4 _model(vec3 p)
{
    // p.xy = p.yx;
    #define SPECULAR_ROUGHNESS 0.1
    #define SPECULAR_EXPONENT 32.0
    float r = 0.1;
    float d = fCylinder(p, 0.5 - r, 0.3 - r) - r;
    d = max(d, -fBox(p - vec3(0.0,0.4,0.0), vec3(0.2)) + 0.1);

    vec3 iso_color = vec3(0.3);
    {
        float iso_thickness = 0.25*0.5;
        float iso_spacing = 0.4;
        float iso_count = 3.0;
        float iso_max = iso_count * iso_spacing + iso_thickness*0.5;
        float a = mod(d - iso_thickness*0.5, iso_spacing);
        float t = step(iso_spacing-iso_thickness, a) * (1.0 - step(iso_max, d));
        iso_color = mix(vec3(1.0), iso_color, t);
    }
    float ground = fBox(p - vec3(0,-1,0), vec3(4,1,4));
    if (ground < d) return vec4(iso_color,ground);
    else return vec4(0.6,0.1,0.1,d);
}

// Adapted from: lumina.sourceforge.net/Tutorials/Noise.html
vec2 seed = vec2(-1,1)*(iSamples*(1.0/12.0) + 1.0);
vec2 noise2f()
{
    seed += vec2(-1, 1);
    return vec2(fract(sin(dot(seed.xy, vec2(12.9898, 78.233))) * 43758.5453),
                fract(cos(dot(seed.xy, vec2(4.898, 7.23))) * 23421.631));
}

vec3 rayPinhole(vec2 fragOffset)
{
    vec2 uv = vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y) + fragOffset - iCameraCenter;
    float d = 1.0/length(vec3(uv, iCameraF));
    return vec3(uv*d, -iCameraF*d);
}

// Adapted from Inigo Quilez
// Source: http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
vec3 normal(vec3 p)
{
    const float ep = 0.0001;
    vec2 e = vec2(1.0,-1.0)*0.5773*ep;
    return normalize( e.xyy*_model( p + e.xyy ).w +
                      e.yyx*_model( p + e.yyx ).w +
                      e.yxy*_model( p + e.yxy ).w +
                      e.xxx*_model( p + e.xxx ).w );
}

bool trace(vec3 ro, vec3 rd, out vec3 hit, out vec3 albedo)
{
    float t = 0.0;
    for (int i = ZERO; i < STEPS; i++)
    {
        vec3 p = ro + t*rd;
        vec4 m = _model(p);
        t += m.w;
        if (m.w <= EPSILON)
        {
            albedo = m.rgb;
            hit = p;
            return true;
        }
        if (t > Z_FAR)
            break;
    }
    return false;
}

bool isVisible(vec3 ro, vec3 rd)
{
    float t = 0.0;
    for (int i = ZERO; i < STEPS; i++)
    {
        vec4 m = _model(ro + t*rd);
        t += m.w;
        if (m.w <= EPSILON)
            return false;
        if (t > Z_FAR_VISIBILITY_TEST)
            break;
    }
    return true;
}

vec3 cosineWeightedSample(vec3 normal)
{
    vec2 u = noise2f();
    float a = 0.99*(1.0 - 2.0*u[0]);
    float b = 0.99*(sqrt(1.0 - a*a));
    float phi = 6.2831853072*u[1];
    float x = normal.x + b*cos(phi);
    float y = normal.y + b*sin(phi);
    float z = normal.z + a;
    return normalize(vec3(x,y,z));
}

vec3 coneSample(vec3 dir, float extent)
{
    vec3 tangent = vec3(1.0,0.0,0.0);
    if (abs(dir.x) > 0.9)
        tangent = vec3(0.0,1.0,0.0);
    vec3 bitangent = cross(tangent, dir);
    tangent = cross(dir, bitangent);
    vec2 r = noise2f();
    r.x *= 2.0*M_PI;
    r.y = 1.0 - r.y*extent;
    float oneminus = sqrt(1.0 - r.y*r.y);
    return cos(r.x)*oneminus*tangent +
           sin(r.x)*oneminus*bitangent +
           r.y * dir;
}

vec3 color(vec3 p, vec3 ro, vec3 albedo)
{
    vec3 n = normal(p);
    vec3 v = normalize(p - ro); // from eye to point
    ro = p + n*2.0*EPSILON;

    vec3 result = vec3(0.0);

    // hemisphere sample
    vec3 rd = cosineWeightedSample(n);
    if (isVisible(ro,rd))
        result += vec3(1.0);

    // sun sample
    rd = iToSun;
    if (isVisible(ro,rd))
        result += vec3(1.0)*max(0.0,dot(n, rd))*(2.0/M_PI);

    result *= albedo;

    // specular
    vec3 w_s = reflect(v, n);
    #define SPECULAR_ROUGHNESS 0.1
    #define SPECULAR_EXPONENT 16.0
    rd = coneSample(w_s, SPECULAR_ROUGHNESS);
    if (isVisible(ro,rd))
    {
        float ndots = max(0.0, dot(iToSun, rd));
        result += vec3(1.0)*smoothstep(0.0, 0.1, pow(ndots,SPECULAR_EXPONENT)) / M_PI;
    }
    return result;
}

void main()
{
    vec3 rd = rayPinhole(2.0*(noise2f() - vec2(0.5)));
    vec3 ro = (iView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    rd = normalize((iView * vec4(rd, 0.0)).xyz);

    fragColor.rgb = vec3(1.0);
    vec3 p,albedo;
    if (trace(ro, rd, p, albedo))
        fragColor.rgb = color(p, ro, albedo);
    fragColor.a = 1.0;
}
