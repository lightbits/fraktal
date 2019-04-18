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
uniform int       iDrawIsolines;
uniform vec3      iIsolineColor;
uniform float     iIsolineThickness;
uniform float     iIsolineSpacing;
uniform float     iIsolineMax;
uniform int       iMaterialGlossy;
uniform float     iMaterialSpecularExponent;
uniform vec3      iMaterialSpecularAlbedo;
uniform vec3      iMaterialAlbedo;
uniform int       iFloorReflective;
uniform float     iFloorHeight;
uniform float     iFloorSpecularExponent;
uniform float     iFloorReflectivity;
out vec4          fragColor;

#define EPSILON 0.0001
#define STEPS 512
#define M_PI 3.1415926535897932384626433832795
#define MAX_DISTANCE 100.0
#define ZERO (min(iFrame,0))
#define MAX_DISTANCE_VISIBILITY_TEST 10.0

float model(vec3 p); // forward declaration

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
    return normalize( e.xyy*model( p + e.xyy ) +
                      e.yyx*model( p + e.yyx ) +
                      e.yxy*model( p + e.yxy ) +
                      e.xxx*model( p + e.xxx ) );
}

float traceFloor(vec3 ro, vec3 rd)
{
    if (rd.y == 0.0) return -1.0;
    else return (iFloorHeight - ro.y)/rd.y;
}

float traceModel(vec3 ro, vec3 rd)
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

float traceModelThickness(vec3 ro, vec3 rd)
{
    float t = 0.0;
    float thickness = 0.0;
    for (int i = ZERO; i < STEPS; i++)
    {
        vec3 p = ro + t*rd;
        float d = model(p);
        if (d >= -EPSILON)
        {
            t += max(EPSILON, d);
        }
        else
        {
            t += max(EPSILON, -d);
            thickness += max(EPSILON, -d);
        }
        if (t > MAX_DISTANCE) break;
    }
    return thickness;
}

bool isVisible(vec3 ro, vec3 rd)
{
    float tFloor = traceFloor(ro, rd);
    if (tFloor > EPSILON)
        return false;
    float t = 0.0;
    for (int i = ZERO; i < STEPS; i++)
    {
        float d = model(ro + t*rd);
        t += d;
        if (d <= EPSILON)
            return false;
        if (t > MAX_DISTANCE_VISIBILITY_TEST)
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

// The corresponding direction pdf is: (exponent + 2)/2pi pow(cosAlpha, exponent)
vec3 phongWeightedSample(vec3 dir, float exponent)
{
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    if (abs(tangent.x) > 0.9)
        tangent = vec3(0.0, 1.0, 0.0);
    vec3 bitangent = cross(tangent, dir);
    tangent = cross(dir, bitangent);
    vec2 u = noise2f();
    float cosAlpha = pow(u[0], 1.0/(exponent + 1.0));
    float sinAlpha = sqrt(1.0 - cosAlpha*cosAlpha);
    float phi = 2.0*M_PI*u[1];
    float x = sinAlpha*cos(phi);
    float y = cosAlpha;
    float z = sinAlpha*sin(phi);
    return x*tangent + y*dir + z*bitangent;
}

vec3 colorModel(vec3 p, vec3 ro)
{
    vec3 n = normal(p);
    vec3 v = normalize(p - ro); // from eye to point
    ro = p + n*2.0*EPSILON;

    vec3 result = vec3(0.0);

    vec3 rd = cosineWeightedSample(n);
    if (isVisible(ro,rd))
        result += vec3(1.0);

    rd = iToSun;
    if (isVisible(ro,rd))
        result += vec3(1.0)*max(0.0,dot(n, rd));

    result *= iMaterialAlbedo;

    if (iMaterialGlossy == 1)
    {
        vec3 w_s = v - 2.0*dot(n, v)*n;
        rd = phongWeightedSample(w_s, iMaterialSpecularExponent);
        if (isVisible(ro, rd) && dot(rd, iToSun) >= iCosSunSize)
            result += iMaterialSpecularAlbedo;
    }
    return result;
}

// Computes a color alternating between white and gray where the gray lines
// indicate points of identical distance to the model.
vec3 colorIsolines(vec3 p)
{
    float d = model(p);
    float a = mod(d - iIsolineThickness*0.5, iIsolineSpacing);
    float t = step(iIsolineSpacing-iIsolineThickness, a) * (1.0 - step(iIsolineMax, d));
    return mix(vec3(1.0), iIsolineColor, t);
}

vec3 colorFloor(vec3 p, vec3 ro)
{
    vec3 albedo = vec3(1.0);
    if (iDrawIsolines==1)
        albedo = colorIsolines(p);

    vec3 n = vec3(0.0, 1.0, 0.0);
    vec3 v = normalize(p - ro);
    ro = p + n*2.0*EPSILON;

    vec3 result = vec3(0.0);

    vec3 rd = cosineWeightedSample(n);
    if (isVisible(ro,rd))
        result += vec3(1.0);

    rd = iToSun;
    if (isVisible(ro,rd))
        result += vec3(1.0)*max(0.0,dot(n, rd));

    if (iFloorReflective == 1)
    {
        vec3 w_s = v - 2.0*dot(n, v)*n;
        rd = phongWeightedSample(w_s, iFloorSpecularExponent);
        float tModel = traceModel(ro, rd);
        if (tModel > 0.0)
            result = mix(result, colorModel(ro + tModel*rd, ro), iFloorReflectivity);
    }

    return result*albedo;
}

void main()
{
    vec3 rd = rayPinhole(2.0*(noise2f() - vec2(0.5)));
    vec3 ro = (iView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    rd = normalize((iView * vec4(rd, 0.0)).xyz);

    // fragColor.rgb = vec3(0.0);
    // float t = traceModelThickness(ro, rd);
    // if (t > 0.0)
    // {
    //     fragColor.rgb = vec3(t/2.0);
    // }
    // fragColor.a = 1.0;

    fragColor.rgb = vec3(1.0);
    float tModel = traceModel(ro, rd);
    float tFloor = traceFloor(ro, rd);
    if (tFloor > 0.0 && ((tModel > 0.0 && tFloor < tModel) || tModel < 0.0))
        fragColor.rgb = colorFloor(ro + rd*tFloor, ro);
    else if (tModel > 0.0 && ((tFloor > 0.0 && tModel < tFloor) || tFloor < 0.0))
        fragColor.rgb = colorModel(ro + rd*tModel, ro);
    fragColor.a = 1.0;
}
