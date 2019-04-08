#define EPSILON 0.001
#define STEPS 512
#define DENOISER_ENABLED
#define BOUNCES 1

#define M_PI 3.1415926535897932384626433832795
#define ZERO (min(iFrame,0))

const float sunSize = 0.05;
const float cosSunSize = cos(sunSize);
const vec3 toSun = normalize(vec3(0.0, 2.0, -1.5));
const vec3 sunStrength = 250.0*vec3(1.0,1.0,0.8);

vec3 sky(vec3 rd)
{
    float cosSun = max(dot(rd, toSun), 0.0);
    return mix(vec3(0.1,0.2,0.7), sunStrength, step(cosSunSize, cosSun));
}

vec4 material(vec3 p, float m)
{
    if (m == MATERIAL1)
        return vec4(1.0,1.0,1.0,0.3);
    else
        return vec4(0.6,0.1,0.1,0.1);
}

#ifdef DENOISER_ENABLED
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

vec3 color(vec3 p, float matIndex)
{
    vec3 n = normal(p);
    vec3 ro = p + n*2.0*EPSILON;
    vec4 m = material(p, matIndex);

    // cosine weighted hemisphere sample
    vec3 result = vec3(0.0);
    vec3 R = m.rgb / M_PI; // lambertian
    vec3 L = vec3(0.0);
    vec3 rd = cosineWeightedSample(n);
    float pdf_hemisphere = 1.0/M_PI;
    float pdf_direct = mix(0.0, 1.0/(2.0*M_PI*(1.0 - cosSunSize)), step(cosSunSize, max(0.0, dot(toSun, rd))));
    float pdf = 0.5*pdf_hemisphere + 0.5*pdf_direct;

    vec3 tr = trace(ro, rd);
    if (tr.y > EPSILON)
    {
        L += sky(rd) / pdf;
    }
    else
    {
        #if BOUNCES > 0
        // cosine weighted hemisphere sample
        vec3 p = ro + rd*tr.x;
        vec3 n = normal(p);
        vec3 ro = p + n*2.0*EPSILON;
        vec4 m = material(p, tr.z);
        vec3 R = m.rgb / M_PI;
        vec3 rd = cosineWeightedSample(n);
        vec3 tr = trace(ro, rd);
        pdf_hemisphere = 1.0/M_PI;
        pdf_direct = mix(0.0, 1.0/(2.0*M_PI*(1.0 - cosSunSize)), step(cosSunSize, max(0.0, dot(toSun, rd))));
        pdf = 0.5*pdf_hemisphere + 0.5*pdf_direct;
        if (tr.y > EPSILON)
            L += sky(rd)*R / pdf;

        // importance-sampled direct light
        rd = uniformConeSample(toSun, cosSunSize);
        pdf_hemisphere = 1.0/M_PI;
        pdf_direct = 1.0/(2.0*M_PI*(1.0 - cosSunSize));
        pdf = 0.5*pdf_hemisphere + 0.5*pdf_direct;
        tr = trace(ro, rd);
        if (tr.y > EPSILON)
            L += sunStrength*max(0.0,dot(n,rd))*R / pdf;
        #endif
    }
    result += L*R;
    R = m.rgb / M_PI;
    L = vec3(0.0);

    // importance-sampled direct light
    rd = uniformConeSample(toSun, cosSunSize);
    pdf_hemisphere = 1.0/M_PI;
    pdf_direct = 1.0/(2.0*M_PI*(1.0 - cosSunSize));
    pdf = 0.5*pdf_hemisphere + 0.5*pdf_direct;
    tr = trace(ro, rd);
    if (tr.y > EPSILON)
        L += sunStrength*max(0.0,dot(n,rd)) / pdf;
    result += L*R;

    return result;
}

void main()
{
    vec3 rd = rayPinhole(noise2f());
    vec3 ro = (iView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    rd = normalize((iView * vec4(rd, 0.0)).xyz);

    fragColor.rgb = sky(rd);
    vec3 tr = trace(ro, rd);
    if (tr.y <= EPSILON)
    {
        vec3 p = ro + tr.x*rd;
        fragColor.rgb = color(p, tr.z);
    }
    fragColor.a = 1.0;
}
