// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#define EPSILON 0.001
#define STEPS 512
#define M_PI 3.1415926535897932384626433832795
#define ZERO (min(iFrame,0))

const vec3 skyDomeColor = vec3(0.1, 0.2, 0.7);

vec3 sky(vec3 rd)
{
    float cosSun = max(dot(rd, iToSun), 0.0);
    return mix(skyDomeColor, iSunStrength, step(iCosSunSize, cosSun));
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

// Adapted from Inigo Quilez (CC BY-NC-SA)
float ambientOcclusion(vec3 p, vec3 n)
{
    float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
        float h = 0.001 + 0.15*float(i)/4.0;
        float d = model( p + h*n ).x;
        occ += (h-d)*sca;
        sca *= 0.95;
    }
    return clamp(1.0 - 1.5*occ, 0.0, 1.0);
}

void main()
{
    vec3 rd = rayPinhole(noise2f());
    vec3 ro = (iView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    rd = normalize((iView * vec4(rd, 0.0)).xyz);

    vec3 tr = trace(ro, rd);
    if (tr.y <= EPSILON)
    {
        vec3 p = ro + tr.x*rd;
        vec3 n = normal(p);
        vec4 m = material(p, tr.z);

        ro = p + n*2.0*EPSILON;

        // ambient
        vec3 col = skyDomeColor;

        // direct sun light
        rd = iToSun;
        tr = trace(ro, rd);
        float ndotl = 0.0;
        if (tr.y > EPSILON)
            ndotl = max(0.0, dot(rd, n));
        col += 2.0*normalize(iSunStrength)*ndotl;

        // fake indirect light (sky)
        rd = normalize(vec3(0.0, 1.0, -1.0));
        ndotl = max(dot(n, rd), 0.0);
        col += skyDomeColor*ndotl;

        // fake indirect light (floor bounce from top surface)
        rd = normalize(vec3(0.0, 1.0, -2.0));
        ndotl = max(dot(n, rd), 0.0);
        col += 0.2*vec3(1.0,0.3,0.1)*ndotl;

        // fake indirect light (floor bounce)
        rd = normalize(vec3(0.0, -1.0, -0.3));
        ndotl = max(dot(n, rd), 0.0);
        col += 0.8*vec3(1.0,0.3,0.1)*ndotl;

        col *= m.rgb*ambientOcclusion(p, n);

        fragColor.rgb = col;
    }
    else
    {
        fragColor.rgb = sky(rd);
    }
    fragColor.a = 1.0;
}
