#pragma once

void compute_view_matrix(float dst[4*4], float3 t, float3 r)
{
    float cx = cosf(r.x);
    float cy = cosf(r.y);
    float cz = cosf(r.z);
    float sx = sinf(r.x);
    float sy = sinf(r.y);
    float sz = sinf(r.z);

    float dtx = t.z*(sx*sz + cx*cz*sy) - t.y*(cx*sz - cz*sx*sy) + t.x*cy*cz;
    float dty = t.y*(cx*cz + sx*sy*sz) - t.z*(cz*sx - cx*sy*sz) + t.x*cy*sz;
    float dtz = t.z*cx*cy              - t.x*sy                 + t.y*cy*sx;

    // q = R*(p + t)
    dst[ 0] = cy*cz; dst[ 1] = cz*sx*sy - cx*sz; dst[ 2] = sx*sz + cx*cz*sy; dst[ 3] = dtx;
    dst[ 4] = cy*sz; dst[ 5] = cx*cz + sx*sy*sz; dst[ 6] = cx*sy*sz - cz*sx; dst[ 7] = dty;
    dst[ 8] = -sy;   dst[ 9] = cy*sx;            dst[10] = cx*cy;            dst[11] = dtz;
    dst[12] = 0.0f;  dst[13] = 0.0f;             dst[14] = 0.0f;             dst[15] = 0.0f;
}

void invert_view_matrix(float dst[4*4], float src[4*4])
{
    dst[ 0] = src[ 0]; dst[ 1] = src[4]; dst[ 2] = src[8];  dst[ 3] = -(src[0]*src[3] + src[4]*src[7] + src[8]*src[11]);
    dst[ 4] = src[ 1]; dst[ 5] = src[5]; dst[ 6] = src[9];  dst[ 7] = -(src[1]*src[3] + src[5]*src[7] + src[9]*src[11]);
    dst[ 8] = src[ 2]; dst[ 9] = src[6]; dst[10] = src[10]; dst[11] = -(src[2]*src[3] + src[6]*src[7] + src[10]*src[11]);
    dst[12] = 0.0f;    dst[13] = 0.0f;   dst[14] = 0.0f;    dst[15] = 0.0f;
}

float3 transform_point(float m[4*4], float3 v)
{
    float3 r =
    {
        m[0]*v.x + m[1]*v.y + m[2]*v.z + m[3],
        m[4]*v.x + m[5]*v.y + m[6]*v.z + m[7],
        m[8]*v.x + m[9]*v.y + m[10]*v.z + m[11]
    };
    return r;
}

struct Widget_Camera : Widget
{
    angle2 dir;
    float3 pos;
    float camera_yfov;
    float2 camera_shift;
    int loc_iView;
    int loc_iCameraF;
    int loc_iCameraCenter;

