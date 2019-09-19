// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

float model(vec3 p) {
    vec3 p0 = p;
    float d;
    p.x = -abs(p.x) + 1.2;
    vec3 q = p;
    {
        pModInterval1(q.z, 1.1, -3.0, +3.0);
        d = fCylinder(q.yxz, 0.5, 0.75);
        q = p;
        d = min(d, fCylinder(q.yzx, 0.5, 3.8));
        d = max(d, -p.y);
        q.y -= 0.32;
        d = max(fBox(q, vec3(0.6, 0.3, 4.0)), -d);
    }
    {
        q = p;
        q.z -= 0.55;
        pModInterval1(q.z, 1.1, -4.0, +3.0);
        q.x -= 0.55;
        d = min(d, fBox(q, vec3(0.08,0.02,0.08)));
        q.y += 0.25;
        d = min(d, fCylinder(q, 0.06, 0.25));
    }

    {
        q = p;
        q.y -= 1.25;
        pModInterval1(q.z, 1.1, -3.0, +3.0);
        float d1 = fCylinder(q.yxz, 0.5, 0.75);
        d1 = min(d1, fCylinder(q.yzx, 0.5, 3.8));
        d1 = max(d1, -p.y);
        q.y -= 0.32;
        // d = min(d, fBox(q, vec3(0.48, 0.3, 3.78)));
        d1 = max(fBox(q, vec3(0.6, 0.3, 4.0)), -d1);
        d = min(d, d1);
    }
    {
        q = p;
        q.y -= 1.25;
        q.z -= 0.55;
        pModInterval1(q.z, 1.1, -4.0, +3.0);
        q.x -= 0.55;
        d = min(d, fBox(q, vec3(0.08,0.01,0.08)));
        q.y += 0.25;
        d = fOpUnionSoft(d, fBox(q, vec3(0.04, 0.25, 0.04)), 0.05);
    }
    {
        q = p;
        q.y -= 0.7;
        q.x -= 0.5;
        d = min(d, fBox(q, vec3(0.1,0.1,4.0)));
    }
    {
        d = min(d, p.x + 0.5);
        d = max(d, -p.x - 3.0);
        d = min(d, p.z + 4.0);
        d = max(d, p0.z - 2.0);
        d = max(d, p.y - 2.0);
        d = min(d, p.y + 0.5);
    }

    d = fOpUnionSoft(d, length(p0 - vec3(0.0,0.6,0.0)) - 0.5, 0.5);
    p0.y -= 0.6;
    d = max(d, -fCylinder(p0.yzx, 0.3, 2.0));

    return d;
}
