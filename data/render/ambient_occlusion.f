// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

uniform vec2      iResolution;
uniform vec2      iCameraCenter;
uniform float     iCameraF;
uniform mat4      iView;
uniform int       iSamples;
out vec4          fragColor;

#ifdef FRAKTAL_GUI
#widget(Camera, yfov=10deg, dir=(-20 deg, 30 deg), pos=(0,0,20))
#endif

#define EPSILON 0.0001
#define STEPS 512
#define DENOISE 1
#define MAX_DISTANCE 100.0
#define MAX_AO_DISTANCE 1.0
#define M_PI 3.1415926535897932384626433832795

float model(vec3 p); // forward-declaration

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
    return normalize( e.xyy*model( p + e.xyy ) +
                      e.yyx*model( p + e.yyx ) +
                      e.yxy*model( p + e.yxy ) +
                      e.xxx*model( p + e.xxx ) );
}

float trace(vec3 ro, vec3 rd)
{
    float t = 0.0;
    for (int i = ZERO; i < STEPS; i++)
    {
        vec3 p = ro + t*rd;
        float d = model(p);
        if (d <= EPSILON) return t;
        t += d;
        if (t > MAX_DISTANCE) break;
    }
    return -1.0;
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
        float d = model(p);
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

    fragColor = vec4(1.0);
    float t = trace(ro, rd);
    if (t > 0.0)
        fragColor.rgb = vec3(1.0)*ambientOcclusion(ro + t*rd);
}
