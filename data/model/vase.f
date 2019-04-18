// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

// #widget(Resolution, width=200, height=200)
#resolution(200, 200)
#widget(Camera, yfov=10deg, dir=(-20 deg, 30 deg), pos=(0,0,20))
#widget(Floor, height=-0.5)
#widget(Material)
#widget(Sun, size=10deg, color=(1,1,0.8), intensity=250)

float fCylinder(vec3 p, float r, float height) {
    vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(r, height);
    return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float fSphere(vec3 p, float r) {
    return length(p) - r;
}

float fBoxCheap(vec3 p, vec3 b) {
    return max(abs(p.x) - b.x, max(abs(p.y) - b.y, abs(p.z) - b.z));
}

vec2 pOpRotate(vec2 p, float a) {
    return cos(a)*p + sin(a)*vec2(p.y, -p.x);
}

float fOpUnionSoft(float a, float b, float r) {
    float e = max(r - abs(a - b), 0);
    return min(a, b) - e*e*0.25/r;
}

float model(vec3 p) {
    float d = fSphere(p, 0.5);

    // // grooves
    // vec3 q = p;
    // float t = q.y + 0.5;
    // q.xz = pOpRotate(q.xz, -smoothstep(0.0,0.5,0.5*t)*0.7);
    // float a = atan(q.z, q.x);
    // d -= 0.03*sin(14.0*a);
    // d *= 0.5; // Lipschitz factor

    // // Top
    // p.y -= 0.5;
    // d = fOpUnionSoft(d, fCylinder(p, 0.2, 1.0), 0.3);
    // p.y -= 1.0;
    // d = fOpUnionSoft(d, fCylinder(p, 0.3, 0.01), 0.1);
    // float inside = fCylinder(p, 0.18, 1.2);
    // p.y += 1.5;
    // inside = fOpUnionSoft(inside, fSphere(p, 0.4), 0.3);
    // d = max(d, -inside);

    return d;
}
