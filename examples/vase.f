// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

float model(vec3 p) {
    float d = fSphere(p, 0.5);

    // grooves
    vec3 q = p;
    float t = q.y + 0.5;
    q.xz = pOpRotate(q.xz, -smoothstep(0.0,0.5,0.5*t)*0.7);
    float a = atan(q.z, q.x);
    d -= 0.03*sin(14.0*a);
    d *= 0.5; // Lipschitz factor

    // Top
    p.y -= 0.5;
    d = fOpUnionSoft(d, fCylinder(p, 0.2, 1.0), 0.3);
    p.y -= 1.0;
    d = fOpUnionSoft(d, fCylinder(p, 0.3, 0.01), 0.1);
    float inside = fCylinder(p, 0.18, 1.2);
    p.y += 1.5;
    inside = fOpUnionSoft(inside, fSphere(p, 0.4), 0.3);
    d = max(d, -inside);

    return d;
}
