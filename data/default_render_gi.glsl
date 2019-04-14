// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#define EPSILON 0.001
#define STEPS 512
#define ZERO (min(iFrame,0))

// lumina.sourceforge.net/Tutorials/Noise.html
// vec2 seed = (vec2(-1.0) + 2.0*gl_FragCoord.xy/iResolution.xy)*(iSamples*(1.0/12.0) + 1.0);
vec2 seed = vec2(-1,1)*(iSamples*(1.0/12.0) + 1.0);
vec2 noise2f()
{
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

vec3 coneSample(vec3 dir, float extent)
{
    vec2 u = noise2f();
    float a = extent*0.99*(1.0 - 2.0*u[0]);
    float b = extent*0.99*(sqrt(1.0 - a*a));
    float phi = 6.2831853072*u[1];
    float x = dir.x + b*cos(phi);
    float y = dir.y + b*sin(phi);
    float z = dir.z + a;
    return normalize(vec3(x,y,z));
}

// offset: between -0.5 and 0.5
vec3 ipinhole(vec2 fragOffset)
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

vec3 shade(vec3 p, vec3 eyeOrigin, float matIndex)
{
    const vec3 toSun = normalize(vec3(0.0, 2.0, -2.0));

    vec3 n = normal(p);
    vec3 ro = p + n*2.0*EPSILON;

    vec4 m = material(p, matIndex);

    #define SUN_SIZE 0.3
    #define COS_SUN_SIZE cos(SUN_SIZE)
    // #define DIFFUSE_LIGHTING
    // #define DIFFUSE_GLOSS_LIGHTING
    #define SPECULAR_LIGHTING

    vec3 diffuse = vec3(0.0);
    #ifdef DIFFUSE_LIGHTING
    if (trace(ro, cosineWeightedSample(n)).y > EPSILON)
        diffuse += vec3(1.0)*m.rgb;
    if (trace(ro, toSun).y > EPSILON)
        diffuse += (2.0/3.14)*vec3(1.0)*dot(n, toSun)*m.rgb;
    #endif

    // Specular
    vec3 specular = vec3(0.0);
    #if defined(SPECULAR_LIGHTING) || defined(DIFFUSE_GLOSS_LIGHTING)
    {
        vec3 fromEye = normalize(p - eyeOrigin);
        vec3 rd = coneSample(reflect(fromEye, n), m.w);
        vec3 tr = trace(ro, rd);
        if (tr.y > EPSILON)
        {
            #if defined(DIFFUSE_GLOSS_LIGHTING)
            float ndots = max(0.0, dot(toSun, rd));
            specular += vec3(1.0)*step(COS_SUN_SIZE, ndots)*mix(m.rgb, vec3(1.0), 0.5);

            #elif defined(SPECULAR_LIGHTING)
            float ndots = max(0.0, dot(toSun, rd));
            specular += vec3(1.0)*step(COS_SUN_SIZE, ndots)*mix(m.rgb, vec3(1.0), 0.5);
            specular += vec3(1.0)*m.rgb;
            #endif
        }
        #if defined(SPECULAR_LIGHTING)
        else
        {
            vec3 _p = ro + rd*tr.x;
            vec3 _n = normal(_p);
            vec3 _ro = _p + _n*2.0*EPSILON;
            vec3 _rd = cosineWeightedSample(_n);
            vec4 _m = material(_p, tr.z);
            if (trace(_ro, _rd).y > EPSILON)
            {
                specular += vec3(1.0)*_m.rgb*m.rgb;
            }
        }
        #endif
    }
    #endif

    #if defined(SPECULAR_LIGHTING) && defined(DIFFUSE_LIGHTING)
    return m.w*diffuse + (1.0-m.w)*specular;
    #elif defined(DIFFUSE_GLOSS_LIGHTING)
    return diffuse + specular;
    #elif defined(DIFFUSE_LIGHTING)
    return diffuse;
    #elif defined(SPECULAR_LIGHTING)
    return specular;
    #endif
}

void main()
{
    vec3 rd = ipinhole(noise2f());
    vec3 ro = vec3(0.0, 0.0, 4.0);

    ro = (iView * vec4(ro, 1.0)).xyz;
    rd = normalize((iView * vec4(rd, 0.0)).xyz);

    fragColor.rgb = vec3(1.0);
    vec3 tr = trace(ro, rd);
    if (tr.y <= EPSILON)
    {
        vec3 p = ro + tr.x*rd;
        fragColor.rgb = shade(p, ro, tr.z);
    }
    fragColor.a = 1.0;
}
