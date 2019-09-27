// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

uniform vec2      iLowResolution;
uniform vec2      iResolution;
uniform vec2      iCameraCenter;
uniform float     iCameraF;
uniform float     iGroundHeight;
uniform mat4      iView;
uniform int       iSamples;
uniform sampler2D iChannel0;
uniform int       iMode;
out vec4          fragColor;

#define EPSILON 0.0007
#define STEPS 512
#define MAX_DISTANCE 100.0
#define INFINITY (9999999999.0)
#define MAX_AO_DISTANCE 1.0
#define M_PI 3.1415926535897932384626433832795

float model(vec3 p); // forward-declaration

// lumina.sourceforge.net/Tutorials/Noise.html
vec2 seed = vec2(-1,1)*(iSamples*(1.0/12.0) + 1.0);
vec2 noise2f()
{
    seed += vec2(-1, 1);
    return vec2(fract(sin(dot(seed.xy, vec2(12.9898, 78.233))) * 43758.5453),
                fract(cos(dot(seed.xy, vec2(4.898, 7.23))) * 23421.631));
}

// http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
vec3 normalModel(vec3 p)
{
    const float ep = 0.0001;
    vec2 e = vec2(1.0,-1.0)*0.5773*ep;
    return normalize( e.xyy*model( p + e.xyy ) +
                      e.yyx*model( p + e.yyx ) +
                      e.yxy*model( p + e.yxy ) +
                      e.xxx*model( p + e.xxx ) );
}

float traceModel(vec3 ro, vec3 rd, float t)
{
    for (int i = ZERO; i < STEPS; i++)
    {
        vec3 p = ro + t*rd;
        float d = model(p);
        if (d <= EPSILON) return t;
        t += d;
        if (t > MAX_DISTANCE) return INFINITY;
    }
    return INFINITY;
}

float traceGround(vec3 ro, vec3 rd)
{
    if (rd.y >= 0.0) return INFINITY;
    else return (iGroundHeight - ro.y)/rd.y;
}

vec3 normalGround(vec3 p)
{
    return vec3(0.0, 1.0, 0.0);
}

// from https://www.shadertoy.com/view/3lsSzf
float occlusion( in vec3 pos, in vec3 nor )
{
    float occ = 0.0;
    float sca = 1.0;
    for( int i=ZERO; i<5; i++ )
    {
        float h = 0.01 + 0.11*float(i)/4.0;
        vec3 opos = pos + h*nor;
        float d = min(opos.y - iGroundHeight, model(opos));
        occ += (h-d)*sca;
        sca *= 0.95;
    }
    return clamp( 1.0 - 2.0*occ, 0.0, 1.0 );
}

float visibility(in vec3 ro, in vec3 n, in vec3 rd)
{
    if (traceGround(ro + 2.0*EPSILON*n, rd) < INFINITY)
        return 0.0;
    if (traceModel(ro + 2.0*EPSILON*n, rd, 0.0) < INFINITY)
        return 0.0;
    return 1.0;
}

float checkerboard(vec3 p)
{
    return mod(floor(p.x) + floor(p.y + 0.001) + floor(p.z), 2.0);
}

vec3 materialGround(vec3 p)
{
    // return vec3(0.07);
    return mix(vec3(0.1), vec3(0.12), checkerboard(p));
}

vec3 materialModel(vec3 p)
{
    // return vec3(0.1,0.03,0.02);
    return vec3(0.07,0.06,0.07)*mix(1.0, 1.3, checkerboard(p*4.0));
}

vec3 render(vec3 ro, vec3 rd, float t)
{
    float tg = traceGround(ro, rd);
    float tm = traceModel(ro, rd, t);
    vec3 n,m,p;
    if (tg < tm)
    {
        p = ro + tg*rd;
        n = normalGround(p);
        m = materialGround(p);
    }
    else if (tm < tg)
    {
        p = ro + tm*rd;
        n = normalModel(p);
        m = materialModel(p);
    }
    else
    {
        return vec3(1.0);
    }

    vec3 col = vec3(0.0);
    vec3 sun = normalize(vec3(0.2, 1.0, 0.5));
    col += max(dot(n, sun), 0.0)*visibility(p, n, sun)*vec3(8.1,6.0,4.2);
    col += vec3(0.3,0.5,0.7)*occlusion(p, n);
    col *= m;
    return col;
}

void main()
{
    if (iMode == 1)
    {
        vec2 scale = iResolution.xy/iLowResolution.xy;
        vec2 fragCoord = gl_FragCoord.xy*scale;
        vec2 uv = vec2(fragCoord.x, iResolution.y - fragCoord.y) - iCameraCenter;
        vec3 rd = normalize(vec3(uv, -iCameraF));
        vec3 ro = (iView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;

        #if 0
        // Conservative cone angle calculation
        float a_all = 2.0*atan(0.5*iResolution.y/iCameraF);
        float fudge_factor = 1.3;
        float a_this = fudge_factor*a_all/(iResolution.y*iScaling);
        float sin_alpha_half = sin(a_this/2.0);
        float cos_alpha_half = sqrt(1.0 - sin_alpha_half*sin_alpha_half);
        #else
        // Exact bound cone angle calculation
        float cos_alpha_half = dot(rd, normalize(vec3(uv + vec2(0.5, 0.5)*scale, -iCameraF)));
        float sin_alpha_half = sqrt(1.0 - cos_alpha_half*cos_alpha_half);
        #endif

        rd = normalize((iView * vec4(rd, 0.0)).xyz);

        // Initial step is shared for *all* rays
        float t = model(ro);

        int i;
        for (i = ZERO; i < STEPS; i++)
        {
            vec3 p = ro + t*rd;
            float d = min(p.y - iGroundHeight, model(p));
            if (d <= sin_alpha_half*t + EPSILON) break;

            #if 0
            // Conservative step, results in more iterations but is cheaper to compute
            // Note: EPSILON in the termination condition is necessary as this iteration scheme
            // converges towards d = sin_alpha_half*t, which causes numerical instability in an
            // exact comparison.
            t = (t + d)/(1.0 + sin_alpha_half);
            #else
            // Tight step, results in fewer iterations
            // Note: if this is used the EPSILON in the termination condition above is unecessary.
            t = t*cos_alpha_half + sqrt(d*d - sin_alpha_half*sin_alpha_half*t*t);
            #endif

            if (t > MAX_DISTANCE) break;
        }

        fragColor = vec4(t);
    }
    else
    {
        vec2 uv = vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y) + (noise2f() - vec2(0.5)) - iCameraCenter;
        vec3 rd = normalize((iView * vec4(uv, -iCameraF, 0.0)).xyz);
        vec3 ro = (iView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;

        float t0 = texelFetch(iChannel0, ivec2(gl_FragCoord.xy*iLowResolution.xy/iResolution.xy), 0).r;
        fragColor.rgb = render(ro, rd, t0);
        // fragColor.rgb = vec3(t0)/4.0;
        fragColor.a = 1.0;
    }
}
