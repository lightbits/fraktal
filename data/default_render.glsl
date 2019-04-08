#define EPSILON 0.001
#define STEPS 256
#define ZERO (min(iFrame,0))

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
    return normalize( e.xyy*model( p + e.xyy ) +
                      e.yyx*model( p + e.yyx ) +
                      e.yxy*model( p + e.yxy ) +
                      e.xxx*model( p + e.xxx ) );
}

vec2 trace(vec3 ro, vec3 rd)
{
    float t = 0.0;
    float min_d = 1000.0;
    for (int i = ZERO; i < STEPS; i++)
    {
        vec3 p = ro + t*rd;
        float d = model(p);
        t += d;
        min_d = min(d, min_d);
        if (d <= EPSILON)
            break;
    }
    return vec2(t, min_d);
}

void main()
{
    vec3 rd = ipinhole(vec2(0.0));
    vec3 ro = vec3(0.0, 0.0, 4.0);

    ro = (iView * vec4(ro, 1.0)).xyz;
    rd = normalize((iView * vec4(rd, 0.0)).xyz);

    fragColor.rgb = vec3(0.0);
    vec2 tr = trace(ro, rd);
    if (tr.y <= EPSILON)
    {
        vec3 p = ro + tr.x*rd;
        vec3 n = normal(p);
        fragColor.rgb = vec3(0.5) + 0.5*n;
    }
    fragColor.a = 1.0;
}
