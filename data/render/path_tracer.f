// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

uniform vec2      iResolution;
uniform int       iFrame;
uniform vec2      iCameraCenter;
uniform float     iCameraF;
uniform mat4      iView;
uniform int       iSamples;
uniform vec3      iToSun;
uniform vec3      iSunStrength;
uniform float     iCosSunSize;
out vec4          fragColor;

#ifdef FRAKTAL_GUI
#widget(Camera, yfov=10deg, dir=(-20 deg, 30 deg), pos=(0,0,20))
#widget(Floor, height=-0.5)
#widget(Material, specular_exponent=256.0)
#widget(Sun, size=10deg, color=(1,1,0.8), intensity=250)
#endif

#define EPSILON 0.001
#define STEPS 512
#define DENOISE 0
#define BOUNCES 2
#define M_PI 3.1415926535897932384626433832795
#define MAX_DISTANCE 100.0

// forward declaration
float model(vec3 p);
vec3 material(vec3 p);

const vec3 skyDomeColor = vec3(0.1,0.2,0.7);

vec3 sky(vec3 rd)
{
    float cosSun = max(dot(rd, iToSun), 0.0);
    return mix(skyDomeColor, iSunStrength, step(iCosSunSize, cosSun));
}

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

vec3 uniformConeSample(vec3 Z, float cosThetaMax)
{
    vec3 X = vec3(1.0,0.0,0.0);
    vec3 Y = cross(Z, X);
    X = cross(Y, Z);
    vec2 u = noise2f();
    float cosTheta = (1.0 - u[0]) + u[0]*cosThetaMax;
    float sinTheta = sqrt(1 - cosTheta*cosTheta);
    float phi = u[1]*2.0*M_PI;
    float x = cos(phi) * sinTheta;
    float y = sin(phi) * sinTheta;
    float z = cosTheta;
    return x*X + y*Y + z*Z;
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

// This integrator implements only one material: Lambertian.
// Uses multiple importance sample, sampling from either BSDF
// or sun light source.
vec3 color(vec3 p, vec3 ro)
{
    vec3 fCosTheta = vec3(1.0);
    float pdf = 1.0;
    for (int bounce = 0; bounce < BOUNCES; bounce++)
    {
        vec3 n = normal(p);
        ro = p + n*2.0*EPSILON;
        vec3 m = material(p);

        // choose one of two sampling strategies
        if (noise2f().x > 0.5)
        {
            // cosine weighted hemisphere sample
            vec3 rd = cosineWeightedSample(n);
            float pdf_bsdf = 1.0/M_PI;
            float pdf_light = (1.0/(2.0*M_PI*(1.0 - iCosSunSize)))*step(iCosSunSize, max(0.0, dot(iToSun, rd)));
            pdf *= (0.5*pdf_bsdf + 0.5*pdf_light);
            fCosTheta *= m.rgb/M_PI; // note: cos(theta) gets cancelled when dividing by pdf

            float t = trace(ro, rd);
            if (t >= 0.0)
                p = ro + rd*t;
            else
                return sky(rd)*fCosTheta/pdf;
        }
        else
        {
            // cone sampled direct light
            vec3 rd = uniformConeSample(iToSun, iCosSunSize);
            float pdf_bsdf = 1.0/M_PI;
            float pdf_light = 1.0/(2.0*M_PI*(1.0 - iCosSunSize));
            pdf *= (0.5*pdf_bsdf + 0.5*pdf_light);
            fCosTheta *= m.rgb*(max(0.0,dot(n,rd))/M_PI);

            float t = trace(ro, rd);
            if (t >= 0.0)
                p = ro + rd*t;
            else
                return iSunStrength*fCosTheta/pdf;
        }
    }

    // Ran out of bounces and did not hit light...
    // Cheap approximation of surface light: ambient
    return skyDomeColor*fCosTheta/pdf;

    // Direct light approximation:
    // vec3 n = normal(p);
    // ro = p + n*2.0*EPSILON;
    // vec4 m = material(p, matIndex);
    // if (trace(ro, iToSun).y > EPSILON)
    //     return normalize(iSunStrength)*max(0.0,dot(n,iToSun))*m.rgb*fCosTheta/pdf;
    // else
    //     return skyDomeColor*fCosTheta/pdf;
}

void main()
{
    vec3 rd = rayPinhole(noise2f());
    vec3 ro = (iView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    rd = normalize((iView * vec4(rd, 0.0)).xyz);

    fragColor.rgb = sky(rd);
    float t = trace(ro, rd);
    if (t >= 0.0)
    {
        vec3 p = ro + t*rd;
        fragColor.rgb = color(p, ro);
    }
    fragColor.a = 1.0;
}