    virtual void default_values()
    {
        dir.theta = -20.0f;
        dir.phi = 30.0f;
        pos.x = 0.0f;
        pos.y = 0.0f;
        pos.z = 24.0f;
        camera_yfov = 10.0f;
        camera_shift.x = 0.0f;
        camera_shift.y = 0.0f;
    }
    virtual void deserialize(const char **cc)
    {
        while (parse_next_in_list(cc)) {
            if (parse_argument_float(cc, "yfov", &camera_yfov)) ;
            else if (parse_argument_float2(cc, "shift", &camera_shift)) ;
            else if (parse_argument_angle2(cc, "dir", &dir)) ;
            else if (parse_argument_float3(cc, "pos", &pos)) ;
            else parse_list_unexpected();
        }
    }
    virtual void serialize(FILE *f)
    {
        fprintf(f, "camera=(");
        fprintf(f, "yfov=%g, ", camera_yfov);
        fprintf(f, "shift=(%g, %g), ", camera_shift.x, camera_shift.y);
        fprintf(f, "dir=(%g deg, %g deg), ", dir.theta, dir.phi);
        fprintf(f, "pos=(%g, %g, %g))", pos.x, pos.y, pos.z);
    }
    virtual void get_param_offsets(fKernel *f)
    {
        loc_iView = fraktal_get_param_offset(f, "iView");
        loc_iCameraCenter = fraktal_get_param_offset(f, "iCameraCenter");
        loc_iCameraF = fraktal_get_param_offset(f, "iCameraF");
    }
    virtual bool is_active()
    {
        if (loc_iView < 0) return false;
        if (loc_iCameraCenter < 0) return false;
        if (loc_iCameraF < 0) return false;
        return true;
    }
    virtual bool update(guiState &g)
    {
        bool changed = false;

        // rotation
        int rotate_step = 5;
        if (g.keys.Left.pressed)  { dir.phi   -= rotate_step; changed = true; }
        if (g.keys.Right.pressed) { dir.phi   += rotate_step; changed = true; }
        if (g.keys.Up.pressed)    { dir.theta -= rotate_step; changed = true; }
        if (g.keys.Down.pressed)  { dir.theta += rotate_step; changed = true; }

        // translation
        // Note: The z_over_f factor ensures that a key press yields the same
        // displacement of the object in image pixels, irregardless of how far
        // away the camera is.
        float f = yfov2pinhole_f(camera_yfov, (float)g.resolution.y);
        float z_over_f = fabsf(pos.z)/f;
        float x_move_step = (g.resolution.x*0.05f)*z_over_f;
        float y_move_step = (g.resolution.y*0.05f)*z_over_f;
        float z_move_step = 0.1f*fabsf(pos.z);
        if (g.keys.Ctrl.pressed)  { pos.y -= y_move_step; changed = true; }
        if (g.keys.Space.pressed) { pos.y += y_move_step; changed = true; }
        if (g.keys.A.pressed)     { pos.x -= x_move_step; changed = true; }
        if (g.keys.D.pressed)     { pos.x += x_move_step; changed = true; }
        if (g.keys.W.pressed)     { pos.z -= z_move_step; changed = true; }
        if (g.keys.S.pressed)     { pos.z += z_move_step; changed = true; }

        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float3 drag_speeds;
            drag_speeds.x = (g.resolution.x*0.005f)*z_over_f;
            drag_speeds.y = (g.resolution.y*0.005f)*z_over_f;
            drag_speeds.z = 0.01f*fabsf(pos.z);

            if (loc_iView != -1)
            {
                changed |= ImGui::DragFloat("\xce\xb8##view_dir", &dir.theta, 1.0f, -90.0f, +90.0f, "%.0f deg");
                changed |= ImGui::DragFloat("\xcf\x86##view_dir", &dir.phi, 1.0f, -180.0f, +180.0f, "%.0f deg");
                changed |= ImGui::DragFloat3("Position##view_pos", &pos.x, &drag_speeds.x);
            }
            if (loc_iCameraF != -1)
                changed |= ImGui::DragFloat("Field of view##camera_yfov", &camera_yfov, 0.1f, 1.0f, 90.0f, "%.1f deg");
            if (loc_iCameraCenter != -1)
                changed |= ImGui::DragFloat2("Shift##camera_shift", &camera_shift.x, 0.01f, -1.0f, +1.0f);
        }

        return changed;
    }
    virtual void set_params(guiState &g)
    {
        float cx = (0.5f + 0.5f*camera_shift.x)*g.resolution.x;
        float cy = (0.5f + 0.5f*camera_shift.y)*g.resolution.y;
        float f = yfov2pinhole_f(camera_yfov, (float)g.resolution.y);
        fraktal_param_2f(loc_iCameraCenter, cx, cy);
        fraktal_param_1f(loc_iCameraF, f);
        float iView[4*4];
        float3 r = {
            deg2rad(dir.theta),
            deg2rad(dir.phi),
            0.0f
        };
        compute_view_matrix(iView, pos, r);
        fraktal_param_transpose_matrix4f(loc_iView, iView);
    }
};
