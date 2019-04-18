// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#define EPSILON 0.001
#define STEPS 512
#define DENOISE 1
#define MAX_AO_DISTANCE 1.0
#define M_PI 3.1415926535897932384626433832795
#define ZERO (min(iFrame,0))

#if DENOISE
vec2 seed = vec2(-1,1)*(iSamples*(1.0/12.0) + 1.0);
#else
vec2 seed = (vec2(-1.0) + 2.0*gl_FragCoord.xy/iResolution.xy)*(iSamples*(1.0/12.0) + 1.0);
#endif
vec2 noise2f()
{
    // lumina.sourceforge.net/Tutorials/Noise.html
    seed += vec2(-1, 1);
    return vec2(fract(sin(dot(seed.xy, vec2(12.9898, 78.233))) * 43758.5453),
                fract(cos(dot(seed.xy, vec2(4.898, 7.23))) * 23421.631));
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

vec3 rayPinhole(vec2 fragOffset)
{
    vec2 uv = vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y) + fragOffset - iCameraCenter;
    float d = 1.0/length(vec3(uv, iCameraF));
    return vec3(uv*d, -iCameraF*d);
}

// http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
vec3 normal(vec3 p)
{
    const float ep = 0.0001;
    vec2 e = vec2(1.0,-1.0)*0.5773*ep;
    return normalize( e.xyy*model( p + e.xyy ).x +
                      e.yyx*model( p + e.yyx ).x +
                      e.yxy*model( p + e.yxy ).x +
                      e.xxx*model( p + e.xxx ).x );
}

vec3 trace(vec3 ro, vec3 rd)
{
    float t = 0.0;
    float min_d = 1000.0;
    for (int i = ZERO; i < STEPS; i++)
    {
        vec3 p = ro + t*rd;
        vec2 dm = model(p);
        float d = dm.x;
        t += d;
        min_d = min(d, min_d);
        if (d <= EPSILON)
            return vec3(t, min_d, dm.y);
    }
    return vec3(t, min_d, MATERIAL0);
}

float ambientOcclusion(vec3 p)
{
    vec3 n = normal(p);
    vec3 ro = p + 2.0*EPSILON*n;
    vec3 rd = cosineWeightedSample(n);
    float t = 0.0;
    for (int i = ZERO; i < STEPS && t < MAX_AO_DISTANCE; i++)
    {
        vec3 p = ro + t*rd;
        float d = model(p).x;
        t += d;
        if (d <= EPSILON)
            return 0.0;
    }
    return 1.0;
}

void main()
{
    vec3 rd = rayPinhole(noise2f());
    vec3 ro = (iView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    rd = normalize((iView * vec4(rd, 0.0)).xyz);

    vec3 tr = trace(ro, rd);
    if (tr.y > EPSILON)
        fragColor = vec4(1.0);
    else
        fragColor = vec4(vec3(1.0)*ambientOcclusion(ro + tr.x*rd), 1.0);
}
