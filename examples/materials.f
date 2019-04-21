// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

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
    float L = 0.8; // Lipschitz constant
    float d = fSphere(p, 1.0);
    d = max(d, -fSphere(p - vec3(0.0,0.0,1.0), 0.5));
    d = min(d, fSphere(p - vec3(2.0,1.0,0.0), 0.9));

    float d_roof = fBoxCheap(p - vec3(0.0,3.0 + 0.5*sin(0.3*p.z + 0.5*p.x),2.0), vec3(4.0,0.1,4.0));
    vec3 q = p;
    q.y += 0.2;
    q.x = mod(q.x + 0.7, 1.4) - 0.7;
    q.z = mod(q.z + 0.7, 1.4) - 0.7;
    d_roof = max(d_roof, -(length(q.xz) - 0.4));

    d = min(d, d_roof);
    d = min(d, fBoxCheap(p + vec3(0.0,0.3,0.0), vec3(4.0,0.1,4.0)));
    return d*L;
}

vec3 material(vec3 p) {
    float d_ground = fBoxCheap(p + vec3(0.0,0.3,0.0), vec3(4.0,0.1,4.0));
    float d_objects = min(fSphere(p, 1.0), fSphere(p - vec3(2.0,1.0,0.0), 0.9));
    float d_roof = fBoxCheap(p - vec3(0.0,3.0,2.0), vec3(4.0,0.1,4.0));

    vec3 col = vec3(0.6,0.1,0.1);
    float d = d_ground;
    if (d_objects < d) { d = d_objects; col = vec3(1.0); }
    if (d_roof < d) { d = d_roof; col = vec3(1.0); }
    return col;
}
